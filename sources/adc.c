/* SECU-3  - An open source, free engine control unit
   Copyright (C) 2007 Alexey A. Shabelnikov. Ukraine, Kiev

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
 * \author Alexey A. Shabelnikov
 * Implementation of ADC related functions (API).
 * Functions for read values from ADC, perform conversion to phisical values etc
 * (������� ��� ������ � ���, ���������� ��������, �������������� � ���������� �������� � �.�.).
 */

#include "port/avrio.h"
#include "port/interrupt.h"
#include "port/intrinsic.h"
#include "port/port.h"
#include <stdlib.h>
#include "adc.h"
#include "bitmask.h"
#include "magnitude.h"

/**����� ������ ������������� ��� ��� */
#define ADCI_MAP                2
/**����� ������ ������������� ��� ���������� �������� ���� */
#define ADCI_UBAT               1
/**����� ������ ������������� ��� ���� */
#define ADCI_TEMP               0
/*channel number for ADD_IO1 */
#define ADCI_ADD_IO1            6
/*channel number for ADD_IO2 */
#define ADCI_ADD_IO2            5
/*channel number for CARB */
#define ADCI_CARB               7
/**����� ������ ������������� ��� ������ ��������� */
#define ADCI_KNOCK              3

#ifdef PA4_INP_IGNTIM
/*channel number for PA4 input */
#define ADCI_PA4                4
#endif

/**Value of the time differential for TPSdot calculation, ticks of timer*/
#define TPSDOT_TIME_DELTA 10000
/**Tics of TCNT1 timer per 1 second */
#define TMR_TICKS_PER_SEC 312500L

#if defined(FUEL_INJECT) || defined(GD_CONTROL)
/**Used for TPSdot calculations*/
typedef struct
{
 int16_t tps_volt;              //!< Voltage
 uint16_t tps_tmr;              //!< Timer value
}tpsval_t;
#endif

/**C�������� ������ ��������� ��� */
typedef struct
{
 volatile uint16_t map_value;    //!< ��������� ���������� �������� ����������� ��������
 volatile uint16_t ubat_value;   //!< ��������� ���������� �������� ���������� �������� ����
 volatile uint16_t temp_value;   //!< ��������� ���������� �������� ����������� ����������� ��������
 volatile uint16_t knock_value;  //!< ��������� ���������� �������� ������� c �������(��) ���������
 volatile uint16_t add_io1_value;//!< last measured value od ADD_IO1
 volatile uint16_t add_io2_value;//!< last measured value of ADD_IO2
 volatile uint16_t carb_value;   //!< last measured value of TPS
#if defined(FUEL_INJECT) || defined(GD_CONTROL)
 volatile tpsval_t tpsdot[2];    //!< two value pairs used for TPSdot calculations
#endif
 volatile uint8_t sensors_ready; //!< ������� ���������� � �������� ������ � ����������
 uint8_t  measure_all;           //!< ���� 1, �� ������������ ��������� ���� ��������
#ifdef PA4_INP_IGNTIM
 volatile uint16_t pa4_value;    //!< last measured value from PA4 input
#endif
}adcstate_t;

/** ���������� ��������� ��� */
adcstate_t adc;

uint16_t adc_get_map_value(void)
{
 uint16_t value;
 _BEGIN_ATOMIC_BLOCK();
 value = adc.map_value;
 _END_ATOMIC_BLOCK();
 return value;
}

uint16_t adc_get_ubat_value(void)
{
 uint16_t value;
 _BEGIN_ATOMIC_BLOCK();
 value = adc.ubat_value;
 _END_ATOMIC_BLOCK();
 return value;
}

uint16_t adc_get_temp_value(void)
{
 uint16_t value;
 _BEGIN_ATOMIC_BLOCK();
 value = adc.temp_value;
 _END_ATOMIC_BLOCK();
 return value;
}

