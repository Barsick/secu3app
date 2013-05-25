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

/** \file hall.c
 * Implementation of Hall sensor's synchronization processing.
 * (���������� ��������� ������������� �� ������� �����).
 */

#ifdef HALL_SYNC

#include <stdlib.h>
#include "port/avrio.h"
#include "port/interrupt.h"
#include "port/intrinsic.h"
#include "port/port.h"
#include "adc.h"
#include "bitmask.h"
#include "ckps.h"
#include "ioconfig.h"
#include "magnitude.h"

#include "secu3.h"
#include "knock.h"

//Phase sensor and phased ignition are not supported when hall sensor is used for synchronization
#if defined(PHASE_SENSOR) || defined(PHASED_IGNITION)
 #error "You can not use phase sensor and phased ignition when Hall sensor is used for synchronization"
#endif
//Hall sensor synchronization is not supported by SECU-3 because classic SECU-3 HW use input which
//has no interrupt ability
#ifndef SECU3T
 #error "Hall sensor synchronization is not supported by SECU-3, please define SECU3T if you use SECU-3T"
#endif
//Dwell control, is not supported when synchronization from a Hall sensor is selected!
#ifdef DWELL_CONTROL
 #error "Dwell control is not supported when synchronization from Hall sensor is selected!"
#endif

/**Maximum number of ignition channels (cylinders) */
#define IGN_CHANNELS_MAX      8

//Define values for controlling of outputs
#define IGN_OUTPUTS_INIT_VAL 1        //!< value used for initialization
#define IGN_OUTPUTS_ON_VAL   1        //!< value used to turn on ignition channel
#define IGN_OUTPUTS_OFF_VAL  0        //!< value used to turn off ignition channel

/**Calibration constant used to compensate delay in interrupts (ticks of timer 1) */
#define CALIBRATION_DELAY    2

//Flags (see flags variable)
#define F_ERROR     0                 //!< Hall sensor error flag, set in the Hall sensor interrupt, reset after processing (������� ������ ��, ��������������� � ���������� �� ��, ������������ ����� ���������)
#define F_HALLSIA   1                 //!< Indicates that Hall sensor(input) is available
#define F_VHTPER    2                 //!< used to indicate that measured period is valid (actually measured)
#define F_STROKE    3                 //!< flag for synchronization with rotation (���� ������������� � ���������)
#define F_USEKNK    4                 //!< flag which indicates using of knock channel (������� ������������� ������ ���������)
#define F_HALLEV    5                 //!< flag indicates presence of Hall sensor event
#define F_IGNIEN    6                 //!< Ignition enabled/disabled
#define F_SPSIGN    7                 //!< Sign of the measured stroke period (time between TDCs)

//Additional flags (see flags2 variable)
#define F_SHUTTER   0                 //!< indicates using of shutter entering for spark generation (used at startup)
#define F_SHUTTER_S 1                 //!< synchronized value of F_SHUTTER

/**Advance value of distributor (angle * ANGLE_MULTIPLAYER)*/
#define HALL_ADVANCE (60*ANGLE_MULTIPLAYER)

/** State variables */
typedef struct
{
 uint16_t measure_start_value;        //!< previous value if timer 1 used for calculation of stroke period
 volatile uint16_t stroke_period;     //!< stores the last measurement of 1 stoke (������ ��������� ��������� ������� ����� ���������)
 volatile uint8_t chan_number;        //!< number of ignition channels (���-�� ������� ���������)
 uint32_t frq_calc_dividend;          //!< divident for calculating of RPM (������� ��� ������� ������� ��������)
 volatile int16_t  advance_angle;     //!< required adv.angle * ANGLE_MULTIPLAYER (��������� ��� * ANGLE_MULTIPLAYER)
 volatile uint8_t t1oc;               //!< Timer 1 overflow counter
 volatile uint8_t t1oc_s;             //!< Contains value of t1oc synchronized with stroke_period value
 volatile fnptr_t io_callback;        //!< Callback used to set state of ignition channel (we use single channel)
 volatile uint16_t degrees_per_stroke;//!< Number of degrees which corresponds to the 1 stroke (���������� �������� ������������ �� 1 ���� ���������)
#ifdef STROBOSCOPE
 uint8_t strobe;                      //!< Flag indicates that strobe pulse must be output on pending ignition stroke
#endif
 volatile uint8_t knkwnd_mode;        //!< used to indicate that knock measuring window is opened
 volatile int16_t knock_wnd_begin;    //!< begin of the phase selection window of detonation in degrees * ANGLE_MULTIPLAYER, relatively to TDC (������ ���� ������� �������� ��������� � �������� ������������ �.�.�)
 volatile int16_t knock_wnd_end;      //!< width of the  phase selection window of detonation in degrees * ANGLE_MULTIPLAYER, (������ ���� ������� �������� ��������� � ��������)
}hallstate_t;

