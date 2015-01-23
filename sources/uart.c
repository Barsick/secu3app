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

/** \file uart.c
 * Implementation of service for performing communication via UART.
 * (���������� ��������� ������ ������� ����� UART).
 */

#include "port/avrio.h"
#include "port/interrupt.h"
#include "port/intrinsic.h"
#include "port/pgmspace.h"
#include "port/port.h"
#include <string.h>
#include "bitmask.h"
#include "eeprom.h"
#include "secu3.h"
#include "uart.h"
#include "ufcodes.h"
#include "wdt.h"

//Idenfifiers used in EDITAB_PAR
#define ETTS_GASOLINE_SET 0 //!< tables's set: gasoline id
#define ETTS_GAS_SET      1 //!< tables's set: gas id

#define ETMT_STRT_MAP 0     //!< start map id
#define ETMT_IDLE_MAP 1     //!< idle map id
#define ETMT_WORK_MAP 2     //!< work map id
#define ETMT_TEMP_MAP 3     //!< temp.corr. map id
#define ETMT_NAME_STR 4     //!< name of tables's set id

/**Define internal state variables */
typedef struct
{
 uint8_t send_mode;                     //!< current descriptor of packets beeing send
 uint8_t recv_buf[UART_RECV_BUFF_SIZE]; //!< receiver's buffer
 uint8_t send_buf[UART_SEND_BUFF_SIZE]; //!< transmitter's buffer
 volatile uint8_t send_size;            //!< size of data to be send
 uint8_t send_index;                    //!< index in transmitter's buffer
 volatile uint8_t recv_size;            //!< size of received data
 uint8_t recv_index;                    //!< index in receiver's buffer
}uartstate_t;

/**State variables */
uartstate_t uart;

#ifdef UART_BINARY //binary mode
// There are several special reserved symbols in binary mode: 0x21, 0x40, 0x0D, 0x0A
#define FIBEGIN  0x21       //!< '!' indicates beginning of the ingoing packet
#define FOBEGIN  0x40       //!< '@' indicates beginning of the outgoing packet
#define FIOEND   0x0D       //!<'\r' indicates ending of the ingoing/outgoing packet
#define FESC     0x0A       //!<'\n' Packet escape (FESC)
// Following bytes are used only in escape sequeces and may appear in the data without any problems
#define TFIBEGIN 0x81       //!< Transposed FIBEGIN
#define TFOBEGIN 0x82       //!< Transposed FOBEGIN
#define TFIOEND  0x83       //!< Transposed FIOEND
#define TFESC    0x84       //!< Transposed FESC

/** Appends transmitter's buffer
 * \param b byte which will be used to append tx buffer
 */
INLINE
void append_tx_buff(uint8_t b)
{
 if (b == FOBEGIN)
 {
  uart.send_buf[uart.send_size++] = FESC;
  uart.send_buf[uart.send_size++] = TFOBEGIN;
 }
 else if ((b) == FIOEND)
 {
  uart.send_buf[uart.send_size++] = FESC;
  uart.send_buf[uart.send_size++] = TFIOEND;
 }
 else if ((b) == FESC)
 {
  uart.send_buf[uart.send_size++] = FESC;
  uart.send_buf[uart.send_size++] = TFESC;
 }
 else
  uart.send_buf[uart.send_size++] = b;
}

/** Takes out byte from receiver's buffer
 * \return byte retrieved from buffer
 */
INLINE
uint8_t takeout_rx_buff(void)
{
 uint8_t b1 = uart.recv_buf[uart.recv_index++];
 if (b1 == FESC)
 {
  uint8_t b2 = uart.recv_buf[uart.recv_index++];
  if (b2 == TFIBEGIN)
   return FIBEGIN;
  else if (b2 == TFIOEND)
   return FIOEND;
  else if (b2 == TFESC)
   return FESC;
  return 0; //wrong code
 }
 else
  return b1;
}

#else //HEX mode

/**For BIN-->HEX encoding */
PGM_DECLARE(uint8_t hdig[]) = "0123456789ABCDEF";

/**Decodes from HEX to BIN */
#define HTOD(h) (((h)<0x3A) ? ((h)-'0') : ((h)-'A'+10))

#endif

//--------��������������� ������� ��� ���������� �������-------------

/**Appends sender's buffer by sequence of bytes from program memory. This function is also used in the bluetooth module
 * note! can NOT be used for binary data! */
void build_fs(uint8_t _PGM *romBuffer, uint8_t size)
{
#ifdef UART_BINARY
 while(size--) append_tx_buff(PGM_GET_BYTE(romBuffer++));
#else
 memcpy_P(&uart.send_buf[uart.send_size], romBuffer, size);
 uart.send_size+=size;
#endif
}

/**Appends sender's buffer by sequence of bytes from RAM. This function is also used in the bluetooth module
 * note! can NOT be used for binary data! */
