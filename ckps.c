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
#include "ckps.h"
#include "bitmask.h"
#include "adc.h"
#include "magnitude.h"

#include "secu3.h"  
#include "knock.h"

//#ifdef COIL_REGULATION todo: implement

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
    
//����� ���������� (�������������) ����, ��������� ���������� � 1! 
#define WHEEL_LAST_COG (WHEEL_COGS_NUM - WHEEL_COGS_LACK)

//���������� ����� ������� ����� ����������� ��� ������ ����� ��������������
#define CKPS_ON_START_SKIP_COGS      30

#define GetICR() (ICR1)

//�������� 4 ������ ���������.
#define IGN_CHANNELS_MAX     4

//������������ ��� �������� ��� �� ���� ����� ��������� �� ������
#define CKPS_CHANNEL_MODENA  255

typedef struct
{
 uint8_t  ckps_error_flag:1;                   //������� ������ ����, ��������������� � ���������� �� ����, ������������ ����� ���������
 uint8_t  ckps_is_valid_half_turn_period:1;
 uint8_t  ckps_is_synchronized:1;
 uint8_t  ckps_new_engine_cycle_happen:1;      //���� ������������� � ���������  
 uint8_t  ckps_use_knock_channel:1;            //������� ������������� ������ ���������
 uint8_t  ckps_need_to_set_channel:1;                      
}ckpsflags_t;

typedef struct
{
 uint16_t icr_prev;                   //���������� �������� �������� �������
 volatile uint16_t period_curr;       //�������� ���������� ��������� ������
 uint16_t period_prev;                //���������� �������� ���������� �������
 volatile uint8_t  cog;               //������� ����� ����� ������, �������� ������� � 1
 uint16_t measure_start_value;        //���������� �������� �������� ������� ��� ��������� ������� �����������
 uint16_t current_angle;              //����������� �������� ��� ��� ����������� ������� ����
 volatile uint16_t half_turn_period;  //������ ��������� ��������� ������� ����������� n ������  
 int16_t  advance_angle;              //��������� ��� * ANGLE_MULTIPLAYER
 volatile int16_t advance_angle_buffered;     
 uint8_t  ignition_cogs;              //���-�� ������ ������������ ������������ ��������� ������� ������������
 uint8_t  starting_mode;              //��������� ��������� �������� ��������� ������ �� �����
 uint8_t  channel_mode;               //���������� ����� ����� ��������� ����� ��������� � ������ ������
 volatile uint8_t  cogs_btdc;         //���-�� ������ �� ����������� �� �.�.� ������� �������� 
 int8_t   knock_wnd_begin_abs;        //������ ���� ������� �������� ��������� � ������ ����� ������������ �.�.� 
 int8_t   knock_wnd_end_abs;          //����� ���� ������� �������� ��������� � ������ ����� ������������ �.�.�  
 volatile uint8_t chan_number;        //���-�� ������� ���������
 uint32_t frq_calc_dividend;          //������� ��� ������� ������� ��������
}ckpsstate_t;
 
//��������������� ������(������� �����) � ������ ��������� ��� ���������� ������ ��������� (���� ���������)
//2�: cylstate_t[0]
//4�: cylstate_t[0], cylstate_t[1]
//6�: cylstate_t[0], cylstate_t[1], cylstate_t[2]
//8�: cylstate_t[0], cylstate_t[1], cylstate_t[2], cylstate_t[3]
typedef struct
{
 volatile uint8_t ignition_pulse_cogs;//����������� ����� �������� ��������� ��� ��������� 1-4
 volatile uint8_t cogs_latch;         //���������� ����� ���� (������������ �.�.�.) �� ������� ���������� "������������" ������
 volatile uint8_t cogs_btdc;          //���������� ����� ���� �� ������� ������������ ��������� ������� �������� ��������� (����� ���. �������)
 volatile uint8_t knock_wnd_begin;    //���������� ����� ���� �� ������� ����������� ���� ������� �������� ������� �� (������ ��������������) 
 volatile uint8_t knock_wnd_end;      //���������� ����� ���� �� ������� ����������� ���� ������� �������� ������� �� (����� ��������������)
}chanstate_t[IGN_CHANNELS_MAX]; 
  
ckpsstate_t ckps;
chanstate_t chanstate;

//��������� � ��������� ��������� �����/������
__no_init volatile ckpsflags_t flags@0x22;

