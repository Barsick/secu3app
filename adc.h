
#ifndef _ADC_H_
#define _ADC_H_

#include <stdint.h>

#define ADC_DISCRETE            0.0025       //���� �������� ��� � �������

#define TSENS_SLOPP             0.01        //������ ������ ������� ����������� �����/������
#define TSENS_ZERO_POINT        2.73        //���������� �� ������ ������� ����������� ��� 0 �������� �������

#define ADC_VREF_TYPE           0xC0

//������ ������������ ������� ���
#define ADCI_MAP                2
#define ADCI_UBAT               1         
#define ADCI_TEMP               0
#define ADCI_KNOCK              3
#define ADCI_STUB               4  //��������, ������������ ��� ADCI_KNOCK

#define MAP_PHYSICAL_MAGNITUDE_MULTIPLAYER  64
#define UBAT_PHYSICAL_MAGNITUDE_MULTIPLAYER (1.0/ADC_DISCRETE) //=400
#define TEMP_PHYSICAL_MAGNITUDE_MULTIPLAYER (TSENS_SLOPP / ADC_DISCRETE) //=4

//��� ������� ���������� ������� �������� �� ������� ����������
uint16_t adc_get_map_value(void);
uint16_t adc_get_ubat_value(void);
uint16_t adc_get_temp_value(void);
uint16_t adc_get_knock_value(void);

//��������� ��������� �������� � ��������, �� ������ ���� ����������  
//��������� ���������.
void adc_begin_measure(void);
//��������� ��������� �������� � ����������� ������ ���������. ��� ��� ����� ���������
//������� INT/HOLD � 0 ����� INTOUT �������� � ��������� ���������� ��������� ������ �����
//20��� (��������������), � ������ ��������� ����� ���� ���������� �����, �� ������ ������
//��������� ��������.
void adc_begin_measure_knock(void);

//��������� ��������� �������� � �������� � ������� � ��. ������� ��������� ��������
//� ��������, ��������� ������ � ��
void adc_begin_measure_all(void);

//���������� �� 0 ���� ��������� ������ (��� �� ������)
uint8_t adc_is_measure_ready(void); 

//������������� ��� � ��� ���������� ���������
void adc_init(void);

int16_t adc_compensate(int16_t adcvalue, int16_t factor, int32_t correction);

//��������� �������� ��� � ���������� �������� - ��������
//���������� �������� * MAP_PHYSICAL_MAGNITUDE_MULTIPLAYER
uint16_t map_adc_to_kpa(int16_t adcvalue, uint16_t offset, uint16_t gradient);

//��������� �������� ��� � ���������� �������� - ����������
//���������� �������� * UBAT_PHYSICAL_MAGNITUDE_MULTIPLAYER
uint16_t ubat_adc_to_v(int16_t adcvalue);

//��������� �������� ��� � ���������� �������� - �����������
//���������� �������� * TEMP_PHYSICAL_MAGNITUDE_MULTIPLAYER
int16_t temp_adc_to_c(int16_t adcvalue);

#endif //_ADC_H_