void build_rs(const uint8_t* ramBuffer, uint8_t size)
{
#ifdef UART_BINARY
 while(size--) append_tx_buff(*ramBuffer++);
#else
 memcpy(&uart.send_buf[uart.send_size], ramBuffer, size);
 uart.send_size+=size;
#endif
}

/**Appends sender's buffer by one HEX byte */
#ifdef UART_BINARY
#define build_i4h(i) {append_tx_buff((i));}
#else
static void build_i4h(uint8_t i)
{
 uart.send_buf[uart.send_size++] = ((i)+0x30);
}
#endif

/**Appends sender's buffer by two HEX bytes
 * \param i 8-bit value to be converted into hex
 */
static void build_i8h(uint8_t i)
{
#ifdef UART_BINARY
 append_tx_buff(i);           //1 ����
#else
 uart.send_buf[uart.send_size++] = PGM_GET_BYTE(&hdig[i/16]);          //������� ���� HEX �����
 uart.send_buf[uart.send_size++] = PGM_GET_BYTE(&hdig[i%16]);          //������� ���� HEX �����
#endif
}

/**Appends sender's buffer by 4 HEX bytes
 * \param i 16-bit value to be converted into hex
 */
static void build_i16h(uint16_t i)
{
#ifdef UART_BINARY
 append_tx_buff(_AB(i,1));    //������� ����
 append_tx_buff(_AB(i,0));    //������� ����
#else
 uart.send_buf[uart.send_size++] = PGM_GET_BYTE(&hdig[_AB(i,1)/16]);   //������� ���� HEX ����� (������� ����)
 uart.send_buf[uart.send_size++] = PGM_GET_BYTE(&hdig[_AB(i,1)%16]);   //������� ���� HEX ����� (������� ����)
 uart.send_buf[uart.send_size++] = PGM_GET_BYTE(&hdig[_AB(i,0)/16]);   //������� ���� HEX ����� (������� ����)
 uart.send_buf[uart.send_size++] = PGM_GET_BYTE(&hdig[_AB(i,0)%16]);   //������� ���� HEX ����� (������� ����)
#endif
}

static void build_i24h(uint32_t i)
{
 build_i8h(i>>16);
 build_i16h(i);
}

/**Appends sender's buffer by 8 HEX bytes
 * \param i 32-bit value to be converted into hex
 */
static void build_i32h(uint32_t i)
{
 build_i16h(i>>16);
 build_i16h(i);
}

/**Appends sender's buffer by sequence of bytes from program memory buffer
 * can be used for binary data */
static void build_fb(uint8_t _PGM *romBuffer, uint8_t size)
{
 while(size--) build_i8h(PGM_GET_BYTE(romBuffer++));
}

/**Appends sender's buffer by sequence of bytes from RAM buffer
 * can be used for binary data */
static void build_rb(const uint8_t* ramBuffer, uint8_t size)
{
 while(size--) build_i8h(*ramBuffer++);
}

//----------��������������� ������� ��� ������������� �������---------
/**Recepts sequence of bytes from receiver's buffer and places it into the RAM buffer
 * can NOT be used for binary data */
static void recept_rs(uint8_t* ramBuffer, uint8_t size)
{
#ifdef UART_BINARY
 while(size-- && uart.recv_index < uart.recv_size) *ramBuffer++ = takeout_rx_buff();
#else
 while(size-- && uart.recv_index < uart.recv_size) *ramBuffer++ = uart.recv_buf[uart.recv_index++];
#endif
}

/**Retrieves from receiver's buffer 4-bit value */
#ifdef UART_BINARY
#define recept_i4h() (takeout_rx_buff())
#else
static uint8_t recept_i4h(void)
{
 return uart.recv_buf[uart.recv_index++] - 0x30;
}
#endif

/**Retrieves from receiver's buffer 8-bit value
 * \return retrieved value
 */
static uint8_t recept_i8h(void)
{
#ifdef UART_BINARY
 return takeout_rx_buff();
#else
 uint8_t i8;
 i8 = HTOD(uart.recv_buf[uart.recv_index])<<4;
 ++uart.recv_index;
 i8|= HTOD(uart.recv_buf[uart.recv_index]);
 ++uart.recv_index;
 return i8;
#endif
}

/**Retrieves from receiver's buffer 16-bit value
 * \return retrieved value
 */
static uint16_t recept_i16h(void)
{
 uint16_t i16;
#ifdef UART_BINARY
 _AB(i16,1) = takeout_rx_buff(); //Hi byte
 _AB(i16,0) = takeout_rx_buff(); //Lo byte
#else
 _AB(i16,1) = (HTOD(uart.recv_buf[uart.recv_index]))<<4;
 ++uart.recv_index;
 _AB(i16,1)|= (HTOD(uart.recv_buf[uart.recv_index]));
 ++uart.recv_index;
 _AB(i16,0) = (HTOD(uart.recv_buf[uart.recv_index]))<<4;
 ++uart.recv_index;
 _AB(i16,0)|= (HTOD(uart.recv_buf[uart.recv_index]));
 ++uart.recv_index;
#endif
 return i16;
}

