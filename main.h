
#ifndef _MAIN_H_
#define _MAIN_H_

#include "tables.h"

#define EEPROM_PARAM_START           0x002                      //����� ��������� ���������� � EEPROM 

#define SND_TIMECONST                4
#define PAR_SAVE_COUNTER             254

#define FORCE_MEASURE_TIMEOUT_VALUE  8
#define ENGINE_STOP_TIMEOUT_VALUE    25


//���� ���������� ��� ���������� � ���������, ��������������� ������� ������� �����(t.d.c.) ������ ������� ��������, �� 
//�������� �������� ���������� ���� ������ ���������� ��� ����� ������������� ������������ ���� (������� ������ 
//����������� �������� �� ����� ������).
#define DPKV_COGS_BEFORE_TDC         20                          //���������� ������ ����� ������ �� �.�.� (18...22)
#define DPKV_DEGREES_PER_COG         6                           //���������� �������� ������������ �� ���� ��� �����
//��� ����������� ������������ ������������ �������� ������� ������ ���� 1/3, ��� ������������ ���������� � ������� ������� 
//�������� ����� ����������� �� �����. 
#define DPKV_IGNITION_PULSE_COGS     10                          //������������ �������� ��������� (� ������ �����)

#define ANGLE_MULTIPLAYER            40                          //����������� ��������������� ����� �������� ���������  

#define FRQ_AVERAGING                16                          //���-�� �������� ��� ���������� ������� �������� �.�.
                                      
#define TIMER2_RELOAD_VALUE          100

//��������� ��� ����� ������� - �� ����������� � ������������ ��������
typedef struct
{
 unsigned int map;                                          //�������� �� �������� ���������� (�����������)
 unsigned int voltage;                                      //���������� �������� ���� (�����������)
 unsigned int temperat;                                     //����������� ����������� �������� (�����������)
 unsigned int frequen;                                      //������� �������� ��������� (�����������)
 unsigned int inst_frq;                                     //���������� ������� ��������
 unsigned char carb;                                        //��������� ��������� ����������� 
 unsigned char gas;                                         //��������� �������� ������� 
}sensors;


//��������� ������ �������, ������������ ������ ��������� ������
typedef struct
{
 params           param;                                      //--���������
 sensors          sens;                                       //--�������
 unsigned char    ephh_valve;                                 //��������� ������� ����
 int              atmos_press;                                //����������� ��������
 unsigned char    airflow;                                    //������ �������
 signed int       curr_angle;                                 //������� ���� ����������
 __flash F_data*  fn_dat;                                     //��������� �� ����� �������������
}ecudata;

typedef struct
{
 unsigned char  new_engine_cycle_happen:1;                    //���� ������������� � ���������
 unsigned char  uart_send_busy:1;                             //���� ������������� ����������� (�����)
 unsigned char  uart_recv_busy:1;                             //���� ������������� ��������� (�����)
 unsigned char  adc_sensors_ready:1;                          //������� ���������� � �������� ������ � ����������           
}bitfield1;


//�������� ����������� ����� ������������ � ����������� ��������� � ��������� ��������� �����/������
__no_init volatile bitfield1 f1@0x22;

#endif  //_MAIN_H_

