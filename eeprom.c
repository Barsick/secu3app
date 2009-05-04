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
#include <inavr.h>

//��������� ���������� ����������� ��� ���������� ������ � EEPROM
typedef struct 
{
 uint16_t ee_addr;             //����� ��� ������ � EEPROM
 uint8_t* sram_addr;           //����� ������ � ��� 
 uint8_t count;                //���������� ������
 uint8_t eews;                 //��������� �������� ������
 uint8_t opcode;
 uint8_t completed_opcode;
}eeprom_wr_desc;

eeprom_wr_desc eewd = {0,0,0,0,0,0};

//���������� ������� ������ ����� � EEPROM
#define EE_START_WR_BYTE()  {EECR|= (1<<EEMWE);  EECR|= (1<<EEWE);}     

uint8_t eeprom_take_completed_opcode(void)  
{
 uint8_t result;
 __disable_interrupt();
 result = eewd.completed_opcode;
 eewd.completed_opcode = 0; 
 __enable_interrupt();
 return result;
}

//��������� ������� ������ � EEPROM ���������� ����� ������
void eeprom_start_wr_data(uint8_t opcode, uint16_t eeprom_addr, uint8_t* sram_addr, uint8_t count)  
{
 eewd.eews = 1;
 eewd.ee_addr = eeprom_addr;
 eewd.sram_addr = sram_addr;
 eewd.count = count;
 eewd.opcode = opcode;
 SETBIT(EECR,EERIE);
}

//���������� �� 0 ���� � ������� ������ ������� �������� �� �����������
uint8_t eeprom_is_idle(void)
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
   eewd.completed_opcode = eewd.opcode;
   break;      
 }//switch  
}

void eeprom_read(void* sram_dest, int16_t eeaddr, uint16_t size)
{
 uint8_t _t;
 uint8_t *dest = (uint8_t*)sram_dest;  
 do
 {
  _t=__save_interrupt();
  __disable_interrupt();
  __EEGET(*dest,eeaddr);
  __restore_interrupt(_t);

  eeaddr++;
  dest++;
 }while(--size); 

 EEAR=0x000;      
}