uint16_t adc_get_add_io1_value(void)
{
 uint16_t value;
 _BEGIN_ATOMIC_BLOCK();
 value = adc.add_io1_value;
 _END_ATOMIC_BLOCK();
 return value;
}
uint16_t adc_get_add_io2_value(void)
{
 uint16_t value;
 _BEGIN_ATOMIC_BLOCK();
 value = adc.add_io2_value;
 _END_ATOMIC_BLOCK();
 return value;
}
uint16_t adc_get_carb_value(void)
{
 uint16_t value;
 _BEGIN_ATOMIC_BLOCK();
 value = adc.carb_value;
 _END_ATOMIC_BLOCK();
 return value;
}
#if defined(FUEL_INJECT) || defined(GD_CONTROL)
int16_t adc_get_tpsdot_value(void)
{
 int16_t dv; uint16_t dt;
 tpsval_t tpsval[2];
 _BEGIN_ATOMIC_BLOCK();
 tpsval[0] = adc.tpsdot[0];
 tpsval[1] = adc.tpsdot[1];
 _END_ATOMIC_BLOCK();

 dv = tpsval[0].tps_volt - tpsval[1].tps_volt;  //calculate voltage change in ADC discretes
 dt = (tpsval[0].tps_tmr - tpsval[1].tps_tmr);  //calculate time change in ticks of timer
 if (abs(dv) > 512) dv = (dv < 0) ? -512 : 512; //limit voltage change to the half of ADC range
 if (dt < TMR_TICKS_PER_SEC/1000) return 0;     //avoid overflow, limit time change to minimum 1ms

 return (((int32_t)dv) * TMR_TICKS_PER_SEC) / dt; //calculate 1-st derivative, num of ADC discr / sec
}
#endif

uint16_t adc_get_knock_value(void)
{
 uint16_t value;
 _BEGIN_ATOMIC_BLOCK();
 value = adc.knock_value;
 _END_ATOMIC_BLOCK();
 return value;
}

#ifdef PA4_INP_IGNTIM
uint16_t adc_get_pa4_value(void)
{
 uint16_t value;
 _BEGIN_ATOMIC_BLOCK();
 value = adc.pa4_value;
 _END_ATOMIC_BLOCK();
 return value;
}
#endif

void adc_begin_measure(uint8_t speed2x)
{
 //�� �� ����� ��������� ����� ���������, ���� ��� �� �����������
 //���������� ���������
 if (!adc.sensors_ready)
  return;

 adc.sensors_ready = 0;
 ADMUX = ADCI_MAP|ADC_VREF_TYPE;
 if (speed2x)
  CLEARBIT(ADCSRA, ADPS0); //250kHz
 else
  SETBIT(ADCSRA, ADPS0);   //125kHz
 SETBIT(ADCSRA, ADSC);
}

void adc_begin_measure_knock(uint8_t speed2x)
{
 //�� �� ����� ��������� ����� ���������, ���� ��� �� �����������
 //���������� ���������
 if (!adc.sensors_ready)
  return;

 adc.sensors_ready = 0;
 adc.measure_all = 1;   //<--one measurement delay will be used
 ADMUX = ADCI_KNOCK|ADC_VREF_TYPE;
 if (speed2x)
  CLEARBIT(ADCSRA, ADPS0); //250kHz
 else
  SETBIT(ADCSRA, ADPS0);   //125kHz
 SETBIT(ADCSRA, ADSC);
}

void adc_begin_measure_all(void)
{
 adc.measure_all = 1;
 adc_begin_measure(0); //<--normal speed
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
 //���������� �������� �������� ���������� ��� ������� ������� �� ����� VREF_5V, ���������� ���������
 ADMUX=ADC_VREF_TYPE;
 ADCSRA=_BV(ADEN)|_BV(ADIE)|_BV(ADPS2)|_BV(ADPS1)|_BV(ADPS0);

 //������ ��� ����� � ������ ���������
 adc.sensors_ready = 1;

 //��������� ���������� - �� ��� �� �����
 ACSR=_BV(ACD);
}

/**���������� �� ���������� �������������� ���. ��������� �������� ���� ���������� ��������. ����� �������
 * ��������� ��� ���������� ����� ���������� ��� ������� �����, �� ��� ��� ���� ��� ����� �� ����� ����������.
 */
