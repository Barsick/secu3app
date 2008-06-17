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
#include "ckps.h"
#include "bitmask.h"
#include "adc.h"

#include "secu3.h"  

/* (c < p*2.5)&&(c > p*1.5) */
//#define CKPS_CHECK_FOR_BAD_GAP(c,p) (((c) < (((p) << 1) + ((p) >> 1))) && ((c) > ((p) + ((p) >> 1))))

/* p * 2, ����������� ������ ��� �������� �����������*/
//#define CKPS_GAP_BARRIER(p) ((p) * 2)               

/* p * 2.5,  ������ ��� �������� ����������� = 2.5 */
#define CKPS_GAP_BARRIER(p) (((p) << 1) + ((p)>>1))  

#define GetICR() (ICR1)
//#define GetICR() ((ICR1 >> 8)||(ICR1 << 8))

typedef struct
{
  unsigned char  ckps_new_engine_cycle_happen:1;      //���� ������������� � ���������
  unsigned char  ckps_returned_to_gap_search:1;       //������� ���� ��� ���� ��������� ��� ����� � �� ����� ��� ��������� � ����� ������ �����������
  unsigned char  ckps_error_flag:1;                   //������� ������ ����, ��������������� � ���������� �� ����, ������������ ����� ���������
  unsigned char  ckps_is_initialized_half_turn_period23:1;
  unsigned char  ckps_delay_prepared:1;
  unsigned char  ckps_gap_occured:1;
}ckps_flags;

typedef struct
{
  unsigned char sm_state;                   //������� ��������� ��������� �������� (��) 
  unsigned int  icr_prev;                   //���������� �������� �������� �������
  unsigned int  period_curr;                //�������� ���������� ��������� ������
  unsigned int  period_prev;                //���������� �������� ���������� �������
  unsigned char cog;                        //������� ����� ����� ������, �������� ������� � 1
  unsigned int  measure_start_value;        //���������� �������� �������� ������� ��� ��������� ������� �����������
  unsigned int  current_angle;              //����������� �������� ��� ��� ����������� ������� ����
  unsigned char ignition_pulse_cogs_14;     //����������� ����� �������� ��������� ��� ��������� 1-4
  unsigned char ignition_pulse_cogs_23;     //����������� ����� �������� ��������� ��� ��������� 2-3
  unsigned int  half_turn_period;           //������ ��������� ��������� ������� ����������� n ������  
  signed   int  ignition_dwell_angle;       //��������� ��� * ANGLE_MULTIPLAYER
  unsigned char ignition_cogs;
}DPKVSTATE;
 
DPKVSTATE ckps;

//��������� � ��������� ��������� �����/������
__no_init volatile ckps_flags f1@0x22;

//������������� ��������� ������/��������� ���� � ������ �� ������� �� ������� 
void ckps_init_state(void)
{
  ckps.sm_state = 0;
  ckps.cog = 0;
  ckps.ignition_pulse_cogs_14 = CKPS_IGNITION_PULSE_COGS;
  ckps.ignition_pulse_cogs_23 = CKPS_IGNITION_PULSE_COGS;
  ckps.half_turn_period = 0xFFFF;                 
  ckps.ignition_dwell_angle = 0;
  ckps.ignition_cogs = CKPS_IGNITION_PULSE_COGS-1;

  //OC1�(PD5) � OC1�(PD4) ������ ���� ���������������� ��� ������
  DDRD|= (1<<DDD5)|(1<<DDD4); 

  //��� ���������� ����� �������������� ������ ������� � ���������� ��� ������������ ����� ������ 
  TCCR1A = (1<<COM1A1)|(1<<COM1B1)|(1<<FOC1A)|(1<<FOC1B); 
  
  //���������� ����, �������� ����� �������, clock = 250kHz
  TCCR1B = (1<<ICNC1)|(1<<ICES1)|(1<<CS11)|(1<<CS10);       

  //��������� ���������� �� ������� � ��������� � � � ������� 1
  TIMSK|= (1<<TICIE1)|(1<<OCIE1A)|(1<<OCIE1B);
}

//������������� ��� ��� ���������� � ���������
void ckps_set_dwell_angle(signed int angle)
{
  __disable_interrupt();    
  ckps.ignition_dwell_angle = angle;
  __enable_interrupt();                
}

//������������ ���������� ������� �������� ��������� �� ����������� ������� ����������� 30 ������ �����.
//������ � ��������� ������� (���� �������� = 4���), � ����� ������ 60 ���, � ����� ������� 1000000 ���, ������:
unsigned int ckps_calculate_instant_freq(void)
{
  unsigned int period;
  __disable_interrupt();
   period = ckps.half_turn_period;           //������������ ��������� ������ � ����������  
  __enable_interrupt();                           

  //���� ����� �������, ������ ��������� ����������� 
  if (period!=0xFFFF)  
    return (7500000L)/(period);
  else
    return 0;
}