/**Retrieves from receiver's buffer 32-bit value
 * \return retrieved value
 */
static uint32_t recept_i32h(void)
{
 uint32_t i = 0;
 i = recept_i16h();
 i = i << 16;
 i|=recept_i16h();
 return i;
}

/**Recepts sequence of bytes from receiver's buffer and places it into the RAM buffer
 * can be used for binary data */
static void recept_rb(uint8_t* ramBuffer, uint8_t size)
{
 while(size-- && uart.recv_index < uart.recv_size) *ramBuffer++ = recept_i8h();
}
//--------------------------------------------------------------------

/**Makes sender to start sending */
void uart_begin_send(void)
{
 uart.send_index = 0;
 _DISABLE_INTERRUPT();
 UCSRB |= _BV(UDRIE); /* enable UDRE interrupt */
 _ENABLE_INTERRUPT();
}

void uart_send_packet(struct ecudata_t* d, uint8_t send_mode)
{
 static uint8_t index = 0;

 //������ �������� �� ����� ������ �������, � ����� ������ ����� ��������� ������ ������
 uart.send_size = 0;

 if (send_mode==0) //���������� ������� ����������
  send_mode = uart.send_mode;

 //����� ����� ��� ���� �������
 uart.send_buf[uart.send_size++] = '@';
 uart.send_buf[uart.send_size++] = send_mode;

 switch(send_mode)
 {
  case TEMPER_PAR:
   build_i4h(d->param.tmp_use);
   build_i4h(d->param.vent_pwm);
   build_i4h(d->param.cts_use_map);
   build_i16h(d->param.vent_on);
   build_i16h(d->param.vent_off);
   break;

  case CARBUR_PAR:
   build_i16h(d->param.ie_lot);
   build_i16h(d->param.ie_hit);
   build_i4h(d->param.carb_invers);
   build_i16h(d->param.fe_on_threshold);
   build_i16h(d->param.ie_lot_g);
   build_i16h(d->param.ie_hit_g);
   build_i8h(d->param.shutoff_delay);
   build_i8h(d->param.tps_threshold);
   break;

  case IDLREG_PAR:
   build_i4h(d->param.idl_regul);
   build_i16h(d->param.ifac1);
   build_i16h(d->param.ifac2);
   build_i16h(d->param.MINEFR);
   build_i16h(d->param.idling_rpm);
   build_i16h(d->param.idlreg_min_angle);
   build_i16h(d->param.idlreg_max_angle);
   build_i16h(d->param.idlreg_turn_on_temp);
   break;

  case ANGLES_PAR:
   build_i16h(d->param.max_angle);
   build_i16h(d->param.min_angle);
   build_i16h(d->param.angle_corr);
   build_i16h(d->param.angle_dec_spead);
   build_i16h(d->param.angle_inc_spead);
   build_i4h(d->param.zero_adv_ang);
   break;

  case FUNSET_PAR:
   build_i8h(d->param.fn_gasoline);
   build_i8h(d->param.fn_gas);
   build_i16h(d->param.map_lower_pressure);
   build_i16h(d->param.map_upper_pressure);
   build_i16h(d->param.map_curve_offset);
   build_i16h(d->param.map_curve_gradient);
   build_i16h(d->param.tps_curve_offset);
   build_i16h(d->param.tps_curve_gradient);
   break;

  case STARTR_PAR:
   build_i16h(d->param.starter_off);
   build_i16h(d->param.smap_abandon);
   break;

  case FNNAME_DAT:
   build_i8h(TABLES_NUMBER + TUNABLE_TABLES_NUMBER);
#ifdef REALTIME_TABLES
   if (index < TABLES_NUMBER) //from FLASH
   {
    build_i8h(index);
    build_fs(fw_data.tables[index].name, F_NAME_SIZE);
   }
   else //from EEPROM
   {
    if (eeprom_is_idle())
    {
     build_i8h(index);
     eeprom_read(&uart.send_buf[uart.send_size], (uint16_t)((f_data_t*)(EEPROM_REALTIME_TABLES_START))[index - TABLES_NUMBER].name, F_NAME_SIZE);
     uart.send_size+=F_NAME_SIZE;
    }
    else //skip this item - will be transferred next time
    {
     index = TABLES_NUMBER - 1;
     build_i8h(index);
     build_fs(fw_data.tables[index].name, F_NAME_SIZE);
    }
   }
#else
   build_i8h(index);
   build_fs(fw_data.tables[index].name, F_NAME_SIZE);
#endif
   ++index;
   if (index>=(TABLES_NUMBER + TUNABLE_TABLES_NUMBER)) index = 0;
    break;

  case SENSOR_DAT:
   build_i16h(d->sens.frequen);           // averaged RPM
   build_i16h(d->sens.map);               // MAP pressure
   build_i16h(d->sens.voltage);           // voltage
   build_i16h(d->sens.temperat);          // coolant temperature
   build_i16h(d->corr.curr_angle);        // advance angle
   build_i16h(d->sens.knock_k);           // knock value
   build_i16h(d->corr.knock_retard);      // knock retard
   build_i8h(d->airflow);                 // index of the map axis curve
   //boolean values
   build_i8h((d->ie_valve   << 0) |       // IE flag
             (d->sens.carb  << 1) |       // carb. limit switch flag
             (d->sens.gas   << 2) |       // gas valve flag
             (d->fe_valve   << 3) |       // power valve flag
             (d->ce_state   << 4) |       // CE flag
             (d->cool_fan   << 5) |       // cooling fan flag
             (d->st_block   << 6));       // starter blocking flag
   build_i8h(d->sens.tps);                // TPS (0...100%, x2)
   build_i16h(d->sens.add_i1);            // ADD_I1 voltage
   build_i16h(d->sens.add_i2);            // ADD_I2 voltage
   build_i16h(d->ecuerrors_for_transfer); // CE errors
   build_i8h(d->choke_pos);               // choke position
#if defined(SPEED_SENSOR) && defined(SECU3T)
   build_i16h(d->sens.speed);             // vehicle speed (2 bytes)
   build_i24h(d->sens.distance);          // distance (3 bytes)
#else
   build_i16h(0);
   build_i24h(0);
#endif
#if defined(AIRTEMP_SENS) && defined(SECU3T)
   build_i16h(d->sens.air_temp);
#else
   build_i16h(0);
#endif

   //corrections
   build_i16h(d->corr.strt_aalt);         // advance angle from start map
   build_i16h(d->corr.idle_aalt);         // advance angle from idle map
   build_i16h(d->corr.work_aalt);         // advance angle from work map
   build_i16h(d->corr.temp_aalt);         // advance angle from coolant temperature correction map
   build_i16h(d->corr.airt_aalt);         // advance angle from air temperature correction map
   build_i16h(d->corr.idlreg_aac);        // advance angle correction from idling RPM regulator
   build_i16h(d->corr.octan_aac);         // octane correction value
   break;

  case ADCCOR_PAR:
   build_i16h(d->param.map_adc_factor);
   build_i32h(d->param.map_adc_correction);
   build_i16h(d->param.ubat_adc_factor);
   build_i32h(d->param.ubat_adc_correction);
   build_i16h(d->param.temp_adc_factor);
   build_i32h(d->param.temp_adc_correction);
   //todo: In the future if we will have a lack of RAM we can split this packet into 2 pieces and decrease size of buffers
   build_i16h(d->param.tps_adc_factor);
   build_i32h(d->param.tps_adc_correction);
   build_i16h(d->param.ai1_adc_factor);
   build_i32h(d->param.ai1_adc_correction);
   build_i16h(d->param.ai2_adc_factor);
   build_i32h(d->param.ai2_adc_correction);
   break;

  case ADCRAW_DAT:
   build_i16h(d->sens.map_raw);
   build_i16h(d->sens.voltage_raw);
   build_i16h(d->sens.temperat_raw);
   build_i16h(d->sens.knock_k);   //<-- knock signal level
   build_i16h(d->sens.tps_raw);
   build_i16h(d->sens.add_i1_raw);
   build_i16h(d->sens.add_i2_raw);
   break;

  case CKPS_PAR:
   build_i4h(d->param.ckps_edge_type);
   build_i4h(d->param.ref_s_edge_type);
   build_i8h(d->param.ckps_cogs_btdc);
   build_i8h(d->param.ckps_ignit_cogs);
   build_i8h(d->param.ckps_engine_cyl);
   build_i4h(d->param.merge_ign_outs);
   build_i8h(d->param.ckps_cogs_num);
   build_i8h(d->param.ckps_miss_num);
   build_i8h(d->param.hall_flags);
   build_i16h(d->param.hall_wnd_width);
   break;

  case OP_COMP_NC:
   build_i16h(d->op_comp_code);
   break;

  case CE_ERR_CODES:
   build_i16h(d->ecuerrors_for_transfer);
   break;

  case KNOCK_PAR:
   build_i4h(d->param.knock_use_knock_channel);
   build_i8h(d->param.knock_bpf_frequency);
   build_i16h(d->param.knock_k_wnd_begin_angle);
   build_i16h(d->param.knock_k_wnd_end_angle);
   build_i8h(d->param.knock_int_time_const);

   build_i16h(d->param.knock_retard_step);
   build_i16h(d->param.knock_advance_step);
   build_i16h(d->param.knock_max_retard);
   build_i16h(d->param.knock_threshold);
   build_i8h(d->param.knock_recovery_delay);
   break;

  case CE_SAVED_ERR:
   build_i16h(d->ecuerrors_saved_transfer);
   break;

  case FWINFO_DAT:
   //�������� �� ��, ����� �� �� ������� �� ������� ������. 3 ������� - ��������� � ����� ������.
#if ((UART_SEND_BUFF_SIZE - 3) < FW_SIGNATURE_INFO_SIZE+8)
 #error "Out of buffer!"
#endif
   build_fs(fw_data.exdata.fw_signature_info, FW_SIGNATURE_INFO_SIZE);
   build_i32h(PGM_GET_DWORD(&fw_data.cddata.config)); //<--compile-time options
   break;

  case MISCEL_PAR:
   build_i16h(d->param.uart_divisor);
   build_i8h(d->param.uart_period_t_ms);
   build_i4h(d->param.ign_cutoff);
   build_i16h(d->param.ign_cutoff_thrd);
   build_i8h(d->param.hop_start_cogs);
   build_i8h(d->param.hop_durat_cogs);
   break;

  case CHOKE_PAR:
   build_i16h(d->param.sm_steps);
   build_i4h(d->choke_testing);      //fake parameter (actually it is command)
   build_i8h(0);                     //fake parameter, not used in outgoing paket
   build_i8h(d->param.choke_startup_corr);
   build_i16h(d->param.choke_rpm[0]);
   build_i16h(d->param.choke_rpm[1]);
   build_i16h(d->param.choke_rpm_if);
   build_i16h(d->param.choke_corr_time);
   build_i16h(d->param.choke_corr_temp);
   break;

  case SECUR_PAR:
   build_i4h(0);
   build_i4h(0);
   build_i8h(d->param.bt_flags);
   build_rb(d->param.ibtn_keys[0], IBTN_KEY_SIZE);  //1st iButton key
   build_rb(d->param.ibtn_keys[1], IBTN_KEY_SIZE);  //2nd iButton key
   break;

#ifdef REALTIME_TABLES
//Following finite state machine will transfer all table's data
  case EDITAB_PAR:
  {
   static uint8_t fuel = 0, state = 0, wrk_index = 0;
   build_i4h(fuel);
   build_i4h(state);
   switch(state)
   {
    case ETMT_STRT_MAP: //start map
     build_i8h(0); //<--not used
     build_rb((uint8_t*)&d->tables_ram[fuel].f_str, F_STR_POINTS);
     state = ETMT_IDLE_MAP;
     break;
    case ETMT_IDLE_MAP: //idle map
     build_i8h(0); //<--not used
     build_rb((uint8_t*)&d->tables_ram[fuel].f_idl, F_IDL_POINTS);
     state = ETMT_WORK_MAP, wrk_index = 0;
     break;
    case ETMT_WORK_MAP: //work map
     build_i8h(wrk_index*F_WRK_POINTS_L);
     build_rb((uint8_t*)&d->tables_ram[fuel].f_wrk[wrk_index][0], F_WRK_POINTS_F);
     if (wrk_index >= F_WRK_POINTS_L-1 )
     {
      wrk_index = 0;
      state = ETMT_TEMP_MAP;
     }
     else
      ++wrk_index;
     break;
    case ETMT_TEMP_MAP: //temper. correction.
     build_i8h(0); //<--not used
     build_rb((uint8_t*)&d->tables_ram[fuel].f_tmp, F_TMP_POINTS);
     state = ETMT_NAME_STR;
     break;
    case ETMT_NAME_STR:
     build_i8h(0); //<--not used
     build_rs(d->tables_ram[fuel].name, F_NAME_SIZE);
     if (fuel >= ETTS_GAS_SET)  //last
      fuel = ETTS_GASOLINE_SET; //first
     else
      ++fuel;
     state = ETMT_STRT_MAP;
     break;
   }
   break;
  }

  //Transferring of RPM grid
  case RPMGRD_PAR:
   build_i8h(0); //<--reserved
   build_fb((uint8_t _PGM*)fw_data.exdata.rpm_grid_points, RPM_GRID_SIZE * sizeof(int16_t));
   break;
#endif

  case ATTTAB_PAR:
  {
   //��������� ����� ������ ������� ��� ������ 16
#if (KC_ATTENUATOR_LOOKUP_TABLE_SIZE % 16)
 #error "KC_ATTENUATOR_LOOKUP_TABLE_SIZE must be a number divisible by 16, if not, you have to change the code below!"
#endif
   static uint8_t tab_index = 0;
   build_i8h(tab_index * 16);
   build_fb(&fw_data.exdata.attenuator_table[tab_index * 16], 16);
   if (tab_index >= (KC_ATTENUATOR_LOOKUP_TABLE_SIZE / 16) - 1)
    tab_index = 0;
   else
    ++tab_index;
   break;
  }

#ifdef DEBUG_VARIABLES
  case DBGVAR_DAT:
   build_i16h(/*Your variable here*/0);
   build_i16h(/*Your variable here*/0);
   build_i16h(/*Your variable here*/0);
   build_i16h(/*Your variable here*/0);
   break;
#endif
#ifdef DIAGNOSTICS
  case DIAGINP_DAT:
   build_i16h(d->diag_inp.voltage);
   build_i16h(d->diag_inp.map);
   build_i16h(d->diag_inp.temp);
   build_i16h(d->diag_inp.add_io1);
   build_i16h(d->diag_inp.add_io2);
   build_i16h(d->diag_inp.carb);
   build_i16h(d->diag_inp.ks_1);
   build_i16h(d->diag_inp.ks_2);
   build_i8h(d->diag_inp.bits);
   break;
#endif
 }//switch

 //����� ����� ��� ���� �������
 uart.send_buf[uart.send_size++] = '\r';

 //����� ����������� �������� ��������� ������� ����� - �������� ��������
 uart_begin_send();
}