//��� ���������� �������/�������� 0 �� 16 ��������, ���������� R15
__no_init __regvar uint8_t TCNT0_H@15;

//�������������� ���������� ��������� ����
__monitor
void ckps_init_state_variables(void)
{
 //��� ������ �� ���������� ����� ������������ ����� �������� ������� ��������� ��� ��������� 1-4   
 uint8_t i;
 for(i = 0; i < IGN_CHANNELS_MAX; i++)
  chanstate[i].ignition_pulse_cogs = 0;

 ckps.cog = 0; 
 ckps.half_turn_period = 0xFFFF;                 
 ckps.advance_angle = 0;
 ckps.advance_angle_buffered = 0;
 ckps.starting_mode = 0;
 ckps.channel_mode = CKPS_CHANNEL_MODENA;
 
 flags.ckps_need_to_set_channel = 0;   
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
  
 //������ Compare ������������ � ������ ������ (���������� ����� ������) 
 TCCR1A = 0; 
  
 //���������� ����, �������� ����� �������, clock = 250kHz
 TCCR1B = (1<<ICNC1)|(1<<ICES1)|(1<<CS11)|(1<<CS10);       

 //��������� ���������� �� ������� � ��������� � ������� 1, � ����� �� ������������ ������� 0
 TIMSK|= (1<<TICIE1)/*|(1<<OCIE1A)*/|(1<<TOIE0);
}

//������������� ��� ��� ���������� � ���������
__monitor
void ckps_set_advance_angle(int16_t angle)
{
 ckps.advance_angle_buffered = angle;
}

void ckps_init_ports(void)
{ 
 //����� ��������� ��������� ����������� �������� ���� � ������ ����������,
 //������� ������������� �� �� ������ ������ �������.
#ifndef INVERSE_IGN_OUTPUTS
 PORTD|= (1<<PD5)|(1<<PD4)|(1<<PD6); //1-� � 2-� ������ ���������, �������� ��� ICP1
 PORTC|= (1<<PC1)|(1<<PC0); //3-� � 4-� ������ ���������
#else //����� �������� �������
 PORTD&= ~((1<<PD5)|(1<<PD4));
 PORTC&= ~((1<<PC1)|(1<<PC0));
 PORTD|=  (1<<PD6); 
#endif 

 //PD5,PD4,PC1,PC0 ������ ���� ���������������� ��� ������
 DDRD|= (1<<DDD5)|(1<<DDD4); //1-2 ������ ��������� (��� 2 � 4 �. ����������)
 DDRC|= (1<<DDC1)|(1<<DDC0); //3-4 ������ ��������� (��� 6 � 8 �. ����������)
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
  return (ckps.frq_calc_dividend)/(period);
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

uint8_t _normalize_tn(int8_t i_tn)
{
 if (i_tn > WHEEL_COGS_NUM) 
  return i_tn - WHEEL_COGS_NUM;
 if (i_tn < 0)
  return i_tn + WHEEL_COGS_NUM;
 return i_tn;    
}

void ckps_set_cogs_btdc(uint8_t cogs_btdc)
{
 uint8_t _t, i;
 // ������� ��������� � ��������� ������� ����� (�����)
 // cogs_per_cycle - ���������� ������ ����� ������������ �� ���� ���� ��������� 
 uint8_t cogs_per_cycle = (WHEEL_COGS_NUM) / ckps.chan_number;
  _t=__save_interrupt();
 __disable_interrupt();
 for(i = 0; i < ckps.chan_number; ++i)
 {
  uint8_t tdc = (cogs_btdc + i * cogs_per_cycle);
  chanstate[i].cogs_btdc = _normalize_tn(tdc);
  chanstate[i].cogs_latch = _normalize_tn(tdc - WHEEL_LATCH_BTDC);
  chanstate[i].knock_wnd_begin = _normalize_tn(tdc + ckps.knock_wnd_begin_abs); 
  chanstate[i].knock_wnd_end = _normalize_tn(tdc + ckps.knock_wnd_end_abs); 
 }
 ckps.cogs_btdc = cogs_btdc;
 __restore_interrupt(_t); 
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

__monitor
void ckps_set_cyl_number(uint8_t i_cyl_number)
{
 ckps.chan_number = i_cyl_number >> 1; //���� ����� ��������� �� 2 ��������
 
 switch(i_cyl_number)
 {
 case 2: ckps.frq_calc_dividend = 15000000L; 
  break;
 case 4: ckps.frq_calc_dividend = 7500000L;
  break;
 case 6: ckps.frq_calc_dividend = 5000000L;
  break;
 case 8: ckps.frq_calc_dividend = 3750000L;
  break; 
 }
 //TODO: calculations previosly made by ckps_set_cogs_btdc()|ckps_set_knock_window() becomes invalid!
 //So, ckps_set_cogs_btdc() must be called again. Do it here or in place where this function called.
}

void ckps_set_knock_window(int16_t begin, int16_t end)
{
 uint8_t _t, i, cogs_per_cycle;
 //��������� �� �������� � �����
 ckps.knock_wnd_begin_abs = begin / (CKPS_DEGREES_PER_COG * ANGLE_MULTIPLAYER);
 ckps.knock_wnd_end_abs = end / (CKPS_DEGREES_PER_COG * ANGLE_MULTIPLAYER);

 cogs_per_cycle = (WHEEL_COGS_NUM) / ckps.chan_number;
 _t=__save_interrupt();
 __disable_interrupt(); 
 for(i = 0; i < ckps.chan_number; ++i)
 {
  uint8_t tdc = (ckps.cogs_btdc + i * cogs_per_cycle);
  chanstate[i].knock_wnd_begin = _normalize_tn(tdc + ckps.knock_wnd_begin_abs); 
  chanstate[i].knock_wnd_end = _normalize_tn(tdc + ckps.knock_wnd_end_abs); 
 }  
 __restore_interrupt(_t);
}

//��������������� ������
//����� �������� ������� (������ �����) ��� 1-��,2-��,3-��,4-�� ������� ��������������
#define TURNON_IGN_CHANNELS(){\
  case 0: PORTD |= (1<<PORTD4);\
   break;\
  case 1: PORTD |= (1<<PORTD5);\
   break;\
  case 2: PORTC |= (1<<PORTC0);\
   break;\
  case 3: PORTC |= (1<<PORTC1);\
   break;}

//��������������� ������
//����� �������� ������� ��������� ��� 1-��,2-��,3-��,4-�� ������� ��������������
#define TURNOFF_IGN_CHANNELS(){\
 case 0: PORTD &= ~(1<<PORTD4);\
  break;\
 case 1: PORTD &= ~(1<<PORTD5);\
  break;\
 case 2: PORTC &= ~(1<<PORTC0);\
  break;\
 case 3: PORTC &= ~(1<<PORTC1);\
  break;}  

