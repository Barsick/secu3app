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
#include "ventilator.h"
#include "secu3.h"

//��������/��������� ����������
#define SET_VENTILATOR_STATE(s) PORTB_Bit1 = (s)

#define PWM_STEPS 10

volatile uint8_t pwm_duty_counter;
volatile uint8_t pwm_duty;

void vent_init_ports(void)
{
 //������������� ����� �����/������       
 DDRB |= (1<<DDB1);   
 PORTB&= ~(1<<PB1);
}

void vent_init_state(void)
{
 //TIMSK|=(1 << OCIE2);
 pwm_duty_counter = 0;
 pwm_duty = 0; //off
}




void set_duty(uint8_t duty)
{
 if (0==duty)
 {
  TIMSK&=~(1 << OCIE2);
  SET_VENTILATOR_STATE(0);   
 }
 else
  TIMSK|=(1 << OCIE2);
}


//���������� �� �������� �/� 2 - ��� ��������� ���
//���������� ������ 10��
#pragma vector=TIMER2_COMP_vect
__interrupt void timer2_comp_isr(void)
{ 
 OCR2 = OCR2 + PWM_STEPS;

 __enable_interrupt(); //��������� ����� ������������ ����������
 
 if (pwm_duty_counter <= pwm_duty)
  SET_VENTILATOR_STATE(1);
 else
  SET_VENTILATOR_STATE(0);  

 if (++pwm_duty_counter >= PWM_STEPS)
  pwm_duty_counter = 0;  
}

void vent_control(ecudata *d)
{
 //���������� ������� ������������ ���������� ���������, ��� ������� ��� ���� ������������ � ������� 
 /*if (d->param.tmp_use)
 {
  if (d->sens.temperat >= d->param.vent_on)
   SET_VENTILATOR_STATE(1);
  if (d->sens.temperat <= d->param.vent_off)   
   SET_VENTILATOR_STATE(0); 
  }*/
  
//  set_duty(0);   
  set_duty(5);  
//  set_duty(10);  
}
