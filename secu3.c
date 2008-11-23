 /****************************************************************
 *       SECU-3  - An open source, free engine control unit
 *    Designed by Alexey A. Shabelnikov. Ukraine, Gorlovka 2007.
 *       Microprocessors systems - design & programming.
 *    contacts:
 *              http://secu-3.narod.ru
 *              ICQ: 405-791-931
 ****************************************************************/

#pragma language=extended  // enable use of extended keywords

#include <inavr.h>
#include <iom16.h>
#include <pgmspace.h>

#include "funconv.h"
#include "secu3.h"
#include "uart.h"
#include "tables.h"
#include "bootldr.h"
#include "ufcodes.h"
#include "crc16.h"
#include "eeprom.h"
#include "bitmask.h"
#include "adc.h"
#include "ckps.h"
#include "vstimer.h"
#include "magnitude.h"
#include "ce_errors.h"

#define OPCODE_EEPROM_PARAM_SAVE 1

//������ ���������
#define EM_START 0   
#define EM_IDLE  1
#define EM_WORK  2


//���-�� �������� ��� ���������� ������� �������� �.�.
#define FRQ_AVERAGING           16                          
#define FRQ4_AVERAGING          4

//������ ������� ���������� �� ������� ����������� �������
#define MAP_AVERAGING           4   
#define BAT_AVERAGING           4   
#define TMP_AVERAGING           8  

#define TIMER2_RELOAD_VALUE          100                         //��� 10 ��
#define EEPROM_PARAM_START           0x002                       //����� ��������� ���������� � EEPROM 
#define EEPROM_ECUERRORS_START       (EEPROM_PARAM_START+(sizeof(params)))

//��������/��������� ����� Check Engine  
#define SET_CE_STATE(s)  {PORTB_Bit2 = s;}

//��������/��������� ����������
#define SET_VENTILATOR_STATE(s) {PORTB_Bit1 = s;}

//���������/�������������� �������
#define SET_STARTER_BLOCKING_STATE(s) {PORTD_Bit7 = s;}

//���������/��������� ������ ����
#define SET_EPHH_VALVE_STATE(s) {PORTB_Bit0 = s;}

//���������/��������� ������ ���
#define SET_EPM_VALVE_STATE(s) {PORTC_Bit7 = s;}

//��������� ��������� �������� �������
#define GET_GAS_VALVE_STATE(s) (PINC_Bit6)

//��������� ��������� ����������� �������� (������ ��������, ��� ��������)
#define GET_THROTTLE_GATE_STATE(s) (PINC_Bit5)

#define GET_DEFEEPROM_JUMPER_STATE() (PINC_Bit2)

#define disable_comparator() {ACSR=(1<<ACD);}
 
//-----------���������� ����������----------------------------
s_timer8  send_packet_interval_counter = 0;
s_timer8  force_measure_timeout_counter = 0;
s_timer16 save_param_timeout_counter = 0;
s_timer8  ce_control_time_counter = CE_CONTROL_STATE_TIME_VALUE;
s_timer8  engine_rotation_timeout_counter = 0;
s_timer8  epxx_delay_time_counter = 0;
s_timer8  idle_period_time_counter = 0;

unsigned int freq_circular_buffer[FRQ_AVERAGING];     //����� ���������� ������� �������� ���������
unsigned int freq4_circular_buffer[FRQ4_AVERAGING];
unsigned int map_circular_buffer[MAP_AVERAGING];      //����� ���������� ����������� ��������
unsigned int ubat_circular_buffer[BAT_AVERAGING];     //����� ���������� ���������� �������� ����
unsigned int temp_circular_buffer[TMP_AVERAGING];     //����� ���������� ����������� ����������� ��������

unsigned char eeprom_parameters_cache[sizeof(params) + 1];

//-------------------------------------------------------------
unsigned int ecuerrors;    //�������� 16 ����� ������

//�������� ��� ��������
#define SET_ECUERROR(error)   SETBIT(ecuerrors,error)
#define CLEAR_ECUERROR(error) CLEARBIT(ecuerrors,error)
//-------------------------------------------------------------