hallstate_t hall;                     //!< instance of state variables

/** Arrange flags in the free I/O register (��������� � ��������� �������� �����/������) 
 *  note: may be not effective on other MCUs or even case bugs! Be aware.
 */
#define flags  TWAR
#define flags2 TWBR

/** Supplement timer/counter 0 up to 16 bits, use R15 (��� ���������� �������/�������� 0 �� 16 ��������, ���������� R15) */
#ifdef __ICCAVR__
 __no_init __regvar uint8_t TCNT0_H@15;
#else //GCC
 uint8_t TCNT0_H __attribute__((section (".noinit")));
#endif

/**Table srtores dividends for calculating of RPM */
#define FRQ_CALC_DIVIDEND(channum) PGM_GET_DWORD(&frq_calc_dividend[channum])
prog_uint32_t frq_calc_dividend[1+IGN_CHANNELS_MAX] =
 //     1          2          3          4         5         6         7         8
 {0, 30000000L, 15000000L, 10000000L, 7500000L, 6000000L, 5000000L, 4285714L, 3750000L};

void ckps_init_state_variables(void)
{
 _BEGIN_ATOMIC_BLOCK();

 hall.stroke_period = 0xFFFF;
 hall.advance_angle = HALL_ADVANCE; //=0

 CLEARBIT(flags, F_STROKE);
 CLEARBIT(flags, F_VHTPER);
 CLEARBIT(flags, F_HALLEV);
 CLEARBIT(flags, F_SPSIGN);
 SETBIT(flags2, F_SHUTTER);
 SETBIT(flags2, F_SHUTTER_S);
 SETBIT(flags, F_IGNIEN);

 TCCR0 = 0; //timer is stopped (������������� ������0)

 MCUCR|=_BV(ISC11); //falling edge for INT1
 MCUCR|=_BV(ISC10);

 TIMSK|=_BV(TOIE1);                   //enable Timer 1 overflow interrupt. Used for correct calculation of very low RPM
 hall.t1oc = 0;                       //reset overflow counter
 hall.t1oc_s = 255;                   //RPM is very low

#ifdef STROBOSCOPE
 hall.strobe = 0;
#endif

 hall.knkwnd_mode = 0;
 _END_ATOMIC_BLOCK();
}

void ckps_init_state(void)
{
 _BEGIN_ATOMIC_BLOCK();
 ckps_init_state_variables();
 CLEARBIT(flags, F_ERROR);

 MCUCR|=_BV(ISC11); //falling edge for INT1
 MCUCR|=_BV(ISC10);

 //set flag indicating that Hall sensor input is available
 WRITEBIT(flags, F_HALLSIA, IOCFG_CHECK(IOP_PS));
 GICR|=  CHECKBIT(flags, F_HALLSIA) ? _BV(INT1) : 0; //INT1 enabled only when Hall sensor input is available

 //Compare channels do not connected to lines of ports (normal port mode)
 //(������ Compare �� ���������� � ������ ������ (���������� ����� ������))
 TCCR1A = 0;

 //Tune timer 1 (clock = 250kHz)
 TCCR1B = _BV(CS11)|_BV(CS10);

 //enable overflow interrupt of timer 0
 //(��������� ���������� �� ������������ ������� 0)
 TIMSK|= _BV(TOIE0);
 _END_ATOMIC_BLOCK();
}

