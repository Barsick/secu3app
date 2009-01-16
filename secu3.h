
#ifndef _SECU3_H_
#define _SECU3_H_

#include "tables.h"

#define SEND_PACKET_INTERVAL_VALUE    8
#define SAVE_PARAM_TIMEOUT_VALUE      3000
#define FORCE_MEASURE_TIMEOUT_VALUE   50
#define CE_CONTROL_STATE_TIME_VALUE   50
#define ENGINE_ROTATION_TIMEOUT_VALUE 15
#define IDLE_PERIOD_TIME_VALUE        50

//��������� ��� ����� ������� - �� ����������� � ������������ ��������
typedef struct
{
 unsigned int map;                                          //�������� �� �������� ���������� (�����������)
 unsigned int voltage;                                      //���������� �������� ���� (�����������)
 signed int temperat;                                       //����������� ����������� �������� (�����������)
 unsigned int frequen;                                      //������� �������� ��������� (�����������)
 unsigned int inst_frq;                                     //���������� ������� ��������
 unsigned char carb;                                        //��������� ��������� ����������� 
 unsigned char gas;                                         //��������� �������� ������� 
 unsigned int frequen4;                                     //������� ����������� ����� �� 4-� �������� 
 unsigned int knock_k;                                      //������� ������� ��������� 

 //����� �������� �������� (�������� ��� � ����������������� �������������)
 signed int map_raw;
 signed int voltage_raw;
 signed int temperat_raw;

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
 char             op_comp_code;   
 char             op_actn_code;                              
 unsigned int     ecuerrors_for_transfer;    
}ecudata;                                     


#endif  //_SECU3_H_

