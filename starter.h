
#ifndef _STARTER_H_
#define _STARTER_H_

struct ecudata;

//������������� ������������ ������
void starter_init_ports(void);

//���������� ���������
void starter_control(struct ecudata* d);

//����������/������������� ��������
void starter_set_blocking_state(uint8_t i_state);

#endif //_STARTER_H_