//������������� ��� ������ ���� (0 - �������������, 1 - �������������)
void ckps_set_edge_type(unsigned char edge_type)
{
  if (edge_type)
    TCCR1B|= (1<<ICES1);
  else
    TCCR1B&=~(1<<ICES1);
}

//������������� ������������ �������� ��������� � ������
void ckps_set_ignition_cogs(unsigned char cogs)
{
 ckps.ignition_cogs = cogs - 1;
}

unsigned char ckps_is_error(void)
{
 return f1.ckps_error_flag;
}

void ckps_reset_error(void)
{
 f1.ckps_error_flag = 0;
}

//��� ������� ���������� 1 ���� ��� ����� ���� ��������� � ����� ���������� �������!
unsigned char ckps_is_cycle_cutover_r()
{
 unsigned char result;
  __disable_interrupt();
 result = f1.ckps_new_engine_cycle_happen;
 f1.ckps_new_engine_cycle_happen = 0;
  __enable_interrupt();                
 return result;
}

//��� ������� ���������� 1 ���� ��������� ��������� ��������� ��������� �� ���� ������ � ����� ���������� �������
unsigned char ckps_is_rotation_cutover_r(void)
{
 unsigned char result;
  __disable_interrupt();
 result = f1.ckps_gap_occured;
 f1.ckps_gap_occured = 0;
  __enable_interrupt();                
 return result;
}

#pragma vector=TIMER1_COMPA_vect
__interrupt void timer1_compa_isr(void)
{
 //����� � ������� ������, ������ ����������� ����� �� ������� � ������ ������� �� ���������� �������.
 //�������� ������ ������������ �������� �� ������  
  TCCR1A&= (~(1<<COM1A0));
  TCCR1A|= (1<<COM1A1);   
  ckps.ignition_pulse_cogs_23 = 0;
}

#pragma vector=TIMER1_COMPB_vect
__interrupt void timer1_compb_isr(void)
{
 //����� � ������� ������, ������ ����������� ����� �� ������� � ������ ������� �� ���������� �������.  
 //�������� ������ ������������ �������� �� ������
  TCCR1A&= (~(1<<COM1B0));
  TCCR1A|= (1<<COM1B1);   
  ckps.ignition_pulse_cogs_14 = 0;
}

