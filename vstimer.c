
#include <ioavr.h>
#include <inavr.h>
#include "vstimer.h"
#include "secu3.h"

#define TIMER2_RELOAD_VALUE          100                         //��� 10 ��

volatile s_timer8_t  send_packet_interval_counter = 0;
volatile s_timer8_t  force_measure_timeout_counter = 0;
volatile s_timer8_t  ce_control_time_counter = CE_CONTROL_STATE_TIME_VALUE;
volatile s_timer8_t  engine_rotation_timeout_counter = 0;
volatile s_timer8_t  epxx_delay_time_counter = 0;
volatile s_timer8_t  idle_period_time_counter = 0;
volatile s_timer16_t save_param_timeout_counter = 0;

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

void s_timer_init(void)
{
 TCCR2|= (1<<CS22)|(1<<CS21)|(1<<CS20);      //clock = 15.625kHz  
 TIMSK|= (1<<TOIE2); //��������� ���������� �� ������������ ������� 2                          
}
