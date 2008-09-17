 /****************************************************************
 *       SECU-3  - An open source, free engine control unit
 *    Designed by Alexey A. Shabelnikov. Ukraine, Gorlovka 2007.
 *       Microprocessors systems - design & programming.
 *    contacts:
 *              http://secu-3.narod.ru
 *              ICQ: 405-791-931
 ****************************************************************/

#include <inavr.h>
#include <iom16.h>
#include "bitmask.h"
#include "adc.h"
#include "secu3.h"

typedef struct
{
 unsigned int map_value;           //��������� ���������� �������� ����������� ��������
 unsigned int ubat_value;          //��������� ���������� �������� ���������� �������� ����
 unsigned int temp_value;          //��������� ���������� �������� ����������� ����������� ��������

 unsigned char  sensors_ready;     //������� ���������� � �������� ������ � ����������
}adc_state;

adc_state adc;  //���������� ��������� ���

__monitor
unsigned int adc_get_map_value(void)
{
  return adc.map_value;
}

__monitor
unsigned int adc_get_ubat_value(void)
{
  return adc.ubat_value;
}

__monitor
unsigned int adc_get_temp_value(void)
{
  return adc.temp_value;
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

char adc_is_measure_ready(void)
{
  return adc.sensors_ready; 
}

//������������� ��� � ��� ���������� ���������
void adc_init(void)
{ 
 //������������� ���, ���������: f = 125.000 kHz, 
 //���������� �������� �������� ���������� - 2.56V, ���������� ��������� 
 ADMUX=ADC_VREF_TYPE;
 ADCSRA=(1<<ADEN)|(1<<ADIE)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);     

 //������ ��� ����� � ������ ���������
 adc.sensors_ready = 1;
}

//���������� �� ���������� �������������� ���. ��������� �������� ���� ���������� ��������. ����� �������
//��������� ��� ���������� ����� ��������� ��� ������� �����, �� ��� ��� ���� ��� ����� �� ����� ����������.
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
      ADMUX = ADCI_MAP|ADC_VREF_TYPE;    
      adc.sensors_ready = 1;                
      break; 
      
/*    nearest future!!!
   case ADCI_KNOCK://��������� ��������� ������� � ����������� ������ ���������
      adc.knock_value = ADC;      
      ADMUX = ADCI_MAP|ADC_VREF_TYPE;    
      adc.sensors_ready = 1;                
      break; */
 } 
}


//������������ ����������� ��� (����������� �������� � ������������ �����������)
// adcvalue - ������� ��� ��� �����������
// factor = 2^14 * gainfactor, 
// correction = 2^14 * (0.5 - offset * gainfactor),
// 2^16 * realvalue = 2^2 * (adcvalue * factor + correction)
signed int adc_compensate(signed int adcvalue, signed int factor, signed long correction)
{
  return (((((signed long)adcvalue*factor)+correction)<<2)>>16);
}



unsigned int map_adc_to_kpa(signed int adcvalue)
{
 //��� �� �������� ������������� ����������, ������ ������������� �������� ����� �������� ����� ����������� ������������.
 //����� ��� ������� ���������� �������������.
 if (adcvalue < 0)
   adcvalue = 0;
   
 //���� ��� ������� ��������������� �� �������� � ������� �������� ���: ((adcvalue + K1) * K2 ) / 128,
 //��� K1,K2 - ���������.   
 return ( ((unsigned long)(adcvalue + ((unsigned int)((MAP_CURVE_OFFSET_V / ADC_DISCRETE)+0.5)))) * 
          ((unsigned long)((128.0 * MAP_CURVE_GRADIENT_KPA * MAP_PHYSICAL_MAGNITUDE_MULTIPLAYER * ADC_DISCRETE)+0.5)) 
        ) >> 7; 
}

unsigned int ubat_adc_to_v(signed int adcvalue)
{
 if (adcvalue < 0)
   adcvalue = 0;
 return adcvalue;
}

signed int temp_adc_to_c(signed int adcvalue)
{   
 if (adcvalue < 0)
   adcvalue = 0;
 return (adcvalue - ((signed int)((TSENS_ZERO_POINT / ADC_DISCRETE)+0.5)) );
}

