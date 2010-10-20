#ifndef _PARAMS_H_
#define _PARAMS_H_

#include <stdint.h>

struct ecudata_t;

//������ ������ � EEPROM - ������� ����� ���������. �� ����� ��������� ����������� � ����������� ���������.
//���������� ������ � EEPROM ���������� ������ ���� �� �������� ����� �� ��������� �� ����� �������� ������ ����������
//�� UART-a � ����������� ��������� ���������� �� �������.        
void save_param_if_need(struct ecudata_t* d);

//��������� ��������� �� EEPROM, ��������� ����������� ������ � ���� ��� ��������� ��
//����� ��������� ����� �� FLASH.
void load_eeprom_params(struct ecudata_t* d);

extern uint8_t eeprom_parameters_cache[];

#endif //_PARAMS_H_
