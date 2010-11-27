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

/* ��������� ������������� ������ �������� ����� ���������� SECU-3
 *        ________________________
 *       |                        |
 *       |       ���              |   
 *       |                        |
 *       |------------------------|
 *       | ��������� ������������ | 
 *       |------------------------|<--- ����� ������ � ��������� ��������� � ����� ������
 *       | �������������� �����.  |
 *       |------------------------| 
 *       |  ���������  ���������  |
 *       |------------------------|
 *       |   ������ ������        |
 *       |                        |
 *       |------------------------|
 *       |     CRC16              |  - ����������� ����� �������� ��� ����� ���� ����  
 *       |________________________|    ����������� ����� � ������ ����������
 *       |                        |
 *       |  boot loader           |
 *        ------------------------
 */



#ifndef _TABLES_H_
#define _TABLES_H_

#include <stdint.h>
#include "bootldr.h"   //��� ���� ����� ����� �������� SECU3BOOTSTART, � ������

//���������� ���������� ����� ������������ ��� ������ �������
#define F_WRK_POINTS_F         16  
#define F_WRK_POINTS_L         16  
#define F_TMP_POINTS           16
#define F_STR_POINTS           16                            
#define F_IDL_POINTS           16     

#define F_NAME_SIZE            16

#define KC_ATTENUATOR_LOOKUP_TABLE_SIZE 128
#define FW_SIGNATURE_INFO_SIZE 48
#define COIL_ON_TIME_LOOKUP_TABLE_SIZE 16

//��������� ���� ��������� �������������, �������� ��� = 0.5 ����.
typedef struct f_data_t
{
  int8_t f_str[F_STR_POINTS];                       // ������� ��� �� ������
  int8_t f_idl[F_IDL_POINTS];                       // ������� ��� ��� ��
  int8_t f_wrk[F_WRK_POINTS_L][F_WRK_POINTS_F];     // �������� ������� ��� (3D)
  int8_t f_tmp[F_TMP_POINTS];                       // ������� �������. ��� �� �����������
  uint8_t name[F_NAME_SIZE];                        // ��������������� ��� (��� ���������)
}f_data_t;


//��������� �������������� ������ �������� � ��������
typedef struct firmware_data_t
{
  uint8_t fw_signature_info[FW_SIGNATURE_INFO_SIZE];
  
  //������� �������� ����������� (����������� �� ��������).
  uint8_t attenuator_table[KC_ATTENUATOR_LOOKUP_TABLE_SIZE]; 
  
  //������� ������� ���������� ������� � �������� ��������� (����������� �� ����������)
  uint16_t coil_on_time[COIL_ON_TIME_LOOKUP_TABLE_SIZE];
  
  //used for checking compatibility with management software. Holds size of all data stored in the firmware.
  uint16_t fw_data_size; 
  
  //��� ����������������� ����� ���������� ��� ���������� �������� �������������
  //����� ������ �������� � ����� ������� ��������. ��� ���������� ����� ������
  //� ���������, ���������� ����������� ��� �����.
  uint8_t reserved[94];  
}firmware_data_t;

