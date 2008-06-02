 /****************************************************************
 *       SECU-3  - An open source, free engine control unit
 *    Designed by Alexey A. Shabelnikov. Ukraine, Gorlovka 2007.
 *       Microprocessors systems - design & programming.
 *    contacts:
 *              http://secu-3.narod.ru
 *              ICQ: 405-791-931
 ****************************************************************/

#include <inavr.h>
#include <iom16.h>
#include "dpkv.h"
#include "bitmask.h"
#include "adc.h"

#include "secu3.h"  //������ ��� ������!

extern unsigned char force_measure_timeout_counter;
extern unsigned char engine_stop_timeout_counter;


typedef struct
{
  unsigned char sm_state;                   //������� ��������� ��������� �������� (��) 
  unsigned int icr_prev;                    //���������� �������� �������� �������
  unsigned int period_curr;                 //�������� ���������� ��������� ������
  unsigned int period_prev;                 //���������� �������� ���������� �������
  unsigned char cog;                        //������� ����� ����� ������, �������� ������� � 1
  unsigned int measure_start_value;         //���������� �������� �������� ������� ��� ��������� ������� �����������
  unsigned int current_angle;               //����������� �������� ��� ��� ����������� ������� ����
  unsigned char ignition_pulse_teeth;       //����������� ����� �������� ��������� 
  unsigned int  half_turn_period;           //������ ��������� ��������� ������� ����������� n ������  
  signed   int  ignition_dwell_angle;       //��������� ��� * ANGLE_MULTIPLAYER
}DPKVSTATE;
 
DPKVSTATE dpkv;

//������������� ��������� ������/��������� ���� 
void dpkv_init_state(void)
{
  dpkv.sm_state = 0;
  dpkv.cog = 0;
  dpkv.ignition_pulse_teeth = DPKV_IGNITION_PULSE_COGS;
  dpkv.half_turn_period = 0xFFFF;                 
  dpkv.ignition_dwell_angle = 0;
}

//������������� ��� ��� ���������� � ���������
void dpkv_set_dwell_angle(signed int angle)
{
  __disable_interrupt();    
  dpkv.ignition_dwell_angle = angle;
  __enable_interrupt();                
}

//������������ ���������� ������� �������� ��������� �� ����������� ������� ����������� 30 ������ �����.
//������ � ��������� ������� (���� �������� = 4���), � ����� ������ 60 ���, � ����� ������� 1000000 ���, ������:
unsigned int dpkv_calculate_instant_freq(void)
{
  unsigned int period;
  __disable_interrupt();
   period = dpkv.half_turn_period;           //������������ ��������� ������ � ����������
  __enable_interrupt();                           

  //���� ����� �������, ������ ��������� ����������� 
  if (period!=0xFFFF)  
    return (7500000L)/(period);
  else
    return 0;
}

#pragma vector=TIMER1_COMPA_vect
__interrupt void timer1_compa_isr(void)
{
 //����� � ������� ������, ������ ����������� ��� ����� �� ������� � ������ ������� �� ���������� �������.
 //�������� ������ ������������ �������� �� ������  
  TCCR1A = (1<<COM1A1)|(1<<COM1B1);   
  dpkv.ignition_pulse_teeth = 0;
}

#pragma vector=TIMER1_COMPB_vect
__interrupt void timer1_compb_isr(void)
{
 //����� � ������� ������, ������ ����������� ��� ����� �� ������� � ������ ������� �� ���������� �������.  
 //�������� ������ ������������ �������� �� ������
  TCCR1A = (1<<COM1A1)|(1<<COM1B1); 
  dpkv.ignition_pulse_teeth = 0;
}

