 /****************************************************************
 *       SECU-3  - An open source, free engine control unit
 *    Designed by Alexey A. Shabelnikov. Ukraine, Gorlovka 2007.
 *       Microprocessors systems - design & programming.
 *    contacts:
 *              http://secu-3.narod.ru
 *              ICQ: 405-791-931
 ****************************************************************/

#include "eeprom.h"
#include "bitmask.h"
#include <iom16.h>


//��������� ���������� ����������� ��� ���������� ������ � EEPROM
typedef struct 
{
  unsigned int ee_addr;               //����� ��� ������ � EEPROM
  unsigned char* sram_addr;           //����� ������ � ��� 
  unsigned char count;                //���������� ������
  unsigned char eews;                 //��������� �������� ������
}eeprom_wr_desc;


eeprom_wr_desc eewd;


//���������� ������� ������ ����� � EEPROM
#define EE_START_WR_BYTE()  {EECR|= (1<<EEMWE);  EECR|= (1<<EEWE);}     


//��������� ������� ������ � EEPROM ���������� ����� ������
void eeprom_start_wr_data(unsigned int eeprom_addr, unsigned char* sram_addr, unsigned char count)  
{
  eewd.eews = 1;
  eewd.ee_addr = eeprom_addr;
  eewd.sram_addr = sram_addr;
  eewd.count = count;
  SETBIT(EECR,EERIE);
}

//���������� �� 0 ���� � ������� ������ ������� �������� �� �����������
unsigned char eeprom_is_idle(void)
{
 return (eewd.eews) ? 0 : 1;
}


//���������� ���������� �� EEPROM
//��� ���������� ������ �������� ������ ������� � ������� ������ - ����� ������� ������
#pragma vector=EE_RDY_vect
__interrupt void ee_ready_isr(void)
{ 
  switch(eewd.eews)
  {
    case 0:   //�� ����������
      break;

    case 1:   //�� � �������� ������
      EEAR = eewd.ee_addr;       
      EEDR = *eewd.sram_addr;
      EE_START_WR_BYTE();                          
      eewd.sram_addr++;
      eewd.ee_addr++;
      if (--eewd.count==0)
       eewd.eews = 2;   //��������� ���� ������� �� ������.
      else      
       eewd.eews = 1;   
      break;    

    case 2:   //��������� ���� �������
      EEAR=0x000;      
      CLEARBIT(EECR,EERIE); //��������� ���������� �� EEPROM        
      eewd.eews = 0;
      break;      
  }//switch  
}