void ckps_set_advance_angle(int16_t angle)
{
 int16_t aad = (HALL_ADVANCE - angle);
 _BEGIN_ATOMIC_BLOCK();
 hall.advance_angle = aad;
 _END_ATOMIC_BLOCK();
}

void ckps_init_ports(void)
{
 PORTD|= _BV(PD6); // pullup for ICP1 (�������� ��� ICP1)

 //after ignition is on, igniters must not be in the accumulation mode,
 //therefore set low level on their inputs
 //(����� ��������� ��������� ����������� �� ������ ���� � ������ ����������,
 //������� ������������� �� �� ������ ������ �������)
 IOCFG_INIT(IOP_IGN_OUT1, IGN_OUTPUTS_INIT_VAL);        //init 1-st (can be remapped)
 IOCFG_INIT(IOP_IGN_OUT2, IGN_OUTPUTS_INIT_VAL);        //init 2-nd (can be remapped)
 IOCFG_INIT(IOP_IGN_OUT3, IGN_OUTPUTS_INIT_VAL);        //init 3-rd (can be remapped)
 IOCFG_INIT(IOP_IGN_OUT4, IGN_OUTPUTS_INIT_VAL);        //init 4-th (can be remapped)
 IOCFG_INIT(IOP_ADD_IO1, IGN_OUTPUTS_INIT_VAL);         //init 5-th (can be remapped)
 IOCFG_INIT(IOP_ADD_IO2, IGN_OUTPUTS_INIT_VAL);         //init 6-th (can be remapped)
 IOCFG_INIT(IOP_IGN_OUT7, IGN_OUTPUTS_INIT_VAL);        //init 7-th (for maniacs)
 IOCFG_INIT(IOP_IGN_OUT8, IGN_OUTPUTS_INIT_VAL);        //init 8-th (for maniacs)

 //init I/O for Hall output if it is enabled
#ifdef HALL_OUTPUT
 IOCFG_INIT(IOP_HALL_OUT, 1);
#endif

 //init I/O for stroboscope
#ifdef STROBOSCOPE
 IOCFG_INIT(IOP_STROBE, 0);
#endif
}

//Instantaneous frequency calculation of crankshaft rotation from the measured period between the engine strokes
//(for example for 4-cylinder, 4-stroke it is 180�)
//Period measured in the discretes of timer (one discrete = 4us), one minute = 60 seconds, one second has 1,000,000 us.
//������������ ���������� ������� �������� ��������� �� ����������� ������� ����� ������� ���������
//(�������� ��� 4-������������, 4-� �������� ��� 180 ��������)
//������ � ��������� ������� (���� �������� = 4���), � ����� ������ 60 ���, � ����� ������� 1000000 ���.
uint16_t ckps_calculate_instant_freq(void)
{
 uint16_t period; uint8_t ovfcnt, sign;
 //ensure atomic acces to variable (������������ ��������� ������ � ����������)
 _DISABLE_INTERRUPT();
 period = hall.stroke_period;        //stroke period
 ovfcnt = hall.t1oc_s;               //number of timer overflows
 sign = CHECKBIT(flags, F_SPSIGN);   //sign of stroke period
 _ENABLE_INTERRUPT();

 //We know period and number of timer overflows, so we can calculate correct value of RPM even if RPM is very low
 if (sign && ovfcnt > 0)
  return hall.frq_calc_dividend / ((((int32_t)ovfcnt) * 65536) - (65536-period));
 else
  return hall.frq_calc_dividend / ((((int32_t)ovfcnt) * 65536) + period);
}

void ckps_set_edge_type(uint8_t edge_type)
{
 //Not used by Hall sensor, we decided to select edge by reading I/O remapping inversion flag.
 //but for now it is not implemented.
}

void ckps_set_cogs_btdc(uint8_t cogs_btdc)
{
 //not supported by Hall sensor
}

void ckps_set_ignition_cogs(uint8_t cogs)
{
 //not supported by Hall sensor
}

uint8_t ckps_is_error(void)
{
 return CHECKBIT(flags, F_ERROR) > 0;
}

void ckps_reset_error(void)
{
 CLEARBIT(flags, F_ERROR);
}