//TODO: remove it from here. It must be in secu3.c, use callback. E.g. on_bl_starting()
/** Initialization of used I/O ports (���������� ������������� ����� ������) */
void ckps_init_ports(void);

uint8_t uart_recept_packet(struct ecudata_t* d)
{
 //����� ��������� �������� ���������� ������ � ������
 uint8_t temp;
 uint8_t descriptor;

 uart.recv_index = 0;

 descriptor = uart.recv_buf[uart.recv_index++];

// TODO: ������� �������� uart_recv_size ��� ������� ���� ������.
// ��������� ����� ������� �� �������������� � ���������������� ��������

 //�������������� ������ ��������� ������ � ����������� �� �����������
 switch(descriptor)
 {
  case CHANGEMODE:
   uart_set_send_mode(uart.recv_buf[uart.recv_index++]);
   break;

  case BOOTLOADER:
   //TODO: in the future use callback and move following code out
   //���������� �����. ���������� ��������� ��� ������������ � ������ ����� ��������� ���������
   while (uart_is_sender_busy()) { wdt_reset_timer(); }
   //���� � ���������� ���� ������� "cli", �� ��� ������� ����� ������
   _DISABLE_INTERRUPT();
   ckps_init_ports();
   //������� �� ��������� ����� �������� ���������
   boot_loader_start();
   break;

  case TEMPER_PAR:
   d->param.tmp_use   = recept_i4h();
   d->param.vent_pwm  = recept_i4h();
   d->param.cts_use_map = recept_i4h();
   d->param.vent_on   = recept_i16h();
   d->param.vent_off  = recept_i16h();
   break;

  case CARBUR_PAR:
   d->param.ie_lot  = recept_i16h();
   d->param.ie_hit  = recept_i16h();
   d->param.carb_invers= recept_i4h();
   d->param.fe_on_threshold= recept_i16h();
   d->param.ie_lot_g = recept_i16h();
   d->param.ie_hit_g = recept_i16h();
   d->param.shutoff_delay = recept_i8h();
   d->param.tps_threshold = recept_i8h();
   break;

  case IDLREG_PAR:
   d->param.idl_regul = recept_i4h();
   d->param.ifac1     = recept_i16h();
   d->param.ifac2     = recept_i16h();
   d->param.MINEFR    = recept_i16h();
   d->param.idling_rpm = recept_i16h();
   d->param.idlreg_min_angle = recept_i16h();
   d->param.idlreg_max_angle = recept_i16h();
   d->param.idlreg_turn_on_temp = recept_i16h();
   break;

  case ANGLES_PAR:
   d->param.max_angle = recept_i16h();
   d->param.min_angle = recept_i16h();
   d->param.angle_corr= recept_i16h();
   d->param.angle_dec_spead = recept_i16h();
   d->param.angle_inc_spead = recept_i16h();
   d->param.zero_adv_ang = recept_i4h();
   break;

  case FUNSET_PAR:
   temp = recept_i8h();
   if (temp < TABLES_NUMBER + TUNABLE_TABLES_NUMBER)
    d->param.fn_gasoline = temp;

   temp = recept_i8h();
   if (temp < TABLES_NUMBER + TUNABLE_TABLES_NUMBER)
    d->param.fn_gas = temp;

   d->param.map_lower_pressure = recept_i16h();
   d->param.map_upper_pressure = recept_i16h();
   d->param.map_curve_offset = recept_i16h();
   d->param.map_curve_gradient = recept_i16h();
   d->param.tps_curve_offset = recept_i16h();
   d->param.tps_curve_gradient = recept_i16h();
   break;

  case STARTR_PAR:
   d->param.starter_off = recept_i16h();
   d->param.smap_abandon= recept_i16h();
   break;

  case ADCCOR_PAR:
   d->param.map_adc_factor     = recept_i16h();
   d->param.map_adc_correction = recept_i32h();
   d->param.ubat_adc_factor    = recept_i16h();
   d->param.ubat_adc_correction= recept_i32h();
   d->param.temp_adc_factor    = recept_i16h();
   d->param.temp_adc_correction= recept_i32h();
   //todo: In the future if we will have a lack of RAM we can split this packet into 2 pieces and decrease size of buffers
   d->param.tps_adc_factor     = recept_i16h();
   d->param.tps_adc_correction = recept_i32h();
   d->param.ai1_adc_factor     = recept_i16h();
   d->param.ai1_adc_correction = recept_i32h();
   d->param.ai2_adc_factor     = recept_i16h();
   d->param.ai2_adc_correction = recept_i32h();
   break;

  case CKPS_PAR:
   d->param.ckps_edge_type = recept_i4h();
   d->param.ref_s_edge_type = recept_i4h();
   d->param.ckps_cogs_btdc  = recept_i8h();
   d->param.ckps_ignit_cogs = recept_i8h();
   d->param.ckps_engine_cyl = recept_i8h();
   d->param.merge_ign_outs = recept_i4h();
   d->param.ckps_cogs_num = recept_i8h();
   d->param.ckps_miss_num = recept_i8h();
   d->param.hall_flags = recept_i8h();
   d->param.hall_wnd_width = recept_i16h();
   break;

  case OP_COMP_NC:
   d->op_actn_code = recept_i16h();
   break;

  case KNOCK_PAR:
   d->param.knock_use_knock_channel = recept_i4h();
   d->param.knock_bpf_frequency   = recept_i8h();
   d->param.knock_k_wnd_begin_angle = recept_i16h();
   d->param.knock_k_wnd_end_angle = recept_i16h();
   d->param.knock_int_time_const = recept_i8h();

   d->param.knock_retard_step = recept_i16h();
   d->param.knock_advance_step = recept_i16h();
   d->param.knock_max_retard = recept_i16h();
   d->param.knock_threshold = recept_i16h();
   d->param.knock_recovery_delay = recept_i8h();
   break;

  case CE_SAVED_ERR:
   d->ecuerrors_saved_transfer = recept_i16h();
   break;

  case MISCEL_PAR:
  {
   uint16_t old_divisor = d->param.uart_divisor;
   d->param.uart_divisor = recept_i16h();
   if (d->param.uart_divisor != old_divisor)
    d->param.bt_flags|= _BV(BTF_SET_BBR); //set flag indicating that we have to set bluetooth baud rate on next reset
   d->param.uart_period_t_ms = recept_i8h();
   d->param.ign_cutoff = recept_i4h();
   d->param.ign_cutoff_thrd = recept_i16h();
   d->param.hop_start_cogs = recept_i8h();
   d->param.hop_durat_cogs = recept_i8h();
  }
  break;

  case CHOKE_PAR:
   d->param.sm_steps = recept_i16h();
   d->choke_testing = recept_i4h(); //fake parameter (actually it is status)
   d->choke_manpos_d = recept_i8h();//fake parameter
   d->param.choke_startup_corr = recept_i8h();
   d->param.choke_rpm[0] = recept_i16h();
   d->param.choke_rpm[1] = recept_i16h();
   d->param.choke_rpm_if = recept_i16h();
   d->param.choke_corr_time = recept_i16h();
   d->param.choke_corr_temp = recept_i16h();
   break;

  case SECUR_PAR:
  {
   uint8_t old_bt_flags = d->param.bt_flags;
   d->bt_name[0] = recept_i4h();
   if (d->bt_name[0] > 8)
    d->bt_name[0] = 8;
   d->bt_pass[0] = recept_i4h();
   if (d->bt_pass[0] > 6)
    d->bt_pass[0] = 6;
   recept_rs(&d->bt_name[1], d->bt_name[0]);
   recept_rs(&d->bt_pass[1], d->bt_pass[0]);
   d->param.bt_flags = recept_i8h();
   if ((old_bt_flags & _BV(BTF_USE_BT)) != (d->param.bt_flags & _BV(BTF_USE_BT)))
    d->param.bt_flags|= _BV(BTF_SET_BBR); //set flag indicating that we have to set bluetooth baud rate on next reset
   recept_rb(d->param.ibtn_keys[0], IBTN_KEY_SIZE);  //1st iButton key
   recept_rb(d->param.ibtn_keys[1], IBTN_KEY_SIZE);  //2nd iButton key
  }
  break;

#ifdef REALTIME_TABLES
  case EDITAB_PAR:
  {
   uint8_t fuel = recept_i4h();
   uint8_t state = recept_i4h();
   uint8_t addr = recept_i8h();
// uart.recv_size-=(1+1+1+PACKET_BYTE_SIZE); //[d][x][x][xx]
   switch(state)
   {
    case ETMT_STRT_MAP: //start map
     recept_rb(((uint8_t*)&d->tables_ram[fuel].f_str) + addr, F_STR_POINTS); /*F_STR_POINTS max*/
     break;
    case ETMT_IDLE_MAP: //idle map
     recept_rb(((uint8_t*)&d->tables_ram[fuel].f_idl) + addr, F_IDL_POINTS); /*F_IDL_POINTS max*/
     break;
    case ETMT_WORK_MAP: //work map
     recept_rb(((uint8_t*)&d->tables_ram[fuel].f_wrk[0][0]) + addr, F_WRK_POINTS_F); /*F_WRK_POINTS_F max*/
     break;
    case ETMT_TEMP_MAP: //temper. correction map
     recept_rb(((uint8_t*)&d->tables_ram[fuel].f_tmp) + addr, F_TMP_POINTS); /*F_TMP_POINTS max*/
     break;
    case ETMT_NAME_STR: //name
     recept_rs((d->tables_ram[fuel].name) + addr, F_NAME_SIZE); /*F_NAME_SIZE max*/
     break;
   }
  }
  break;
#endif
#ifdef DIAGNOSTICS
  case DIAGOUT_DAT:
   d->diag_out = recept_i16h();
   break;
#endif
 }//switch

 return descriptor;
}


