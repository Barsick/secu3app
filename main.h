
#ifndef _MAIN_H_
#define _MAIN_H_


#include "tables.h"
#include "bootldr.h"

#define ADC_DISCRETE            0.0025       //���� �������� ��� � �������

#define TSENS_SLOPP             0.01        //������ ������ ������� ����������� �����/������
#define TSENS_ZERO_POINT        2.73        //���������� �� ������ ������� ����������� ��� 0 �������� �������

#define EEPROM_PARAM_START      0x002       //����� ��������� ���������� � EEPROM 

#define SND_TIMECONST           4
#define PAR_SAVE_COUNTER        254

#define FORCE_MEASURE_TIMEOUT_VALUE 8


//���� ���������� ��� ���������� � ���������, ��������������� ������� ������� ����� ������ ������� ��������, �� 
//�������� �������� ���������� ���� ������ ���������� ��� ����� ������������� ������������ ���� (������� ������ 
//����������� �������� �� ����� ������).
#define TEETH_BEFORE_UP         20                                      //���������� ������ ����� ������ �� �.�.� (18...22)
#define ANGLE_MULTIPLAYER       40                                      //����������� ��������������� ����� �������� ���������  
#define DEGREES_PER_TEETH       6                                       //���������� �������� ������������ �� ���� ��� �����

//��� ����������� ������������ ������������ �������� ������� ������ ���� 1/3, ��� ������������ ���������� � ������� ������� 
//�������� ����� ����������� �� �����. 
#define IGNITION_TEETH          10                                      //������������ �������� ��������� (� ������ �����)


#define IGNITION_PULSE_14       0x10                                    //����� ��� ������ �������� ��������� �� 1-4 ��������
#define IGNITION_PULSE_23       0x20                                    //����� ��� ������ �������� ��������� �� 2-3 ��������
#define IGNITION_PULSE_OFF      0xCF                                    //����� �������� ������ ��������� ��� ���� ��� ��������� ������������

//���-�� �������� ��� ���������� ���������� ������� 
#define FRQ_AVERAGING           16                                          
#define T2_RELOAD_VALUE         100

//��������� ����������� �� �������� ������� � �������� ���
#define T_TO_DADC(Tc) ((unsigned int)((TSENS_ZERO_POINT + (Tc*TSENS_SLOPP))/ADC_DISCRETE)) 


#define TSCALE_LO_VALUE     T_TO_DADC(-16)                      //-16 �������� ����� ������ ����� ����� ����������� (� �������� �������)
#define TSCALE_STEP      ((unsigned int)((11.0*TSENS_SLOPP)/ADC_DISCRETE)) // 11 �������� ����� ������ ������������ �� �������������� ��� (� ��������� ���)

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
 unsigned char  timer1_overflow_happen:1;                     //���� ������������ ������� 1
 unsigned char  new_engine_cycle_happen:1;                    //���� ������������� � ���������
 unsigned char  uart_send_busy:1;                             //���� ������������� ����������� (�����)
 unsigned char  uart_recv_busy:1;                             //���� ������������� ��������� (�����)
 unsigned char  adc_sensors_ready:1;                          //������� ���������� � �������� ������ � ����������           
}bitfield1;


//�������� ����������� ����� ������������ � ����������� ��������� � ��������� ��������� �����/������
__no_init volatile bitfield1 f1@0x22;

#endif  //_MAIN_H_

