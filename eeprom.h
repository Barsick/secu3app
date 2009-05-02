
#ifndef _EEPROM_H_
#define _EEPROM_H_

#include <stdint.h>

//==================��������� ������===============================
//��������� ������� ������ � EEPROM ���������� ����� ������
void eeprom_start_wr_data(uint8_t opcode, uint16_t eeprom_addr, uint8_t* sram_addr, uint8_t count);  

//���������� �� 0 ���� � ������� ������ ������� �������� �� �����������
uint8_t eeprom_is_idle(void);

//������ ��������� ���� ������ �� EEPROM
void eeprom_read(void* sram_dest, int16_t eeaddr, uint16_t size);

uint8_t eeprom_take_completed_opcode(void);  
//=================================================================


#endif //_EEPROM_H_