void uart_notify_processed(void)
{
 uart.recv_size = 0;
}

uint8_t uart_is_sender_busy(void)
{
 return (uart.send_size > 0);
}

uint8_t uart_is_packet_received(void)
{
 return (uart.recv_size > 0);
}

uint8_t uart_get_send_mode(void)
{
 return uart.send_mode;
}

uint8_t uart_set_send_mode(uint8_t descriptor)
{
 return uart.send_mode = descriptor;
}

/** Clears sender's buffer
 */
void uart_reset_send_buff(void)
{
 uart.send_size = 0;
}

/** Append sender's buffer by one byte. This function is used in the bluetooth module
 * \param ch Byte value to be appended to the buffer
 */
void uart_append_send_buff(uint8_t ch)
{
 uart.send_buf[uart.send_size++] = ch;
}

#ifdef _PLATFORM_M644_
/**Used to convert baud rate ID to baud rate value*/
PGM_DECLARE(uint16_t brtoid[CBRID_NUM][2]) = {
      {CBR_2400, CBRID_2400},   {CBR_4800, CBRID_4800},   {CBR_9600, CBRID_9600},   {CBR_14400, CBRID_14400}, 
      {CBR_19200, CBRID_19200}, {CBR_28800, CBRID_28800}, {CBR_38400, CBRID_38400}, {CBR_57600, CBRID_57600}};