//��������� ��������� �������
typedef struct params_t
{
  uint8_t  tmp_use;                      //������� ������������ ����-��
  uint8_t  carb_invers;                  //�������� ��������� �� �����������
  uint8_t  idl_regul;                    //������������ �������� ������� �� �������������� ���
  uint8_t  fn_benzin;                    //����� ������ ������������� ������������ ��� �������
  uint8_t  fn_gas;                       //����� ������ ������������� ������������ ��� ����
  uint16_t map_lower_pressure;           //������ ������� ��� �� ��� ������� (���)
  uint16_t ephh_lot;                     //������ ����� ���� (���-1)
  uint16_t ephh_hit;                     //������� ����� ���� (���-1)
  uint16_t starter_off;                  //����� ���������� �������� (���-1)
  int16_t  map_upper_pressure;           //������� �������� ��� �� ��� ������� (���)
  uint16_t smap_abandon;                 //������� �������� � �������� ����� �� �������  (���-1) 
  int16_t  max_angle;                    //����������� ������������� ���
  int16_t  min_angle;                    //����������� ������������ ���
  int16_t  angle_corr;                   //�����-��������� ���    
  uint16_t idling_rpm;                   //�������� ������� �� ��� ����������� �������������� ���   
  int16_t  ifac1;                        //������������ ���������� �������� ��, ��� ������������� �
  int16_t  ifac2;                        //������������� ������ ��������������.
  int16_t  MINEFR;                       //���� ������������������ ���������� (�������)
  int16_t  vent_on;                      //����������� ��������� �����������
  int16_t  vent_off;                     //����������� ���������� �����������  

  int16_t  map_adc_factor;               // �������� ��� ��������� ������������ ���
  int32_t  map_adc_correction;           //
  int16_t  ubat_adc_factor;              //
  int32_t  ubat_adc_correction;          //
  int16_t  temp_adc_factor;              //
  int32_t  temp_adc_correction;          //
  
  uint8_t  ckps_edge_type;                
  uint8_t  ckps_cogs_btdc;
  uint8_t  ckps_ignit_cogs;
  
  int16_t  angle_dec_spead;
  int16_t  angle_inc_spead;  
  int16_t  idlreg_min_angle;
  int16_t  idlreg_max_angle;
  uint16_t map_curve_offset;
  uint16_t map_curve_gradient;
  
  int16_t  epm_on_threshold;             //����� ��������� ������������ ���������� �������
  
  uint16_t ephh_lot_g;                   //������ ����� ���� (���)
  uint16_t ephh_hit_g;                   //������� ����� ���� (���)
  uint8_t  shutoff_delay;                //�������� ���������� �������
 
  uint16_t uart_divisor;                 //�������� ��� ��������������� �������� UART-a
  uint8_t  uart_period_t_ms;             //������ ������� ������� � �������� �����������
  
  uint8_t ckps_engine_cyl;               //���-�� ��������� ��������� 
  
  //--knock 
  uint8_t  knock_use_knock_channel;      //������� ������������� ������ ���������
  uint8_t  knock_bpf_frequency;          //����������� ������� ���������� �������
  int16_t  knock_k_wnd_begin_angle;      //������ �������������� ���� (�������)
  int16_t  knock_k_wnd_end_angle;        //����� �������������� ���� (�������)
  uint8_t  knock_int_time_const;         //���������� ������� �������������� (���)
  //--
  int16_t knock_retard_step;             //��� �������� ��� ��� ��������� 
  int16_t knock_advance_step;            //��� �������������� ��� 
  int16_t knock_max_retard;              //������������ �������� ���
  uint16_t knock_threshold;              //����� ��������� - ����������
  uint8_t knock_recovery_delay;          //�������� �������������� ��� � ������� ������ ���������
  //--/knock
  
  uint8_t vent_pwm;                      //flag - control ventilator by using PWM
  //��� ����������������� ����� ���������� ��� ���������� �������� �������������
  //����� ������ �������� � ����� ������� ��������. ��� ���������� ����� ������
  //� ���������, ���������� ����������� ��� �����.
  uint8_t  reserved[9];
  
  uint16_t crc;                         //����������� ����� ������ ���� ��������� (��� �������� ������������ ������ ����� ���������� �� EEPROM)  
}params_t;

//================================================================================
//���������� ������ ������ � �������� ������������ �� ����������

//������ ���������� ����������� ����� ���������� � ������
#define PAR_CRC_SIZE   sizeof(uint16_t) 

//������ ���������� ����������� ����� �������� � ������
#define CODE_CRC_SIZE   sizeof(uint16_t) 

//������ ���� ��������� ��� ����� ����������� �����
#define CODE_SIZE (SECU3BOOTSTART-CODE_CRC_SIZE)

//���������� ������� ������������� �������� � ������ ��������
#define TABLES_NUMBER  8   

//����� ����������� ����� � ��������
#define CODE_CRC_ADDR (SECU3BOOTSTART-CODE_CRC_SIZE)

//����� ������� ������ - �������� �������������
#define TABLES_START (CODE_CRC_ADDR-(sizeof(f_data_t)*TABLES_NUMBER))

//����� ��������� ���������� ���������� (���������� EEPROM �� ���������)
#define DEFPARAM_START (TABLES_START-sizeof(params_t))

//����� �������������� ����������
#define FIRMWARE_DATA_START (DEFPARAM_START-sizeof(firmware_data_t))

//================================================================================

//�������������� ������ �� ���������
#pragma object_attribute=__root
extern firmware_data_t __flash fwdata;

//������ � �������� �� ���������
#pragma object_attribute=__root
extern f_data_t __flash tables[TABLES_NUMBER];

//��������� ���������
#pragma object_attribute=__root
extern params_t __flash def_param;

#pragma object_attribute=__root
extern uint16_t __flash code_crc;

#endif //_TABLES_H_