//��� ������������� ����� ������, �� ���������� �� ������������� �����. ���� ������ �� �������� (�������� �������� ��� ���������),
//�� CE ����� ������ ����������. ��� ������� ��������� �� ���������� �� 0.5 ���. ��� ������������� �����������������. 
void check_engine(ecudata* d)
{
  unsigned int temp_errors;
  static unsigned int merged_errors = 0; //�������� ������ ��� ���������� ������� EEPROM
  static unsigned int write_errors;      //�. eeprom_start_wr_data() ��������� ������� �������!
  static unsigned char need_to_save = 0; 

  //���� ���� ������ ���� �� ������������� ��� ��������������� ������
  if (ckps_is_error())
  {
    SET_ECUERROR(ECUERROR_CKPS_MALFUNCTION);
    ckps_reset_error();        
  }
  else
  {
    CLEAR_ECUERROR(ECUERROR_CKPS_MALFUNCTION);  
  }

  //���� ������ �������� �����, �� ����� ��
  if (s_timer_is_action(ce_control_time_counter))
    SET_CE_STATE(0);       

  //���� ���� ���� �� ���� ������ - �������� �� � ��������� ������ 
  if (ecuerrors!=0)
  {
   s_timer_set(ce_control_time_counter, CE_CONTROL_STATE_TIME_VALUE);
   SET_CE_STATE(1);  
  }

  temp_errors = (merged_errors | ecuerrors);
  if (temp_errors!=merged_errors) //��������� �� ������ ������� ��� � merged_errors?
  {
   //��� ��� �� ������ ������������� ����� ������ EEPROM ����� ���� ������ (�������� ����������� ����������),
   //�� ���������� ���������� ������ �������, ������� ����� ���������� ������������� �� ��� ��� ���� EEPROM
   //�� ����������� � ������ �� ����� ���������. 
   need_to_save = 1;
  }

  merged_errors = temp_errors;

  //���� EEPROM �� ������ � ���� ����� ������. �� ���������� ��������� ����� ������.
  //��� ���������� ������� EEPROM c��������� ������ ���������� ������ � ��� ������, ���� 
  //��� ��� �� ���� ���������. ��� ����� ������������ ������ � ���������.  
  if (eeprom_is_idle() && need_to_save)
  {
   eeprom_read(&temp_errors,EEPROM_ECUERRORS_START,sizeof(unsigned int));
   write_errors = temp_errors | merged_errors; 
   if (write_errors!=temp_errors)    
    eeprom_start_wr_data(0,EEPROM_ECUERRORS_START,(unsigned char*)&write_errors,sizeof(unsigned int));      
   need_to_save = 0;
  }
  
  //��������� ���� ������ � ��� ��� ��������.
  d->ecuerrors_for_transfer|= ecuerrors;
}

//���������� �� ����������� �/� 2 - ��� ������� ��������� ���������� � ������� (��� ������ �������������). 
//���������� ������ 10��
#pragma vector=TIMER2_OVF_vect
__interrupt void timer2_ovf_isr(void)
{ 
  TCNT2 = TIMER2_RELOAD_VALUE; 
  __enable_interrupt();     
    
  s_timer_update(force_measure_timeout_counter);
  s_timer_update(save_param_timeout_counter);
  s_timer_update(send_packet_interval_counter);  
  s_timer_update(ce_control_time_counter);
  s_timer_update(engine_rotation_timeout_counter);   
  s_timer_update(epxx_delay_time_counter);
  s_timer_update(idle_period_time_counter);  
}

