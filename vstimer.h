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

#ifndef _VSTIMER_H_
#define _VSTIMER_H_

#include <stdint.h>

//�������������� ��� ���������� ����������� �������

//���� �������� ��������. 8-�� ��������� ������ ����� ����������� �������
//�� 2.56 ���. 16-�� ��������� ������ ����� ����������� ������� �� 655 ���.
typedef uint8_t   s_timer8_t;
typedef uint16_t  s_timer16_t;

//���������� ��������� ���������� �������
#define s_timer_update(T)    { if ((T) > 0) (T)--; }

//������������� ��������� ���������� �������. ���� ��� ������� ����� 10 ��
#define s_timer_set(T, V)    { (T) = (V); }

//��������� �������� �� ��������� ������
#define s_timer_is_action(T) ((T)==0)

//����, �������� ������� ��� 16-�� ��������� ����������� ��������. 
//��� ��� ��� ���� �������� ������������ �� ��������� ��� ������, �� 
//���������� ��������� ����������.

#define s_timer16_set(T, V)  \
{                            \
 __disable_interrupt();      \
 (T) = (V);                  \
 __enable_interrupt();       \
}
 
#pragma inline  //� � ������� "�" ������ ��� ;-), ������� ������������� �����������.
__monitor uint8_t s_timer16_is_action(s_timer16_t i_timer) 
{
 return (i_timer==0);
}

void s_timer_init(void);

//////////////////////////////////////////////////////////////////
extern volatile s_timer8_t  send_packet_interval_counter;
extern volatile s_timer8_t  force_measure_timeout_counter;
extern volatile s_timer8_t  ce_control_time_counter;
extern volatile s_timer8_t  engine_rotation_timeout_counter;
extern volatile s_timer8_t  epxx_delay_time_counter;
extern volatile s_timer8_t  idle_period_time_counter;
extern volatile s_timer16_t save_param_timeout_counter;
//////////////////////////////////////////////////////////////////

#endif //_VSTIMER_H_
