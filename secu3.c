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

#define FRQ_AVERAGING                16                          //���-�� �������� ��� ���������� ������� �������� �.�.
#define FRQ4_AVERAGING               4

#define TIMER2_RELOAD_VALUE          100                         //��� 10 ��
#define EEPROM_PARAM_START           0x002                       //����� ��������� ���������� � EEPROM 

//��������/��������� ����� Check Engine  
#define SET_CE_STATE(s)  {PORTB_Bit2 = s;}

//��������/��������� ����������
#define SET_VENTILATOR_STATE(s) {PORTB_Bit1 = s;}

//���������/�������������� �������
#define SET_STARTER_BLOCKING_STATE(s) {PORTD_Bit7 = s;}

//���������/��������� ������ ����
#define SET_EPHH_VALVE_STATE(s) {PORTB_Bit0 = s;}

//��������� ��������� �������� �������
#define GET_GAS_VALVE_STATE(s) (PINC_Bit6)

//��������� ��������� ����������� �������� (������ ��������, ��� ��������)
#define GET_THROTTLE_GATE_STATE(s) (PINC_Bit5)

#define GET_DEFEEPROM_JUMPER_STATE() (PINC_Bit2)

#define disable_comparator() {ACSR=(1<<ACD);}

 
//--���������� ����������     
unsigned char send_packet_interval_counter = 0;
unsigned char force_measure_timeout_counter = 0;
unsigned char engine_stop_timeout_counter = 0;
unsigned char save_param_timeout_counter = 0;
unsigned char ce_control_time_counter = 0;

unsigned int freq_average_buf[FRQ_AVERAGING];                     //����� ���������� ������� �������� ���������
unsigned int freq4_average_buf[FRQ4_AVERAGING];

unsigned char eeprom_parameters_cache[64];

//���������� �� ����������� �/� 2 - ��� ������� ��������� ���������� � ������� (��� ������ �������������). 
//���������� ������ 10��
#pragma vector=TIMER2_OVF_vect
__interrupt void timer2_ovf_isr(void)
{ 
  TCNT2 = TIMER2_RELOAD_VALUE; 

  if (force_measure_timeout_counter > 0)
    force_measure_timeout_counter--;
  else
  {
    force_measure_timeout_counter = FORCE_MEASURE_TIMEOUT_VALUE;
    adc_begin_measure();
  }      
  __enable_interrupt();     
    
  if (save_param_timeout_counter > 0)
    save_param_timeout_counter--; 

  if (send_packet_interval_counter > 0)
    send_packet_interval_counter--;

  if (engine_stop_timeout_counter > 0)
    engine_stop_timeout_counter--;

  //-----------------Check engine---------------------------
  if (ckps_is_error())
  {
    ce_control_time_counter = CE_CONTROL_STATE_TIME_VALUE;
    SET_CE_STATE(1);  
    ckps_reset_error();        
  }

  if (ce_control_time_counter > 0) 
    ce_control_time_counter--;
  else 
    SET_CE_STATE(0);       
 //--------------------------------------------------------
}

//���������� ������ ���������� ��� ������� ��������
void update_buffer_freq(ecudata* d)
{
  static unsigned char frq_ai = FRQ_AVERAGING-1;
  static unsigned char frq4_ai = FRQ4_AVERAGING-1;

  if ((engine_stop_timeout_counter == 0)||(ckps_is_cycle_cutover_r()))
  {
    //��������� ���������� ������ ����������  � �������� ��� �������
    freq_average_buf[frq_ai] = d->sens.inst_frq;      
    (frq_ai==0) ? (frq_ai = FRQ_AVERAGING - 1): frq_ai--; 
        
    freq4_average_buf[frq4_ai] = d->sens.inst_frq;      
    (frq4_ai==0) ? (frq4_ai = FRQ4_AVERAGING - 1): frq4_ai--; 

    engine_stop_timeout_counter = ENGINE_STOP_TIMEOUT_VALUE;   
  }
}

