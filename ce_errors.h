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

#ifndef _CE_ERRORS_H_
#define _CE_ERRORS_H_

#include <stdint.h>
#include "vstimer.h"

//���������� ���� (������ �����) ������ (Check Engine)
#define ECUERROR_CKPS_MALFUNCTION       0
#define ECUERROR_EEPROM_PARAM_BROKEN    1  // �E �������� ����� ��������� ������ ����� ������� ���������
#define ECUERROR_PROGRAM_CODE_BROKEN    2  // �� �������� ����� ��������� ������ ����� ������� ���������
#define ECUERROR_KSP_CHIP_FAILED        3

struct ecudata_t;

//���������� �������� ������� ������ � ��������� ������ CE.
void ce_check_engine(struct ecudata_t* d, volatile s_timer8_t* ce_control_time_counter);

//���������/������ ��������� ������ (����� ����)
void ce_set_error(uint8_t error);  
void ce_clear_error(uint8_t error);

//���������� ���������� ���� ����������� �� ��������� ������ ������ � EEPROM. 
//�������� ������ ���� EEPROM ������!
void ce_save_merged_errors(void);

//������� ������ ����������� � EEPROM
void ce_clear_errors(void);

//������������� ������������ ������ �����/������
void ce_init_ports(void);

//��������/��������� ����� Check Engine  
#define ce_set_state(s)  {PORTB_Bit2 = s;}
#define ce_get_state() (PORTB_Bit2)

#endif //_CE_ERRORS_H_
