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

/**Reserved value used to indicate that value is not used in corresponding mode*/
#define AAV_NOTUSED 0x7FFF

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
   angle = d->corr.strt_aalt = start_function(d);//������� ��� - ������� ��� �����
   d->corr.idle_aalt = d->corr.work_aalt = d->corr.temp_aalt = d->corr.airt_aalt = d->corr.idlreg_aac = AAV_NOTUSED;
   d->airflow = 0;                         //� ������ ����� ��� �������
   break;

  case EM_IDLE: //����� ��������� ����
   if (d->sens.carb)//������ ���� ������ - � ������� �����
   {
    d->engine_mode = EM_WORK;
   }
   work_function(d, 1);                    //��������� �������� ������� �������
   angle = d->corr.idle_aalt = idling_function(d); //������� ��� - ������� ��� ��
   d->corr.temp_aalt = coolant_function(d);//��������� � ��� ������������� ���������
   angle+=d->corr.temp_aalt;
#ifdef AIRTEMP_SENS
   d->corr.airt_aalt = airtemp_function(d);//add air temperature correction
   angle+=d->corr.airt_aalt;
#else
   d->corr.airt_aalt = 0;
#endif
   d->corr.idlreg_aac = idling_pregulator(d,&idle_period_time_counter);//��������� �����������
   angle+=d->corr.idlreg_aac;
   d->corr.strt_aalt = d->corr.work_aalt = AAV_NOTUSED;
   break;

  case EM_WORK: //������� �����
   if (!d->sens.carb)//������ ���� ��������� - � ���������� ����� ��
   {
    d->engine_mode = EM_IDLE;
    idling_regulator_init();
   }

#ifdef SM_CONTROL
   //air flow will be always 1 if choke RPM regulator is active
   if (d->choke_rpm_reg)
   {
    work_function(d, 1);                    //��������� �������� ������� �������
    angle = d->corr.idle_aalt = idling_function(d);//������� ��� - ������� ��� ��
    d->corr.work_aalt = AAV_NOTUSED;
   }
   else
   {
    angle = d->corr.work_aalt = work_function(d, 0);//������� ��� - ������� �������� ������
    d->corr.idle_aalt = AAV_NOTUSED;
   }
#else
   angle = d->corr.work_aalt = work_function(d, 0);//������� ��� - ������� �������� ������
   d->corr.idle_aalt = AAV_NOTUSED;
#endif

   d->corr.temp_aalt = coolant_function(d);//��������� � ��� ������������� ���������;
   angle+=d->corr.temp_aalt;
#ifdef AIRTEMP_SENS
   d->corr.airt_aalt = airtemp_function(d);//add air temperature correction;
   angle+=d->corr.airt_aalt;
#else
   d->corr.airt_aalt = 0;
#endif
   //�������� �������� ���������� �� ���������� �� ���������
   angle-=d->corr.knock_retard;
   d->corr.strt_aalt = d->corr.idlreg_aac = AAV_NOTUSED;
   break;

  default:  //���������� �������� - ���� � ����
   angle = 0;
   break;
 }
 return angle; //return calculated advance angle
}
