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
#include "knock.h"
#include "suspendop.h"
#include "measure.h"
#include "knklogic.h"

//������ ���������
#define EM_START 0   
#define EM_IDLE  1
#define EM_WORK  2

//��������/��������� ����������
#define SET_VENTILATOR_STATE(s) {PORTB_Bit1 = s;}

//���������/�������������� �������
#define SET_STARTER_BLOCKING_STATE(s) {PORTD_Bit7 = s;}

//���������/��������� ������ ����
#define SET_EPHH_VALVE_STATE(s) {PORTB_Bit0 = s;}

//���������/��������� ������ ���
#define SET_EPM_VALVE_STATE(s) {PORTC_Bit7 = s;}

#define GET_DEFEEPROM_JUMPER_STATE() (PINC_Bit2)

#define disable_comparator() {ACSR=(1<<ACD);}
 
 
uint8_t eeprom_parameters_cache[sizeof(params) + 1];

ecudata edat;

//���������� ���������� ������ ��������� � ���������� ������ � ��������� 
//��������� �����������, �������� �������, ������� ����
void control_engine_units(ecudata *d)
{
  int16_t discharge;

  //���������� ������� ����. ���� �������� ����������� ������� � frq > [����.�����] ���
  //�������� ����������� ������� � frq > [���.�����] �� ������ ��� ������, �� ������������
  //���������� ������ ������� ����� ����������� ������ ���������� �� ������� ��.�������. ����� - ������ �������.  
  if (d->sens.carb) //���� �������� ������, �� ��������� ������, �������� ������ � ������� �� �������.
  {
   d->ephh_valve = 1; 
   s_timer_set(epxx_delay_time_counter, d->param.shutoff_delay);
  }
  else //���� �������� ������, �� ��������� ������� ������� �� ��������, ����������� ��������� �������, ������� � ���� �������.
    if (d->sens.gas) //������� �������
      d->ephh_valve = ((s_timer_is_action(epxx_delay_time_counter))
      &&(((d->sens.frequen > d->param.ephh_lot_g)&&(!d->ephh_valve))||(d->sens.frequen > d->param.ephh_hit_g)))?0:1;
    else //������
      d->ephh_valve = ((s_timer_is_action(epxx_delay_time_counter))
      &&(((d->sens.frequen > d->param.ephh_lot)&&(!d->ephh_valve))||(d->sens.frequen > d->param.ephh_hit)))?0:1;     
  SET_EPHH_VALVE_STATE(d->ephh_valve);

#ifndef VPSEM   
  //���������� ����������� �������� (������� ����������� ����� ���������� ��������� ��������, �� ������� �� ����������!)
  if (d->sens.frequen4 > d->param.starter_off)
    SET_STARTER_BLOCKING_STATE(1);  
#else 
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
  d->epm_valve = discharge < d->param.epm_on_threshold;
  SET_EPM_VALVE_STATE(d->epm_valve);
}

