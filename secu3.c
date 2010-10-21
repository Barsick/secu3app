/* SECU-3  - An open source, free engine control unit
   Copyright (C) 2007 Alexey A. Shabelnikov. Ukraine, Gorlovka

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   contacts:
              http://secu-3.narod.ru
              email: secu-3@yandex.ru
*/

#include <inavr.h>
#include <ioavr.h>

#include "adc.h"
#include "bitmask.h"
#include "bootldr.h"
#include "ce_errors.h"
#include "ckps.h"
#include "crc16.h"
#include "eeprom.h"
#include "ephh.h"
#include "epm.h"
#include "funconv.h"
#include "jumper.h"
#include "ignlogic.h"
#include "knklogic.h"
#include "knock.h"
#include "magnitude.h"
#include "measure.h"
#include "params.h"
#include "procuart.h"
#include "secu3.h"
#include "starter.h"
#include "suspendop.h"
#include "tables.h"
#include "uart.h"
#include "ventilator.h"
#include "vstimer.h"
 
struct ecudata_t edat;

//���������� ���������� ������ ��������� � ���������� ������ � ��������� 
//��������� �����������, �������� �������, ������� ����
void control_engine_units(struct ecudata_t *d)
{
 //���������� ������� ����. 
 ephh_control(d);

 //���������� ����������� ��������
 starter_control(d);
 
 //���������� ������� ������������ ���������� ���������, ��� ������� ��� ���� ������������ � ������� 
 vent_control(d);
  
 //���������� ��� (����������� ���������� �������)
 epm_control(d);
}
         
void init_ecu_data(struct ecudata_t* d)
{
 edat.op_comp_code = 0;
 edat.op_actn_code = 0;
 edat.sens.inst_frq = 0;
 edat.curr_angle = 0;
 edat.knock_retard = 0;
 edat.ecuerrors_for_transfer = 0;
 edat.eeprom_parameters_cache = &eeprom_parameters_cache[0];
 edat.engine_mode = EM_START;
 edat.ce_state = 0;   
}      
            
__C_task void main(void)
{
 uint8_t turnout_low_priority_errors_counter = 255;
 int16_t advance_angle_inhibitor_state = 0;
 retard_state_t retard_state;     
  
 //���������� ��������� ������ ���������� ��������� �������
 init_ecu_data(&edat);
 knklogic_init(&retard_state);
    
 //������������� ����� �����/������
 ckps_init_ports();
 vent_init_ports();
 epm_init_ports();
 ephh_init_ports();
 starter_init_ports();
 ce_init_ports();
 knock_init_ports();
 jumper_init_ports();
  
 //���� ��� ��������� �������� - �������� ��
 if (crc16f(0, CODE_SIZE)!=code_crc)
  ce_set_error(ECUERROR_PROGRAM_CODE_BROKEN); 

 //������ ���������
 load_eeprom_params(&edat);

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

 adc_init();

 //�������� ��������� ������ ��������� �������� ��� ������������� ������
 meas_initial_measure(&edat);   
   
 //������� ���������� ��������
 starter_set_blocking_state(0); 
        
 s_timer_init();
  
 //�������������� UART
 uart_init(edat.param.uart_divisor);
  
 //�������������� ������ ����              
 ckps_init_state();  
 ckps_set_cyl_number(edat.param.ckps_engine_cyl);   
 ckps_set_edge_type(edat.param.ckps_edge_type);
 ckps_set_cogs_btdc(edat.param.ckps_cogs_btdc); //<--only partial initialization
 ckps_set_ignition_cogs(edat.param.ckps_ignit_cogs);
 ckps_set_knock_window(edat.param.knock_k_wnd_begin_angle,edat.param.knock_k_wnd_end_angle);  
 ckps_use_knock_channel(edat.param.knock_use_knock_channel);
 ckps_set_cogs_btdc(edat.param.ckps_cogs_btdc); //<--now valid initialization
    
 vent_init_state();   
    
 //��������� ��������� ����������            
 __enable_interrupt();    

 sop_init_operations();
 //------------------------------------------------------------------------     
 while(1)
 {                        
  if (ckps_is_cog_changed())
   s_timer_set(engine_rotation_timeout_counter,ENGINE_ROTATION_TIMEOUT_VALUE);    
     
  if (s_timer_is_action(engine_rotation_timeout_counter))
  { //��������� ����������� (��� ������� ���� �����������)
   ckps_init_state_variables();
   edat.engine_mode = EM_START; //����� ����� 	      
   starter_set_blocking_state(0); //������� ���������� ��������
     
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
   ckps_set_advance_angle(edat.curr_angle);        
    
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