uint16_t convert_id_to_br(uint16_t id)
{
 uint8_t i = 0;
 for(; i < CBRID_NUM; ++i)
  if (brtoid[i][1] == id)
   return brtoid[i][0];
 return CBR_9600; 
}
#endif

void uart_init(uint16_t baud)
{
#ifdef _PLATFORM_M644_
 baud = convert_id_to_br(baud);
#endif

 // Set baud rate
 UBRRH = (uint8_t)(baud>>8);
 UBRRL = (uint8_t)baud;
 UCSRA = _BV(U2X);                                           //�������� ���������� ��� ����������� ������
 UCSRB=_BV(RXCIE)|_BV(RXEN)|_BV(TXEN);                       //��������,���������� �� ������ � ���������� ���������
#ifdef URSEL
 UCSRC=_BV(URSEL)/*|_BV(USBS)*/|_BV(UCSZ1)|_BV(UCSZ0);       //8 ���, 1 ����, ��� �������� ��������
#else
 UCSRC=/*_BV(USBS)|*/_BV(UCSZ1)|_BV(UCSZ0);                  //8 ���, 1 ����, ��� �������� ��������
#endif

 uart.send_size = 0;                                         //���������� �� ��� �� ��������
 uart.recv_size = 0;                                         //��� �������� ������
 uart.send_mode = SENSOR_DAT;
}


