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

/** \file tables.c
 * Tables and datastructures stored in the frmware (instances).
 * (������� � ��������� ������ �������� � ��������).
 */

#include "port/port.h"
#include "compilopt.h"
#include "bitmask.h"
#include "ioconfig.h"
#include "tables.h"

/**Helpful macro used for pointer conversion */
#define _FNC(a) ((fnptr_t)(a))

/**Helpful macro used pointer conversions of functions which are available in SECU-3T*/
#ifdef SECU3T
 #define _FNC_S_SECU3T(a) (_FNC(a))
 #define _FNC_G_SECU3T(a) (_FNC(a))
#else
 #define _FNC_S_SECU3T(a) (_FNC(iocfg_s_stub))
 #define _FNC_G_SECU3T(a) (_FNC(iocfg_g_stub))
#endif

/**Helpful macro for creating of I/O remapping version number*/
#define IOREMVER(maj, min) ((maj)<<4 | ((min) & 0xf))

/**Fill whole firmware data */
PGM_FIXED_ADDR_OBJ(fw_data_t fw_data, ".firmware_data") =
{
 /**Fill data residing in code area, has floating address. Address is identified by a unique signature*/
 {
  //Add new data here

  /**I/O remapping. Match slots and plugs for default configuration*/
  {
   //2 bytes - size of this structure
   sizeof(iorem_slots_t),

   //Version of this structure - 1.2
   IOREMVER(1,2),

   //normal slots (initialization)
   {_FNC(iocfg_i_ign_out1), _FNC(iocfg_i_ign_out2), _FNC(iocfg_i_ign_out3), _FNC(iocfg_i_ign_out4),
    _FNC_S_SECU3T(iocfg_i_add_io1), _FNC_S_SECU3T(iocfg_i_add_io2), _FNC(iocfg_i_ecf), _FNC(iocfg_i_st_block),
    _FNC(iocfg_i_ie), _FNC(iocfg_i_fe),
    _FNC(iocfg_i_ps),              //PS input initialization
    _FNC_S_SECU3T(iocfg_i_add_i1), //ADD_IO1 input initialization
    _FNC_S_SECU3T(iocfg_i_add_i2), //ADD_IO2 input initialization
    0, 0, 0                        //<-- zero means that these slots are not implemented in this firmware
   },//inverted slots (initialization)
   {_FNC(iocfg_i_ign_out1i), _FNC(iocfg_i_ign_out2i), _FNC(iocfg_i_ign_out3i), _FNC(iocfg_i_ign_out4i),
    _FNC_S_SECU3T(iocfg_i_add_io1i), _FNC_S_SECU3T(iocfg_i_add_io2i), _FNC(iocfg_i_ecfi), _FNC(iocfg_i_st_blocki),
    _FNC(iocfg_i_iei), _FNC(iocfg_i_fei),
    _FNC(iocfg_i_psi),              //PS input initialization
    _FNC_S_SECU3T(iocfg_i_add_i1i), //ADD_IO1 input initialization
    _FNC_S_SECU3T(iocfg_i_add_i2i), //ADD_IO2 input initialization
    0, 0, 0                         //<-- zero means that these slots are not implemented in this firmware
   },//normal slots (get/set value)
   {_FNC(iocfg_s_ign_out1), _FNC(iocfg_s_ign_out2), _FNC(iocfg_s_ign_out3), _FNC(iocfg_s_ign_out4),
    _FNC_S_SECU3T(iocfg_s_add_io1), _FNC_S_SECU3T(iocfg_s_add_io2), _FNC(iocfg_s_ecf), _FNC(iocfg_s_st_block),
    _FNC(iocfg_s_ie), _FNC(iocfg_s_fe),
    _FNC(iocfg_g_ps),              //PS input get value
    _FNC_G_SECU3T(iocfg_g_add_i1), //ADD_IO1 input get value
    _FNC_G_SECU3T(iocfg_g_add_i2), //ADD_IO2 input get value
    0, 0, 0                        //<-- zero means that these slots are not implemented in this firmware
   },//inverted slots (get/set value)
   {_FNC(iocfg_s_ign_out1i), _FNC(iocfg_s_ign_out2i), _FNC(iocfg_s_ign_out3i), _FNC(iocfg_s_ign_out4i),
    _FNC_S_SECU3T(iocfg_s_add_io1i), _FNC_S_SECU3T(iocfg_s_add_io2i), _FNC(iocfg_s_ecfi), _FNC(iocfg_s_st_blocki),
    _FNC(iocfg_s_iei), _FNC(iocfg_s_fei),
    _FNC(iocfg_g_psi),              //PS input get value
    _FNC_G_SECU3T(iocfg_g_add_i1i), //ADD_IO1 input get value
    _FNC_G_SECU3T(iocfg_g_add_i2i), //ADD_IO2 input get value
    0, 0, 0                         //<-- zero means that these slots are not implemented in this firmware
   },
   //plugs
   {_FNC(iocfg_i_ign_out1), _FNC(iocfg_i_ign_out2), _FNC(iocfg_i_ign_out3), _FNC(iocfg_i_ign_out4),
    _FNC_S_SECU3T(iocfg_i_add_io1), _FNC_S_SECU3T(iocfg_i_add_io2), _FNC(iocfg_i_ecf), _FNC(iocfg_i_st_block),
    _FNC(iocfg_i_ie), _FNC(iocfg_i_fe), _FNC(iocfg_i_ps), _FNC_S_SECU3T(iocfg_i_add_i1),
    _FNC_S_SECU3T(iocfg_i_add_i2), 0, 0, 0,          //<-- mapped to slots by default
    _FNC(iocfg_s_stub), _FNC(iocfg_s_stub), _FNC(iocfg_s_stub), _FNC(iocfg_s_stub),
    _FNC(iocfg_s_stub), _FNC(iocfg_s_stub), _FNC(iocfg_s_stub), _FNC(iocfg_s_stub),
    _FNC(iocfg_s_stub), _FNC(iocfg_s_stub), _FNC(iocfg_s_stub), _FNC(iocfg_s_stub),
    _FNC(iocfg_s_stub), _FNC(iocfg_s_stub), _FNC(iocfg_s_stub), _FNC(iocfg_s_stub),
   },
   {_FNC(iocfg_s_ign_out1), _FNC(iocfg_s_ign_out2), _FNC(iocfg_s_ign_out3), _FNC(iocfg_s_ign_out4),
    _FNC_S_SECU3T(iocfg_s_add_io1), _FNC_S_SECU3T(iocfg_s_add_io2), _FNC(iocfg_s_ecf), _FNC(iocfg_s_st_block),
    _FNC(iocfg_s_ie), _FNC(iocfg_s_fe), _FNC(iocfg_g_ps), _FNC_G_SECU3T(iocfg_g_add_i1),
    _FNC_G_SECU3T(iocfg_g_add_i2), 0, 0, 0,          //<-- mapped to slots by default
    _FNC(iocfg_s_stub), _FNC(iocfg_s_stub), _FNC(iocfg_s_stub), _FNC(iocfg_s_stub),
    _FNC(iocfg_g_stub), _FNC(iocfg_s_stub), _FNC(iocfg_s_stub), _FNC(iocfg_g_stub),
    _FNC(iocfg_s_stub), _FNC(iocfg_s_stub), _FNC(iocfg_s_stub), _FNC(iocfg_s_stub),
    _FNC(iocfg_s_stub), _FNC(iocfg_s_stub), _FNC(iocfg_s_stub), _FNC(iocfg_s_stub),
   },

   _FNC(iocfg_s_stub), _FNC(iocfg_g_stub), //<-- stub, stub
  },

  /**32-bit config data*/
  _CBV32(COPT_ATMEGA16, 0) | _CBV32(COPT_ATMEGA32, 1) | _CBV32(COPT_ATMEGA64, 2) | _CBV32(COPT_ATMEGA128, 3) |
  _CBV32(COPT_VPSEM, 4) | _CBV32(0/*not used*/, 5) | _CBV32(0/*not used*/, 6) | _CBV32(COPT_DWELL_CONTROL, 7) |
  _CBV32(COPT_COOLINGFAN_PWM, 8) | _CBV32(COPT_REALTIME_TABLES, 9) | _CBV32(COPT_ICCAVR_COMPILER, 10) | _CBV32(COPT_AVRGCC_COMPILER, 11) |
  _CBV32(COPT_DEBUG_VARIABLES, 12) | _CBV32(COPT_PHASE_SENSOR, 13) | _CBV32(COPT_PHASED_IGNITION, 14) | _CBV32(COPT_FUEL_PUMP, 15) |
  _CBV32(COPT_THERMISTOR_CS, 16) | _CBV32(COPT_SECU3T, 17) | _CBV32(COPT_DIAGNOSTICS, 18) | _CBV32(COPT_HALL_OUTPUT, 19) |
  _CBV32(COPT_REV9_BOARD, 20) | _CBV32(COPT_STROBOSCOPE, 21) | _CBV32(COPT_SM_CONTROL, 22),

  /**A reserved byte*/
  0,

  /**2 bytes - size of this structure. This value stored in big-endian format (for compatibility reason)*/
  ((sizeof(cd_data_t) >> 8) & 0x00FF) | ((sizeof(cd_data_t) << 8) & 0xFF00)
 },

 /**�������������� ������ �� ��������� Fill additional data with default values */
 {
  /**����� ���� ������ ������ ���� ����� FW_SIGNATURE_INFO_SIZE.
   * ���� � ������� Mmm dd yyyy.
   * Size of this string must be equal to FW_SIGNATURE_INFO_SIZE!
   * Date in format Mmm dd yyyy. */
  {"SECU-3 firmware v3.6. Build ["__DATE__"]       "},

  /**������� �����������, �� ��������� k = 1.000
   * attenuator's lookup table (for knock channel), by default k = 1.000 */
  {0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,
   0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,
   0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,
   0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,
   0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,
   0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,
   0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,
   0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E
  },

  /**Lookup table for controlling coil's accumulation time (dwell control) */
  {0x200,0x200,0x200,0x200,0x200,0x200,0x200,0x200,0x200,0x200,0x200,0x200,0x200,0x200,0x200,0x200,
   0x200,0x200,0x200,0x200,0x200,0x200,0x200,0x200,0x200,0x200,0x200,0x200,0x200,0x200,0x200,0x200
  },

  /**Size of all data for checking */
  (sizeof(fw_data_t) - sizeof(cd_data_t)),

  /**reserved 32 bit value*/
  0, 

  /**Fill coolant tempersture sensor lookup table*/
  {400, 312, 262, 223, 191, 166, 142, 120, 99, 78, 56, 34, 10, -15, -54, -112},
  154, 1708,

  /**Fill choke closing vs. temperarure lookup table*/
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

  /**reserved bytes*/
  {0}
 },

 /**��������� ��������� Fill reserve parameters with default values */
 {1, 0, 0, 6, 6, 1920, 1900, 2100, 600, 6400, 650, 1600, -320, 0, 800, 4,
  4,10,392,384,16384,8192,16384,8192,16384,8192, 0, 20, 10, 96, 96, -320,
  320, 066, 1089, 392, 1900, 2100, 0, 0x00CF, 8, 4, 0, 35, 0, 800, 23, 128,
  8, 512, 1000, 2, 0, 0, 7500, 0, 0, 0, 10, 0, 60, 2, 0, 16384, 8192, 
  16384,8192, 16384,8192, 160, 1050, 0, {0,0,0,0,0,0,0,0}, /*crc*/(sizeof(fw_data_t) - sizeof(cd_data_t))
 },

 /**������ � �������� �� ��������� Fill tables with default data */
 {
  {
   {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x08,0x0C,0x10,0x14,0x14,0x14},  //�������� �����
   {0x0A,0x0A,0x0A,0x00,0x00,0x00,0x00,0x0A,0x19,0x28,0x37,0x37,0x37,0x37,0x37,0x37},  //�� �����
   {
    {0x04,0x05,0x06,0x07,0x0A,0x0C,0x10,0x15,0x1A,0x20,0x24,0x27,0x28,0x28,0x28,0x28}, //����� ��������� ������
    {0x04,0x05,0x06,0x08,0x0B,0x0D,0x10,0x16,0x1B,0x21,0x26,0x29,0x2A,0x2A,0x2A,0x2A},
    {0x04,0x05,0x08,0x08,0x0C,0x0E,0x12,0x18,0x1E,0x23,0x28,0x2A,0x2C,0x2C,0x2C,0x2C},
    {0x04,0x06,0x08,0x0A,0x0C,0x10,0x14,0x1B,0x21,0x26,0x28,0x2A,0x2C,0x2C,0x2C,0x2D},
    {0x06,0x07,0x0A,0x0C,0x0E,0x12,0x18,0x20,0x26,0x2A,0x2C,0x2D,0x2E,0x2E,0x2F,0x30},
    {0x08,0x08,0x0A,0x0E,0x12,0x16,0x1E,0x27,0x2D,0x2F,0x30,0x31,0x33,0x34,0x35,0x36},
    {0x0A,0x0B,0x0D,0x10,0x14,0x1B,0x24,0x2D,0x32,0x33,0x35,0x37,0x39,0x3A,0x3A,0x3B},
    {0x0C,0x10,0x12,0x15,0x1A,0x21,0x29,0x32,0x36,0x37,0x39,0x3C,0x3E,0x40,0x40,0x40},
    {0x10,0x16,0x18,0x1C,0x22,0x28,0x2E,0x36,0x3A,0x3B,0x3D,0x3F,0x41,0x44,0x44,0x44},
    {0x16,0x1C,0x1E,0x22,0x27,0x2E,0x34,0x3A,0x3D,0x3E,0x40,0x42,0x44,0x48,0x48,0x48},
    {0x1C,0x21,0x22,0x26,0x2A,0x31,0x38,0x3F,0x41,0x42,0x44,0x45,0x48,0x4A,0x4A,0x4B},
    {0x1E,0x22,0x24,0x27,0x2B,0x33,0x3B,0x42,0x45,0x45,0x47,0x48,0x4A,0x4C,0x4C,0x4C},
    {0x20,0x24,0x26,0x29,0x2E,0x36,0x3D,0x45,0x47,0x47,0x48,0x49,0x4A,0x4C,0x4C,0x4D},
    {0x20,0x24,0x27,0x2B,0x31,0x37,0x3F,0x45,0x47,0x48,0x49,0x49,0x4B,0x4D,0x4D,0x4D},
    {0x1E,0x22,0x26,0x2C,0x31,0x39,0x40,0x46,0x48,0x4A,0x4A,0x4B,0x4D,0x4E,0x4E,0x4E},
    {0x1E,0x21,0x25,0x29,0x2F,0x36,0x3F,0x45,0x49,0x4B,0x4C,0x4D,0x4F,0x4F,0x4F,0x4F}
   },
   {0x22,0x1C,0x19,0x16,0x13,0x0F,0x0C,0x0A,0x07,0x05,0x02,0x00,0x00,0xFD,0xF6,0xEC},  //����� ������������� ��������� ���
   {'2','1','0','8','3',' ','�','�','�','�','�','�','�','�',' ',' '}
  },

  {
   {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x08,0x0C,0x10,0x14,0x14,0x14},  //start map
   {0x0A,0x0A,0x0A,0x00,0x00,0x00,0x00,0x0A,0x19,0x28,0x37,0x37,0x37,0x37,0x37,0x37},  //idling map
   {
    {0x09,0x09,0x09,0x09,0x11,0x14,0x1A,0x20,0x26,0x24,0x30,0x33,0x3B,0x43,0x45,0x45}, //work map
    {0x09,0x09,0x09,0x09,0x11,0x14,0x1A,0x20,0x26,0x24,0x30,0x33,0x3B,0x43,0x45,0x45},
    {0x09,0x09,0x09,0x09,0x11,0x14,0x1A,0x20,0x26,0x24,0x30,0x33,0x3B,0x43,0x45,0x45},
    {0x09,0x09,0x09,0x09,0x13,0x14,0x1A,0x20,0x26,0x26,0x32,0x33,0x3F,0x43,0x45,0x45},
    {0x09,0x09,0x09,0x0A,0x13,0x16,0x1C,0x22,0x28,0x2F,0x39,0x3B,0x3F,0x47,0x49,0x49},
    {0x09,0x09,0x09,0x0B,0x15,0x28,0x2C,0x35,0x3A,0x41,0x42,0x43,0x45,0x45,0x4B,0x4B},
    {0x0B,0x0D,0x17,0x1D,0x25,0x2D,0x33,0x38,0x3F,0x45,0x4A,0x48,0x48,0x48,0x4A,0x4C},
    {0x16,0x1A,0x22,0x26,0x2D,0x33,0x39,0x3D,0x46,0x48,0x4D,0x4A,0x4A,0x4A,0x4A,0x50},
    {0x21,0x27,0x2F,0x35,0x33,0x36,0x3D,0x41,0x49,0x4B,0x4F,0x4C,0x4C,0x4C,0x4A,0x52},
    {0x28,0x2E,0x3A,0x3A,0x37,0x37,0x3D,0x45,0x4C,0x4D,0x4F,0x4F,0x4F,0x52,0x52,0x56},
    {0x2E,0x38,0x3E,0x40,0x38,0x37,0x45,0x49,0x4E,0x4F,0x51,0x50,0x50,0x54,0x58,0x58},
    {0x30,0x3E,0x42,0x40,0x38,0x3D,0x47,0x4F,0x52,0x50,0x4F,0x4E,0x4E,0x54,0x54,0x5A},
    {0x32,0x40,0x46,0x48,0x48,0x49,0x4B,0x4E,0x51,0x52,0x4E,0x4D,0x4B,0x54,0x58,0x58},
    {0x2E,0x3C,0x40,0x42,0x46,0x41,0x47,0x4B,0x4E,0x4E,0x4E,0x4D,0x45,0x54,0x54,0x56},
    {0x28,0x32,0x36,0x38,0x36,0x39,0x43,0x49,0x4B,0x4E,0x48,0x48,0x49,0x50,0x54,0x54},
    {0x24,0x28,0x28,0x28,0x30,0x35,0x3F,0x47,0x4B,0x4E,0x47,0x46,0x48,0x4C,0x50,0x50},
   },
   {0x22,0x1C,0x19,0x16,0x13,0x0F,0x0C,0x0A,0x07,0x05,0x02,0x00,0x00,0xFD,0xF6,0xEC},  //coolant temperature correction map
   {'2','1','0','8','3',' ','�','�','�','�','�','�','�','�','�','�'}
  },

  {
   {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x08,0x0C,0x10,0x14,0x14,0x14},
   {0x0A,0x0A,0x0A,0x00,0x00,0x00,0x00,0x0A,0x19,0x28,0x37,0x37,0x37,0x37,0x37,0x37},
   {
    {0x0C,0x0C,0x0E,0x10,0x11,0x12,0x14,0x16,0x1B,0x1D,0x1F,0x21,0x22,0x24,0x27,0x27},
    {0x0E,0x0E,0x10,0x11,0x12,0x13,0x15,0x17,0x1C,0x20,0x24,0x25,0x26,0x26,0x29,0x29},
    {0x10,0x10,0x12,0x14,0x15,0x16,0x17,0x19,0x1E,0x22,0x26,0x27,0x29,0x28,0x2C,0x2C},
    {0x12,0x12,0x14,0x16,0x17,0x17,0x18,0x1D,0x21,0x26,0x2A,0x2B,0x2C,0x2D,0x2F,0x2F},
    {0x14,0x14,0x16,0x18,0x19,0x1B,0x23,0x2E,0x30,0x30,0x30,0x30,0x31,0x33,0x33,0x33},
    {0x16,0x16,0x18,0x1A,0x1B,0x23,0x2A,0x32,0x3C,0x3A,0x3A,0x3A,0x3B,0x38,0x38,0x38},
    {0x18,0x18,0x1A,0x1E,0x23,0x26,0x32,0x3A,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F},
    {0x1B,0x1B,0x20,0x20,0x24,0x2C,0x36,0x40,0x43,0x43,0x43,0x43,0x43,0x46,0x46,0x46},
    {0x1E,0x1E,0x20,0x22,0x26,0x2C,0x38,0x42,0x46,0x47,0x47,0x47,0x47,0x48,0x49,0x49},
    {0x1E,0x1E,0x20,0x22,0x26,0x32,0x3C,0x45,0x47,0x48,0x48,0x48,0x49,0x4A,0x4B,0x4B},
    {0x1E,0x1E,0x20,0x22,0x26,0x34,0x40,0x48,0x4A,0x4A,0x4A,0x4A,0x4A,0x4C,0x4C,0x4C},
    {0x1E,0x24,0x28,0x30,0x36,0x3C,0x42,0x48,0x4C,0x4C,0x4C,0x4C,0x4C,0x4E,0x4E,0x4E},
    {0x1B,0x24,0x28,0x30,0x36,0x3C,0x42,0x48,0x4C,0x4C,0x4C,0x4C,0x4C,0x4E,0x4E,0x4E},
    {0x19,0x24,0x28,0x30,0x36,0x3C,0x42,0x46,0x4A,0x4A,0x4A,0x4A,0x4A,0x4B,0x4B,0x4B},
    {0x17,0x24,0x28,0x30,0x36,0x3C,0x42,0x46,0x4A,0x4A,0x4A,0x4A,0x4A,0x4A,0x4A,0x4A},
    {0x15,0x24,0x28,0x30,0x36,0x3C,0x42,0x43,0x43,0x43,0x43,0x44,0x45,0x49,0x49,0x49},
   },
   {0x22,0x1C,0x19,0x16,0x13,0x0F,0x0C,0x0A,0x07,0x05,0x02,0x00,0x00,0xFD,0xF6,0xEC},
   {'�','�','�','�','�','�','�','�',' ','1','.','5',' ',' ',' ',' '}
  },

  {
   {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x08,0x0C,0x10,0x14,0x14,0x14},
   {0x0A,0x0A,0x0A,0x00,0x00,0x00,0x00,0x0A,0x19,0x28,0x37,0x37,0x37,0x37,0x37,0x37},
   {
    {0x14,0x15,0x17,0x19,0x1B,0x1D,0x20,0x24,0x29,0x2D,0x2F,0x31,0x33,0x35,0x20,0x20},
    {0x16,0x18,0x1A,0x1C,0x1E,0x20,0x23,0x27,0x2C,0x31,0x34,0x36,0x38,0x3A,0x20,0x20},
    {0x18,0x1A,0x1C,0x1E,0x21,0x24,0x28,0x2D,0x33,0x38,0x3C,0x3E,0x3F,0x41,0x20,0x20},
    {0x1A,0x1C,0x1E,0x21,0x24,0x28,0x2D,0x33,0x39,0x3F,0x43,0x45,0x46,0x47,0x20,0x20},
    {0x1C,0x1E,0x20,0x23,0x26,0x2B,0x31,0x39,0x40,0x46,0x49,0x4A,0x4B,0x4C,0x20,0x20},
    {0x1E,0x20,0x22,0x25,0x29,0x30,0x36,0x3E,0x46,0x4D,0x51,0x52,0x52,0x52,0x20,0x20},
    {0x20,0x22,0x24,0x28,0x2C,0x34,0x3A,0x42,0x4A,0x51,0x55,0x57,0x58,0x59,0x20,0x20},
    {0x22,0x24,0x28,0x2C,0x30,0x38,0x3E,0x46,0x4E,0x54,0x59,0x5B,0x5D,0x5E,0x1C,0x1C},
    {0x24,0x28,0x2C,0x30,0x34,0x3D,0x42,0x49,0x51,0x57,0x5C,0x5E,0x5F,0x5F,0x18,0x18},
    {0x26,0x2A,0x2E,0x32,0x38,0x40,0x44,0x4B,0x53,0x59,0x5E,0x5F,0x5F,0x5F,0x14,0x14},
    {0x28,0x2C,0x2E,0x32,0x3A,0x42,0x46,0x4C,0x54,0x5A,0x5E,0x5F,0x5F,0x5F,0x10,0x10},
    {0x28,0x2C,0x2E,0x32,0x3A,0x42,0x48,0x4C,0x54,0x5A,0x5E,0x5F,0x5F,0x5F,0x10,0x10},
    {0x24,0x24,0x24,0x24,0x2C,0x42,0x48,0x4C,0x54,0x5A,0x5C,0x5E,0x5F,0x5F,0x10,0x10},
    {0x24,0x24,0x24,0x24,0x2C,0x42,0x48,0x4C,0x54,0x5A,0x5C,0x5E,0x5F,0x5F,0x10,0x10},
    {0x24,0x24,0x24,0x24,0x2C,0x40,0x46,0x48,0x50,0x54,0x56,0x58,0x58,0x58,0x10,0x10},
    {0x24,0x24,0x24,0x24,0x2C,0x2C,0x2C,0x2C,0x2C,0x2C,0x2C,0x2C,0x2C,0x2C,0x10,0x10},
   },
   {0x22,0x1C,0x19,0x16,0x13,0x0F,0x0C,0x0A,0x07,0x05,0x02,0x00,0x00,0xFD,0xF6,0xEC},
   {'�','�','�','�','�','�','�','�',' ','1','.','6',' ',' ',' ',' '}
  },

  {
   {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x08,0x0C,0x10,0x14,0x14,0x14},
   {0x0A,0x0A,0x0A,0x00,0x00,0x00,0x00,0x0A,0x19,0x28,0x37,0x37,0x37,0x37,0x37,0x37},
   {
    {0x04,0x04,0x04,0x06,0x12,0x12,0x18,0x1C,0x22,0x28,0x2E,0x32,0x38,0x40,0x42,0x42},
    {0x04,0x04,0x04,0x06,0x12,0x12,0x18,0x1E,0x22,0x28,0x30,0x32,0x38,0x40,0x42,0x42},
    {0x04,0x04,0x04,0x06,0x12,0x14,0x1A,0x20,0x24,0x28,0x30,0x32,0x3A,0x40,0x42,0x42},
    {0x04,0x04,0x06,0x06,0x12,0x16,0x1C,0x24,0x28,0x2C,0x30,0x34,0x3C,0x40,0x42,0x42},
    {0x04,0x06,0x08,0x08,0x12,0x18,0x20,0x28,0x2C,0x32,0x36,0x3A,0x3C,0x44,0x46,0x46},
    {0x08,0x08,0x0A,0x0B,0x14,0x24,0x28,0x32,0x38,0x3E,0x40,0x40,0x42,0x44,0x48,0x48},
    {0x08,0x0C,0x16,0x19,0x21,0x2A,0x30,0x38,0x40,0x44,0x48,0x48,0x48,0x48,0x48,0x4A},
    {0x12,0x16,0x1E,0x22,0x29,0x30,0x36,0x3C,0x44,0x47,0x4A,0x4A,0x4A,0x4A,0x4A,0x4E},
    {0x1D,0x23,0x2B,0x31,0x2F,0x33,0x3A,0x40,0x48,0x4A,0x4C,0x4C,0x4C,0x4C,0x4C,0x50},
    {0x24,0x2A,0x36,0x36,0x33,0x34,0x3E,0x44,0x4A,0x4C,0x4C,0x4E,0x4E,0x50,0x50,0x54},
    {0x2A,0x34,0x3A,0x3C,0x34,0x34,0x42,0x48,0x4C,0x4E,0x4E,0x4E,0x4E,0x52,0x54,0x56},
    {0x2C,0x3A,0x3E,0x40,0x38,0x3A,0x44,0x4E,0x50,0x50,0x4E,0x4E,0x4C,0x52,0x54,0x58},
    {0x2E,0x3C,0x42,0x44,0x44,0x46,0x48,0x4C,0x50,0x50,0x4C,0x4C,0x4C,0x52,0x54,0x56},
    {0x2A,0x38,0x3C,0x3E,0x42,0x42,0x44,0x4A,0x4B,0x4C,0x4C,0x4B,0x48,0x52,0x52,0x54},
    {0x24,0x2E,0x32,0x34,0x32,0x36,0x40,0x48,0x48,0x4C,0x46,0x46,0x48,0x4E,0x52,0x52},
    {0x20,0x24,0x24,0x24,0x2C,0x32,0x3C,0x46,0x48,0x4B,0x44,0x44,0x46,0x4A,0x4E,0x4E},
   },
   {0x22,0x1C,0x19,0x16,0x13,0x0F,0x0C,0x0A,0x07,0x05,0x02,0x00,0x00,0xFD,0xF6,0xEC},
   {'�','�','�','�',' ','1','.','7',' ',' ',' ',' ',' ',' ',' ',' '}
  },

  {
   {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x08,0x0C,0x10,0x14,0x14,0x14},
   {0x0A,0x0A,0x0A,0x00,0x00,0x00,0x00,0x0A,0x19,0x28,0x37,0x37,0x37,0x37,0x37,0x37},
   {
    {0x12,0x14,0x14,0x16,0x1E,0x22,0x2C,0x32,0x34,0x36,0x3A,0x3E,0x44,0x44,0x44,0x39},
    {0x12,0x14,0x14,0x16,0x1F,0x22,0x2C,0x34,0x37,0x38,0x3B,0x3E,0x44,0x44,0x44,0x39},
    {0x12,0x14,0x14,0x16,0x20,0x23,0x2C,0x36,0x38,0x3A,0x3E,0x3E,0x44,0x44,0x44,0x39},
    {0x17,0x19,0x1A,0x1D,0x21,0x26,0x2C,0x3C,0x3E,0x3E,0x41,0x44,0x46,0x46,0x48,0x39},
    {0x14,0x16,0x16,0x1F,0x28,0x31,0x3A,0x3E,0x40,0x3E,0x44,0x45,0x46,0x46,0x46,0x39},
    {0x1C,0x1D,0x1E,0x1F,0x27,0x38,0x40,0x43,0x42,0x44,0x46,0x48,0x4C,0x4D,0x4C,0x39},
    {0x18,0x1A,0x22,0x2C,0x35,0x3C,0x46,0x48,0x4A,0x4A,0x48,0x4C,0x4E,0x4F,0x50,0x39},
    {0x1A,0x1E,0x2B,0x34,0x3A,0x43,0x49,0x4B,0x4C,0x4C,0x4C,0x4E,0x4E,0x4E,0x50,0x45},
    {0x20,0x24,0x2C,0x36,0x43,0x48,0x4C,0x4E,0x4E,0x4E,0x4E,0x50,0x50,0x52,0x50,0x4E},
    {0x26,0x2A,0x36,0x48,0x4E,0x53,0x54,0x56,0x56,0x56,0x56,0x58,0x52,0x56,0x50,0x54},
    {0x2C,0x2E,0x3C,0x46,0x50,0x52,0x56,0x56,0x58,0x5A,0x56,0x58,0x5A,0x5C,0x5E,0x5A},
    {0x2C,0x30,0x3A,0x3C,0x48,0x52,0x54,0x58,0x5A,0x5E,0x5A,0x5E,0x60,0x5E,0x5E,0x5A},
    {0x2C,0x2E,0x2E,0x3C,0x4A,0x53,0x58,0x58,0x5E,0x5E,0x5E,0x5E,0x65,0x65,0x64,0x5A},
    {0x2C,0x2C,0x2C,0x3A,0x54,0x56,0x5A,0x5A,0x5C,0x5E,0x60,0x5F,0x64,0x64,0x62,0x5A},
    {0x2C,0x2C,0x2F,0x2E,0x2E,0x4E,0x54,0x58,0x5B,0x5E,0x5F,0x61,0x63,0x63,0x62,0x5A},
    {0x2C,0x2C,0x2C,0x2C,0x2E,0x4A,0x51,0x54,0x58,0x5C,0x5F,0x61,0x62,0x62,0x62,0x5A},
   },
   {0x22,0x1C,0x19,0x16,0x13,0x0F,0x0C,0x0A,0x07,0x05,0x02,0x00,0x00,0xFD,0xF6,0xEC},
   {'�','�','�','�',' ','1','.','8',' ',' ',' ',' ',' ',' ',' ',' '}
  },

  {
   {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x08,0x0C,0x10,0x14,0x14,0x14},
   {0x0A,0x0A,0x0A,0x00,0x00,0x00,0x00,0x0A,0x19,0x28,0x37,0x37,0x37,0x37,0x37,0x37},
   {
    {0x15,0x15,0x15,0x15,0x10,0x02,0x02,0x05,0x0E,0x12,0x18,0x21,0x2A,0x2A,0x2A,0x2A},
    {0x15,0x15,0x15,0x15,0x10,0x02,0x0C,0x12,0x15,0x18,0x1E,0x24,0x2E,0x2E,0x2E,0x2E},
    {0x15,0x15,0x15,0x15,0x12,0x05,0x15,0x21,0x2A,0x2E,0x2E,0x31,0x31,0x31,0x31,0x31},
    {0x15,0x15,0x15,0x15,0x1B,0x15,0x1B,0x24,0x2E,0x31,0x37,0x3A,0x3E,0x2E,0x2E,0x2E},
    {0x15,0x15,0x15,0x1E,0x22,0x1B,0x28,0x31,0x34,0x37,0x3E,0x41,0x41,0x2E,0x2E,0x2E},
    {0x15,0x15,0x1B,0x28,0x24,0x22,0x2A,0x31,0x34,0x3A,0x3E,0x40,0x44,0x2E,0x2E,0x2E},
    {0x15,0x15,0x1E,0x31,0x2B,0x28,0x2E,0x31,0x3A,0x3A,0x40,0x44,0x44,0x2E,0x2E,0x2E},
    {0x15,0x15,0x18,0x3A,0x34,0x2B,0x31,0x34,0x3A,0x3B,0x40,0x44,0x4A,0x31,0x31,0x31},
    {0x15,0x15,0x22,0x37,0x2E,0x2E,0x31,0x34,0x37,0x3A,0x44,0x47,0x4D,0x2E,0x2E,0x2E},
    {0x15,0x15,0x22,0x3D,0x37,0x31,0x37,0x37,0x3A,0x44,0x47,0x4A,0x50,0x31,0x31,0x31},
    {0x18,0x18,0x24,0x40,0x34,0x31,0x34,0x37,0x3A,0x3E,0x47,0x53,0x54,0x2E,0x2E,0x2E},
    {0x18,0x18,0x18,0x31,0x40,0x34,0x37,0x3A,0x3E,0x44,0x50,0x4D,0x31,0x31,0x31,0x31},
    {0x15,0x15,0x18,0x2B,0x44,0x34,0x34,0x3A,0x3E,0x44,0x4A,0x4D,0x4D,0x2E,0x2E,0x2E},
    {0x15,0x15,0x18,0x2B,0x44,0x34,0x34,0x3A,0x3E,0x44,0x4A,0x4D,0x4D,0x2E,0x2E,0x2E},
    {0x15,0x15,0x18,0x2B,0x44,0x34,0x34,0x3A,0x3E,0x44,0x4A,0x4D,0x4D,0x2E,0x2E,0x2E},
    {0x15,0x15,0x18,0x2B,0x44,0x34,0x34,0x3A,0x3E,0x44,0x4A,0x4D,0x4D,0x2E,0x2E,0x2E},
   },
   {0x22,0x1C,0x19,0x16,0x13,0x0F,0x0C,0x0A,0x07,0x05,0x02,0x00,0x00,0xFD,0xF6,0xEC},
   {'�','�','�','�','3','3','1',' ',' ',' ',' ',' ',' ',' ',' ',' '}
  },

  {
   {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x08,0x0C,0x10,0x14,0x14,0x14},
   {0x0A,0x0A,0x0A,0x00,0x00,0x00,0x00,0x0A,0x19,0x28,0x37,0x37,0x37,0x37,0x37,0x37},
   {
    {0x14,0x14,0x14,0x14,0x14,0x01,0x01,0x07,0x0E,0x1A,0x1A,0x1E,0x27,0x27,0x27,0x27},
    {0x14,0x14,0x14,0x14,0x14,0x01,0x0E,0x17,0x1A,0x1E,0x21,0x21,0x27,0x27,0x27,0x27},
    {0x14,0x14,0x14,0x14,0x14,0x11,0x17,0x27,0x27,0x2C,0x2C,0x2D,0x2D,0x27,0x27,0x27},
    {0x14,0x14,0x14,0x14,0x14,0x1E,0x1E,0x27,0x2A,0x30,0x34,0x36,0x3C,0x27,0x27,0x27},
    {0x17,0x17,0x17,0x1E,0x21,0x24,0x27,0x2D,0x34,0x34,0x3A,0x3D,0x3D,0x27,0x27,0x27},
    {0x21,0x21,0x21,0x27,0x2E,0x27,0x2A,0x30,0x34,0x34,0x3A,0x3D,0x40,0x27,0x27,0x27},
    {0x2D,0x2D,0x30,0x34,0x36,0x30,0x2A,0x30,0x34,0x36,0x3A,0x3C,0x40,0x27,0x27,0x27},
    {0x34,0x34,0x34,0x3A,0x30,0x2A,0x2D,0x34,0x3A,0x3A,0x3D,0x40,0x43,0x27,0x27,0x27},
    {0x36,0x36,0x36,0x3A,0x36,0x34,0x30,0x34,0x36,0x3A,0x3C,0x43,0x49,0x27,0x27,0x27},
    {0x3A,0x3A,0x3A,0x3A,0x3A,0x30,0x34,0x34,0x36,0x3C,0x43,0x46,0x49,0x27,0x27,0x27},
    {0x21,0x21,0x21,0x40,0x40,0x34,0x34,0x34,0x3A,0x3D,0x43,0x4C,0x50,0x27,0x27,0x27},
    {0x17,0x17,0x1A,0x2D,0x2D,0x2D,0x30,0x36,0x3A,0x3C,0x43,0x4C,0x49,0x27,0x27,0x27},
    {0x17,0x17,0x17,0x1E,0x1E,0x1E,0x30,0x36,0x3C,0x40,0x46,0x4C,0x49,0x27,0x27,0x27},
    {0x17,0x17,0x17,0x1E,0x1E,0x1E,0x30,0x36,0x3C,0x40,0x46,0x4C,0x49,0x27,0x27,0x27},
    {0x17,0x17,0x17,0x1E,0x1E,0x1E,0x30,0x36,0x3C,0x40,0x46,0x4C,0x49,0x27,0x27,0x27},
    {0x17,0x17,0x17,0x1E,0x1E,0x1E,0x30,0x36,0x3C,0x40,0x46,0x4C,0x49,0x27,0x27,0x27},
   },
   {0x22,0x1C,0x19,0x16,0x13,0x0F,0x0C,0x0A,0x07,0x05,0x02,0x00,0x00,0xFD,0xF6,0xEC},
   {'�','�','�','�','3','3','1','7',' ',' ',' ',' ',' ',' ',' ',' '}
  },
 },

 /**Contains check sum for whole firmware */
 0x0000
};

