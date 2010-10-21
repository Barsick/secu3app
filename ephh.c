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

#include <ioavr.h>
#include "secu3.h"
#include "vstimer.h"
#include "ephh.h"

//���������/��������� ������ ����
#define SET_EPHH_VALVE_STATE(s) {PORTB_Bit0 = s;}

void ephh_init_ports(void)
{
 PORTB|= (1<<PB0); //������ ���� �������
 DDRB |= (1<<DDB0);   
}

//���������� ������� ����. ���� �������� ����������� ������� � frq > [����.�����] ���
//�������� ����������� ������� � frq > [���.�����] �� ������ ��� ������, �� ������������
//���������� ������ ������� ����� ����������� ������ ���������� �� ������� ��.�������. ����� - ������ �������.  
void ephh_control(struct ecudata_t* d)
{
 if (d->sens.carb) //���� �������� ������, �� ��������� ������, �������� ������ � ������� �� �������.
 {
  d->ephh_valve = 1; 
  s_timer_set(epxx_delay_time_counter, d->param.shutoff_delay);
 }
 else //���� �������� ������, �� ��������� ������� ������� �� ��������, ����������� ��������� �������, ������� � ���� �������.
  if (d->sens.gas) //������� �������
   d->ephh_valve = ((s_timer_is_action(epxx_delay_time_counter))
   &&(((d->sens.frequen > d->param.ephh_lot_g)&&(!d->ephh_valve))||(d->sens.frequen > d->param.ephh_hit_g)))?0:1;
  else //������
   d->ephh_valve = ((s_timer_is_action(epxx_delay_time_counter))
   &&(((d->sens.frequen > d->param.ephh_lot)&&(!d->ephh_valve))||(d->sens.frequen > d->param.ephh_hit)))?0:1;     
 SET_EPHH_VALVE_STATE(d->ephh_valve);
}
