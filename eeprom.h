
#ifndef _EEPROM_H_
#define _EEPROM_H_


//==================��������� ������===============================
//��������� ������� ������ � EEPROM ���������� ����� ������
void eeprom_start_wr_data(unsigned int eeprom_addr, unsigned char* sram_addr, unsigned char count);  

//���������� �� 0 ���� � ������� ������ ������� �������� �� �����������
unsigned char eeprom_is_idle(void);
//=================================================================


#endif //_EEPROM_H_
