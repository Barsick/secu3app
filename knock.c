/****************************************************************
 *       SECU-3  - An open source, free engine control unit
 *    Designed by Alexey A. Shabelnikov. Ukraine, Gorlovka 2007.
 *       Microprocessors systems - design & programming.
 *    contacts:
 *              http://secu-3.narod.ru
 *              ICQ: 405-791-931
 ****************************************************************/
#include <inavr.h>
#include <ioavr.h>
#include "knock.h"

//HIP9011 - Knock Signal Processor. 

//Command codes and quick description (crib)
#define KSP_SET_BANDPASS       0x00   //00FFFFFF, F - BPF frequency code
#define KSP_SET_GAIN           0x80   //10GGGGGG, G - gain code
#define KSP_SET_INTEGRATOR     0xC0   //110IIIII, I - integrator time constant code
#define KSP_SET_PRESCALER      0x40   //01XPPPPZ, X - don't care, P - prescaler code, Z - SO terminal status
#define KSP_SET_CHANNEL        0xE0   //111TTTTC, T - diagnostic mode code, C - channel code
// if Z = 0, then SO terminal is active, otherwise Hi Z
// if C = 0, then channel 0 selected, otherwise channel 1  
// Question. How to use T - bits, how many diagnostic modes we have? Datasheet doesn't contain 
//such information...
// SO directly corresponds to SI,(if enabled) without delay.


//SO status values
#define KSP_SO_TERMINAL_ACTIVE 0x00
#define KSP_SO_TERMINAL_HIZ    0x01

//channel selection values
#define KSP_CHANNEL_0          0x00
#define KSP_CHANNEL_1          0x01 

//prescaler
#define KSP_PRESCALER_4MHZ     0x00


#define KSP_CS PORTB_Bit4        //SS ��������� �������� ���������
#define KSP_INTHOLD PORTD_Bit3   //����������� ������ ��������������/��������
#define KSP_TEST PORTB_Bit3      //��������� ���������� ��������� � ��������������� �����


//������ ��������� ������������� ��� ������������ ������ ��������
//��������� ����������� ����������.
typedef struct
{
 uint8_t ksp_bpf; 
 volatile uint8_t ksp_gain;
 volatile uint8_t ksp_inttime;
 volatile uint8_t ksp_interrupt_state;  
 uint8_t ksp_error;
 volatile uint8_t ksp_last_word;
}kspstate_t;

kspstate_t ksp;

//��� ������ � ���������� ������ SPI
void spi_master_init(void);
void spi_master_transmit(uint8_t i_byte);

void knock_set_integration_mode(uint8_t mode) 
{
 KSP_INTHOLD = mode;
}

__monitor //��� ���������� ������ ���� ���������!
uint8_t knock_module_initialize(void)
{
 uint8_t i, response;
 uint8_t init_data[2] = {KSP_SET_PRESCALER | KSP_PRESCALER_4MHZ | KSP_SO_TERMINAL_ACTIVE,
                               KSP_SET_CHANNEL | KSP_CHANNEL_0};  
 spi_master_init();
 ksp.ksp_interrupt_state = 0; //KA �����
 ksp.ksp_error = 0;
 
 //������������� HOLD mode ��� ����������� � "Run" mode ���  ���� ������.
 KSP_TEST = 1;
 KSP_INTHOLD = KNOCK_INTMODE_HOLD;
  
 //������������� SO terminal �������� � �������� �������������. �������� �� ����� � ������������
 //�������� ������ �������� ��� ������� ���������.
 for(i = 0; i < 2; ++i)
 {
  spi_master_transmit(init_data[i]);  
  response = SPDR;  
  if (response!=init_data[i])
   return 0; //������ - ���������� �� ��������!  
 }
  
 //������������� ������ �������
 return 1; 
}

//�������������� SPI � ������ �������
__monitor
void spi_master_init(void)
{ 
 // ��������� SPI, ������, clock = fck/16, ������ �� ���������� ������ SCK
 SPCR = (1 << SPE)|(1 << MSTR)|(1 << SPR0)|(1 << CPHA);
}


//�������� ���� ���� ����� SPI
//i_byte - ���� ��� ��������
void spi_master_transmit(uint8_t i_byte)
{
 KSP_CS = 0;
 __no_operation();
 __no_operation();
 //������ ��������
 SPDR = i_byte;
 //���� ���������� ��������
 while(!(SPSR & (1 << SPIF)));
 __no_operation();
 __no_operation();
 KSP_CS = 1;
}

void knock_start_settings_latching(void)
{
 if (ksp.ksp_interrupt_state)
  ksp.ksp_error = 1;
 
 KSP_CS = 0;
 ksp.ksp_interrupt_state = 1;
 SPDR = ksp.ksp_last_word = ksp.ksp_bpf;
 //��������� ����������, �������� ��������� ������ ����� ��������� � ����������
 SPCR|= (1 << SPIE); 
}

uint8_t knock_is_latching_idle(void)
{
 return (ksp.ksp_interrupt_state) ? 0 : 1;
}

__monitor
void knock_set_band_pass(uint8_t freq)
{ 
 ksp.ksp_bpf = KSP_SET_BANDPASS | (freq & 0x3F);
}

__monitor
void knock_set_gain(uint8_t gain)
{
 ksp.ksp_gain = KSP_SET_GAIN | (gain & 0x3F);
}

__monitor
void knock_set_int_time_constant(uint8_t inttime)
{
 ksp.ksp_inttime = KSP_SET_INTEGRATOR | (inttime & 0x1F);
}

uint8_t knock_is_error(void)
{
 return ksp.ksp_error;
}

void knock_reset_error(void)
{
 ksp.ksp_error = 0;
}

#pragma vector=SPI_STC_vect
__interrupt void spi_dataready_isr(void)
{ 
 uint8_t t; 
 //���������� ��������� ������� �������� CS � ������� ������� ����� ������� �����,
 //�� ����� ��� �� 200ns
 KSP_CS = 1; 
  
 t = SPDR;
  
 switch(ksp.ksp_interrupt_state)
 {
  case 0:   //�� ����������
   break;  
          
  case 1: //BPF ���������   
   KSP_CS = 0;    
   ksp.ksp_interrupt_state = 2;
   if (t!=ksp.ksp_last_word)  
    ksp.ksp_error = 1;
   SPDR = ksp.ksp_last_word = ksp.ksp_gain;
   break;     
    
  case 2: //Gain ���������
   KSP_CS = 0;
   ksp.ksp_interrupt_state = 3;
   if (t!=ksp.ksp_last_word)  
    ksp.ksp_error = 1;    
   SPDR = ksp.ksp_last_word = ksp.ksp_inttime;
   break; 
        
  case 3: //Int.Time ���������
   if (t!=ksp.ksp_last_word)  
    ksp.ksp_error = 1;
   //��������� ���������� � ������������� �������� ������� � ���������
   //���������� � ����� ��������. ����� ��������� ������ ������� ���������
   SPCR&= ~(1 << SPIE); 
   ksp.ksp_interrupt_state = 0;   
   break;    
 }      
}

void knock_init_ports(void)
{
 PORTB|= (1<<PB4)|(1<<PB3); //��������� � HIP �������� (CS=1, TEST=1)
 PORTD&=~(1<<PD3);          //����� �������� ��� HIP
 DDRB |= (1<<DDB7)|(1<<DDB5)|(1<<DDB4)|(1<<DDB3);   
 DDRD |= (1<<DDD3);
}
