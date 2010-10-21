/* SECU-3  - An open source, free engine control unit
   Copyright (C) 2007 Alexey A. Shabelnikov. Ukraine, Gorlovka

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   contacts:
              http://secu-3.narod.ru
              email: secu-3@yandex.ru
*/

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