//���������� ���������� ������ ��������� � ���������� ������ � ��������� 
//��������� �����������, �������� �������, ������� ����
void control_engine_units(ecudata *d)
{
  int discharge;
  //--�������� ��������� ����������� ���� ����������, ���������/���������� ������� ����
  d->sens.carb=d->param.carb_invers^GET_THROTTLE_GATE_STATE(); //���������: 0 - �������� ������, 1 - ������

  //��������� � ��������� ��������� �������� �������
  d->sens.gas = GET_GAS_VALVE_STATE();      

#ifndef VPSEM /* ������� �������� ���� ��� ������������� ������� � ��������� */
  //���������� ������� ����. ���� �������� ����������� ������� � frq > [����.�����] ���
  //�������� ����������� ������� � frq > [���.�����] �� ������ ��� ������, �� ������������
  //���������� ������ ������� ����� ����������� ������ ���������� �� ������� ��.�������. ����� - ������ �������.
  d->ephh_valve = ((!d->sens.carb)&&(((d->sens.frequen > d->param.ephh_lot)&&(!d->ephh_valve))||
                             (d->sens.frequen > d->param.ephh_hit)))?0:1;
  SET_EPHH_VALVE_STATE(d->ephh_valve);
  
  //���������� ����������� �������� (������� ����������� ����� ���������� ��������� ��������, �� ������� �� ����������!)
  if (d->sens.frequen4 > d->param.starter_off)
    SET_STARTER_BLOCKING_STATE(1);
  
#else /* ������� �������� ���� � �������������� ������� �� ����������, � ���������� ��������� ���� � � ������ ���� �������.
    ������������� ���������� ��������� �������� ����: d->param.ephh_lot - ������� ����� ��� ����
    d->param.ephh_hit - ������� ����� ��� �������. ������ ������ �� 50 ������ ������ �������������. */
  if (d->sens.carb) //���� �������� ������, �� ��������� ������, �������� ������ � ������� �� �������.
  {d->ephh_valve = 1; s_timer_set(epxx_delay_time_counter, EPXX_DELAY_TIME_VALUE);}
  else //���� �������� ������, �� ��������� ������� ������� �� ��������, ����������� ��������� �������, ������� � ���� �������.
    if (d->sens.gas) // ���� ������� �������, �� ���������� ��������� d->param.ephh_lot � d->param.ephh_lot-50
      d->ephh_valve = ((s_timer_is_action(epxx_delay_time_counter))
      &&(((d->sens.frequen > d->param.ephh_lot-50)&&(!d->ephh_valve))||(d->sens.frequen > d->param.ephh_lot)))?0:1;
    else // ���� ������, �� ���������� ��������� d->param.ephh_hit �  d->param.ephh_hit-50
      d->ephh_valve = ((s_timer_is_action(epxx_delay_time_counter))
      &&(((d->sens.frequen > d->param.ephh_hit-50)&&(!d->ephh_valve))||(d->sens.frequen > d->param.ephh_hit)))?0:1;     
  SET_EPHH_VALVE_STATE(d->ephh_valve);
  //���������� ����������� �������� (������� ����������� ��� �������� ������ ���������)
  //� ��������� ��������� ������� ���� (������������ ����� ���������� ��������) 
  SET_STARTER_BLOCKING_STATE( (d->sens.frequen4 > d->param.starter_off)&&(d->ephh_valve) ? 1 : 0);
  //���� ������ ������� ������������ - �������� �� � ��������� ������ 
    if (d->airflow > 15)
     {
      s_timer_set(ce_control_time_counter, CE_CONTROL_STATE_TIME_VALUE);
      SET_CE_STATE(1);  
     }
#endif

  //���������� ������� ������������ ���������� ���������, ��� ������� ��� ���� ������������ � ������� 
  if (d->param.tmp_use)
  {
    if (d->sens.temperat >= d->param.vent_on)
       SET_VENTILATOR_STATE(1);
    if (d->sens.temperat <= d->param.vent_off)   
       SET_VENTILATOR_STATE(0); 
  }  
  
  //���������� ��� (����������� ���������� �������)
  discharge = (d->param.map_upper_pressure - d->sens.map);
  if (discharge < 0) discharge = 0;    
  SET_EPM_VALVE_STATE(discharge < d->param.epm_on_threshold);
}


//���������� ������� ���������� (������� ��������, �������...)
void update_values_buffers(ecudata* d)
{
  static unsigned char  map_ai  = MAP_AVERAGING-1;
  static unsigned char  bat_ai  = BAT_AVERAGING-1;
  static unsigned char  tmp_ai  = TMP_AVERAGING-1;      
  static unsigned char  frq_ai  = FRQ_AVERAGING-1;
  static unsigned char  frq4_ai = FRQ4_AVERAGING-1;  

  map_circular_buffer[map_ai] = adc_get_map_value();      
  (map_ai==0) ? (map_ai = MAP_AVERAGING - 1): map_ai--;            

  ubat_circular_buffer[bat_ai] = adc_get_ubat_value();      
  (bat_ai==0) ? (bat_ai = BAT_AVERAGING - 1): bat_ai--;            

  temp_circular_buffer[tmp_ai] = adc_get_temp_value();      
  (tmp_ai==0) ? (tmp_ai = TMP_AVERAGING - 1): tmp_ai--;               

  freq_circular_buffer[frq_ai] = d->sens.inst_frq;      
  (frq_ai==0) ? (frq_ai = FRQ_AVERAGING - 1): frq_ai--; 
        
  freq4_circular_buffer[frq4_ai] = d->sens.inst_frq;      
  (frq4_ai==0) ? (frq4_ai = FRQ4_AVERAGING - 1): frq4_ai--;   
}


