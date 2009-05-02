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
#include "magnitude.h"

#include "secu3.h"  
#include "knock.h"

#ifndef WHEEL_36_1 //60-2
 #define WHEEL_COGS_NUM   60  //���������� ������ (������� �������������)
 #define WHEEL_COGS_LACK  2   //���������� ������������� ������
 #define WHEEL_LATCH_BTDC 11  //���-�� ������ �� �.�.� ������������ ������ �������� ���, ����� ��������� ��������, �������� �������� � HIP  
 #define CKPS_GAP_BARRIER(p) (((p) << 1) + ((p)>>1))  // p * 2.5,  ������ ��� �������� ����������� = 2.5
#else //36-1
 #define WHEEL_COGS_NUM   36  
 #define WHEEL_COGS_LACK  1   
 #define WHEEL_LATCH_BTDC 7   //70 ��������  
 #define CKPS_GAP_BARRIER(p) ((p) + ((p)>>1))  // p * 1.5
#endif
  
//���������� �������� ������������ �� ���� ��� �����
#define CKPS_DEGREES_PER_COG (360 / WHEEL_COGS_NUM)                           
    
//���-�� ������ ����� ������������ �� ���� ������� ���� ��������� (4-� ������������)
#define WHEEL_COGS_PER_CYCLE (WHEEL_COGS_NUM / 2)

//����� ���������� (�������������) ����, ��������� ���������� � 1! 
#define WHEEL_LAST_COG (WHEEL_COGS_NUM - WHEEL_COGS_LACK)

//���������� ����� ������� ����� ����������� ��� ������ ����� ��������������
#define CKPS_ON_START_SKIP_COGS      30

#define GetICR() (ICR1)

#define CKPS_CHANNEL_MODE14  0
#define CKPS_CHANNEL_MODE23  1
#define CKPS_CHANNEL_MODENA  2

typedef struct
{
  uint8_t  ckps_error_flag:1;                   //������� ������ ����, ��������������� � ���������� �� ����, ������������ ����� ���������
  uint8_t  ckps_is_valid_half_turn_period:1;
  uint8_t  ckps_is_synchronized:1;
  uint8_t  ckps_new_engine_cycle_happen:1;      //���� ������������� � ���������  
  uint8_t  ckps_use_knock_channel:1;            //������� ������������� ������ ���������     
}CKPSFLAGS;

typedef struct
{
  uint16_t icr_prev;                   //���������� �������� �������� �������
  uint16_t period_curr;                //�������� ���������� ��������� ������
  uint16_t period_prev;                //���������� �������� ���������� �������
  uint8_t  cog;                        //������� ����� ����� ������, �������� ������� � 1
  uint16_t measure_start_value;        //���������� �������� �������� ������� ��� ��������� ������� �����������
  uint16_t current_angle;              //����������� �������� ��� ��� ����������� ������� ����
  uint8_t  ignition_pulse_cogs_14;     //����������� ����� �������� ��������� ��� ��������� 1-4
  uint8_t  ignition_pulse_cogs_23;     //����������� ����� �������� ��������� ��� ��������� 2-3
  uint16_t half_turn_period;           //������ ��������� ��������� ������� ����������� n ������  
  int16_t  advance_angle;              //��������� ��� * ANGLE_MULTIPLAYER
  int16_t  advance_angle_buffered;
  uint8_t  ignition_cogs;              //���-�� ������ ������������ ������������ ��������� ������� ������������
  uint8_t  cogs_latch14;
  uint8_t  cogs_latch23;
  uint8_t  cogs_btdc14;
  uint8_t  cogs_btdc23;
  uint8_t  starting_mode;
  uint8_t  channel_mode;
  uint8_t  cogs_btdc;    
  int8_t   knock_wnd_begin_abs;         // ������ ���� ������� �������� ��������� � ������ ����� ������������ �.�.� 
  int8_t   knock_wnd_end_abs;           // ����� ���� ������� �������� ��������� � ������ ����� ������������ �.�.�  
  uint8_t  knock_wnd_begin;             
  uint8_t  knock_wnd_end;               
}CKPSSTATE;
 
CKPSSTATE ckps;

//��������� � ��������� ��������� �����/������
__no_init volatile CKPSFLAGS flags@0x22;

//��� ���������� �������/�������� 0 �� 16 ��������, ���������� R15
__no_init __regvar uint8_t TCNT0_H@15;

