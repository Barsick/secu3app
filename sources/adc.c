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
              http://secu-3.org
              email: shabelnikov@secu-3.org
*/

/** \file adc.c
 * Implementation of ADC related functions (API).
 * Functions for read values from ADC, perform conversion to phisical values etc
 * (������� ��� ������ � ���, ���������� ��������, �������������� � ���������� �������� � �.�.).
 */

#include <inavr.h>
#include <ioavr.h>
#include "bitmask.h"
#include "adc.h"
#include "secu3.h"

/**����� ������ ������������� ��� ��� */
#define ADCI_MAP                2
/**����� ������ ������������� ��� ���������� �������� ���� */
#define ADCI_UBAT               1         
/**����� ������ ������������� ��� ���� */
#define ADCI_TEMP               0
/**����� ������ ������������� ��� ������ ��������� */
#define ADCI_KNOCK              3
/**��������, ������������ ��� ADCI_KNOCK ����� ������������ �������� */
#define ADCI_STUB               4 

/**C�������� ������ ��������� ��� */
typedef struct
{
 volatile uint16_t map_value;   //!< ��������� ���������� �������� ����������� ��������
 volatile uint16_t ubat_value;  //!< ��������� ���������� �������� ���������� �������� ����
 volatile uint16_t temp_value;  //!< ��������� ���������� �������� ����������� ����������� ��������
 volatile uint16_t knock_value; //!< ��������� ���������� �������� ������� ���������
 
 volatile uint8_t sensors_ready;//!< ������� ���������� � �������� ������ � ���������� 
 uint8_t  measure_all;          //!< ���� 1, �� ������������ ��������� ���� ��������
}adcstate_t;

/** ���������� ��������� ��� */
adcstate_t adc;

__monitor
uint16_t adc_get_map_value(void)
{
 return adc.map_value;
}

__monitor
uint16_t adc_get_ubat_value(void)
{
 return adc.ubat_value;
}

__monitor
uint16_t adc_get_temp_value(void)
{
 return adc.temp_value;
}

__monitor
uint16_t adc_get_knock_value(void)
{
 return adc.knock_value;
}

void adc_begin_measure(void) 
{ 
 //�� �� ����� ��������� ����� ���������, ���� ��� �� �����������
 //���������� ���������
 if (!adc.sensors_ready)  
  return;

 adc.sensors_ready = 0; 
 ADMUX = ADCI_MAP|ADC_VREF_TYPE; 
 SETBIT(ADCSRA,ADSC);
}  

void adc_begin_measure_knock(void) 
{ 
 //�� �� ����� ��������� ����� ���������, ���� ��� �� �����������
 //���������� ���������
 if (!adc.sensors_ready)  
  return;

 adc.sensors_ready = 0; 
 ADMUX = ADCI_STUB|ADC_VREF_TYPE; 
 SETBIT(ADCSRA,ADSC);
}  

void adc_begin_measure_all(void)
{
 adc.measure_all = 1;
 adc_begin_measure();
}

uint8_t adc_is_measure_ready(void)
{
 return adc.sensors_ready; 
}

void adc_init(void)
{ 
 adc.knock_value = 0;
 adc.measure_all = 0;

 //������������� ���, ���������: f = 125.000 kHz, 
 //���������� �������� �������� ���������� - 2.56V, ���������� ��������� 
 ADMUX=ADC_VREF_TYPE;
 ADCSRA=(1<<ADEN)|(1<<ADIE)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);     

 //������ ��� ����� � ������ ���������
 adc.sensors_ready = 1;

 //��������� ���������� - �� ��� �� �����
 ACSR=(1<<ACD);
}

/**���������� �� ���������� �������������� ���. ��������� �������� ���� ���������� ��������. ����� �������
 * ��������� ��� ���������� ����� ��������� ��� ������� �����, �� ��� ��� ���� ��� ����� �� ����� ����������.
 */
#pragma vector=ADC_vect
__interrupt void ADC_isr(void)
{
 __enable_interrupt(); 

 switch(ADMUX&0x07)
 {
  case ADCI_MAP: //��������� ��������� ����������� ��������
   adc.map_value = ADC;      
   ADMUX = ADCI_UBAT|ADC_VREF_TYPE;   
   SETBIT(ADCSRA,ADSC);
   break;

  case ADCI_UBAT://��������� ��������� ���������� �������� ����
   adc.ubat_value = ADC;      
   ADMUX = ADCI_TEMP|ADC_VREF_TYPE;   
   SETBIT(ADCSRA,ADSC);
   break;

  case ADCI_TEMP://��������� ��������� ����������� ����������� ��������
   adc.temp_value = ADC;      
   if (0==adc.measure_all)
   {
    ADMUX = ADCI_MAP|ADC_VREF_TYPE;    
    adc.sensors_ready = 1;                
   }
   else
   {
    adc.measure_all = 0;
    ADMUX = ADCI_KNOCK|ADC_VREF_TYPE; 
    SETBIT(ADCSRA,ADSC);
   }     
   break; 
         
  case ADCI_STUB: //��� �������� ��������� ���������� ������ ��� �������� ����� ���������� ������� ���������
   ADMUX = ADCI_KNOCK|ADC_VREF_TYPE;
   SETBIT(ADCSRA,ADSC);         
   break; 
            
  case ADCI_KNOCK://��������� ��������� ������� � ����������� ������ ���������
   adc.knock_value = ADC;      
   adc.sensors_ready = 1;                
   break; 
 } 
}

int16_t adc_compensate(int16_t adcvalue, int16_t factor, int32_t correction)
{
 return (((((int32_t)adcvalue*factor)+correction)<<2)>>16);
}

uint16_t map_adc_to_kpa(int16_t adcvalue, uint16_t offset, uint16_t gradient)
{
 //��� �� �������� ������������� ����������, ������ ������������� �������� ����� �������� ����� ����������� ������������.
 //����� ��� ������� ���������� �������������.
 if (adcvalue < 0)
  adcvalue = 0;
   
 //��������� �������� ���: ((adcvalue + K1) * K2 ) / 128, ��� K1,K2 - ���������.   
 return ( ((uint32_t)(adcvalue + offset)) * ((uint32_t)gradient) ) >> 7; 
}

uint16_t ubat_adc_to_v(int16_t adcvalue)
{
 if (adcvalue < 0)
  adcvalue = 0;
 return adcvalue;
}

int16_t temp_adc_to_c(int16_t adcvalue)
{   
 if (adcvalue < 0)
  adcvalue = 0;
 return (adcvalue - ((int16_t)((TSENS_ZERO_POINT / ADC_DISCRETE)+0.5)) );
}
