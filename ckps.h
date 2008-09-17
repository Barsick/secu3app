#ifndef _CKPS_H_
#define _CKPS_H_

//���������� �������� ������������ �� ���� ��� �����
#define CKPS_DEGREES_PER_COG         6                           

//���������� ����� ������� ����� ����������� ��� ������ ����� ��������������
#define CKPS_ON_START_SKIP_COGS      30

//����������� ��������������� ����� �������� ���������, ���������� � ����������� � ��������� �������
//������� �� ������ ���� ������ ������� 2
#define ANGLE_MULTIPLAYER            32                           

#define ANGLE_MAGNITUDE(a) ((a) * ANGLE_MULTIPLAYER)

void ckps_init_state(void);

//��� ������ ����
// 0 - �������������, 1 - �������������
void ckps_set_edge_type(unsigned char edge_type);

//���� ���������� ��� ���������� � ���������, ��������������� ������� ������� �����(t.d.c.) ������ ������� ��������, �� 
//�� ��������� �������� �������� ���������� ���� ������ ���������� 20-� ��� ����� ������������� (������� ������ 
//����������� �������� �� ����� ������). ���������� ��������: 18,19,20,21,22.
void ckps_set_cogs_btdc(unsigned char cogs_btdc);

//��� ����������� ������������ ������������ �������� ������� ������ ���� 1/3, ��� ������������ ���������� � ������� ������� 
//�������� ����� ����������� �� �����.  ���� ��������� ��� ������ ������ ��� ������ �����������, �� ���������� �������
//�������� 10, ���� ������������� ����� �� 40.
void ckps_set_ignition_cogs(unsigned char cogs);

void ckps_set_dwell_angle(signed int angle);
unsigned int ckps_calculate_instant_freq(void);

unsigned char ckps_is_error(void);
void ckps_reset_error(void);
unsigned char ckps_is_cycle_cutover_r(void);
void ckps_init_state_variables(void);

//���������� ����� �������� ����
unsigned char ckps_get_current_cog(void);

//���������� 1, ���� ����� �������� ���� ���������.
unsigned char ckps_is_cog_changed(void);


#endif //_CKPS_H_
