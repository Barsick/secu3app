
#ifndef _VENTILATOR_H_
#define _VENTILATOR_H_

struct ecudata;

//������������� ������������ ������
void vent_init_ports(void);

//���������� ������������ ���������� ���������
void vent_control(struct ecudata *d);

#endif //_VENTILATOR_H_
