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

/** \file secu3.h
 * Main module of the firmware.
 * This file contains main data structures used in the firmware.
 * (������� ������ ��������. ���� ���� �������� �������� ��������� ������ ������������ � ��������).
 */

#ifndef _SECU3_H_
#define _SECU3_H_

#include "tables.h"

#define SAVE_PARAM_TIMEOUT_VALUE      3000  //!< timeout value used to count time before automatic saving of parameters
#define FORCE_MEASURE_TIMEOUT_VALUE   20    //!< timeout value used to perform measurements when engine is stopped
#define CE_CONTROL_STATE_TIME_VALUE   50    //!< used for CE (flashing)
#ifdef HALL_SYNC
#define ENGINE_ROTATION_TIMEOUT_VALUE 150   //!< timeout value used to determine that engine is stopped (used for Hall sensor)
#else
#define ENGINE_ROTATION_TIMEOUT_VALUE 20    //!< timeout value used to determine that engine is stopped (this value must not exceed 25)
#endif
#define IDLE_PERIOD_TIME_VALUE        50    //!< used by idling regulator

#ifdef DIAGNOSTICS
/**Describes diagnostics inputs data */
typedef struct diagnost_inp_t
{
 uint16_t voltage;                       //!< board voltage
 uint16_t map;                           //!< MAP sensor
 uint16_t temp;                          //!< coolant temperature
 uint16_t add_io1;                       //!< additional input 1 (analog)
 uint16_t add_io2;                       //!< additional input 2 (analog)
 uint16_t carb;                          //!< carburetor switch, throttle position sensor (analog)
 uint8_t  bits;                          //!< bits describing states of: gas valve, CKP sensor, VR type cam sensor, Hall-effect cam sensor, BL jmp, DE jmp
 uint16_t ks_1;                          //!< knock sensor 1
 uint16_t ks_2;                          //!< knock sensor 2
}diagnost_inp_t;
#endif

/**Describes all system's inputs - theirs derivative and integral magnitudes
 * ��������� ��� ����� ������� - �� ����������� � ������������ ��������
 */
typedef struct sensors_t
{
 uint16_t map;                           //!< Input Manifold Pressure (�������� �� �������� ���������� (�����������))
 uint16_t voltage;                       //!< Board voltage (���������� �������� ���� (�����������))
 int16_t  temperat;                      //!< Coolant temperature (����������� ����������� �������� (�����������))
 uint16_t frequen;                       //!< Averaged RPM (������� �������� ��������� (�����������))
 uint16_t inst_frq;                      //!< Instant RPM - not averaged  (���������� ������� ��������)
 uint8_t  carb;                          //!< State of carburetor's limit switch (��������� ��������� �����������)
 uint8_t  gas;                           //!< State of gas valve (��������� �������� �������)
 uint16_t knock_k;                       //!< Knock signal level (������� ������� ���������)
 uint8_t  tps;                           //!< Throttle position sensor (0...100%, x2)
 uint16_t add_i1;                        //!< ADD_I1 input voltage
 uint16_t add_i2;                        //!< ADD_I2 input voltage
#if defined(SPEED_SENSOR) && defined(SECU3T)
 uint16_t speed;                         //!< Vehicle speed expressed by period between speed sensor pulses (1 tick = 4us)
 uint32_t distance;                      //!< Distance expressed by number of speed sensor pulses since last ignition turn on
#endif
#if defined(AIRTEMP_SENS) && defined(SECU3T)
 int16_t air_temp;                       //!< Intake air temperature
#endif

 //����� �������� �������� (�������� ��� � ����������������� �������������)
 int16_t  map_raw;                       //!< raw ADC value from MAP sensor
 int16_t  voltage_raw;                   //!< raw ADC value from voltage
 int16_t  temperat_raw;                  //!< raw ADC value from coolant temperature sensor
 int16_t  tps_raw;                       //!< raw ADC value from TPS sensor
 int16_t  add_i1_raw;                    //!< raw ADC value from ADD_I1 input
 int16_t  add_i2_raw;                    //!< raw ADC value from ADD_I2 input
}sensors_t;