//������������ ������������/����������� ������ UART-a
void process_uart_interface(ecudata* d)
{ 
 uint8_t descriptor;

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
    case KNOCK_PAR:  
    case MISCEL_PAR:     
      //���� ���� �������� ��������� �� ���������� ������� �������
      s_timer16_set(save_param_timeout_counter, SAVE_PARAM_TIMEOUT_VALUE);
      break;        
    case OP_COMP_NC: 
      if (d->op_actn_code == OPCODE_EEPROM_PARAM_SAVE) //������� ������� ���������� ����������
      {
       sop_set_operation(SOP_SAVE_PARAMETERS);     
       d->op_actn_code = 0; //���������� 
      }
      if (d->op_actn_code == OPCODE_CE_SAVE_ERRORS) //������� ������� ������ ����������� ����� ������  
      {
       sop_set_operation(SOP_READ_CE_ERRORS);     
       d->op_actn_code = 0; //���������� 
      }
      if (d->op_actn_code == OPCODE_READ_FW_SIG_INFO) //������� ������� ������ � �������� ���������� � ��������
      {
       sop_set_operation(SOP_SEND_FW_SIG_INFO);
       d->op_actn_code = 0; //����������        
      }
      break;    
      
    case CE_SAVED_ERR:
      sop_set_operation(SOP_SAVE_CE_ERRORS);
      break;       
  }
  
  //���� ���� �������� ��������� ����, �� ���������� ��������� �� �� ���������� ���������
  if (descriptor == CKPS_PAR)
  {
    ckps_set_edge_type(d->param.ckps_edge_type);
    ckps_set_cogs_btdc(d->param.ckps_cogs_btdc);
    ckps_set_ignition_cogs(d->param.ckps_ignit_cogs);
  }
  
  //���������� ��� ��������� ���������, ����������� ����� CKPS_PAR!
  if (descriptor == KNOCK_PAR)
  {
    //�������������� ��������� ��������� � ������ ���� �� �� �������������, � ������ ��������� ������� ��� ������������.
    if (!d->use_knock_channel_prev && d->param.knock_use_knock_channel)
     if (!knock_module_initialize())
     {//��� ����������� ���������� ��������� ���������� - �������� ��
      ce_set_error(ECUERROR_KSP_CHIP_FAILED);   
     }    

    ckps_set_knock_window(d->param.knock_k_wnd_begin_angle, d->param.knock_k_wnd_end_angle);  
    knock_set_band_pass(edat.param.knock_bpf_frequency);
    ckps_use_knock_channel(d->param.knock_use_knock_channel);   
    
    //���������� ��������� ����� ��� ���� ����� ����� ����� ���� ���������� ����� ����������������
    //��������� ��������� ��� ���.   
    d->use_knock_channel_prev = d->param.knock_use_knock_channel;  
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
   s_timer_set(send_packet_interval_counter, d->param.uart_period_t_ms);
   
   //����� �������� ������� ��� ������
   d->ecuerrors_for_transfer = 0;
  }
 }
}