//���������� �� ������� ������� 1 (���������� ��� ����������� ���������� ����)
#pragma vector=TIMER1_CAPT_vect
__interrupt void timer1_capt_isr(void)
{  
  unsigned int diff;
 
  ckps.period_curr = GetICR() - ckps.icr_prev;
  
  //�������� ������� ��� �������������, ��������� �������� �������� ���������, ������� ��������� � ������ �����  
  switch(ckps.sm_state)
  {
   case 0://----------------�������� ����� (���������� ��������� �����)------------------- 
     f1.ckps_gap_occured = 0;
     if (ckps.cog >= CKPS_ON_START_SKIP_COGS) 
      {
       ckps.sm_state = 1;
       f1.ckps_returned_to_gap_search = 0;
       f1.ckps_is_initialized_half_turn_period23 = 0;
      }
     break;

   case 1://-----------------����� �����������--------------------------------------------
     if (ckps.period_curr > CKPS_GAP_BARRIER(ckps.period_prev)) 
     {
      f1.ckps_gap_occured = 1; //������������� ������� ���������� �����������

      if (f1.ckps_returned_to_gap_search)
      {
       if ((ckps.cog != 58)/*||CKPS_CHECK_FOR_BAD_GAP(ckps.period_curr,ckps.period_prev)*/)
         f1.ckps_error_flag = 1; //ERROR             
       f1.ckps_returned_to_gap_search = 0;
      }

      ckps.cog = 0;
      ckps.sm_state = 2;
      ckps.ignition_pulse_cogs_14+=2;
      ckps.ignition_pulse_cogs_23+=2;
      f1.ckps_delay_prepared = 0;
    
      //�������� ������ ���� ����������
      ckps.current_angle = (ANGLE_MULTIPLAYER * CKPS_DEGREES_PER_COG) * (CKPS_COGS_BEFORE_TDC - 1);
     }
     break;

   case 2: //--------------���������� ��� ��� 1-4-----------------------------------------
     //������ ��� - ���� �� �.�.�. ���������� �� 6 ����.
     ckps.current_angle-= ANGLE_MULTIPLAYER * CKPS_DEGREES_PER_COG;

     if (!f1.ckps_delay_prepared)
     {
      diff = ckps.current_angle - ckps.ignition_dwell_angle;
      if (diff <= ((ANGLE_MULTIPLAYER * CKPS_DEGREES_PER_COG) * 2) )
      {
       //�� ������� ��������� �������� ��������� ������ 2-x �����. ���������� ����������� ������ ���������
       OCR1B = GetICR() + ((unsigned long)diff * (ckps.period_curr/* * 2*/)) / ((ANGLE_MULTIPLAYER * CKPS_DEGREES_PER_COG)/* * 2*/);  
     
       //���������� ���� ����������, �������� ����� ��������� ����� B � ������� ������� ��� ����������
       SETBIT(TIFR,OCF1B);
       TCCR1A|= (1<<COM1B1)|(1<<COM1B0);     
       ckps.ignition_pulse_cogs_14 = 0;   
       f1.ckps_delay_prepared = 1;
      }
     }

     if (ckps.cog==2) //������������� ��� ���������� ��������� ������� �������� ��� 2-3
     {
      //���� ���� ������������ �� ������������� ����������� ��������� �����
      ckps.half_turn_period = ((ckps.period_curr > 1250) || !f1.ckps_is_initialized_half_turn_period23) 
                              ? 0xFFFF : (GetICR() - ckps.measure_start_value);             
      ckps.measure_start_value = GetICR();
      f1.ckps_new_engine_cycle_happen = 1;      //������������� ������� �������� ������������� 
      adc_begin_measure();                 //������ �������� ��������� �������� ���������� ������        
     }

     if (ckps.cog == 30) //������� � ����� ���������� ��� ��� 2-3
     {
      //�������� ������ ���� ����������
      ckps.current_angle = (ANGLE_MULTIPLAYER * CKPS_DEGREES_PER_COG) * (CKPS_COGS_BEFORE_TDC - 1);
      ckps.sm_state = 3;
      f1.ckps_delay_prepared = 0;
     }

     if (ckps.period_curr > 12500) //������� ����������� ���� ������� ������ - ��������� �����������
       ckps.sm_state = 0;

     break;

   case 3: //--------------���������� ��� ��� 2-3-----------------------------------------
     //������ ��� - ���� �� �.�.�. ���������� �� 6 ����.
     ckps.current_angle-= ANGLE_MULTIPLAYER * CKPS_DEGREES_PER_COG;

     if (!f1.ckps_delay_prepared)
     {
      diff = ckps.current_angle - ckps.ignition_dwell_angle;
      if (diff <= ((ANGLE_MULTIPLAYER * CKPS_DEGREES_PER_COG) * 2) )
      {
       //�� ������� ��������� �������� ��������� ������ 2-x �����. ���������� ����������� ������ ���������
       OCR1A = GetICR() + ((unsigned long)diff * (ckps.period_curr/* * 2*/)) / ((ANGLE_MULTIPLAYER * CKPS_DEGREES_PER_COG) /** 2*/);    

       //���������� ���� ����������, �������� ����� ��������� ����� � � ������� ������� ��� ����������
       SETBIT(TIFR,OCF1A);
       TCCR1A|= (1<<COM1A1)|(1<<COM1A0);      
       ckps.ignition_pulse_cogs_23 = 0;   
       f1.ckps_delay_prepared = 1;
      }
     }

     if (ckps.cog == 32) //������������� ��� ���������� ��������� ������� �������� ��� 1-4
     {
      //���� ���� ������������ �� ������������� ����������� ��������� �����
      ckps.half_turn_period = (ckps.period_curr > 1250) ? 0xFFFF : (GetICR() - ckps.measure_start_value);             
      ckps.measure_start_value = GetICR();    
      f1.ckps_new_engine_cycle_happen = 1;      //������������� ������� �������� ������������� 
      f1.ckps_is_initialized_half_turn_period23 = 1;
      adc_begin_measure();                 //������ �������� ��������� �������� ���������� ������
     } 

     if (ckps.cog > 55) //������� � ����� ������ �����������
     {
      ckps.sm_state = 1;
      f1.ckps_returned_to_gap_search = 1; 
      f1.ckps_delay_prepared = 0;
     }

     if (ckps.period_curr > 12500) //������� ����������� ���� ������� ������ - ��������� �����������
       ckps.sm_state = 0;

     break;
  }
   
  if (ckps.ignition_pulse_cogs_14 >= ckps.ignition_cogs)
    TCCR1A|= (1<<FOC1B); //����� �������� ������� ��������� ��� ��������� 1-4

  if (ckps.ignition_pulse_cogs_23 >= ckps.ignition_cogs)
    TCCR1A|= (1<<FOC1A); //����� �������� ������� ��������� ��� ��������� 2-3
      
  ckps.icr_prev = GetICR();
  ckps.period_prev = ckps.period_curr;  
  ckps.cog++; 
  ckps.ignition_pulse_cogs_14++; 
  ckps.ignition_pulse_cogs_23++; 
}