typedef struct correct_t
{
 int16_t curr_angle;                     //!< Current advance angle (������� ���� ����������)
 int16_t knock_retard;                   //!< Correction of advance angle from knock detector (�������� ��� �� ���������� �� ���������)
 int16_t idlreg_aac;                     //!< Idle regulator advance angle correction
 int16_t octan_aac;                      //!< Octane advance angle correction
 int16_t strt_aalt;                      //!< Advance angle from start map
 int16_t idle_aalt;                      //!< Advance angle from idle map
 int16_t work_aalt;                      //!< Advance angle from work map
 int16_t temp_aalt;                      //!< Advance angle from coolant temp. corr. map
 int16_t airt_aalt;                      //!< Advance angle from air temp. corr. map
#ifdef FUEL_INJECT
 int16_t lambda;                         //!< Current value of lambda (EGO) correction, can be negative
#endif
}correct_t;
 
/**Describes system's data (main ECU data structure)
 * ��������� ������ �������, ������������ ������ ��������� ������
 */
typedef struct ecudata_t
{
 struct params_t  param;                 //!< --parameters (���������)
 struct sensors_t sens;                  //!< --sensors (�������)
 struct correct_t corr;                  //!< --calculated corrections and lookup tables' values

 uint8_t  ie_valve;                      //!< State of Idle Economizer valve (��������� ������� ����)
 uint8_t  fe_valve;                      //!< State of Fuel Economizer valve (��������� ������� ���)
 uint8_t  cool_fan;                      //!< State of the cooling fan (��������� ������������������)
 uint8_t  st_block;                      //!< State of the starter blocking output (��������� ������ ���������� ��������)
 uint8_t  ce_state;                      //!< State of CE lamp (��������� ����� "CE")
 uint8_t  airflow;                       //!< Air flow (������ �������)
 uint8_t  choke_pos;                     //!< Choke position in % * 2

#ifdef REALTIME_TABLES
 f_data_t tables_ram;                    //!< set of tables in RAM
 uint8_t mm_ram;                         //!< flag indicated that selected set of tables is in RAM
#endif
 f_data_t _PGM *fn_dat;                  //!< Pointer to the set of tables (��������� �� ����� �������������)

 uint16_t op_comp_code;                  //!< Contains code of operation for packet being sent - OP_COMP_NC (�������� ��� ������� ���������� ����� UART (����� OP_COMP_NC))
 uint16_t op_actn_code;                  //!< Contains code of operation for packet being received - OP_COMP_NC (�������� ��� ������� ����������� ����� UART (����� OP_COMP_NC))
 uint16_t ecuerrors_for_transfer;        //!< Buffering of error codes being sent via UART in real time (������������ ���� ������ ������������ ����� UART � �������� �������)
 uint16_t ecuerrors_saved_transfer;      //!< Buffering of error codes for read/write from/to EEPROM which is being sent/received (������������ ���� ������ ��� ������/������ � EEPROM, ������������/����������� ����� UART)
 uint8_t  use_knock_channel_prev;        //!< Previous state of knock channel's usage flag (���������� ��������� �������� ������������� ������ ���������)

 //TODO: To reduce memory usage it is possible to use crc or some simple hash algorithms to control state of memory(changes). So this variable becomes unneeded.
 uint8_t* eeprom_parameters_cache;       //!< Pointer to an array of EEPROM parameters cache (reduces redundant write operations)

 uint8_t engine_mode;                    //!< Current engine mode(start, idle, work) (������� ����� ��������� (����, ��, ��������))

#ifdef DIAGNOSTICS
 diagnost_inp_t diag_inp;                //!< diagnostic mode: values of inputs
 uint16_t       diag_out;                //!< diagnostic mode: values of outputs
#endif

 uint8_t choke_testing;                  //!< Used to indcate that choke testing is on/off (so it is applicable only if SM_CONTROL compilation option is used)
 int8_t choke_manpos_d;                  //!< Muanual position setting delta value used for choke control
 uint8_t choke_rpm_reg;                  //!< Used to indicate that at the moment system regulates RPM by means of choke position

 uint8_t bt_name[9];                     //!< received name for Bluetooth (8 chars max), zero element is size
 uint8_t bt_pass[7];                     //!< received password for Bluetooth (6 chars max), zero element is size
 uint8_t sys_locked;                     //!< used by immobilizer to indicate that system is locked

#ifdef FUEL_INJECT
 uint16_t inj_pw;                        //!< current value of injector pulse width
#endif
}ecudata_t;


#endif  //_SECU3_H_