//������ ������ � EEPROM - ������� ����� ���������. �� ����� ��������� ����������� � ����������� ���������.
//���������� ������ � EEPROM ���������� ������ ���� �� �������� ����� �� ��������� �� ����� �������� ������ ����������
//�� UART-a � ����������� ��������� ���������� �� �������.        
void save_param_if_need(ecudata* d)
{   
 //��������� �� ���������� �� �������� �����?
 if (s_timer16_is_action(save_param_timeout_counter)) 
 {
  //������� � ����������� ��������� ����������?
  if (memcmp(eeprom_parameters_cache,&d->param,sizeof(params)-PAR_CRC_SIZE))   
   sop_set_operation(SOP_SAVE_PARAMETERS);       
  s_timer16_set(save_param_timeout_counter, SAVE_PARAM_TIMEOUT_VALUE);
 }   
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
   
   if (crc16((uint8_t*)&d->param,(sizeof(params)-PAR_CRC_SIZE))!=d->param.crc)
   {
     memcpy_P(&d->param,&def_param,sizeof(params));
     ce_set_error(ECUERROR_EEPROM_PARAM_BROKEN);
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

void advance_angle_state_machine(int16_t* padvance_angle_inhibitor_state, ecudata* d)
{
 switch(d->engine_mode)
 {
  case EM_START: //����� �����
   if (d->sens.inst_frq > d->param.smap_abandon)
   {                   
    d->engine_mode = EM_IDLE;    
    idling_regulator_init();    
   }      
   d->curr_angle=start_function(d);               //������� ��� - ������� ��� �����
   d->airflow = 0;                                //� ������ ����� ��� �������
   *padvance_angle_inhibitor_state = d->curr_angle;//� ������ ����� ������ ��������
   break;     
              
  case EM_IDLE: //����� ��������� ����
   if (d->sens.carb)//������ ���� ������ - � ������� �����
   {
    d->engine_mode = EM_WORK;
   }             
   work_function(d, 1);                           //��������� �������� ������� ������� 
   d->curr_angle = idling_function(d);            //������� ��� - ������� ��� �� 
   d->curr_angle+=coolant_function(d);            //��������� � ��� ������������� ���������
   d->curr_angle+=idling_pregulator(d,&idle_period_time_counter);//��������� �����������
   break;            
                                             
  case EM_WORK: //������� ����� 
   if (!d->sens.carb)//������ ���� ��������� - � ���������� ����� ��
   {
    d->engine_mode = EM_IDLE;
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
      
void init_ecu_data(ecudata* d)
{
 edat.op_comp_code = 0;
 edat.op_actn_code = 0;
 edat.sens.inst_frq = 0;
 edat.curr_angle = 0;
 edat.knock_retard = 0;
 edat.ecuerrors_for_transfer = 0;
 edat.eeprom_parameters_cache = &eeprom_parameters_cache[0];
 edat.engine_mode = EM_START;   
}      
            
__C_task void main(void)
{
  uint8_t turnout_low_priority_errors_counter = 255;
  int16_t advance_angle_inhibitor_state = 0;
  retard_state_t retard_state;     
  
  //���������� ��������� ������ ���������� ��������� �������
  init_ecu_data(&edat);
  knklogic_init(&retard_state);
    
  init_io_ports();
  
  //���� ��� ��������� �������� - �������� ��
  if (crc16f(0,CODE_SIZE)!=code_crc)
   ce_set_error(ECUERROR_PROGRAM_CODE_BROKEN); 

  adc_init();

  //��������� ���������� - �� ��� �� �����  
  disable_comparator();             

  //�������� ��������� ������ ��������� �������� ��� ������������� ������
  meas_initial_measure(&edat);   
   
  //������� ���������� ��������
  SET_STARTER_BLOCKING_STATE(0); 
     
  //������ ���������
  load_eeprom_params(&edat);
   
  s_timer_init();
  
  //�������������� UART
  uart_init(edat.param.uart_divisor);
  
  //�������������� ������ ����             
  ckps_init_state();  
  ckps_set_edge_type(edat.param.ckps_edge_type);
  ckps_set_cogs_btdc(edat.param.ckps_cogs_btdc);
  ckps_set_ignition_cogs(edat.param.ckps_ignit_cogs);
  ckps_set_knock_window(edat.param.knock_k_wnd_begin_angle,edat.param.knock_k_wnd_end_angle);  
  ckps_use_knock_channel(edat.param.knock_use_knock_channel);
    
  //��������� ��������� ����������            
  __enable_interrupt();    

  //��������������� ������������� ���������� ����������� ���������� ���������
  knock_set_band_pass(edat.param.knock_bpf_frequency);
  knock_set_gain(fwdata.attenuator_table[0]);
  knock_set_int_time_constant(edat.param.knock_int_time_const);

  if (edat.param.knock_use_knock_channel)
   if (!knock_module_initialize())
   {//��� ����������� ���������� ��������� ���������� - �������� ��
    ce_set_error(ECUERROR_KSP_CHIP_FAILED);   
   }
  edat.use_knock_channel_prev = edat.param.knock_use_knock_channel;  

  sop_init_operations();
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
     edat.engine_mode = EM_START; //����� ����� 	      
     SET_STARTER_BLOCKING_STATE(0); //������� ���������� ��������
     
     if (edat.param.knock_use_knock_channel)
      knock_start_settings_latching();     
    }
      
    //��������� ��������� ���, ����� ������ ���������� �������. ��� ����������� ������� ��������
    //����� ���� ������ ��������������������. ����� �������, ����� ������� �������� ��������� ��������
    //������������ ��������, ��� ������� ���������� ����������.
    if (s_timer_is_action(force_measure_timeout_counter))
    {
     if (!edat.param.knock_use_knock_channel)
     {
      __disable_interrupt();
       adc_begin_measure();            
      __enable_interrupt();     
     }
     else
     {     
      //���� ������ ���������� �������� �������� � HIP, �� ����� ��������� �� ����������.
      while(!knock_is_latching_idle());
      __disable_interrupt();
      //�������� ����� �������������� � ���� ����� 20���, ���� ���������� ������ ������������� (����������
      //�� ��� ������ ������ �� ��������). � ������ ������ ��� ������ ��������� � ���, ��� �� ������ ����������
      //������������ 20-25���, ��� ��� ��� ���������� �� ����� ��������� ��������.  
      knock_set_integration_mode(KNOCK_INTMODE_INT);
      __delay_cycles(350);     
      knock_set_integration_mode(KNOCK_INTMODE_HOLD);    
      adc_begin_measure_all(); //�������� ������ � �� ����            
      __enable_interrupt();     
     }
          
     s_timer_set(force_measure_timeout_counter, FORCE_MEASURE_TIMEOUT_VALUE);
     meas_update_values_buffers(&edat);          
    }      
  
   //----------����������� ����������-----------------------------------------
    //���������� ���������� ��������
    sop_execute_operations(&edat);
    //���������� ������������� � �������������� ����������� ������
    ce_check_engine(&edat, &ce_control_time_counter);
    //��������� ����������/�������� ������ ����������������� �����
    process_uart_interface(&edat);  
    //���������� ����������� ��������
    save_param_if_need(&edat);    
    //������ ���������� ������� �������� ���������
    edat.sens.inst_frq = ckps_calculate_instant_freq();                           
    //���������� ���������� ������� ���������� � ��������� �������
    meas_average_measured_values(&edat);        
    //c�������� ���������� ����� ������� � ����������� ��� �������
    meas_take_discrete_inputs(&edat);
    //���������� ����������
    control_engine_units(&edat);      
    //�� ��������� ������� (��������� ������� - ������ ��������� �����)
    advance_angle_state_machine(&advance_angle_inhibitor_state,&edat);
    //��������� � ��� �����-���������
    edat.curr_angle+=edat.param.angle_corr;       
    
    //---------------------------------------------- 
    //�������� �������� ���������� �� ���������     
    edat.curr_angle-=edat.knock_retard;     
    //---------------------------------------------- 
    
    //������������ ������������ ��� �������������� ���������
    restrict_value_to(&edat.curr_angle, edat.param.min_angle, edat.param.max_angle);  
    //------------------------------------------------------------------------
    
    //��������� �������� ������� ���������� ��������� ������ ��� ������� �������� �����.      
    if (ckps_is_cycle_cutover_r())
    {
     meas_update_values_buffers(&edat);       
     s_timer_set(force_measure_timeout_counter, FORCE_MEASURE_TIMEOUT_VALUE);
    
     //������������ ������� ��������� ���, �� �� ����� ��������� ������ ��� �� ������������ ��������
     //�� ���� ������� ����. 
     edat.curr_angle = advance_angle_inhibitor(edat.curr_angle, &advance_angle_inhibitor_state, edat.param.angle_inc_spead, edat.param.angle_dec_spead);         
         
     //---------------------------------------------- 
     if (edat.param.knock_use_knock_channel)
     {
      knklogic_detect(&edat, &retard_state);
      knklogic_retard(&edat, &retard_state);
     }
     else     
      edat.knock_retard = 0;     
     //----------------------------------------------  
     
     //��������� ��� ��� ���������� � ��������� �� ������� ����� ���������       
     ckps_set_dwell_angle(edat.curr_angle);        
    
     //��������� ��������� ����������� � ����������� �� ��������
     if (edat.param.knock_use_knock_channel)
      knock_set_gain(knock_attenuator_function(&edat));
    
     // ������������� ���� ������ ���������� ��� ������ �������� ��������� 
     //(��� ���������� N-�� ���������� ������)
     if (turnout_low_priority_errors_counter == 1)
     {    
      ce_clear_error(ECUERROR_EEPROM_PARAM_BROKEN);  
      ce_clear_error(ECUERROR_PROGRAM_CODE_BROKEN);        
     }
     if (turnout_low_priority_errors_counter > 0)
      turnout_low_priority_errors_counter--;      
    }   
     
   }//main loop
  //------------------------------------------------------------------------     
}