//���������� ���������� ������� ��������� ������� �������� ��������� ������� ����������, ����������� 
//������������ ���, ������� ���������� �������� � ���������� ��������.
void average_measured_values(ecudata* d)
{     
  unsigned char i;  unsigned long sum;       
            
  for (sum=0,i = 0; i < MAP_AVERAGING; i++)  //��������� �������� � ������� ����������� ��������
   sum+=map_circular_buffer[i];       
  d->sens.map_raw = adc_compensate((sum/MAP_AVERAGING)*2,d->param.map_adc_factor,d->param.map_adc_correction); 
  d->sens.map = map_adc_to_kpa(d->sens.map_raw, d->param.map_curve_offset, d->param.map_curve_gradient);
          
  for (sum=0,i = 0; i < BAT_AVERAGING; i++)   //��������� ���������� �������� ����
   sum+=ubat_circular_buffer[i];      
  d->sens.voltage_raw = adc_compensate((sum/BAT_AVERAGING)*6,d->param.ubat_adc_factor,d->param.ubat_adc_correction);
  d->sens.voltage = ubat_adc_to_v(d->sens.voltage_raw);  
     
  if (d->param.tmp_use) 
  {       
   for (sum=0,i = 0; i < TMP_AVERAGING; i++) //��������� ����������� (����)
    sum+=temp_circular_buffer[i];      
   d->sens.temperat_raw = adc_compensate((5*(sum/TMP_AVERAGING))/3,d->param.temp_adc_factor,d->param.temp_adc_correction); 
   d->sens.temperat = temp_adc_to_c(d->sens.temperat_raw);
  }  
  else                                       //���� �� ������������
   d->sens.temperat = 0;
               
  for (sum=0,i = 0; i < FRQ_AVERAGING; i++)  //��������� ������� �������� ���������
   sum+=freq_circular_buffer[i];      
  d->sens.frequen=(sum/FRQ_AVERAGING);           

  for (sum=0,i = 0; i < FRQ4_AVERAGING; i++) //��������� ������� �������� ���������
   sum+=freq4_circular_buffer[i];      
  d->sens.frequen4=(sum/FRQ4_AVERAGING);           

}


//������������ ������������/����������� ������ UART-a
void process_uart_interface(ecudata* d)
{ 
 unsigned char descriptor;

 if (uart_is_packet_received())//������� ����� ����� ?
 { 
  descriptor = uart_recept_packet(d);
  switch(descriptor)
  {
    case TEMPER_PAR:            
    case CARBUR_PAR:   
    case IDLREG_PAR:   
    case ANGLES_PAR:   
    case FUNSET_PAR:   
    case STARTR_PAR:
    case ADCCOR_PAR: 
    case CKPS_PAR:        
      //���� ���� �������� ��������� �� ���������� ������� �������
      s_timer16_set(save_param_timeout_counter, SAVE_PARAM_TIMEOUT_VALUE);
      break;  
  }
  
  //���� ���� �������� ��������� ����, �� ���������� ��������� �� �� ���������� ���������
  if (descriptor == CKPS_PAR)
  {
    ckps_set_edge_type(d->param.ckps_edge_type);
    ckps_set_cogs_btdc(d->param.ckps_cogs_btdc);
    ckps_set_ignition_cogs(d->param.ckps_ignit_cogs);
  }

  //�� ���������� �������� ������ - �������� ����� ������ �� ��������
  uart_notify_processed();         
 }

 //������������ �������� ������ � �������
 if (s_timer_is_action(send_packet_interval_counter))
 {
  if (!uart_is_sender_busy())
  {                
   uart_send_packet(d,0);    //������ ���������� �������� ��������� ������
   s_timer_set(send_packet_interval_counter,SEND_PACKET_INTERVAL_VALUE);
   
   //����� �������� ������� ��� ������
   d->ecuerrors_for_transfer = 0;
  }
 }

 //�������� ��������������� ��� ���������� ��������� �������� 
 if ((0!=d->op_comp_code)&&(!uart_is_sender_busy()))
  {                
   uart_send_packet(d,OP_COMP_NC);    //������ ���������� �������� ��������� ������
   d->op_comp_code = 0;
  } 
 
}