#pragma vector=TIMER1_COMPA_vect //������ ���������� �� ���������� ������ � ������� �1
__interrupt void timer1_compa_isr(void)
{
  TIMSK&= ~(1<<OCIE1A); //��������� ����������

 //����� ����� � ������ ������, ������ ��������� � � ������� ������� - ���������� ���������� ���������� 
 //���������� ������� � ������� ���������� (�����).
 switch(ckps.channel_mode)
 {
#ifndef INVERSE_IGN_OUTPUTS 
  TURNON_IGN_CHANNELS();
#else
  TURNOFF_IGN_CHANNELS();
#endif   
  default:
   return; //������� ����� �� ������ - CKPS_CHANNEL_MODENA
 }
 //�������� ������ ������������ �������� � ������
 chanstate[ckps.channel_mode].ignition_pulse_cogs = 0;
}

#pragma inline
void turn_off_ignition_channel(uint8_t i_channel)
{
 //���������� �������� ������� �����������, ������� ����� ����� � ������ ������� - ����������
 //���������� ������� � ����� ���������� �������
 switch(i_channel)
 {
#ifndef INVERSE_IGN_OUTPUTS 
  TURNOFF_IGN_CHANNELS();
#else
  TURNON_IGN_CHANNELS();
#endif 
 }
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
 ++ckps.cog; 
 return 0; //����������� �������� �������������
} 

