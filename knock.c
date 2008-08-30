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
#include "knock.h"

//HIP9011 - Knock Signal Processor. 

//SS ��������� �������� ���������
#define KSP_CS PORTB_Bit4

//������ ��������� ������������� ��� ������������ ������ ��������
//��������� ����������� ����������.
typedef struct
{
 unsigned char ksp_bpf; 
 unsigned char ksp_gain;
 unsigned char ksp_inttime;
 unsigned char ksp_interrupt_state;
}KSPSTATE;

KSPSTATE ksp;


void spi_master_init(void);
void spi_master_transmit(char cdata);

void knock_set_integration_mode(char mode) 
{
 PORTD_Bit3 = mode;
}

void knock_module_initialize(void)
{
 char so_terminal_status = 0x01; //Hi Z
 spi_master_init();
 KSP_CS = 0;
 //set clock and terminal status, clock = 4mHz
 spi_master_transmit(0x40 | so_terminal_status);
 //Test/Channel Select Control: Channel0 
 spi_master_transmit(0xE0);
 KSP_CS = 1;
 ksp.ksp_interrupt_state = 0;
}

void spi_master_init(void)
{
 //������������� MOSI � SCK ��� ������, ��������� �� ������ 
 DDRB|= (1 << DDB5)|(1 << DDB7);
 // ��������� SPI, ������, clock = fck/16, ������ �� ���������� ������ SCK
 SPCR = (1 << SPE)|(1 << MSTR)|(1 << SPR0)|(1 << CPHA);
}

void spi_master_transmit(char cdata)
{
 //������ ��������
 SPDR = cdata;
 //���� ���������� ��������
 while(!(SPSR & (1 << SPIF)));
}

void knock_start_settings_latching(void)
{
 KSP_CS = 0;
 SPDR = ksp.ksp_bpf;
 //��������� ����������, �������� ��������� ������ ����� ��������� � ����������
 SPCR|= (1 << SPIE); 
}

unsigned char knock_is_latching_idle(void)
{
 return (ksp.ksp_interrupt_state) ? 0 : 1;
}

void knock_set_band_pass(unsigned char freq)
{
  __disable_interrupt();    
 //00XXXXXX, 00 - ��� ���� ������
 ksp.ksp_bpf = freq & 0x3F;
 __enable_interrupt(); 
}

void knock_set_gain(unsigned char gain)
{
  __disable_interrupt();    
 //10XXXXXX, 10 - ��� ���� ������
 ksp.ksp_gain = (gain & 0x3F) | 0x80;
 __enable_interrupt(); 
}

void knock_set_integration_time(unsigned char inttime)
{
  __disable_interrupt();    
 //110XXXXX, 110 - ��� ���� ������
 ksp.ksp_inttime = (inttime & 0x1F) | 0xC0;
 __enable_interrupt(); 
}

#pragma vector=SPI_STC_vect
__interrupt void spi_dataready_isr(void)
{
  switch(ksp.ksp_interrupt_state)
  {
   case 0: //BPF ���������
    SPDR = ksp.ksp_gain;
    ksp.ksp_interrupt_state = 1;
    break; 
   case 1: //Gain ���������
    SPDR = ksp.ksp_inttime;
    ksp.ksp_interrupt_state = 2;
    break; 
   case 2: //Int.Time ���������
    //��������� ���������� � ������������� �������� ������� � ���������
    //���������� � ����� ��������. ����� ��������� ������ ������� ���������
    SPCR&= ~(1 << SPIE); 
    ksp.ksp_interrupt_state = 0;
    KSP_CS = 1;
    break; 
  }
}