//��������������� ��������� ����� ������ ���������
void InitialMeasure(ecudata* d)
{ 
  unsigned char i = 16;
  __enable_interrupt();
  do
  {
    adc_begin_measure();                                                     
    while(!adc_is_measure_ready()); 
    update_values_buffers(d);
  }while(--i);  
  __disable_interrupt();
  average_measured_values(d);  
  d->atmos_press = d->sens.map;      //��������� ����������� �������� � ���!
}

//������ ������ � EEPROM - ������� ����� ���������. �� ����� ��������� ����������� � ����������� ���������,
//� ��� ����������� ����������� �������� ������ � ��������� ����� � �� ���� �� ����� ����� � EEPROM.
//���������� ������ � EEPROM ���������� ������ ���� �� �������� ����� �� ��������� �� ����� �������� ������ ����������
//�� UART-a � ����������� ��������� ���������� �� �������.        
void save_param_if_need(ecudata* d)
{
  char opcode;
  
  if (d->op_actn_code == OPCODE_EEPROM_PARAM_SAVE)
    goto force_parameters_save; //goto - ��� ���.
    
  //��������� �� ���������� �� �������� �����?
  if (s_timer16_is_action(save_param_timeout_counter)) 
  {
    //������� � ����������� ��������� ����������?
    if (memcmp(eeprom_parameters_cache,&d->param,sizeof(params)-PAR_CRC_SIZE)) 
    {
force_parameters_save:    
    //�� �� ����� ������ ���������� ����������, ��� ��� EEPROM �� ������ ������ ������ - ���������� 
    //������������� � ����� ������������ ����� EEPROM ����������� � ����� ����� ������� ��� �������.
    if (!eeprom_is_idle())
      return;

     memcpy(eeprom_parameters_cache,&d->param,sizeof(params));  
     ((params*)eeprom_parameters_cache)->crc=crc16(eeprom_parameters_cache,sizeof(params)-PAR_CRC_SIZE); //������� ����������� �����
     eeprom_start_wr_data(OPCODE_EEPROM_PARAM_SAVE,EEPROM_PARAM_START,eeprom_parameters_cache,sizeof(params));
     
     //���� ���� ��������������� ������, �� ��� ������ ����� ����� ���� ��� � EEPROM �����
     //�������� ����� ��������� � ���������� ����������� ������ 
     CLEAR_ECUERROR(ECUERROR_EEPROM_PARAM_BROKEN);
     d->op_actn_code = 0; //����������       
    }
    s_timer16_set(save_param_timeout_counter, SAVE_PARAM_TIMEOUT_VALUE);
  }
  
  //���� ���� ����������� �������� �� ��������� �� ��� ��� �������� �����������
  opcode = eeprom_take_completed_opcode();
  if (opcode)
   d->op_comp_code = opcode;   
}

//��������� ��������� �� EEPROM, ��������� ����������� ������ � ���� ��� ��������� ��
//����� ��������� ����� �� FLASH.
void load_eeprom_params(ecudata* d)
{
 if (GET_DEFEEPROM_JUMPER_STATE())
 { 
   //��������� ��������� �� EEPROM, � ����� ��������� �����������.
   //��� �������� ����������� ����� �� ��������� ����� ����� ����������� �����
   //���� ����������� ����� �� ��������� - ��������� ��������� ��������� �� FLASH
   eeprom_read(&d->param,EEPROM_PARAM_START,sizeof(params));  
   
   if (crc16((unsigned char*)&d->param,(sizeof(params)-PAR_CRC_SIZE))!=d->param.crc)
   {
     memcpy_P(&d->param,&def_param,sizeof(params));
     SET_ECUERROR(ECUERROR_EEPROM_PARAM_BROKEN);
   }
   
   //�������������� ��� ����������, ����� ����� ������ ��������� ���������� �������� 
   //�� ����������. 
   memcpy(eeprom_parameters_cache,&d->param,sizeof(params));         
 }
 else
 { //��������� ������� - ��������� ���������� ���������, ������� ����� ����� ���������    
   memcpy_P(&d->param,&def_param,sizeof(params)); 
 }
} 
   

