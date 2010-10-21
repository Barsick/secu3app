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

#ifndef _CKPS_H_
#define _CKPS_H_

#include <stdint.h>

//����������� ��������������� ����� �������� ���������, ���������� � ����������� � ��������� �������
//������� �� ������ ���� ������ ������� 2
#define ANGLE_MULTIPLAYER            32                           

void ckps_init_state(void);

//��� ������ ����
// 0 - �������������, 1 - �������������
void ckps_set_edge_type(uint8_t edge_type);

//���� ���������� ��� ���������� � ���������, ��������������� ������� ������� �����(t.d.c.) ������ ������� ��������, �� 
//�� ��������� �������� �������� ���������� ���� ������ ���������� 20-� ��� ����� ������������� (������� ������ 
//����������� �������� �� ����� ������). ���������� ��������: 18,19,20,21,22 (��� 60-2 �����), 10,11,12,13,14 (��� 36-1 �����).
void ckps_set_cogs_btdc(uint8_t cogs_btdc);

//��� ����������� ������������ ������������ �������� ������� ������ ���� 1/3, ��� ������������ ���������� � ������� ������� 
//�������� ����� ����������� �� �����.  ���� ��������� ��� ������ ������ ��� ������ �����������, �� ���������� �������
//�������� 10, ���� ������������� ����� �� 40. �������� ������� ��� ����� 60-2.
void ckps_set_ignition_cogs(uint8_t cogs);

void ckps_set_advance_angle(int16_t angle);
uint16_t ckps_calculate_instant_freq(void);

//��������� ���� ������� �������� ���������. ��������� begin, end � �������� ������������ �.�.�.  
void ckps_set_knock_window(int16_t begin, int16_t end);

//������������� ����������� ��� ������������� ����� ���������
void ckps_use_knock_channel(uint8_t use_knock_channel);

uint8_t ckps_is_error(void);
void ckps_reset_error(void);
uint8_t ckps_is_cycle_cutover_r(void);
void ckps_init_state_variables(void);

//���������� ����� �������� ����
uint8_t ckps_get_current_cog(void);

//���������� 1, ���� ����� �������� ���� ���������.
uint8_t ckps_is_cog_changed(void);

//��������� ���-�� ��������� ��������� (������ �����)
//���������� ��������: 2,4,6,8 
void ckps_set_cyl_number(uint8_t i_cyl_number);

//���������� ������������� ����� ������
void ckps_init_ports(void);

#endif //_CKPS_H_
