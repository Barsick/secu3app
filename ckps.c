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

// p * 2.5,  ������ ��� �������� ����������� = 2.5 
#define CKPS_GAP_BARRIER(p) (((p) << 1) + ((p)>>1))  

#define GetICR() (ICR1)

typedef struct
{
  unsigned char  ckps_error_flag:1;                   //������� ������ ����, ��������������� � ���������� �� ����, ������������ ����� ���������
  unsigned char  ckps_is_valid_half_turn_period:1;
  unsigned char  ckps_delay_prepared14:1;
  unsigned char  ckps_delay_prepared23:1;
  unsigned char  ckps_is_synchronized:1;
  unsigned char  ckps_new_engine_cycle_happen:1;      //���� ������������� � ���������
  unsigned char  ckps_gap_occured:1;
}CKPSFLAGS;

typedef struct
{
  unsigned int  icr_prev;                   //���������� �������� �������� �������
  unsigned int  period_curr;                //�������� ���������� ��������� ������
  unsigned int  period_prev;                //���������� �������� ���������� �������
  unsigned char cog;                        //������� ����� ����� ������, �������� ������� � 1
  unsigned int  measure_start_value;        //���������� �������� �������� ������� ��� ��������� ������� �����������
  unsigned int  current_angle;              //����������� �������� ��� ��� ����������� ������� ����
  unsigned char ignition_pulse_cogs_14;     //����������� ����� �������� ��������� ��� ��������� 1-4
  unsigned char ignition_pulse_cogs_23;     //����������� ����� �������� ��������� ��� ��������� 2-3
  unsigned int  half_turn_period;           //������ ��������� ��������� ������� ����������� n ������  
  signed   int  advance_angle;              //��������� ��� * ANGLE_MULTIPLAYER
  signed   int  advance_angle_buffered;
  unsigned char ignition_cogs;              //���-�� ������ ������������ ������������ ��������� ������� ������������
  unsigned char cogs_latch14;
  unsigned char cogs_latch23;
  unsigned char cogs_btdc14;
  unsigned char cogs_btdc23;
  unsigned char starting_mode;
}CKPSSTATE;
 
CKPSSTATE ckps;

//��������� � ��������� ��������� �����/������
__no_init volatile CKPSFLAGS flags@0x22;

//������������� ��������� ������/��������� ���� � ������ �� ������� �� ������� 
void ckps_init_state(void)
{
  ckps.cog = 0;
  //��� ������ �� ���������� ����� ������������ ����� �������� ������� ��������� ��� ��������� 1-4
  ckps.ignition_pulse_cogs_14 = 128; 
  ckps.ignition_pulse_cogs_23 = 128; 
  ckps.half_turn_period = 0xFFFF;                 
  ckps.advance_angle = 0;
  ckps.advance_angle_buffered = 0;
  ckps.starting_mode = 0;
  
  flags.ckps_error_flag = 0;
  flags.ckps_new_engine_cycle_happen = 0;
  flags.ckps_gap_occured = 0;
  flags.ckps_is_synchronized = 0;
  
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
  ckps.advance_angle_buffered = angle;
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
  unsigned char _t;
  _t=__save_interrupt();
  __disable_interrupt();
  if (edge_type)
    TCCR1B|= (1<<ICES1);
  else
    TCCR1B&=~(1<<ICES1);
   __restore_interrupt(_t);
}

void ckps_set_cogs_btdc(unsigned char cogs_btdc)
{
  unsigned char _t;
  _t=__save_interrupt();
  __disable_interrupt();
  //11 ������ = 66 ����. �� �.�.�. 
  ckps.cogs_latch14 = cogs_btdc - 11;
  ckps.cogs_latch23 = cogs_btdc + 19;
  ckps.cogs_btdc14  = cogs_btdc;
  ckps.cogs_btdc23  = cogs_btdc + 30;
  __restore_interrupt(_t);
}

//������������� ������������ �������� ��������� � ������
void ckps_set_ignition_cogs(unsigned char cogs)
{
  unsigned char _t;
  _t=__save_interrupt();
  __disable_interrupt();
  ckps.ignition_cogs = cogs;
  __restore_interrupt(_t);
}