/**Interrupt handler for the transfer of bytes through the UART (transmitter data register empty)
 *���������� ���������� �� �������� ������ ����� UART (������� ������ ����������� ����)
 */
ISR(USART_UDRE_vect)
{
 if (uart.send_size > 0)
 {
  UDR = uart.send_buf[uart.send_index];
  --uart.send_size;
  ++uart.send_index;
 }
 else
 {//��� ������ ��������
  UCSRB &= ~_BV(UDRIE); // disable UDRE interrupt
 }
}

/**Interrupt handler for receive data through the UART */
ISR(USART_RXC_vect)
{
 static uint8_t state=0;
 uint8_t chr = UDR;

 _ENABLE_INTERRUPT();
 switch(state)
 {
  case 0:            //��������� (������� ������ ������ �������)
   if (uart.recv_size!=0) //���������� �������� ����� ��� �� ���������, � ��� ��� �������� �����.
    break;

   if (chr=='!')   //������ ������?
   {
    state = 1;
    uart.recv_index = 0;
   }
   break;

  case 1:           //����� ������ �������
   if (chr=='\r')
   {
    state = 0;       //�� � �������� ���������
    uart.recv_size = uart.recv_index; //������ ������, ��������� �� ������
   }
   else
   {
    if (uart.recv_index >= UART_RECV_BUFF_SIZE)
    {
     //������: ������������! - �� � �������� ���������, ����� ������ ������� ��������!
     state = 0;
    }
    else
     uart.recv_buf[uart.recv_index++] = chr;
   }
   break;
 }
}
