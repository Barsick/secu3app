
#ifndef _CE_ERRORS_H_
#define _CE_ERRORS_H_

#include <stdint.h>
#include "vstimer.h"

//���������� ���� (������ �����) ������ (Check Engine)
#define ECUERROR_CKPS_MALFUNCTION       0
#define ECUERROR_EEPROM_PARAM_BROKEN    1
#define ECUERROR_PROGRAM_CODE_BROKEN    2
#define ECUERROR_KSP_CHIP_FAILED        3

//��������/��������� ����� Check Engine  
#define SET_CE_STATE(s)  {PORTB_Bit2 = s;}

struct ecudata;

//���������� �������� ������� ������ � ��������� ������ CE.
void ce_check_engine(struct ecudata* d, s_timer8* ce_control_time_counter);

//���������/������ ��������� ������ (����� ����)
void ce_set_error(uint8_t error);  
void ce_clear_error(uint8_t error);

//���������� ���������� ���� ����������� �� ��������� ������ ������ � EEPROM. 
//�������� ������ ���� EEPROM ������!
void ce_save_marged_errors(void);

//������������� ������������ ������ �����/������
void ce_init_ports(void);


#endif //_CE_ERRORS_H_
