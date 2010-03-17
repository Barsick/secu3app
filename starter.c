 /****************************************************************
 *       SECU-3  - An open source, free engine control unit
 *    Designed by Alexey A. Shabelnikov. Ukraine, Gorlovka 2007.
 *       Microprocessors systems - design & programming.
 *    contacts:
 *              http://secu-3.narod.ru
 *              ICQ: 405-791-931
 ****************************************************************/

#include <iom16.h>
#include "secu3.h"
#include "starter.h"
#include "vstimer.h"
#include "ce_errors.h"

//���������/�������������� �������
#define SET_STARTER_BLOCKING_STATE(s) {PORTD_Bit7 = s;}

void starter_set_blocking_state(uint8_t i_state)
{
 SET_STARTER_BLOCKING_STATE(i_state);
}

void starter_init_ports(void)
{
 DDRD |= (1<<DDD7);   //����� ��� ��������
 PORTD|= (1<<PD7);    //������� ������������
}

void starter_control(struct ecudata* d)
{
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
  ce_set_state(1);  
 }
#endif
}