//�������������� ���������� ��������� ����
__monitor
void ckps_init_state_variables(void)
{
  ckps.cog = 0;
  //��� ������ �� ���������� ����� ������������ ����� �������� ������� ��������� ��� ��������� 1-4
  ckps.ignition_pulse_cogs_14 = 128; 
  ckps.ignition_pulse_cogs_23 = 128; 
  ckps.half_turn_period = 0xFFFF;                 
  ckps.advance_angle = 0;
  ckps.advance_angle_buffered = 0;
  ckps.starting_mode = 0;
  ckps.channel_mode = CKPS_CHANNEL_MODENA;
    
  flags.ckps_new_engine_cycle_happen = 0;
  flags.ckps_is_synchronized = 0;  
  TCCR0 = 0; //������������� ������0  
}

//������������� ��������� ������/��������� ���� � ������ �� ������� �� ������� 
__monitor
void ckps_init_state(void)
{
  ckps_init_state_variables();
  flags.ckps_error_flag = 0; 
  
  //OC1�(PD5) � OC1�(PD4) ������ ���� ���������������� ��� ������
  DDRD|= (1<<DDD5)|(1<<DDD4); 

  //��� ���������� ����� �������������� ������ ������� � ���������� ��� ������������ ����� ������ 
  TCCR1A = (1<<COM1A1)|(1<<COM1B1)|(1<<FOC1A)|(1<<FOC1B); 
  
  //���������� ����, �������� ����� �������, clock = 250kHz
  TCCR1B = (1<<ICNC1)|(1<<ICES1)|(1<<CS11)|(1<<CS10);       

  //��������� ���������� �� ������� � ��������� � � � ������� 1, � ����� �� ������������ ������� 0
  TIMSK|= (1<<TICIE1)|(1<<OCIE1A)|(1<<OCIE1B)|(1<<TOIE0);
}

//������������� ��� ��� ���������� � ���������
__monitor
void ckps_set_dwell_angle(int16_t angle)
{
  ckps.advance_angle_buffered = angle;
}