void ckps_use_knock_channel(uint8_t use_knock_channel)
{
 WRITEBIT(flags, F_USEKNK, use_knock_channel);
}

uint8_t ckps_is_stroke_event_r()
{
 uint8_t result;
 _BEGIN_ATOMIC_BLOCK();
 result = CHECKBIT(flags, F_STROKE) > 0;
 CLEARBIT(flags, F_STROKE);
 _END_ATOMIC_BLOCK();
 return result;
}

uint8_t ckps_is_cog_changed(void)
{
 uint8_t result;
 _BEGIN_ATOMIC_BLOCK();
 result = CHECKBIT(flags, F_HALLEV) > 0;
 CLEARBIT(flags, F_HALLEV);
 _END_ATOMIC_BLOCK();
 return result;
}

void ckps_set_cyl_number(uint8_t i_cyl_number)
{
 uint16_t degrees_per_stroke;
 uint8_t _t;
 _t = _SAVE_INTERRUPT();
 _DISABLE_INTERRUPT();
 hall.chan_number = i_cyl_number; //set new value
 _RESTORE_INTERRUPT(_t);

 hall.frq_calc_dividend = FRQ_CALC_DIVIDEND(i_cyl_number);

 //precalculate value of degrees per 1 engine stroke (value * ANGLE_MULTIPLAYER)
 degrees_per_stroke = (720 * ANGLE_MULTIPLAYER) / i_cyl_number;

 _t = _SAVE_INTERRUPT();
 _DISABLE_INTERRUPT();
 hall.io_callback = IOCFG_CB(IOP_IGN_OUT1); //use single output
 hall.degrees_per_stroke = degrees_per_stroke;
 _RESTORE_INTERRUPT(_t);
}

void ckps_set_knock_window(int16_t begin, int16_t end)
{
 int16_t begin_d = (HALL_ADVANCE + begin); //start of window
 int16_t end_d = (end - begin); //width of window
 _BEGIN_ATOMIC_BLOCK();
 hall.knock_wnd_begin = begin_d;
 hall.knock_wnd_end = end_d;
 _END_ATOMIC_BLOCK();
}

void ckps_enable_ignition(uint8_t i_cutoff)
{
 WRITEBIT(flags, F_IGNIEN, i_cutoff); //enable/disable ignition
}

void ckps_set_merge_outs(uint8_t i_merge)
{
 //not suitable when Hall sensor synchronization is used
}

#ifdef HALL_OUTPUT
void ckps_set_hall_pulse(int8_t i_offset, uint8_t i_duration)
{
 //not supported by Hall sensor
}
#endif

void ckps_set_cogs_num(uint8_t norm_num, uint8_t miss_num)
{
 //not supported by Hall sensor
}

void ckps_set_shutter_spark(uint8_t i_shutter)
{
 WRITEBIT(flags2, F_SHUTTER, i_shutter);
}

/** Turn OFF specified ignition channel
 * \param i_channel number of ignition channel to turn off
 */
INLINE
void turn_off_ignition_channel(void)
{
 if (!CHECKBIT(flags, F_IGNIEN))
  return; //ignition disabled
 //Completion of igniter's ignition drive pulse, transfer line of port into a low level - makes
 //the igniter go to the regime of energy accumulation
 //���������� �������� ������� �����������, ������� ����� ����� � ������ ������� - ����������
 //���������� ������� � ����� ���������� �������
 ((iocfg_pfn_set)hall.io_callback)(IGN_OUTPUTS_OFF_VAL);
}

/**Interrupt handler for Compare/Match channel A of timer T1
 * ������ ���������� �� ���������� ������ � ������� �1
 */
