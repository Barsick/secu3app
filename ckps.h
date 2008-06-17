#ifndef _CKPS_H_
#define _CKPS_H_

//���� ���������� ��� ���������� � ���������, ��������������� ������� ������� �����(t.d.c.) ������ ������� ��������, �� 
//�������� �������� ���������� ���� ������ ���������� ��� ����� ������������� ������������ ���� (������� ������ 
//����������� �������� �� ����� ������).
#define CKPS_COGS_BEFORE_TDC         20                          //���������� ������ ����� ������ �� �.�.� (18...22)
#define CKPS_DEGREES_PER_COG         6                           //���������� �������� ������������ �� ���� ��� �����
//��� ����������� ������������ ������������ �������� ������� ������ ���� 1/3, ��� ������������ ���������� � ������� ������� 
//�������� ����� ����������� �� �����.  ���� ��������� ��� ������ ������ ��� ������ �����������, �� ���������� �������
//�������� 10, ���� ������������� ����� �� 40.
#define CKPS_IGNITION_PULSE_COGS     10                          //������������ �������� ��������� (� ������ �����)

//���������� ����� ������� ����� ����������� ��� ������ ����� ��������������
#define CKPS_ON_START_SKIP_COGS      30

//����������� ��������������� ����� �������� ���������, ���������� � ����������� � ��������� �������
//������� �� ������ ���� ������ ������� 2
#define ANGLE_MULTIPLAYER            32                           


void ckps_init_state(void);
void ckps_set_edge_type(unsigned char edge_type);
void ckps_set_ignition_cogs(unsigned char cogs);

void ckps_set_dwell_angle(signed int angle);
unsigned int ckps_calculate_instant_freq(void);

unsigned char ckps_is_error(void);
void ckps_reset_error(void);
unsigned char ckps_is_cycle_cutover_r(void);
unsigned char ckps_is_rotation_cutover_r(void);

#endif //_CKPS_H_