//���������� ���������� ������ ��������� � ���������� ������ � ��������� 
//��������� �����������, �������� �������, ������� ����
void control_engine_units(ecudata *d)
{
  //���������� ������� ����. ���� �������� ����������� ������� � frq > [����.�����] ���
  //�������� ����������� ������� � frq > [���.�����] �� ������ ��� ������, �� ������������
  //���������� ������ ������� ����� ����������� ������ ���������� �� ������� ��.�������. ����� - ������ �������.
  //--�������� ��������� ����������� ���� ����������, ���������/���������� ������� ����
  d->sens.carb=d->param.carb_invers^GET_THROTTLE_GATE_STATE(); //���������: 0 - �������� ������, 1 - ������
  d->ephh_valve = ((!d->sens.carb)&&(((d->sens.frequen > d->param.ephh_lot)&&(!d->ephh_valve))||
                             (d->sens.frequen > d->param.ephh_hit)))?0:1;
  SET_EPHH_VALVE_STATE(d->ephh_valve);
 
#ifndef VPSEM_STARTER_BLOCKING
  //���������� ����������� �������� (������� ����������� ����� ���������� ��������� ��������)
  if (d->sens.frequen4 > d->param.starter_off)
    SET_STARTER_BLOCKING_STATE(1);
#else
  //���������� ����������� �������� (������� ����������� ����� ���������� ��������� ��������)
  //� ���������� ���������� ��������� ������� ���� (������������ ����� ���������� ��������) 
  SET_STARTER_BLOCKING_STATE( (d->sens.frequen4 > d->param.starter_off)&&(d->ephh_valve) ? 1 : 0);
#endif
 
  if (d->param.tmp_use)
  {
    //���������� ������� ������������ ���������� ���������
    if (d->sens.temperat >= d->param.vent_on)
       SET_VENTILATOR_STATE(1);
    if (d->sens.temperat <= d->param.vent_off)   
       SET_VENTILATOR_STATE(0); 
  }  
  
  //��������� � ��������� ��������� �������� �������
  d->sens.gas = GET_GAS_VALVE_STATE();    
}


//���������� ���������� ������� ��������� ������� �������� ������� ����������
void average_measured_values(ecudata* d)
{     
  unsigned char i;unsigned long sum;       
  ADCSRA&=~((1<<ADIF)|(1<<ADIE));             //��������� ���������� �� ��� �� ��������� ���� ����������
            
  for (sum=0,i = 0; i < MAP_AVERAGING; i++)
   sum+=adc_get_map_value(i);      
  d->sens.map=(sum/MAP_AVERAGING)*2; 
          
  for (sum=0,i = 0; i < BAT_AVERAGING; i++)   //��������� ���������� �������� ����
   sum+=adc_get_ubat_value(i);      
  d->sens.voltage=(sum/BAT_AVERAGING)*6; 
       
  if (d->param.tmp_use) 
  {       
   for (sum=0,i = 0; i < TMP_AVERAGING; i++) //��������� ����������� (����)
    sum+=adc_get_temp_value(i);      
   d->sens.temperat=((sum/TMP_AVERAGING)*5)/3; 
  }  
  else             //���� �� ������������
   d->sens.temperat=0;
    
  ADCSRA=(ADCSRA&(~(1<<ADIF)))|(1<<ADIE);    //��������� ���������� �� ��� �� ��������� ���� ����������
           
  for (sum=0,i = 0; i < FRQ_AVERAGING; i++)    //��������� ������� �������� ���������
   sum+=freq_average_buf[i];      
  d->sens.frequen=(sum/FRQ_AVERAGING);           

  for (sum=0,i = 0; i < FRQ4_AVERAGING; i++)    //��������� ������� �������� ���������
   sum+=freq4_average_buf[i];      
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
      //���� ���� �������� ��������� �� ���������� ������� �������
      save_param_timeout_counter = SAVE_PARAM_TIMEOUT_VALUE;
      break;  
  }

  //�� ���������� �������� ������ - �������� ����� ������ �� ��������
  uart_notify_processed();         
 }

 //������������ �������� ������ � �������
 if (send_packet_interval_counter==0)
 {
  if (!uart_is_sender_busy())
  {                
   uart_send_packet(d);    //������ ���������� �������� ��������� ������
   send_packet_interval_counter = SEND_PACKET_INTERVAL_VALUE;
  }
 }
}


