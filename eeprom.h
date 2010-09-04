
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
