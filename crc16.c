 /****************************************************************
 *       SECU-3  - An open source, free engine control unit
 *    Designed by Alexey A. Shabelnikov. Ukraine, Gorlovka 2007.
 *       Microprocessors systems - design & programming.
 *    contacts:
 *              http://secu-3.narod.ru
 *              ICQ: 405-791-931
 ****************************************************************/

#include "crc16.h"

#define      P_16   0xA001     //�������



/*  ����������� ����� crc16
*    BYTE *buf - ��������� �� �������� �����
*    short num - ������ ������ � ������
*  ����������
*    unsigned short crc16 ������
*/
uint16_t crc16( uint8_t *buf, uint16_t num )
{
uint16_t i;
uint16_t crc = 0xffff;

  while ( num-- )
  { 
    crc ^= *buf++;
    i = 8;
    do
    { 
      if ( crc & 1 )
        crc = ( crc >> 1 ) ^ P_16;
      else
        crc >>= 1;
    } while ( --i );
  }  
  return( crc );
}

//������� ��� ������ �� flash
uint16_t crc16f(uint8_t __flash *buf, uint16_t num )
{
uint16_t i;
uint16_t crc = 0xffff;

  while ( num-- )
  { 
    crc ^= *buf++;   
    i = 8;
    do
    { 
      if ( crc & 1 )
        crc = ( crc >> 1 ) ^ P_16;
      else
        crc >>= 1;
    } while ( --i );
  }
  
  return( crc );
}