//���������� �� ������� ������� 1 (���������� ��� ����������� ���������� ����)
#pragma vector=TIMER1_CAPT_vect
__interrupt void timer1_capt_isr(void)
{  
  unsigned int diff;
 
  dpkv.period_curr = ICR1 - dpkv.icr_prev;
  
  //�������� ������� ��� �������������, ��������� �������� �������� ���������, ������� ��������� � ������ �����  
  switch(dpkv.sm_state)
  {
   case 0://----------------�������� ����� (���������� ��������� �����)------------------- 
     if (dpkv.cog >= DPKV_ON_START_SKIP_COGS)
      {
       dpkv.sm_state = 1;
       f1.dpkv_returned_to_gap_search = 0;
      }
     break;

   case 1://-----------------����� �����������--------------------------------------------
     if (dpkv.period_curr > dpkv.period_prev)
     {
     force_measure_timeout_counter = FORCE_MEASURE_TIMEOUT_VALUE;  
     engine_stop_timeout_counter = ENGINE_STOP_TIMEOUT_VALUE;
     if (f1.dpkv_returned_to_gap_search)
     {
      if (dpkv.cog != 58)
        f1.dpkv_error_flag = 1; //ERROR             
      f1.dpkv_returned_to_gap_search = 0;
     }

     dpkv.cog = 1;
     dpkv.sm_state = 2;
     dpkv.ignition_pulse_teeth+=2;
    
     //�������� ������ ���� ����������
     dpkv.current_angle = (ANGLE_MULTIPLAYER * DPKV_DEGREES_PER_COG) * (DPKV_COGS_BEFORE_TDC - 1);
     }
     break;

   case 2: //--------------���������� ��� ��� 1-4-----------------------------------------
     //������ ��� - ���� �� �.�.�. ���������� �� 6 ����.
     dpkv.current_angle-= ANGLE_MULTIPLAYER * DPKV_DEGREES_PER_COG;

     diff = dpkv.current_angle - dpkv.ignition_dwell_angle;
     if (diff <= ((ANGLE_MULTIPLAYER * DPKV_DEGREES_PER_COG) * 2) )
     {
     //�� ������� ��������� �������� ��������� ������ 2-x �����. ���������� ����������� ������ ���������
     OCR1B = ICR1 + ((unsigned long)diff * (dpkv.period_curr * 2)) / ((ANGLE_MULTIPLAYER * DPKV_DEGREES_PER_COG) * 2);  
     
     //���������� ���� ����������, �������� ����� ��������� ����� B � ������� �������, � A � ������
     SETBIT(TIFR,OCF1B);
     TCCR1A = (1<<COM1B1)|(1<<COM1B0)|(1<<COM1A1);        
     }

     if (dpkv.cog==2) //������������� ��� ��������� ������� �������� ��� 2-3
     {
     //���� ���� ������������ �� ������������� ����������� ��������� �����
     dpkv.half_turn_period = (dpkv.period_curr > 1250) ? 0xFFFF : (ICR1 - dpkv.measure_start_value);             
     dpkv.measure_start_value = ICR1;
     f1.dpkv_new_engine_cycle_happen = 1;      //������������� ������� �������� ������������� 
     adc_begin_measure();                 //������ �������� ��������� �������� ���������� ������        
     }

     if (dpkv.cog == 30) //������� � ����� ���������� ��� ��� 2-3
     {
     //�������� ������ ���� ����������
     dpkv.current_angle = (ANGLE_MULTIPLAYER * DPKV_DEGREES_PER_COG) * (DPKV_COGS_BEFORE_TDC - 1);
     dpkv.sm_state = 3;
     }
     break;

   case 3: //--------------���������� ��� ��� 2-3-----------------------------------------
     //������ ��� - ���� �� �.�.�. ���������� �� 6 ����.
     dpkv.current_angle-= ANGLE_MULTIPLAYER * DPKV_DEGREES_PER_COG;

     diff = dpkv.current_angle - dpkv.ignition_dwell_angle;
     if (diff <= ((ANGLE_MULTIPLAYER * DPKV_DEGREES_PER_COG) * 2) )
     {
     //�� ������� ��������� �������� ��������� ������ 2-x �����. ���������� ����������� ������ ���������
     OCR1A = ICR1 + ((unsigned long)diff * (dpkv.period_curr * 2)) / ((ANGLE_MULTIPLAYER * DPKV_DEGREES_PER_COG) * 2);    

     //���������� ���� ����������, �������� ����� ��������� ����� � � ������� �������, � � � ������
     SETBIT(TIFR,OCF1A);
     TCCR1A = (1<<COM1A1)|(1<<COM1A0)|(1<<COM1B1);      
     }

     if (dpkv.cog == 32) //������������� ��� ��������� ������� �������� ��� 1-4
     {
     //���� ���� ������������ �� ������������� ����������� ��������� �����
     dpkv.half_turn_period = (dpkv.period_curr > 1250) ? 0xFFFF : (ICR1 - dpkv.measure_start_value);             
     dpkv.measure_start_value = ICR1;    
     f1.dpkv_new_engine_cycle_happen = 1;      //������������� ������� �������� ������������� 
     adc_begin_measure();                 //������ �������� ��������� �������� ���������� ������
     } 

     if (dpkv.cog > 55) //������� � ����� ������ �����������
     {
     dpkv.sm_state = 1;
     f1.dpkv_returned_to_gap_search = 1; 
     }
     break;
  }
    
  if (dpkv.ignition_pulse_teeth >= (DPKV_IGNITION_PULSE_COGS-1))
    TCCR1A = (1<<COM1A1)|(1<<COM1B1)|(1<<FOC1A)|(1<<FOC1B); //����� �������� ������� ��������� 
  
  dpkv.icr_prev = ICR1;
  dpkv.period_prev = dpkv.period_curr * 2;  //����������� ������ ��� �������� �����������
  dpkv.cog++; 
  dpkv.ignition_pulse_teeth++; 
}
