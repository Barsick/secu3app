
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
 
 uint8_t  ephh_valve;                   //��������� ������� ����
 uint8_t  epm_valve;                    //��������� ������� ���
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