//������������ ���������� ������� �������� ��������� �� ����������� ������� ����������� 30 ������ �����.
//������ � ��������� ������� (���� �������� = 4���), � ����� ������ 60 ���, � ����� ������� 1000000 ���, ������:
uint16_t ckps_calculate_instant_freq(void)
{
  uint16_t period;
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
__monitor
void ckps_set_edge_type(uint8_t edge_type)
{
  if (edge_type)
    TCCR1B|= (1<<ICES1);
  else
    TCCR1B&=~(1<<ICES1);
}

__monitor
void ckps_set_cogs_btdc(uint8_t cogs_btdc)
{
  //������� ��������� � ��������� ������� ����� (�����) 
  ckps.cogs_latch14 = cogs_btdc - WHEEL_LATCH_BTDC;
  ckps.cogs_latch23 = cogs_btdc + (WHEEL_COGS_PER_CYCLE - WHEEL_LATCH_BTDC);
  ckps.cogs_btdc14  = cogs_btdc;
  ckps.cogs_btdc23  = cogs_btdc + WHEEL_COGS_PER_CYCLE;  
  ckps.knock_wnd_begin = cogs_btdc + ckps.knock_wnd_begin_abs;
  ckps.knock_wnd_end = cogs_btdc + ckps.knock_wnd_end_abs;
  ckps.cogs_btdc = cogs_btdc;
}

//������������� ������������ �������� ��������� � ������
__monitor
void ckps_set_ignition_cogs(uint8_t cogs)
{
  ckps.ignition_cogs = cogs;
}

uint8_t ckps_is_error(void)
{
 return flags.ckps_error_flag;
}

void ckps_reset_error(void)
{
 flags.ckps_error_flag = 0;
}

void ckps_use_knock_channel(uint8_t use_knock_channel)
{
 flags.ckps_use_knock_channel = use_knock_channel;
}

//��� ������� ���������� 1 ���� ��� ����� ���� ��������� � ����� ���������� �������!
__monitor
uint8_t ckps_is_cycle_cutover_r()
{
 uint8_t result;
 result = flags.ckps_new_engine_cycle_happen;
 flags.ckps_new_engine_cycle_happen = 0;
 return result;
}

__monitor
uint8_t ckps_get_current_cog(void)
{
 return ckps.cog;
}

__monitor
uint8_t ckps_is_cog_changed(void)
{
 static uint8_t prev_cog = 0;
 if (prev_cog!=ckps.cog)
  {
  prev_cog = ckps.cog; 
  return 1;
  }
 return 0;
}

void ckps_set_knock_window(int16_t begin, int16_t end)
{
 uint8_t _t;
 //��������� �� �������� � �����
 ckps.knock_wnd_begin_abs = begin / (CKPS_DEGREES_PER_COG * ANGLE_MULTIPLAYER);
 ckps.knock_wnd_end_abs = end / (CKPS_DEGREES_PER_COG * ANGLE_MULTIPLAYER);
 _t=__save_interrupt();
 __disable_interrupt();
 ckps.knock_wnd_begin = ckps.cogs_btdc + ckps.knock_wnd_begin_abs;
 ckps.knock_wnd_end = ckps.cogs_btdc + ckps.knock_wnd_end_abs;
 __restore_interrupt(_t);
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
  
//������������� ������� 0 ��������� ��������� � ������, clock = 250kHz.
//�������������� ��� ����� ���� ������� ����� ����������� ��� ����������� �����������.
#pragma inline
void set_timer0(uint16_t value)
 {                            
  TCNT0_H = GETBYTE(value, 1);            
  TCNT0 = 255 - GETBYTE(value, 0);      
  TCCR0  = (1<<CS01)|(1<<CS00);
 }    

//��������������� �������, ������������ �� ����� �����
//���������� 1 ����� ������������� ��������, ����� 0.
uint8_t sync_at_startup(void) 
{
 switch(ckps.starting_mode)
   {
   case 0: //������� ������������� ���-�� ������
    /////////////////////////////////////////
    flags.ckps_is_valid_half_turn_period = 0;
    /////////////////////////////////////////
    if (ckps.cog >= CKPS_ON_START_SKIP_COGS) 
     ckps.starting_mode = 1;
    break;    
   case 1: //����� �����������
    if (ckps.period_curr > CKPS_GAP_BARRIER(ckps.period_prev)) 
    {
     flags.ckps_is_synchronized = 1;
     ckps.cog = 1; //1-� ���
     return 1; //����� �������� �������������
    }
    break;
   }
 ckps.icr_prev = GetICR();
 ckps.period_prev = ckps.period_curr;  
 ckps.cog++; 
 return 0; //����������� �������� �������������
} 

//���������. ���������� ��� ���� ������ ����� (������������ � ����������������)
void process_ckps_cogs(void)
{
  uint16_t diff;
  
  if (flags.ckps_use_knock_channel)
  {
   //�������� ������� ��������� (�������� ����)
   if (ckps.cog == ckps.knock_wnd_begin)
   {
    knock_set_integration_mode(KNOCK_INTMODE_INT);
   }
  
   //����������� ������� ��������� (�������� ����) � ��������� ������� ��������� 
   if (ckps.cog == ckps.knock_wnd_end)
   {
    knock_set_integration_mode(KNOCK_INTMODE_HOLD); 
    adc_begin_measure_knock(); 
   }
  }
  
  //�� 66 �������� �� �.�.� ����� ������� ������ ������������� ����� ��� ��� ����������, ���
  //�� ����� �������� �� ��������� ������.
  if (ckps.cog == ckps.cogs_latch14)
  {
   //�������� ������ ���� ����������
   ckps.current_angle = ANGLE_MAGNITUDE(CKPS_DEGREES_PER_COG) * WHEEL_LATCH_BTDC;
   ckps.advance_angle = ckps.advance_angle_buffered;
   ckps.channel_mode = CKPS_CHANNEL_MODE14;
   knock_start_settings_latching();//��������� ������� �������� �������� � HIP  
   adc_begin_measure();            //������ �������� ��������� �������� ���������� ������   
  }
  if (ckps.cog == ckps.cogs_latch23)
  {
   //�������� ������ ���� ����������
   ckps.current_angle = ANGLE_MAGNITUDE(CKPS_DEGREES_PER_COG) * WHEEL_LATCH_BTDC;
   ckps.advance_angle = ckps.advance_angle_buffered;
   ckps.channel_mode = CKPS_CHANNEL_MODE23;
   knock_start_settings_latching();//��������� ������� �������� �������� � HIP   
   adc_begin_measure();            //������ �������� ��������� �������� ���������� ������
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
   /////////////////////////////////////////
  }

  //���������� � ������� ��������� ��� �������� ������
 switch(ckps.channel_mode)
  {
  case CKPS_CHANNEL_MODE14:
   diff = ckps.current_angle - ckps.advance_angle;
   if (diff <= (ANGLE_MAGNITUDE(CKPS_DEGREES_PER_COG) * 2))
   {
   //�� ������� ��������� �������� ��������� ������ 2-x �����. ���������� ����������� ������ ���������
   prepare_channel14(GetICR() + ((uint32_t)diff * (ckps.period_curr)) / ANGLE_MAGNITUDE(CKPS_DEGREES_PER_COG));       
   ckps.ignition_pulse_cogs_14 = 0;   
   ckps.channel_mode = CKPS_CHANNEL_MODENA;
   }
   break;
   
  case CKPS_CHANNEL_MODE23:
   diff = ckps.current_angle - ckps.advance_angle;
   if (diff <= (ANGLE_MAGNITUDE(CKPS_DEGREES_PER_COG) * 2))
   {
    //�� ������� ��������� �������� ��������� ������ 2-x �����. ���������� ����������� ������ ���������
    prepare_channel23(GetICR() + ((uint32_t)diff * (ckps.period_curr)) / ANGLE_MAGNITUDE(CKPS_DEGREES_PER_COG));    
    ckps.ignition_pulse_cogs_23 = 0;   
   ckps.channel_mode = CKPS_CHANNEL_MODENA;
   }
   break;
  }

  if (ckps.ignition_pulse_cogs_14 >= ckps.ignition_cogs)
    TCCR1A|= (1<<FOC1B); //����� �������� ������� ��������� ��� ��������� 1-4

  if (ckps.ignition_pulse_cogs_23 >= ckps.ignition_cogs)
    TCCR1A|= (1<<FOC1A); //����� �������� ������� ��������� ��� ��������� 2-3

  //������ ��� - ���� �� �.�.�. ���������� �� 6 ����.
  ckps.current_angle-= ANGLE_MAGNITUDE(CKPS_DEGREES_PER_COG);  
  ckps.cog++;
  ckps.ignition_pulse_cogs_14++;
  ckps.ignition_pulse_cogs_23++;
}



//���������� �� ������� ������� 1 (���������� ��� ����������� ���������� ����)
#pragma vector=TIMER1_CAPT_vect
__interrupt void timer1_capt_isr(void)
{    
  ckps.period_curr = GetICR() - ckps.icr_prev;

  //��� ������ ���������, ���������� ������������ ���-�� ������ ��� ������������� 
  //������ ���������� ��������. ����� ���� �����������.
  if (!flags.ckps_is_synchronized)
  {
   if (sync_at_startup())
     goto synchronized_enter;   
   return;
  }

  //������ ������ ��������� �� �����������, � ���� ����� ����������� �����������
  //��������� ��� ���-�� ������ ������������, �� ������������� ������� ������.
  if (ckps.period_curr > CKPS_GAP_BARRIER(ckps.period_prev)) 
  {
   if ((ckps.cog != (WHEEL_COGS_NUM + 1))) //��������� ����� ��������������� �����
     flags.ckps_error_flag = 1; //ERROR               
   ckps.cog = 1; //1-� ���           
  }
  
synchronized_enter:   
  //���� ��������� ��� ����� ������������, �� �������� ������ ������� ��� 
  //�������������� ������������� �����, � �������� �������� ������ ���������� 
  //��������� �������� ���������� �������.
  if (ckps.cog == WHEEL_LAST_COG)
    set_timer0(ckps.period_curr); 
    
  //�������� ���������� ��� ���������� ������
  process_ckps_cogs(); 
  
  ckps.icr_prev = GetICR();
  ckps.period_prev = ckps.period_curr;  
}


//������ ����� ����������� ��������� ������ �� 16-�� �������� � �������� ���������
//��������� ������ �� ��������� �������������� 16-�� ���������� �������.                  
#pragma vector=TIMER0_OVF_vect
__interrupt void timer0_ovf_isr(void)
{
 if (TCNT0_H!=0)  //������� ���� �� �������� ?
 {
  TCNT0 = 0;
  TCNT0_H--;         
 }  
 else  
 {//������ ������� ����������    
 TCCR0 = 0; //������������� ������
 
#ifndef WHEEL_36_1 //60-2
 //��������� ������ ����� ������������ 60-� ���
 if (ckps.cog == 59)
   set_timer0(ckps.period_curr);
#endif
 
 //�������� ���������� ��� ������������� ������
 process_ckps_cogs();
 }
}