#ifdef REALTIME_TABLES
/**Fill default data for tunable tables */
PGM_DECLARE(f_data_t tt_def_data[TUNABLE_TABLES_NUMBER]) =
{
 {
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x08,0x0C,0x10,0x14,0x14,0x14},  //�������� �����
  {0x0A,0x0A,0x0A,0x00,0x00,0x00,0x00,0x0A,0x19,0x28,0x37,0x37,0x37,0x37,0x37,0x37},  //�� �����
  {
   {0x04,0x05,0x06,0x07,0x0A,0x0C,0x10,0x15,0x1A,0x20,0x24,0x27,0x28,0x28,0x28,0x28}, //����� ��������� ������
   {0x04,0x05,0x06,0x08,0x0B,0x0D,0x10,0x16,0x1B,0x21,0x26,0x29,0x2A,0x2A,0x2A,0x2A},
   {0x04,0x05,0x08,0x08,0x0C,0x0E,0x12,0x18,0x1E,0x23,0x28,0x2A,0x2C,0x2C,0x2C,0x2C},
   {0x04,0x06,0x08,0x0A,0x0C,0x10,0x14,0x1B,0x21,0x26,0x28,0x2A,0x2C,0x2C,0x2C,0x2D},
   {0x06,0x07,0x0A,0x0C,0x0E,0x12,0x18,0x20,0x26,0x2A,0x2C,0x2D,0x2E,0x2E,0x2F,0x30},
   {0x08,0x08,0x0A,0x0E,0x12,0x16,0x1E,0x27,0x2D,0x2F,0x30,0x31,0x33,0x34,0x35,0x36},
   {0x0A,0x0B,0x0D,0x10,0x14,0x1B,0x24,0x2D,0x32,0x33,0x35,0x37,0x39,0x3A,0x3A,0x3B},
   {0x0C,0x10,0x12,0x15,0x1A,0x21,0x29,0x32,0x36,0x37,0x39,0x3C,0x3E,0x40,0x40,0x40},
   {0x10,0x16,0x18,0x1C,0x22,0x28,0x2E,0x36,0x3A,0x3B,0x3D,0x3F,0x41,0x44,0x44,0x44},
   {0x16,0x1C,0x1E,0x22,0x27,0x2E,0x34,0x3A,0x3D,0x3E,0x40,0x42,0x44,0x48,0x48,0x48},
   {0x1C,0x21,0x22,0x26,0x2A,0x31,0x38,0x3F,0x41,0x42,0x44,0x45,0x48,0x4A,0x4A,0x4B},
   {0x1E,0x22,0x24,0x27,0x2B,0x33,0x3B,0x42,0x45,0x45,0x47,0x48,0x4A,0x4C,0x4C,0x4C},
   {0x20,0x24,0x26,0x29,0x2E,0x36,0x3D,0x45,0x47,0x47,0x48,0x49,0x4A,0x4C,0x4C,0x4D},
   {0x20,0x24,0x27,0x2B,0x31,0x37,0x3F,0x45,0x47,0x48,0x49,0x49,0x4B,0x4D,0x4D,0x4D},
   {0x1E,0x22,0x26,0x2C,0x31,0x39,0x40,0x46,0x48,0x4A,0x4A,0x4B,0x4D,0x4E,0x4E,0x4E},
   {0x1E,0x21,0x25,0x29,0x2F,0x36,0x3F,0x45,0x49,0x4B,0x4C,0x4D,0x4F,0x4F,0x4F,0x4F}
  },
  {0x22,0x1C,0x19,0x16,0x13,0x0F,0x0C,0x0A,0x07,0x05,0x02,0x00,0x00,0xFD,0xF6,0xEC},  //����� ������������� ��������� ���
  {'T','u','n','a','b','l','e','_','1','(','p',')',' ',' ',' ',' '}
 },

 {
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x08,0x0C,0x10,0x14,0x14,0x14},  //start map
  {0x0A,0x0A,0x0A,0x00,0x00,0x00,0x00,0x0A,0x19,0x28,0x37,0x37,0x37,0x37,0x37,0x37},  //idling map
  {
   {0x09,0x09,0x09,0x09,0x11,0x14,0x1A,0x20,0x26,0x24,0x30,0x33,0x3B,0x43,0x45,0x45}, //work map
   {0x09,0x09,0x09,0x09,0x11,0x14,0x1A,0x20,0x26,0x24,0x30,0x33,0x3B,0x43,0x45,0x45},
   {0x09,0x09,0x09,0x09,0x11,0x14,0x1A,0x20,0x26,0x24,0x30,0x33,0x3B,0x43,0x45,0x45},
   {0x09,0x09,0x09,0x09,0x13,0x14,0x1A,0x20,0x26,0x26,0x32,0x33,0x3F,0x43,0x45,0x45},
   {0x09,0x09,0x09,0x0A,0x13,0x16,0x1C,0x22,0x28,0x2F,0x39,0x3B,0x3F,0x47,0x49,0x49},
   {0x09,0x09,0x09,0x0B,0x15,0x28,0x2C,0x35,0x3A,0x41,0x42,0x43,0x45,0x45,0x4B,0x4B},
   {0x0B,0x0D,0x17,0x1D,0x25,0x2D,0x33,0x38,0x3F,0x45,0x4A,0x48,0x48,0x48,0x4A,0x4C},
   {0x16,0x1A,0x22,0x26,0x2D,0x33,0x39,0x3D,0x46,0x48,0x4D,0x4A,0x4A,0x4A,0x4A,0x50},
   {0x21,0x27,0x2F,0x35,0x33,0x36,0x3D,0x41,0x49,0x4B,0x4F,0x4C,0x4C,0x4C,0x4A,0x52},
   {0x28,0x2E,0x3A,0x3A,0x37,0x37,0x3D,0x45,0x4C,0x4D,0x4F,0x4F,0x4F,0x52,0x52,0x56},
   {0x2E,0x38,0x3E,0x40,0x38,0x37,0x45,0x49,0x4E,0x4F,0x51,0x50,0x50,0x54,0x58,0x58},
   {0x30,0x3E,0x42,0x40,0x38,0x3D,0x47,0x4F,0x52,0x50,0x4F,0x4E,0x4E,0x54,0x54,0x5A},
   {0x32,0x40,0x46,0x48,0x48,0x49,0x4B,0x4E,0x51,0x52,0x4E,0x4D,0x4B,0x54,0x58,0x58},
   {0x2E,0x3C,0x40,0x42,0x46,0x41,0x47,0x4B,0x4E,0x4E,0x4E,0x4D,0x45,0x54,0x54,0x56},
   {0x28,0x32,0x36,0x38,0x36,0x39,0x43,0x49,0x4B,0x4E,0x48,0x48,0x49,0x50,0x54,0x54},
   {0x24,0x28,0x28,0x28,0x30,0x35,0x3F,0x47,0x4B,0x4E,0x47,0x46,0x48,0x4C,0x50,0x50},
  },
  {0x22,0x1C,0x19,0x16,0x13,0x0F,0x0C,0x0A,0x07,0x05,0x02,0x00,0x00,0xFD,0xF6,0xEC},  //coolant temperature correction map
  {'T','u','n','a','b','l','e','_','2','(','g',')',' ',' ',' ',' '}
 }
};
#endif