void init_system_timer(void)
{
  TCCR2 = (1<<CS22)|(1<<CS21)|(1<<CS20);      //clock = 15.625kHz  
  TIMSK|= (1<<TOIE2); //��������� ���������� �� ������������ ������� 2                          
}

void init_io_ports(void)
{
  //������������� ����� �����/������
  PORTA  = 0;   
  DDRA   = 0;       
  PORTB  = (1<<PB2)|(1<<PB4)|(1<<PB3)|(1<<PB0);             //CE �����(��� ��������), ������ ���� �������, ��������� � HIP �������� (CS=1, TEST=1)
  DDRB   = (1<<DDB4)|(1<<DDB3)|(1<<DDB2)|(1<<DDB1)|(1<<DDB0);   
  PORTC  = (1<<PC3)|(1<<PC2)/*|(1<<PC7)*/; //��� ��������
  DDRC   = (1<<DDC7);  //����� ��� ���������� �������� ���
  PORTD  = (1<<PD6)|(1<<PD3)|(1<<PD7);                      //������� ������������, ����� �������������� ��� HIP
  DDRD   = (1<<DDD7)|(1<<DDD5)|(1<<DDD4)|(1<<DDD3)|(1<<DDD1); //���. PD1 ���� UART �� ������������������ TxD 
}

void advanve_angle_state_machine(unsigned char* pmode, signed int* padvance_angle_inhibitor_state, ecudata* d)
{
 switch(*pmode)
 {
  case EM_START: //����� �����
   if (d->sens.inst_frq > d->param.smap_abandon)
   {                   
    *pmode = EM_IDLE;    
    idling_regulator_init();    
   }      
   d->curr_angle=start_function(d);               //������� ��� - ������� ��� �����
   d->airflow = 0;                                //� ������ ����� ��� �������
   *padvance_angle_inhibitor_state = d->curr_angle;//� ������ ����� ������ ��������
   break;     
              
  case EM_IDLE: //����� ��������� ����
   if (d->sens.carb)//������ ���� ������ - � ������� �����
   {
    *pmode = EM_WORK;
   }             
   work_function(d, 1);                           //��������� �������� ������� ������� 
   d->curr_angle = idling_function(d);            //������� ��� - ������� ��� �� 
   d->curr_angle+=coolant_function(d);            //��������� � ��� ������������� ���������
   d->curr_angle+=idling_pregulator(d,&idle_period_time_counter);//��������� �����������
   break;            
                                             
  case EM_WORK: //������� ����� 
   if (!d->sens.carb)//������ ���� ��������� - � ���������� ����� ��
   {
    *pmode = EM_IDLE;
    idling_regulator_init();    
   }
   d->curr_angle=work_function(d, 0);           //������� ��� - ������� �������� ������
   d->curr_angle+=coolant_function(d);          //��������� � ��� ������������� ���������
   break;     
       
  default:  //���������� �������� - ���� � ����       
   d->curr_angle = 0;
   break;     
 }
}

void switch_fuel_type(ecudata* d)
{
 //��������� � ��������� ��������� �������� �������
 d->sens.gas = GET_GAS_VALVE_STATE();  
 if (d->sens.gas)
  d->fn_dat = (__flash F_data*)&tables[d->param.fn_gas];    //�� ����
 else  
  d->fn_dat = (__flash F_data*)&tables[d->param.fn_benzin];//�� �������  
}   
      
