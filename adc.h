
#ifndef _ADC_H_
#define _ADC_H_

#define ADC_DISCRETE            0.0025       //���� �������� ��� � �������

#define TSENS_SLOPP             0.01        //������ ������ ������� ����������� �����/������
#define TSENS_ZERO_POINT        2.73        //���������� �� ������ ������� ����������� ��� 0 �������� �������

//��������� ����������� �� �������� ������� � �������� ���
#define T_TO_DADC(Tc) ((unsigned int)((TSENS_ZERO_POINT + (Tc*TSENS_SLOPP))/ADC_DISCRETE)) 

#define ADC_VREF_TYPE           0xC0

//������ ������������ ������� ���
#define ADCI_MAP                2
#define ADCI_UBAT               1         
#define ADCI_TEMP               0

//������ ������� ���������� �� ������� �������
#define MAP_AVERAGING           4   
#define BAT_AVERAGING           4   
#define TMP_AVERAGING           8  

#define PHYSICAL_MAGNITUDE_MULTIPLAYER 32

//��� ������� ���������� ������� �������� �� ������� ����������
unsigned int adc_get_map_value(unsigned char index);
unsigned int adc_get_ubat_value(unsigned char index);
unsigned int adc_get_temp_value(unsigned char index);

//��������� ��������� �������� � ��������, �� ������ ���� ����������  
//��������� ���������.
void adc_begin_measure(void);

//���������� �� 0 ���� ��������� ������ (��� �� ������)
char adc_is_measure_ready(void); 

//������������� ��� � ��� ���������� ���������
void adc_init(void);

signed int adc_compensate(signed int adcvalue, signed int factor, signed long correction);

//��������� �������� ��� � ���������� �������� - ��������
//���������� �������� * PHYSICAL_MAGNITUDE_MULTIPLAYER
unsigned int map_adc_to_kpa(signed int adcvalue);

//��������� �������� ��� � ���������� �������� - ����������
//���������� �������� * PHYSICAL_MAGNITUDE_MULTIPLAYER
unsigned int ubat_adc_to_v(signed int adcvalue);

//��������� �������� ��� � ���������� �������� - �����������
//���������� �������� * PHYSICAL_MAGNITUDE_MULTIPLAYER
signed int temp_adc_to_c(signed int adcvalue);


#endif //_ADC_H_
