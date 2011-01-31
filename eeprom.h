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

#ifndef _EEPROM_H_
#define _EEPROM_H_

#include <stdint.h>

//����� ��������� ���������� � EEPROM
#define EEPROM_PARAM_START     0x002

//����� ������� ������ (Check Engine) � EEPROM
#define EEPROM_ECUERRORS_START (EEPROM_PARAM_START+(sizeof(params_t)))


//==================��������� ������===============================
//��������� ������� ������ � EEPROM ���������� ����� ������
void eeprom_start_wr_data(uint8_t opcode, uint16_t eeprom_addr, uint8_t* sram_addr, uint8_t count);  

//���������� �� 0 ���� � ������� ������ ������� �������� �� �����������
uint8_t eeprom_is_idle(void);

//������ ��������� ���� ������ �� EEPROM (��� ������������� ����������)
void eeprom_read(void* sram_dest, int16_t eeaddr, uint16_t size);

//���������� ��������� ���� ������ � EEPROM (��� ������������� ����������)
void eeprom_write(const void* sram_src, int16_t eeaddr, uint16_t size);

//���������� ��� ����������� �������� (��� ���������� � ������� eeprom_start_wr_data())
uint8_t eeprom_take_completed_opcode(void);  
//=================================================================


#endif //_EEPROM_H_