__C_task void main(void)
{
  unsigned char mode = EM_START;   
  unsigned char turnout_low_priority_errors_counter = 100;
  signed int advance_angle_inhibitor_state = 0;  
  ecudata edat; 
  
  edat.op_comp_code = 0;
  edat.op_actn_code = 0;
  edat.sens.inst_frq = 0;
  edat.curr_angle = 0;
  edat.ecuerrors_for_transfer = 0;
    
  init_io_ports();
  
  if (crc16f(0,CODE_SIZE)!=code_crc)
  { //��� ��������� �������� - �������� ��
    SET_ECUERROR(ECUERROR_PROGRAM_CODE_BROKEN); 
  }

  adc_init();

  //��������� ���������� - �� ��� �� �����  
  disable_comparator();             

  //�������� ��������� ������ ��������� �������� ��� ������������� ������
  InitialMeasure(&edat);   
   
  //������� ���������� ��������
  SET_STARTER_BLOCKING_STATE(0); 
     
  //������ ���������
  load_eeprom_params(&edat);

  init_system_timer();
  
  //�������������� UART
  uart_init(CBR_9600);
  
  //�������������� ������ ����             
  ckps_init_state();  
  ckps_set_edge_type(edat.param.ckps_edge_type);
  ckps_set_cogs_btdc(edat.param.ckps_cogs_btdc);
  ckps_set_ignition_cogs(edat.param.ckps_ignit_cogs);
  
  //��������� ��������� ����������            
  __enable_interrupt();    

  //------------------------------------------------------------------------     
  while(1)
  {                          
    if (ckps_is_cog_changed())
    {
     s_timer_set(engine_rotation_timeout_counter,ENGINE_ROTATION_TIMEOUT_VALUE);    
    }
     
    if (s_timer_is_action(engine_rotation_timeout_counter))
    { //��������� ����������� (��� ������� ���� �����������)
     ckps_init_state_variables();
     mode = EM_START; //����� ����� 	 
    }
      
    //��������� ��������� ���, ����� ������ ���������� �������. ��� ����������� ������� ��������
    //����� ���� ������ ��������������������. ����� �������, ����� ������� �������� ��������� ��������
    //������������ ��������, ��� ������� ���������� ����������.
    if (s_timer_is_action(force_measure_timeout_counter))
    {
     __disable_interrupt();
     adc_begin_measure();
     __enable_interrupt();
     
     s_timer_set(force_measure_timeout_counter, FORCE_MEASURE_TIMEOUT_VALUE);
     update_values_buffers(&edat);     
    }      
  
   //----------����������� ����������-----------------------------------------
    //���������� ������������� � �������������� ����������� ������
    check_engine(&edat);
    //��������� ����������/�������� ������ ����������������� �����
    process_uart_interface(&edat);  
    //���������� ����������� ��������
    save_param_if_need(&edat);    
    //������ ���������� ������� �������� ���������
    edat.sens.inst_frq = ckps_calculate_instant_freq();                           
    //���������� ���������� ������� ���������� � ��������� �������
    average_measured_values(&edat);        
    //� ����������� �� �������� ���� ������� �������� ��������������� ����� ������             
    switch_fuel_type(&edat);
     //���������� ����������
    control_engine_units(&edat);      
     //�� ��������� ������� (��������� ������� - ������ ��������� �����)
    advanve_angle_state_machine(&mode,&advance_angle_inhibitor_state,&edat);
     //��������� � ��� �����-���������
    edat.curr_angle+=edat.param.angle_corr;       
     //������������ ������������ ��� �������������� ���������
    restrict_value_to(&edat.curr_angle, edat.param.min_angle, edat.param.max_angle);  
    //------------------------------------------------------------------------
    
    //��������� �������� ������� ���������� ��������� ������ ��� ������� �������� �����.      
    if (ckps_is_cycle_cutover_r())
    {
     update_values_buffers(&edat);       
     s_timer_set(force_measure_timeout_counter, FORCE_MEASURE_TIMEOUT_VALUE);
    
     //������������ ������� ��������� ���, �� �� ����� ��������� ������ ��� �� ������������ ��������
     //�� ���� ������� ����. 
     edat.curr_angle = advance_angle_inhibitor(edat.curr_angle, &advance_angle_inhibitor_state, edat.param.angle_inc_spead, edat.param.angle_dec_spead);         
     //��������� ��� ��� ���������� � ��������� �� ������� ����� ���������       
     ckps_set_dwell_angle(edat.curr_angle);        
    
     // ������������� ���� ������ ���������� ��� ������ �������� ��������� 
     //(��� ���������� N-�� ���������� ������)
     if (turnout_low_priority_errors_counter == 1)
     {    
      CLEAR_ECUERROR(ECUERROR_EEPROM_PARAM_BROKEN);  
      CLEAR_ECUERROR(ECUERROR_PROGRAM_CODE_BROKEN);  
     }
     if (turnout_low_priority_errors_counter > 0)
      turnout_low_priority_errors_counter--;      
    }   
     
   }//main loop
  //------------------------------------------------------------------------     
}
