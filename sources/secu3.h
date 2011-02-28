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

#ifndef _SECU3_H_
#define _SECU3_H_

#include "tables.h"

#define SAVE_PARAM_TIMEOUT_VALUE      3000
#define FORCE_MEASURE_TIMEOUT_VALUE   50
#define CE_CONTROL_STATE_TIME_VALUE   50
#define ENGINE_ROTATION_TIMEOUT_VALUE 15
#define IDLE_PERIOD_TIME_VALUE        50

//��������� ��� ����� ������� - �� ����������� � ������������ ��������
typedef struct sensors_t
{
 uint16_t map;                           //�������� �� �������� ���������� (�����������)
 uint16_t voltage;                       //���������� �������� ���� (�����������)
 int16_t  temperat;                      //����������� ����������� �������� (�����������)
 uint16_t frequen;                       //������� �������� ��������� (�����������)
 uint16_t inst_frq;                      //���������� ������� ��������
 uint8_t  carb;                          //��������� ��������� �����������
 uint8_t  gas;                           //��������� �������� �������
 uint16_t frequen4;                      //������� ����������� ����� �� 4-� ��������
 uint16_t knock_k;                       //������� ������� ���������

 //����� �������� �������� (�������� ��� � ����������������� �������������)
 int16_t  map_raw;
 int16_t  voltage_raw;
 int16_t  temperat_raw;

}sensors_t;

//��������� ������ �������, ������������ ������ ��������� ������
typedef struct ecudata_t
{
 struct params_t  param;                //--���������
 struct sensors_t sens;                 //--�������

 uint8_t  ie_valve;                     //��������� ������� ����
 uint8_t  fe_valve;                     //��������� ������� ���
 uint8_t  ce_state;                     //��������� ����� "CE"
 uint8_t  airflow;                      //������ �������
 int16_t  curr_angle;                   //������� ���� ����������
 int16_t  knock_retard;                 //�������� ��� �� ���������� �� ���������

 __flash f_data_t*  fn_dat;             //��������� �� ����� �������������

 uint8_t  op_comp_code;                 //�������� ��� ������� ���������� ����� UART (����� OP_COMP_NC)
 uint8_t  op_actn_code;                 //�������� ��� ������� ����������� ����� UART (����� OP_COMP_NC)
 uint16_t ecuerrors_for_transfer;       //������������ ���� ������ ������������ ����� UART � �������� �������.
 uint16_t ecuerrors_saved_transfer;     //������������ ���� ������ ��� ������/������ � EEPROM, ������������/����������� ����� UART.
 uint8_t  use_knock_channel_prev;       //���������� ��������� �������� ������������� ������ ���������

 uint8_t* eeprom_parameters_cache;

 uint8_t engine_mode;                  //������� ����� ��������� (����, ��, ��������)
}ecudata_t;


#endif  //_SECU3_H_
