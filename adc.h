
#ifndef _ADC_H_
#define _ADC_H_

#define ADC_DISCRETE            0.0025       //���� �������� ��� � �������

#define TSENS_SLOPP             0.01        //������ ������ ������� ����������� �����/������
#define TSENS_ZERO_POINT        2.73        //���������� �� ������ ������� ����������� ��� 0 �������� �������

#define ADC_VREF_TYPE           0xC0

//������ ������������ ������� ���
#define ADCI_MAP                2
#define ADCI_UBAT               1         
#define ADCI_TEMP               0
#define ADCI_KNOCK              3

#define MAP_PHYSICAL_MAGNITUDE_MULTIPLAYER  64
#define UBAT_PHYSICAL_MAGNITUDE_MULTIPLAYER (1.0/ADC_DISCRETE) //=400
#define TEMP_PHYSICAL_MAGNITUDE_MULTIPLAYER (TSENS_SLOPP / ADC_DISCRETE) //=4

#define MAP_CURVE_OFFSET_V      0.547  //�����
#define MAP_CURVE_GRADIENT_KPA  20.9   //���

//��� ������� ���������� ������� �������� �� ������� ����������
unsigned int adc_get_map_value(void);
unsigned int adc_get_ubat_value(void);
unsigned int adc_get_temp_value(void);

//��������� ��������� �������� � ��������, �� ������ ���� ����������  
//��������� ���������.
void adc_begin_measure(void);

//���������� �� 0 ���� ��������� ������ (��� �� ������)
char adc_is_measure_ready(void); 

//������������� ��� � ��� ���������� ���������
void adc_init(void);

signed int adc_compensate(signed int adcvalue, signed int factor, signed long correction);

//��������� �������� ��� � ���������� �������� - ��������
//���������� �������� * MAP_PHYSICAL_MAGNITUDE_MULTIPLAYER
unsigned int map_adc_to_kpa(signed int adcvalue);

//��������� �������� ��� � ���������� �������� - ����������
//���������� �������� * UBAT_PHYSICAL_MAGNITUDE_MULTIPLAYER
unsigned int ubat_adc_to_v(signed int adcvalue);

//��������� �������� ��� � ���������� �������� - �����������
//���������� �������� * TEMP_PHYSICAL_MAGNITUDE_MULTIPLAYER
signed int temp_adc_to_c(signed int adcvalue);


#define TEMPERATURE_MAGNITUDE(t) ((t) * TEMP_PHYSICAL_MAGNITUDE_MULTIPLAYER)
#define VOLTAGE_MAGNITUDE(t) ((t) * UBAT_PHYSICAL_MAGNITUDE_MULTIPLAYER)
#define PRESSURE_MAGNITUDE(t) ((t) * MAP_PHYSICAL_MAGNITUDE_MULTIPLAYER)

#endif //_ADC_H_
