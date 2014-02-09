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
              http://secu-3.org
              email: shabelnikov@secu-3.org
*/

/** \file ignlogic.c
 * Implementation of logic determining calculation and regulation of anvance angle
 * (���������� ������ ������������ ���������� � ������������� ���� ����������).
 */

#include "port/port.h"
#include "funconv.h"
#include "ignlogic.h"
#include "secu3.h"

int16_t advance_angle_state_machine(struct ecudata_t* d)
{
 int16_t angle;
 switch(d->engine_mode)
 {
  case EM_START: //����� �����
   if (d->sens.inst_frq > d->param.smap_abandon)
   {
    d->engine_mode = EM_IDLE;
    idling_regulator_init();
   }
   angle=start_function(d);                //������� ��� - ������� ��� �����
   d->airflow = 0;                         //� ������ ����� ��� �������
   break;

  case EM_IDLE: //����� ��������� ����
   if (d->sens.carb)//������ ���� ������ - � ������� �����
   {
    d->engine_mode = EM_WORK;
   }
   work_function(d, 1);                    //��������� �������� ������� �������
   angle = idling_function(d);             //������� ��� - ������� ��� ��
   angle+=coolant_function(d);             //��������� � ��� ������������� ���������
   angle+=idling_pregulator(d,&idle_period_time_counter);//��������� �����������
#ifdef AIRTEMP_SENS
   angle+=airtemp_function(d);                  //add air temperature correction
#endif
   break;

  case EM_WORK: //������� �����
   if (!d->sens.carb)//������ ���� ��������� - � ���������� ����� ��
   {
    d->engine_mode = EM_IDLE;
    idling_regulator_init();
   }
   angle=work_function(d, 0);              //������� ��� - ������� �������� ������
   angle+=coolant_function(d);             //��������� � ��� ������������� ���������
#ifdef AIRTEMP_SENS
   angle+=airtemp_function(d);                  //add air temperature correction
#endif
   //�������� �������� ���������� �� ���������� �� ���������
   angle-=d->knock_retard;
   break;

  default:  //���������� �������� - ���� � ����
   angle = 0;
   break;
 }
 return angle; //return calculated advance angle
}
