#ifndef _KNKLOGIC_H_
#define _KNKLOGIC_H_

#include <stdint.h>

//������ ���������� ��� ������ ������������� ��� �� ���������

typedef struct retard_state_t
{
 uint8_t delay_counter; 
 uint8_t knock_flag;
}retard_state_t;

struct ecudata_t;

//����������: 0 - ��� ���������, 1 - ���� 
uint8_t knklogic_detect(struct ecudata_t* d, retard_state_t* p_rs);

//������������� ���������� ���������
void knklogic_init(retard_state_t* p_rs);

//���������� � ������ ������� �����
void knklogic_retard(struct ecudata_t* d, retard_state_t* p_rs);

#endif //_KNKLOGIC_H_
