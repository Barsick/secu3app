
#ifndef _VENTILATOR_H_
#define _VENTILATOR_H_

struct ecudata_t;

//������������� ������������ ������
void vent_init_ports(void);

//���������� ������������ ���������� ���������
void vent_control(struct ecudata_t *d);

//������������� ���������
void vent_init_state(void);

#endif //_VENTILATOR_H_