unsigned char ckps_is_error(void)
{
 return flags.ckps_error_flag;
}

void ckps_reset_error(void)
{
 flags.ckps_error_flag = 0;
}

//��� ������� ���������� 1 ���� ��� ����� ���� ��������� � ����� ���������� �������!
unsigned char ckps_is_cycle_cutover_r()
{
 unsigned char result;
 __disable_interrupt();
 result = flags.ckps_new_engine_cycle_happen;
 flags.ckps_new_engine_cycle_happen = 0;
 __enable_interrupt();                
 return result;
}

//��� ������� ���������� 1 ���� ��������� ��������� ��������� ��������� �� ���� ������ � ����� ���������� �������
unsigned char ckps_is_rotation_cutover_r(void)
{
 unsigned char result;
 __disable_interrupt();
 result = flags.ckps_gap_occured;
 flags.ckps_gap_occured = 0;
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

//���������� ���� ����������, �������� ����� ��������� ����� B � ������� ������� ��� ����������
#define prepare_channel14(value)     \
  {                                  \
   OCR1B = value;                    \
   SETBIT(TIFR,OCF1B);               \
   TCCR1A|= (1<<COM1B1)|(1<<COM1B0); \
   }

//���������� ���� ����������, �������� ����� ��������� ����� A � ������� ������� ��� ����������
#define prepare_channel23(value)     \
  {                                  \
   OCR1A = value;                    \
   SETBIT(TIFR,OCF1A);               \
   TCCR1A|= (1<<COM1A1)|(1<<COM1A0); \
  }


//���������� �� ������� ������� 1 (���������� ��� ����������� ���������� ����)
#pragma vector=TIMER1_CAPT_vect
__interrupt void timer1_capt_isr(void)
{  
  unsigned int diff;
 
  ckps.period_curr = GetICR() - ckps.icr_prev;

  //��� ������ ���������, ���������� ������������ ���-�� ������ ��� ������������� 
  //������ ���������� ��������. ����� ���� �����������.
  if (!flags.ckps_is_synchronized)
  {
   switch(ckps.starting_mode)
   {
   case 0:
    /////////////////////////////////////////
    flags.ckps_gap_occured = 0;
    flags.ckps_is_valid_half_turn_period = 0;
    /////////////////////////////////////////
    if (ckps.cog >= CKPS_ON_START_SKIP_COGS) 
     ckps.starting_mode = 1;
    break;
   case 1:
    if (ckps.period_curr > CKPS_GAP_BARRIER(ckps.period_prev)) 
    {
     flags.ckps_is_synchronized = 1;
     ckps.cog = 1; //1-� ���
     goto synchronized_enter;
    }
    break;
   }
   ckps.icr_prev = GetICR();
   ckps.period_prev = ckps.period_curr;  
   ckps.cog++; 
   return;
  }

  //������ ������ ��������� �� �����������, � ���� ����� ����������� �����������
  //��������� ��� ���-�� ������ ������������, �� ������������� ������� ������.
  if (ckps.period_curr > CKPS_GAP_BARRIER(ckps.period_prev)) 
  {
   /////////////////////////////////////////
   flags.ckps_gap_occured = 1; //������������� ������� ���������� �����������
   /////////////////////////////////////////
   if ((ckps.cog != 59))
     flags.ckps_error_flag = 1; //ERROR             
  
   ckps.cog = 1; //1-� ���         
   ckps.ignition_pulse_cogs_14+=2;
   ckps.ignition_pulse_cogs_23+=2;
  }
  else
  {
   //���� ��� �� ����������� � ������� ����������� ���� ������������� ������ - ��������� �����������,
   //�� ��������� � ����� ������� � �������������
   if (ckps.period_curr > 12500) 
   {
    ckps.cog = 0; //�� ������ ������ ���������� �����
    ckps.starting_mode = 0;
    flags.ckps_is_synchronized = 0;       
    return;
   }
  }

synchronized_enter:     

  //�� 66 �������� �� �.�.� ����� ������� ������ ������������� ����� ��� ��� ����������, ���
  //�� ����� �������� �� ��������� ������.
  if (ckps.cog == ckps.cogs_latch14)
  {
   //�������� ������ ���� ����������
   ckps.current_angle = ANGLE_MAGNITUDE(CKPS_DEGREES_PER_COG) * 11;
   ckps.advance_angle = ckps.advance_angle_buffered;
   flags.ckps_delay_prepared14 = 0;
   //knock_start_settings_latching();    nearest future!!!
   //adc_begin_measure();                nearest future!!!   
  }
  if (ckps.cog == ckps.cogs_latch23)
  {
   //�������� ������ ���� ����������
   ckps.current_angle = ANGLE_MAGNITUDE(CKPS_DEGREES_PER_COG) * 11;
   ckps.advance_angle = ckps.advance_angle_buffered;
   flags.ckps_delay_prepared23 = 0;
   //knock_start_settings_latching();    nearest future!!!
   //adc_begin_measure();                nearest future!!!   
  }

  
  //������������� ����� ����������/������ ��������� �������� ��������  - �.�.�.   
  //���������� � ���������� ����������� �������, ����� ������������ �������� �������� ��������
  //��� ���������� ��������� 
  if (ckps.cog==ckps.cogs_btdc14 || ckps.cog==ckps.cogs_btdc23) 
  {
   //���� ���� ������������ �� ������������� ����������� ��������� �����
   if (((ckps.period_curr > 1250) || !flags.ckps_is_valid_half_turn_period))
    ckps.half_turn_period = 0xFFFF;
   else
    {     
     ckps.half_turn_period = (GetICR() - ckps.measure_start_value);
    }
   ckps.measure_start_value = GetICR();
   flags.ckps_is_valid_half_turn_period = 1;
   /////////////////////////////////////////
   flags.ckps_new_engine_cycle_happen = 1; //������������� ������� �������� ������������� 
   adc_begin_measure();                    //������ �������� ��������� �������� ���������� ������
   /////////////////////////////////////////
  }

  //���������� � ������� ��������� ��� �������� ������
  if (!flags.ckps_delay_prepared14)
  {
   diff = ckps.current_angle - ckps.advance_angle;
   if (diff <= (ANGLE_MAGNITUDE(CKPS_DEGREES_PER_COG) * 2))
   {
   //�� ������� ��������� �������� ��������� ������ 2-x �����. ���������� ����������� ������ ���������
   prepare_channel14(GetICR() + ((unsigned long)diff * (ckps.period_curr)) / ANGLE_MAGNITUDE(CKPS_DEGREES_PER_COG));       
   ckps.ignition_pulse_cogs_14 = 0;   
   flags.ckps_delay_prepared14 = 1;
   }
  }
  if (!flags.ckps_delay_prepared23)
  {
   diff = ckps.current_angle - ckps.advance_angle;
   if (diff <= (ANGLE_MAGNITUDE(CKPS_DEGREES_PER_COG) * 2))
   {
    //�� ������� ��������� �������� ��������� ������ 2-x �����. ���������� ����������� ������ ���������
    prepare_channel23(GetICR() + ((unsigned long)diff * (ckps.period_curr)) / ANGLE_MAGNITUDE(CKPS_DEGREES_PER_COG));    
    ckps.ignition_pulse_cogs_23 = 0;   
    flags.ckps_delay_prepared23 = 1;
   }
  }

  if (ckps.ignition_pulse_cogs_14 >= ckps.ignition_cogs)
    TCCR1A|= (1<<FOC1B); //����� �������� ������� ��������� ��� ��������� 1-4

  if (ckps.ignition_pulse_cogs_23 >= ckps.ignition_cogs)
    TCCR1A|= (1<<FOC1A); //����� �������� ������� ��������� ��� ��������� 2-3

  //������ ��� - ���� �� �.�.�. ���������� �� 6 ����.
  ckps.current_angle-= ANGLE_MAGNITUDE(CKPS_DEGREES_PER_COG);
  ckps.icr_prev = GetICR();
  ckps.period_prev = ckps.period_curr;  
  ckps.cog++;
  ckps.ignition_pulse_cogs_14++;
  ckps.ignition_pulse_cogs_23++; 
}