ISR(ADC_vect)
{
 _ENABLE_INTERRUPT();

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
   ADMUX = ADCI_CARB|ADC_VREF_TYPE;
   SETBIT(ADCSRA,ADSC);
   break;

  case ADCI_CARB:
   adc.carb_value = ADC;
   ADMUX = ADCI_ADD_IO1|ADC_VREF_TYPE;
   SETBIT(ADCSRA,ADSC);

#if defined(FUEL_INJECT) || defined(GD_CONTROL)
   if ((TCNT1 - adc.tpsdot[1].tps_tmr) >= TPSDOT_TIME_DELTA)
   {
    //save values for TPSdot calculations
    adc.tpsdot[1] = adc.tpsdot[0];          //previous = current
    adc.tpsdot[0].tps_volt = ADC;           //save voltage
    adc.tpsdot[0].tps_tmr = TCNT1;          //save timer value
   }
#endif
   break;

  case ADCI_ADD_IO1:
   adc.add_io1_value = ADC;
   ADMUX = ADCI_ADD_IO2|ADC_VREF_TYPE;
   SETBIT(ADCSRA,ADSC);
   break;

  case ADCI_ADD_IO2:
   adc.add_io2_value = ADC;

#ifdef PA4_INP_IGNTIM
   ADMUX = ADCI_PA4|ADC_VREF_TYPE;
   SETBIT(ADCSRA,ADSC);
   break;

  case ADCI_PA4:
   adc.pa4_value = ADC;
#endif
   if (0==adc.measure_all)
   {
    ADMUX = ADCI_MAP|ADC_VREF_TYPE;
    adc.sensors_ready = 1; //finished
   }
   else
   { //continue (knock)
    adc.measure_all = 0;
    ADMUX = ADCI_KNOCK|ADC_VREF_TYPE;
    SETBIT(ADCSRA,ADSC);
   }
   break;

  case ADCI_KNOCK://��������� ��������� ������� � ����������� ������ ���������
   if (adc.measure_all)
   {                      //waste measurement is required (for delay)
    adc.measure_all = 0;
    SETBIT(ADCSRA,ADSC);  //change nothing and start ADC again
    break;
   }

   adc.knock_value = ADC;
   adc.sensors_ready = 1;
   break;
 }
}

int16_t adc_compensate(int16_t adcvalue, int16_t factor, int32_t correction)
{
 return (((((int32_t)adcvalue*factor)+correction)<<2)>>16);
}

uint16_t map_adc_to_kpa(int16_t adcvalue, int16_t offset, int16_t gradient)
{
 int16_t t;
 //��� �� �������� ������������� ����������, ������ ������������� �������� ����� �������� ����� ����������� ������������.
 //����� ��� ������� ���������� �������������.
 if (adcvalue < 0)
  adcvalue = 0;

 //��������� �������� ���: ((adcvalue + offset) * gradient ) / 128, ��� offset,gradient - ���������.
 t = adcvalue + offset;
 if (gradient > 0)
 {
  if (t < 0)
   t = 0;    //restrict value
 }
 else
 {
  if (t > 0)
   t = 0;    //restrict value
 }
 return ( ((int32_t)t) * gradient ) >> 7;
}

uint16_t ubat_adc_to_v(int16_t adcvalue)
{
 if (adcvalue < 0)
  adcvalue = 0;
 return adcvalue;
}

//Coolant sensor has linear output. 10mV per C (e.g. LM235)
int16_t temp_adc_to_c(int16_t adcvalue)
{
 if (adcvalue < 0)
  adcvalue = 0;
 return (adcvalue - ((int16_t)((TSENS_ZERO_POINT / ADC_DISCRETE)+0.5)) );
}

uint8_t tps_adc_to_pc(int16_t adcvalue, int16_t offset, int16_t gradient)
{
 int16_t t;
 if (adcvalue < 0)
  adcvalue = 0;
 t = adcvalue + offset;
 if (gradient > 0)
 {
  if (t < 0)
   t = 0;
 }
 else
 {
  if (t > 0)
   t = 0;
 }

 t = (((int32_t)t) * gradient) >> (7+6);

 //TODO: use restrict_value function instead of following code
 if (t > TPS_MAGNITUDE(100)) //restrict to 100%
  t = TPS_MAGNITUDE(100);
 if (t < TPS_MAGNITUDE(0))
  t = TPS_MAGNITUDE(0);

 return t;
}

#if defined(FUEL_INJECT) || defined(GD_CONTROL)
int16_t tpsdot_adc_to_pc(int16_t adcvalue, int16_t gradient)
{
 return (((int32_t)adcvalue) * gradient) >> (7+6+1);
}
#endif