ISR(TIMER1_COMPA_vect)
{
 uint16_t tmr = TCNT1;
 ((iocfg_pfn_set)hall.io_callback)(IGN_OUTPUTS_ON_VAL);
 TIMSK&= ~_BV(OCIE1A); //disable interrupt (��������� ����������)

 //-----------------------------------------------------
 //Software PWM is very sensitive even to small delays. So, we need to allow OCF2 and TOV2
 //interrupts occur during processing of this handler.
#ifdef COOLINGFAN_PWM
 _ENABLE_INTERRUPT();
#endif
 //-----------------------------------------------------

 //set timer for pulse completion, use fast division by 3
 if ((CHECKBIT(flags, F_SPSIGN) && hall.t1oc_s < 2) || (!CHECKBIT(flags, F_SPSIGN) && !hall.t1oc_s))
  OCR1B = tmr + (((uint32_t)hall.stroke_period * 0xAAAB) >> 17); //pulse width = 1/3
 else
  OCR1B = tmr + 21845;  //pulse width is limited to 87.38ms

#ifdef COOLINGFAN_PWM
 _DISABLE_INTERRUPT();
#endif

 TIFR = _BV(OCF1B);
 TIMSK|= _BV(OCIE1B);

#ifdef HALL_OUTPUT
 IOCFG_SET(IOP_HALL_OUT, 1);//begin of pulse
#endif

#ifdef STROBOSCOPE
 if (1==hall.strobe)
 {
  IOCFG_SET(IOP_STROBE, 1); //start pulse
  hall.strobe = 2;          //and set flag to next state
  OCR1A = TCNT1 + 25;       //We will generate 100uS pulse
  TIMSK|= _BV(OCIE1A);      //pulse will be ended in the next interrupt
 }
 else if (2==hall.strobe)
 {
  IOCFG_SET(IOP_STROBE, 0); //end pulse
  hall.strobe = 0;          //and reset flag
  return;
 }
#endif
}

/**Interrupt handler for Compare/Match channel B of timer T1.
 * ������ ���������� �� ���������� ������ B ������� �1.
 */
ISR(TIMER1_COMPB_vect)
{
 turn_off_ignition_channel();//finish ignition pulse
 TIMSK&= ~_BV(OCIE1B);      //disable interrupt (��������� ����������)
#ifdef HALL_OUTPUT
 IOCFG_SET(IOP_HALL_OUT, 0);//end of pulse
#endif
}

/**Initialization of timer 0 using specified value and start, clock = 250kHz
 * It is assumed that this function called when all interrupts are disabled
 * (������������� ������� 0 ��������� ��������� � ������, clock = 250kHz.
 * �������������� ��� ����� ���� ������� ����� ����������� ��� ����������� �����������)
 * \param value value for load into timer (�������� ��� �������� � ������)
 */
INLINE
void set_timer0(uint16_t value)
{
 TCNT0_H = _AB(value, 1);
 TCNT0 = ~(_AB(value, 0));  //One's complement is faster than 255 - low byte
 TCCR0  = _BV(CS01)|_BV(CS00);
}

/**Input capture interrupt of timer 1 (���������� �� ������� ������� 1) */
ISR(TIMER1_CAPT_vect)
{
 //not used by Hall sensor
}