//���������. ���������� ��� ���� ������ ����� (������������ � ����������������)
void process_ckps_cogs(void)
{
 uint16_t diff; 
 uint8_t i;  

#ifdef VENTILATOR_PWM
 //CKP processing creates a big delay which negatively affects ventilator's PWM. We
 //need to enable T/C 2 interrupts. TODO: it is bad idea to enable all interrupts 
 //here. We need only OCIE2 and TOIE2.
 __enable_interrupt();
#endif 

 if (flags.ckps_use_knock_channel)
 {
  for(i = 0; i < ckps.chan_number; ++i)
  {
   //�������� ������� ��������� (�������� ����)
   if (ckps.cog == chanstate[i].knock_wnd_begin)
    knock_set_integration_mode(KNOCK_INTMODE_INT);
      
   //����������� ������� ��������� (�������� ����) � ��������� ������� ��������� 
   if (ckps.cog == chanstate[i].knock_wnd_end)
   {
    knock_set_integration_mode(KNOCK_INTMODE_HOLD); 
    adc_begin_measure_knock(); 
   }
  }  
 }
  
 for(i = 0; i < ckps.chan_number; ++i)
 {
  //�� 66 �������� �� �.�.� ����� ������� ������ ������������� ����� ��� ��� ����������, ���
  //�� ����� �������� �� ��������� ������.
  if (ckps.cog == chanstate[i].cogs_latch)
  {
   ckps.channel_mode = i;          //���������� ����� ������
   flags.ckps_need_to_set_channel = 1; //������������� ������� ����, ��� ����� ����������� ���
   //�������� ������ ���� ����������
   ckps.current_angle = ANGLE_MAGNITUDE(CKPS_DEGREES_PER_COG) * WHEEL_LATCH_BTDC; // �� ����� 66�
   ckps.advance_angle = ckps.advance_angle_buffered; // ���������� �� ����� ��������������� (��������, 15�)
   knock_start_settings_latching();//��������� ������� �������� �������� � HIP  
   adc_begin_measure();            //������ �������� ��������� �������� ���������� ������    
  }
  
  //����� ����������/������ ��������� �������� ��������  - �.�.�. ���������� � ���������� ����������� �������, 
  //����� ������������ �������� �������� �������� ��� ���������� ��������� 
  if (ckps.cog==chanstate[i].cogs_btdc) 
  { 
   //���� ���� ������������ �� ������������� ����������� ��������� �����
   if (((ckps.period_curr > 1250) || !flags.ckps_is_valid_half_turn_period))
    ckps.half_turn_period = 0xFFFF;
   else       
    ckps.half_turn_period = (GetICR() - ckps.measure_start_value);
  
   ckps.measure_start_value = GetICR();
   flags.ckps_is_valid_half_turn_period = 1;
   /////////////////////////////////////////
   flags.ckps_new_engine_cycle_happen = 1; //������������� ������� �������� ������������� 
   /////////////////////////////////////////
  }  
 }  
 
 //���������� � ������� ��������� ��� �������� ������ (���� �������� ������ ������)
 if (flags.ckps_need_to_set_channel && ckps.channel_mode!= CKPS_CHANNEL_MODENA)
 {
  diff = ckps.current_angle - ckps.advance_angle;
  if (diff <= (ANGLE_MAGNITUDE(CKPS_DEGREES_PER_COG) * 2))
  {
   //�� ������� ��������� �������� ��������� ������ 2-x �����. ���������� ����������� ������ ���������
   //TODO: replace heavy division by multiplication with magic number. This will reduce up to 40uS !
   OCR1A = GetICR() + ((uint32_t)diff * (ckps.period_curr)) / ANGLE_MAGNITUDE(CKPS_DEGREES_PER_COG);
   TIFR = (1 << OCF1A);
   chanstate[ckps.channel_mode].ignition_pulse_cogs = 0;
   flags.ckps_need_to_set_channel = 0; // ����� �� ����� � ����� ��������� ��� ���
   TIMSK|= (1<<OCIE1A); //��������� ����������
  }
 }

 //����������� �������� ������� �����������(��) � ����� ����������� ����� ���� ��� ������������� ������
 for(i = 0; i < ckps.chan_number; ++i)
 {
  if (chanstate[i].ignition_pulse_cogs >= ckps.ignition_cogs)
   turn_off_ignition_channel(i);  
  ++(chanstate[i].ignition_pulse_cogs);
 }

 //������ ��� - ���� �� �.�.�. ���������� �� 6 ����.
 ckps.current_angle-= ANGLE_MAGNITUDE(CKPS_DEGREES_PER_COG);  
 ++ckps.cog;
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
  --TCNT0_H;         
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
