
#ifndef _CKPS_H_
#define _CKPS_H_

#include <stdint.h>

//uncomment following line if you wish to use 36-1 wheel
/*#define WHEEL_36_1*/

//����������� ��������������� ����� �������� ���������, ���������� � ����������� � ��������� �������
//������� �� ������ ���� ������ ������� 2
#define ANGLE_MULTIPLAYER            32                           

void ckps_init_state(void);

//��� ������ ����
// 0 - �������������, 1 - �������������
void ckps_set_edge_type(uint8_t edge_type);

//���� ���������� ��� ���������� � ���������, ��������������� ������� ������� �����(t.d.c.) ������ ������� ��������, �� 
//�� ��������� �������� �������� ���������� ���� ������ ���������� 20-� ��� ����� ������������� (������� ������ 
//����������� �������� �� ����� ������). ���������� ��������: 18,19,20,21,22 (��� 60-2 �����), 10,11,12,13,14 (��� 36-1 �����).
void ckps_set_cogs_btdc(uint8_t cogs_btdc);

//��� ����������� ������������ ������������ �������� ������� ������ ���� 1/3, ��� ������������ ���������� � ������� ������� 
//�������� ����� ����������� �� �����.  ���� ��������� ��� ������ ������ ��� ������ �����������, �� ���������� �������
//�������� 10, ���� ������������� ����� �� 40. �������� ������� ��� ����� 60-2.
void ckps_set_ignition_cogs(uint8_t cogs);

void ckps_set_dwell_angle(int16_t angle);
uint16_t ckps_calculate_instant_freq(void);

//��������� ���� ������� �������� ���������. ��������� begin, end � �������� ������������ �.�.�.  
void ckps_set_knock_window(int16_t begin, int16_t end);

//������������� ����������� ��� ������������� ����� ���������
void ckps_use_knock_channel(uint8_t use_knock_channel);

uint8_t ckps_is_error(void);
void ckps_reset_error(void);
uint8_t ckps_is_cycle_cutover_r(void);
void ckps_init_state_variables(void);

//���������� ����� �������� ����
uint8_t ckps_get_current_cog(void);

//���������� 1, ���� ����� �������� ���� ���������.
uint8_t ckps_is_cog_changed(void);

//��������� ���-�� ��������� ��������� (������ �����)
//���������� ��������: 2,4,6,8 
void ckps_set_cyl_number(uint8_t i_cyl_number);

//���������� ������������� ����� ������
void ckps_init_ports(void);

#endif //_CKPS_H_