//��������������� ��������� ����� ������ ���������
void InitialMeasure(ecudata* d)
{ 
  unsigned char i=16;
  __enable_interrupt();
  do
  {
    adc_begin_measure();                                                     
    while(!adc_is_measure_ready()); 
  }while(--i);  
  __disable_interrupt();
  average_measured_values(d);  
  d->atmos_press = d->sens.map;      //��������� ����������� ��������
}

//������ ������ � EEPROM - ������� ����� ���������. �� ����� ��������� ����������� � ����������� ���������,
//� ��� ����������� ����������� �������� ������ � ��������� ����� � �� ���� �� ����� ����� � EEPROM.
//���������� ������ � EEPROM ���������� ������ ���� �� �������� ����� �� ��������� �� ����� �������� ������ ����������
//�� UART-a � ����������� ��������� ���������� �� �������.        
void save_param_if_need(ecudata* d)
{
  //��������� �� ���������� �� �������� �����
  if (save_param_timeout_counter==0) 
  {
    //������� � ����������� ��������� ����������?
    if (memcmp(eeprom_parameters_cache,&d->param,sizeof(params)-PAR_CRC_SIZE)) 
    {
     memcpy(eeprom_parameters_cache,&d->param,sizeof(params));  
     ((params*)eeprom_parameters_cache)->crc=crc16(eeprom_parameters_cache,sizeof(params)-PAR_CRC_SIZE); //������� ����������� �����
     eeprom_start_wr_data(EEPROM_PARAM_START,eeprom_parameters_cache,sizeof(params));
    }
   save_param_timeout_counter = SAVE_PARAM_TIMEOUT_VALUE;
  }
}


//��������� ��������� �� EEPROM, ��������� ����������� ������ � ���� ��� ��������� ��
//����� ��������� ����� �� FLASH.
void load_eeprom_params(ecudata* d)
{
 unsigned char* e = (unsigned char*)&d->param;
 int count=sizeof(params);
 int adr=EEPROM_PARAM_START;
 
 if (GET_DEFEEPROM_JUMPER_STATE())
 { 
   //��������� ��������� �� EEPROM, � ����� ��������� �����������
   do
   {
     __EEGET(*e,adr);
     adr++;
     e++;
   }while(--count); 
 
   EEAR=0x000;      
 
   //��� �������� ����������� ����� �� ��������� ����� ����� ����������� �����
   //���� ����������� ����� �� ��������� - ��������� ��������� ��������� �� FLASH
   if (crc16((unsigned char*)&d->param,(sizeof(params)-PAR_CRC_SIZE))!=d->param.crc)
   {
     memcpy_P(&d->param,&def_param,sizeof(params));
   }
 }
 else
 { //��������� ������� - ��������� ���������� ���������
   memcpy_P(&d->param,&def_param,sizeof(params)); 
 }

 //�������������� ��� ����������, ����� ����� ������ ��������� ���������� �������� 
 //�� ����������. 
 memcpy(eeprom_parameters_cache,&d->param,sizeof(params));       
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
  PORTB  = (1<<PB4)|(1<<PB3)|(1<<PB0);                      //������ ���� �������, ��������� � HIP �������� (CS=1, TEST=1)
  DDRB   = (1<<DDB4)|(1<<DDB3)|(1<<DDB2)|(1<<DDB1)|(1<<DDB0);   
  PORTC  = (1<<PC3)|(1<<PC2);
  DDRC   = 0;  
  PORTD  = (1<<PD6)|(1<<PD3)|(1<<PD7);                      //������� ������������, ����� �������������� ��� HIP
  DDRD   = (1<<DDD7)|(1<<DDD5)|(1<<DDD4)|(1<<DDD3);
}