/**Interrupt from a Hall sensor*/
ISR(INT1_vect)
{
 uint16_t tmr = TCNT1; //remember current value of timer 1
 //toggle edge
 if (MCUCR & _BV(ISC10))
 { //falling

  //save period value if it is correct. We need to do it forst of all to have fresh stroke_period value
  if (CHECKBIT(flags, F_VHTPER))
  {
   //calculate stroke period
   hall.stroke_period = tmr - hall.measure_start_value;
   WRITEBIT(flags, F_SPSIGN, tmr < hall.measure_start_value); //save sign
   hall.t1oc_s = hall.t1oc, hall.t1oc = 0; //save value and reset counter
  }
  SETBIT(flags, F_VHTPER);
  SETBIT(flags, F_STROKE); //set the stroke-synchronization event (������������� ������� �������� �������������)
  hall.measure_start_value = tmr;

  if (!CHECKBIT(flags2, F_SHUTTER_S))
  {
   uint16_t delay;
#ifdef STROBOSCOPE
   hall.strobe = 1; //strobe!
#endif

   //-----------------------------------------------------
   //Software PWM is very sensitive even to small delays. So, we need to allow OCF2 and TOV2
   //interrupts occur during processing of this handler.
#ifdef COOLINGFAN_PWM
   _ENABLE_INTERRUPT();
#endif
   //-----------------------------------------------------

   //start timer for counting out of advance angle (spark)
   delay = (((uint32_t)hall.advance_angle * hall.stroke_period) / hall.degrees_per_stroke);
#ifdef COOLINGFAN_PWM
   _DISABLE_INTERRUPT();
#endif

   OCR1A = tmr + ((delay < 15) ? 15 : delay) - CALIBRATION_DELAY; //set compare channel, additionally prevent spark missing when advance angle is near to 60�
   TIFR = _BV(OCF1A);
   TIMSK|= _BV(OCIE1A);

   //start timer for countiong out of knock window opening
   if (CHECKBIT(flags, F_USEKNK))
   {
#ifdef COOLINGFAN_PWM
   _ENABLE_INTERRUPT();
#endif
    delay = ((uint32_t)hall.knock_wnd_begin * hall.stroke_period) / hall.degrees_per_stroke;
#ifdef COOLINGFAN_PWM
   _DISABLE_INTERRUPT();
#endif
    set_timer0(delay);
    hall.knkwnd_mode = 0;
   }

   knock_start_settings_latching();//start the process of downloading the settings into the HIP9011 (��������� ������� �������� �������� � HIP)
   adc_begin_measure(_AB(hall.stroke_period, 1) < 4);//start the process of measuring analog input values (������ �������� ��������� �������� ���������� ������)
  }

  MCUCR&= ~(_BV(ISC10));  //next edge will be rising
 }
 else
 { //rising
  //spark on rising at the startup, force COMPA interrupt
  if (CHECKBIT(flags2, F_SHUTTER_S))
  {

#ifdef STROBOSCOPE
   hall.strobe = 1; //strobe!
#endif

   OCR1A = TCNT1 + 2;
   TIFR = _BV(OCF1A);
   TIMSK|= _BV(OCIE1A);
  }

  MCUCR|=_BV(ISC10); //next will be falling
 }

 WRITEBIT(flags2, F_SHUTTER_S, CHECKBIT(flags2, F_SHUTTER)); //synchronize
 SETBIT(flags, F_HALLEV); //set event flag
}

/**Purpose of this interrupt handler is to supplement timer up to 16 bits and call procedures
 * for opening and closing knock measuring window
 * (������ ����� ����������� ��������� ������ �� 16-�� �������� � �������� ���������
 * ��������/�������� ���� ��������� ������ ��������� �� ��������� �������������� 16-�� ���������� �������). */
ISR(TIMER0_OVF_vect)
{
 if (TCNT0_H!=0)  //Did high byte exhaust (������� ���� �� ��������) ?
 {
  TCNT0 = 0;
  --TCNT0_H;
 }
 else
 {//the countdown is over (������ ������� ����������)
  TCCR0 = 0;     //stop timer (������������� ������)

  if (!hall.knkwnd_mode)
  {//start listening detonation (opening the window)
   uint16_t delay;
   knock_set_integration_mode(KNOCK_INTMODE_INT);
   ++hall.knkwnd_mode;
   //-----------------------------------------------------
   //Software PWM is very sensitive even to small delays. So, we need to allow OCF2 and TOV2
   //interrupts occur during processing of this handler.
#ifdef COOLINGFAN_PWM
   _ENABLE_INTERRUPT();
#endif
   //-----------------------------------------------------
   delay = ((uint32_t)hall.knock_wnd_end * hall.stroke_period) / hall.degrees_per_stroke;
#ifdef COOLINGFAN_PWM
   _DISABLE_INTERRUPT();
#endif
   set_timer0(delay);
  }
  else
  {//finish listening a detonation (closing the window) and start the process of measuring integrated value
   knock_set_integration_mode(KNOCK_INTMODE_HOLD);
   adc_begin_measure_knock(_AB(hall.stroke_period, 1) < 4);
   hall.knkwnd_mode = 0;
  }
 }
}

/** Timer 1 overflow interrupt.
 * Used to count timer 1 overflows to obtain correct revolution period at very low RPMs (10...400 min-1)
 */
ISR(TIMER1_OVF_vect)
{
 ++hall.t1oc;
}

#endif //HALL_SYNC
