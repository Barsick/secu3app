
#ifndef _CE_ERRORS_H_
#define _CE_ERRORS_H_

#include <stdint.h>
#include "vstimer.h"

//���������� ���� (������ �����) ������ (Check Engine)
#define ECUERROR_CKPS_MALFUNCTION       0
#define ECUERROR_EEPROM_PARAM_BROKEN    1
#define ECUERROR_PROGRAM_CODE_BROKEN    2
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