//---------------[TEST]------------------------
const float  K1=0.008654,       K2=0.004688,   K3=0.0;
const float  B1=-5.19,          B2=7.49,       B3=30;
const int    N1=600,  N2=3200,  N3=4800;

float func(int it,float tkorr)
{
    if (it < N1)   
        return 0.0;  
    if (it < N2)
        return (K1 * it + B1) + tkorr;
    if (it < N3)
        return (K2 * it + B2) + tkorr;
    else
        return (K3 * it + B3) + tkorr;
}
//--------------[/TEST]-------------------------


      
__C_task void main(void)
{
  unsigned char mode=0;
  ecudata edat; 
    
  init_io_ports();

  if (crc16f(0,CODE_SIZE)!=code_crc)
     SET_CE_STATE(1);                                       //��� ��������� �������� - �������� ��

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
               
  ckps_init_state();  
  ckps_set_edge_type(0);
  ckps_set_ignition_cogs(10);
  
  //��������� ��������� ����������            
  __enable_interrupt();    
     
  while(1)
  {
    //��������� ����������/�������� ������ ����������������� �����
    process_uart_interface(&edat);  
   
    //���������� ����������� ��������
    save_param_if_need(&edat);                        
    
    edat.sens.inst_frq = ckps_calculate_instant_freq();                           

    update_buffer_freq(&edat);

    average_measured_values(&edat);        

    control_engine_units(&edat);
       
    //� ����������� �� �������� ���� ������� �������� ��������������� ����� ������             
    if (edat.sens.gas)
      edat.fn_dat = (__flash F_data*)&tables[edat.param.fn_gas];    //�� ����
    else  
      edat.fn_dat = (__flash F_data*)&tables[edat.param.fn_benzin];//�� �������
    
    /*
    //�� ��������� �������
    switch(mode)
    {
      case 0: //����� �����
        if (edat.sens.inst_frq > edat.param.smap_abandon)
        {                   
         mode=1;        
        }      
        edat.curr_angle=str_func(&edat);                //������� ��� - ������� ��� �����
        edat.airflow=0;
        break;            
      case 1: //����� ��������� ����
       if (edat.sens.carb)//������ ���� ������ - � ������� �����
       {
        mode=2;
       }      
        edat.curr_angle=idl_func(&edat);               //������� ��� - ������� ��� �� 
        edat.curr_angle+=tmp_func(&edat);              //��������� � ��� ������������� ���������
        edat.curr_angle+=idl_pregul(&edat);            //��������� �����������
        edat.airflow=0;
        break;            
      case 2: //������� ����� 
       if (edat.sens.carb)//������ ���� ��������� - � ����� ��
       {
        mode=1;
       }
       edat.curr_angle=wrk_func(&edat);                //������� ��� - ������� �������� ������
       edat.curr_angle+=tmp_func(&edat);               //��������� � ��� ������������� ���������
        break;     
      default:  //���������� �������� - ���� � ����       
        edat.curr_angle=0;
        break;     
    }
      
    //��������� � ��� �����-���������
    edat.curr_angle+=edat.param.angle_corr;
      
    //������������ ������������ ��� �������������� ���������
    if (edat.curr_angle > edat.param.max_angle)
               edat.curr_angle = edat.param.max_angle;  
    if (edat.curr_angle < edat.param.min_angle)
               edat.curr_angle = edat.param.min_angle; 
    */
    
    //--------------[TEST]-----------------------
    edat.curr_angle = func(edat.sens.inst_frq, 4.0 ) * ANGLE_MULTIPLAYER;    
    //--------------[/TEST]----------------------

    //��������� ��� ��� ���������� � ��������� �� ������� ����� ���������       
    ckps_set_dwell_angle(edat.curr_angle);  
  }
}
