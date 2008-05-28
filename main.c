 /****************************************************************
 *       SECU-3  - An open source, free engine control unit
 *    Designed by Alexey A. Shabelnikov. Ukraine, Gorlovka 2007.
 *       Microprocessors systems - design & programming.
 *    contacts:
 *              http://secu-3.narod.ru
 *              ICQ: 405-791-931
 ****************************************************************/

#pragma language=extended  // enable use of extended keywords

#include <inavr.h>
#include <iom16.h>
#include <pgmspace.h>

#include "funconv.h"
#include "main.h"
#include "uart.h"
#include "tables.h"
#include "bootldr.h"
#include "ufcodes.h"
#include "crc16.h"
#include "eeprom.h"
#include "bitmask.h"
#include "adc.h"
   
//--���������� ����������     
unsigned int  half_turn_period = 0xFFFF;                  //������ ��������� ��������� ������� ����������� n ������  
signed   int  ignition_dwell_angle = 0;                   //��������� ��� * ANGLE_MULTIPLAYER
unsigned char send_packet_interval_counter = 0;
unsigned char force_measure_timeout_counter = 0;
unsigned char save_param_timeout_counter = 0;
unsigned char engine_stop_timeout_counter = 0;
unsigned char ignition_pulse_teeth;

unsigned int freq_average_buf[FRQ_AVERAGING];                     //����� ���������� ������� �������� ���������
unsigned char eeprom_parameters_cache[64];


#pragma vector=TIMER1_COMPA_vect
__interrupt void timer1_compa_isr(void)
{
 //����� � ������� ������, ������ ����������� ��� ����� �� ������� � ������ ������� �� ���������� �������.
 //�������� ������ ������������ �������� �� ������  
  TCCR1A = (1<<COM1A1)|(1<<COM1B1);   
  ignition_pulse_teeth = 0;
}

#pragma vector=TIMER1_COMPB_vect
__interrupt void timer1_compb_isr(void)
{
 //����� � ������� ������, ������ ����������� ��� ����� �� ������� � ������ ������� �� ���������� �������.  
 //�������� ������ ������������ �������� �� ������
  TCCR1A = (1<<COM1A1)|(1<<COM1B1); 
  ignition_pulse_teeth = 0;
}


//���������� �� ������� ������� 1 (���������� ��� ����������� ���������� ����)
#pragma vector=TIMER1_CAPT_vect
__interrupt void timer1_capt_isr(void)
{  
  static unsigned char sm_state=0;    //������� ��������� ��������� �������� (��) 
  static unsigned int icr_prev;
  static unsigned int period_curr;  
  static unsigned int period_prev; 
  static unsigned char cog;
  static unsigned int measure_start_value;
  static unsigned int current_angle;
  unsigned int diff;
 
  period_curr = ICR1 - icr_prev;
  
  //�������� ������� ��� �������������, ��������� �������� �������� ���������, ������� ��������� � ������ �����  
  switch(sm_state)
  {
   case 0://-----------------����� �����������--------------------------------------------
     if (period_curr > period_prev)
     {
     force_measure_timeout_counter = FORCE_MEASURE_TIMEOUT_VALUE;  
     engine_stop_timeout_counter = ENGINE_STOP_TIMEOUT_VALUE;
     cog = 1;
     sm_state = 1;
     ignition_pulse_teeth+=2;
    
     //�������� ������ ���� ����������
     current_angle = (ANGLE_MULTIPLAYER * DPKV_DEGREES_PER_COG) * (DPKV_COGS_BEFORE_TDC - 1);
     }
     break;

   case 1: //--------------���������� ��� ��� 1-4-----------------------------------------
     //������ ��� - ���� �� �.�.�. ���������� �� 6 ����.
     current_angle-= ANGLE_MULTIPLAYER * DPKV_DEGREES_PER_COG;

     diff = current_angle - ignition_dwell_angle;
     if (diff <= ((ANGLE_MULTIPLAYER * DPKV_DEGREES_PER_COG) * 2) )
     {
     //�� ������� ��������� �������� ��������� ������ 2-x �����. ���������� ����������� ������ ���������
     OCR1A = ICR1 + ((unsigned long)diff * (period_curr * 2)) / ((ANGLE_MULTIPLAYER * DPKV_DEGREES_PER_COG) * 2);    

     //���������� ���� ����������, ��������� ��������� ����� � � ������� ������� � ��������� ���������� 
     SETBIT(TIFR,OCF1A);
     TCCR1A = (1<<COM1A1)|(1<<COM1A0)|(1<<COM1B1);      
     }

     if (cog==2) //������������� ��� ��������� ������� �������� ��� 2-3
     {
     //���� ���� ������������ �� ������������� ����������� ��������� �����
     half_turn_period = (period_curr > 1250) ? 0xFFFF : (ICR1 - measure_start_value);             
     measure_start_value = ICR1;
     f1.new_engine_cycle_happen = 1;      //������������� ������� �������� ������������� 
     adc_begin_measure();                 //������ �������� ��������� �������� ���������� ������        
     }

     if (cog == 30) //������� � ����� ���������� ��� ��� 2-3
     {
     //�������� ������ ���� ����������
     current_angle = (ANGLE_MULTIPLAYER * DPKV_DEGREES_PER_COG) * (DPKV_COGS_BEFORE_TDC - 1);
     sm_state = 2;
     }
     break;

   case 2: //--------------���������� ��� ��� 2-3-----------------------------------------
     //������ ��� - ���� �� �.�.�. ���������� �� 6 ����.
     current_angle-= ANGLE_MULTIPLAYER * DPKV_DEGREES_PER_COG;

     diff = current_angle - ignition_dwell_angle;
     if (diff <= ((ANGLE_MULTIPLAYER * DPKV_DEGREES_PER_COG) * 2) )
     {
     //�� ������� ��������� �������� ��������� ������ 2-x �����. ���������� ����������� ������ ���������
     OCR1B = ICR1 + ((unsigned long)diff * (period_curr * 2)) / ((ANGLE_MULTIPLAYER * DPKV_DEGREES_PER_COG) * 2);  
     
     //���������� ���� ����������, ��������� ��������� ����� B � ������� ������� � ��������� ���������� 
     SETBIT(TIFR,OCF1B);
     TCCR1A = (1<<COM1B1)|(1<<COM1B0)|(1<<COM1A1);        
     }

     if (cog == 32) //������������� ��� ��������� ������� �������� ��� 1-4
     {
     //���� ���� ������������ �� ������������� ����������� ��������� �����
     half_turn_period = (period_curr > 1250) ? 0xFFFF : (ICR1 - measure_start_value);             
     measure_start_value = ICR1;    
     f1.new_engine_cycle_happen = 1;      //������������� ������� �������� ������������� 
     adc_begin_measure();                 //������ �������� ��������� �������� ���������� ������
     } 

     if (cog > 55) //������� � ����� ������ �����������
     {
     sm_state = 0; 
     }
     break;
  }
    
  if (ignition_pulse_teeth >= (DPKV_IGNITION_PULSE_COGS-1))
    TCCR1A = (1<<COM1A1)|(1<<COM1B1)|(1<<FOC1A)|(1<<FOC1B); //����� �������� ������� ��������� 
  
  icr_prev = ICR1;
  period_prev = period_curr * 2;  //����������� ������ ��� �������� �����������
  cog++; 
  ignition_pulse_teeth++; 
}


//���������� �� ����������� �/� 2 - ��� ������� ��������� ���������� � ������� (��� ������ �������������). 
//���������� ������ 10��
#pragma vector=TIMER2_OVF_vect
__interrupt void timer2_ovf_isr(void)
{ 
  TCNT2 = TIMER2_RELOAD_VALUE; 

  if (force_measure_timeout_counter > 0)
    force_measure_timeout_counter--;
  else
  {
    force_measure_timeout_counter = FORCE_MEASURE_TIMEOUT_VALUE;
    adc_begin_measure();
  }      
  __enable_interrupt();     
    
  if (save_param_timeout_counter > 0)
    save_param_timeout_counter--; 

  if (send_packet_interval_counter > 0)
    send_packet_interval_counter--;

  if (engine_stop_timeout_counter > 0)
    engine_stop_timeout_counter--;
}

  
//������������ ���������� ������� �������� ��������� �� ����������� ������� ����������� 30 ������ �����.
//time_nt - ����� � ��������� ������� (���� �������� = 4���), � ����� ������ 60 ���, ���� �������� 60 ������,
//� ����� ������� 1000000 ���, ������:
void calculate_instant_freq(ecudata* d)
{
  unsigned int period;
  __disable_interrupt();
   period = half_turn_period;           //������������ ��������� ������ � ����������
  __enable_interrupt();                           

  //���� ����� �������, ������ ��������� ����������� 
  if (period!=0xFFFF)  
    d->sens.inst_frq = (7500000L)/(period);
  else
    d->sens.inst_frq = 0;
}

//���������� ������ ���������� ��� ������� ��������
void update_buffer_freq(ecudata* d)
{
  static unsigned char frq_ai = FRQ_AVERAGING-1;

  if ((engine_stop_timeout_counter == 0)||(f1.new_engine_cycle_happen))
  {
    //��������� ���������� ������ ����������  � �������� ��� �������
    freq_average_buf[frq_ai] = d->sens.inst_frq;      
    (frq_ai==0) ? (frq_ai = FRQ_AVERAGING - 1): frq_ai--; 

    f1.new_engine_cycle_happen = 0;
    engine_stop_timeout_counter = ENGINE_STOP_TIMEOUT_VALUE;   
  }
}

//���������� ���������� ������ ��������� � ���������� ������ � ��������� 
//��������� �����������, �������� �������, ������� ����
void control_engine_units(ecudata *d)
{
  //���������� ������� ����. ���� �������� ����������� ������� � frq > [����.�����] ���
  //�������� ����������� ������� � frq > [���.�����] �� ������ ��� ������, �� ������������
  //���������� ������ ������� ����� ����������� ������ ���������� �� ������� ��.�������. ����� - ������ �������.
  //--�������� ��������� ����������� ���� ����������, ���������/���������� ������� ����
  d->sens.carb=d->param.carb_invers^PINC_Bit5; //���������: 0 - �������� ������, 1 - ������
  d->ephh_valve=PORTB_Bit0 = ((!d->sens.carb)&&(((d->sens.frequen > d->param.ephh_lot)&&(!d->ephh_valve))||
                             (d->sens.frequen > d->param.ephh_hit)))?0:1;
 
  //���������� ����������� �������� (������� ����������� ����� ���������� ��������� ��������)
  if (d->sens.inst_frq > d->param.starter_off)
    SETBIT(PORTD,PD7);
 
  if (d->param.tmp_use)
  {
    //���������� ������� ������������ ���������� ���������
    if (d->sens.temperat >= d->param.vent_on)
       PORTB_Bit1 = 1;
    if (d->sens.temperat <= d->param.vent_off)   
       PORTB_Bit1 = 0; 
  }  
  
  //��������� � ��������� ��������� �������� �������
  d->sens.gas = PINC_Bit6;    
}


//���������� ���������� ������� ��������� ������� �������� ������� ����������
void average_measured_values(ecudata* d)
{     
  unsigned char i;unsigned long sum;unsigned long s;       
  ADCSRA&=0xE7;                                //��������� ���������� �� ���
            
  for (sum=0,i = 0; i < MAP_AVERAGING; i++)
   sum+=adc_get_map_value(i);      
  d->sens.map=(sum/MAP_AVERAGING)*2; 
          
  for (sum=0,i = 0; i < BAT_AVERAGING; i++)   //��������� ���������� �������� ����
   sum+=adc_get_ubat_value(i);      
  d->sens.voltage=(sum/BAT_AVERAGING)*6; 
       
  if (d->param.tmp_use) 
  {       
   for (sum=0,i = 0; i < TMP_AVERAGING; i++) //��������� ����������� (����)
    sum+=adc_get_temp_value(i);      
   d->sens.temperat=((sum/TMP_AVERAGING)*5)/3; 
  }  
  else             //���� �� ������������
   d->sens.temperat=0;
    
  ADCSRA=(ADCSRA&0xEF)|(1<<ADIE);            //��������� ���������� �� ��� �� ��������� ���� ����������
           
  for (s=0,i = 0; i < FRQ_AVERAGING; i++)    //��������� ������� �������� ���������
   s+=freq_average_buf[i];      
  d->sens.frequen=(s/FRQ_AVERAGING);           
}


//������������ ������ ����� ��������� ������� � ������� UART-a
void process_uart_interface(ecudata* d)
{ 
 static unsigned char index=0;

 if (uart_is_packet_received())//������� ����� ����� ?
 { 
  switch(uart_get_recv_mode())  //�������������� ������ ��������� ������ � ����������� �� �����������
  {
    case CHANGEMODE:       
        if (uart_is_sender_busy())    //���������� �����. ���������� ��������� ��� ������������ � ������ ����� ������ ���������� ������ 
            return; //���������� ������� �������������
        uart_set_send_mode(uart_recv_buf.snd_mode);
        goto completed;     
    case BOOTLOADER:
        if (uart_is_sender_busy())    //���������� �����. ���������� ��������� ��� ������������ � ������ ����� ��������� ���������
            return;  //���������� ������� �������������
        __disable_interrupt(); //���� � ���������� ���� ������� "cli", �� ��� ������� ����� ������
        boot_loader_start();         //������� �� ��������� ����� �������� ���������            
        goto completed;     
    case TEMPER_PAR:            
       d->param.tmp_use   = uart_recv_buf.tmp_use;
       d->param.vent_on   = uart_recv_buf.vent_on;
       d->param.vent_off  = uart_recv_buf.vent_off; 
       goto param_changed;     
    case CARBUR_PAR:   
       d->param.ephh_lot  = uart_recv_buf.ephh_lot;
       d->param.ephh_hit  = uart_recv_buf.ephh_hit;
       d->param.carb_invers=uart_recv_buf.carb_invers;
       goto param_changed;     
    case IDLREG_PAR:   
       d->param.idl_regul = uart_recv_buf.idl_regul;
       d->param.ifac1     = uart_recv_buf.ifac1;        
       d->param.ifac2     = uart_recv_buf.ifac2;       
       d->param.MINEFR    = uart_recv_buf.MINEFR;       
       d->param.idl_turns = uart_recv_buf.idl_turns;    
       goto param_changed;     
    case ANGLES_PAR:   
       d->param.max_angle = uart_recv_buf.max_angle;    
       d->param.min_angle = uart_recv_buf.min_angle;    
       d->param.angle_corr= uart_recv_buf.angle_corr;   
       goto param_changed;     
    case FUNSET_PAR:   
       if (uart_recv_buf.fn_benzin < TABLES_NUMBER)
          d->param.fn_benzin = uart_recv_buf.fn_benzin;    
       if (uart_recv_buf.fn_gas < TABLES_NUMBER)    
          d->param.fn_gas = uart_recv_buf.fn_gas;              
       d->param.map_grad  = uart_recv_buf.map_grad;     
       d->param.press_swing=uart_recv_buf.press_swing;  
       goto param_changed;     
    case STARTR_PAR:   
       d->param.starter_off=uart_recv_buf.starter_off;  
       d->param.smap_abandon=uart_recv_buf.smap_abandon;
       goto param_changed;     
  }//switch     
param_changed:                   //���� ���� �������� ��������� �� ���������� ������� �������
  save_param_timeout_counter = PAR_SAVE_COUNTER;
completed:    
  uart_notify_processed();  //�� ��������� �������� ������ - �������� ����� ������ �� ��������       
 }

 //������������ �������� ������ � �������
 if (send_packet_interval_counter==0)
 {
  if (!uart_is_sender_busy())
  {                
  //���������� ����� �� �������� - ������ ����� ���������� ������ 
  //� ����������� �� �������� ����������� ���������� ������� ������������ ��������������� ������
  switch(uart_get_send_mode())
  {
    case TEMPER_PAR:   
       uart_send_buf.tmp_use     = d->param.tmp_use;
       uart_send_buf.vent_on     = d->param.vent_on;
       uart_send_buf.vent_off    = d->param.vent_off;
       break;
    case CARBUR_PAR:   
       uart_send_buf.ephh_lot    = d->param.ephh_lot;
       uart_send_buf.ephh_hit    = d->param.ephh_hit;
       uart_send_buf.carb_invers = d->param.carb_invers;
       break;
    case IDLREG_PAR:   
       uart_send_buf.idl_regul   = d->param.idl_regul;
       uart_send_buf.ifac1       = d->param.ifac1;
       uart_send_buf.ifac2       = d->param.ifac2;
       uart_send_buf.MINEFR      = d->param.MINEFR;
       uart_send_buf.idl_turns   = d->param.idl_turns;
       break;
    case ANGLES_PAR:   
       uart_send_buf.max_angle   = d->param.max_angle;
       uart_send_buf.min_angle   = d->param.min_angle;
       uart_send_buf.angle_corr  = d->param.angle_corr;
       break;
   case FUNSET_PAR:   
       uart_send_buf.fn_benzin   = d->param.fn_benzin;
       uart_send_buf.fn_gas      = d->param.fn_gas;
       uart_send_buf.map_grad    = d->param.map_grad;
       uart_send_buf.press_swing = d->param.press_swing;
       break;
   case STARTR_PAR:   
       uart_send_buf.starter_off = d->param.starter_off;
       uart_send_buf.smap_abandon = d->param.smap_abandon;
       break;
    case FNNAME_DAT: 
       memcpy_P(uart_send_buf.name,tables[index].name,F_NAME_SIZE);
       uart_send_buf.tables_num = TABLES_NUMBER;
       uart_send_buf.index      = index++;       
       if (index>=TABLES_NUMBER) index=0;              
       break;
    case SENSOR_DAT:
       memcpy(&uart_send_buf.sens,&d->sens,sizeof(sensors));
       uart_send_buf.ephh_valve  = d->ephh_valve;
       uart_send_buf.airflow     = d->airflow;
       uart_send_buf.curr_angle  = d->curr_angle;       
       break;
  }//switch
  uart_send_packet();    //������ ���������� �������� ��������� ������
  send_packet_interval_counter = SND_TIMECONST;
  }
 }
}


//��������������� ��������� ����� ������ ���������
void InitialMeasure(ecudata* d)
{ 
  unsigned char i=16;
  __enable_interrupt();
  do
  {
    adc_begin_measure();                                                     
    while(!adc_is_measure_ready()); 
  }while(--i);  
  __disable_interrupt();
  average_measured_values(d);  
  d->atmos_press = d->sens.map;      //��������� ����������� ��������
}

//������ ������ � EEPROM - ������� ����� ���������. �� ����� ��������� ����������� � ����������� ���������,
//� ��� ����������� ����������� �������� ������ � ��������� ����� � �� ���� �� ����� ����� � EEPROM.
//���������� ������ � EEPROM ���������� ������ ���� �� �������� ����� �� ��������� �� ����� �������� ������ ����������
//�� UART-a � ����������� ��������� ���������� �� �������.        
void save_param_if_need(ecudata* d)
{
  //��������� �� ���������� �� �������� �����
  if (save_param_timeout_counter==0) 
  {
    //������� � ����������� ��������� ����������?
    if (memcmp(eeprom_parameters_cache,&d->param,sizeof(params)-PAR_CRC_SIZE)) 
    {
     memcpy(eeprom_parameters_cache,&d->param,sizeof(params));  
     ((params*)eeprom_parameters_cache)->crc=crc16(eeprom_parameters_cache,sizeof(params)-PAR_CRC_SIZE); //������� ����������� �����
     eeprom_start_wr_data(EEPROM_PARAM_START,eeprom_parameters_cache,sizeof(params));
    }
   save_param_timeout_counter = PAR_SAVE_COUNTER;
  }
}


//��������� ��������� �� EEPROM, ��������� ����������� ������ � ���� ��� ��������� ��
//����� ��������� ����� �� FLASH.
void load_eeprom_params(ecudata* d)
{
 unsigned char* e = (unsigned char*)&d->param;
 int count=sizeof(params);
 int adr=EEPROM_PARAM_START;
 
 do
 {
   __EEGET(*e,adr);
    adr++;
    e++;
 }while(--count); 
 
 EEAR=0x000;      
 
 //��� �������� ����������� ����� �� ��������� ����� ����� ����������� �����
 //���� ����������� ����� �� ��������� - ��������� ��������� ��������� �� FLASH
 if (crc16((unsigned char*)&d->param,(sizeof(params)-PAR_CRC_SIZE))!=d->param.crc)
 {
  memcpy_P(&d->param,&def_param,sizeof(params));
 }

 //�������������� ��� ����������, ����� ����� ������ ��������� ���������� �������� 
 //�� ����������. 
 memcpy(eeprom_parameters_cache,&d->param,sizeof(params));       
} 
   
      
__C_task void main(void)
{
  unsigned char mode=0;
  ecudata edat; 
  
  //������������� ����� �����/������
  PORTA  = 0;   
  DDRA   = 0;       
  PORTB  = (1<<PB4)|(1<<PB3)|(1<<PB0);                      //������ ���� �������, ��������� � HIP �������� (CS=1, TEST=1)
  DDRB   = (1<<DDB4)|(1<<DDB3)|(1<<DDB2)|(1<<DDB1)|(1<<DDB0);   
  PORTC  = (1<<PC3)|(1<<PC2);
  DDRC   = 0;  
  PORTD  = (1<<PD6)|(1<<PD3)|(1<<PD7);                      //������� ������������, ����� �������������� ��� HIP
  DDRD   = (1<<DDD7)|(1<<DDD5)|(1<<DDD4)|(1<<DDD3);
  
  if (crc16f(0,CODE_SIZE)!=code_crc)
     SETBIT(PORTB,PB2);                                    //��� ��������� �������� - �������� ��

  adc_init();

  //��������� ���������� - �� ��� �� �����  
  ACSR=(1<ACD);             

  InitialMeasure(&edat);   //�������� ��������� ������ ��������� �������� ��� ������������� ������

  PORTD_Bit7 = 0;     //������� ���������� ��������
  
  //������������� ������� T0 � T1
  TCCR0  = (1<<CS01)|(1<<CS00);                             //clock = 250kHz
  TCCR2  = (1<<CS22)|(1<<CS21)|(1<<CS20);                   //clock = 15.625kHz
  TCCR1B = (1<<ICNC1)|(1<<ICES1)|(1<<CS11)|(1<<CS10);       //���������� ����, �������� ����� �������, clock = 250kHz
  TIMSK  = (1<<TICIE1)|(1<<TOIE2)|(1<<OCIE1A)|(1<<OCIE1B);//��������� ���������� �� ������� � ��������� � � � �/C 1, ������������ T/C 2,
  TCCR1A = (1<<COM1A1)|(1<<COM1B1)|(1<<FOC1A)|(1<<FOC1B); //��� ���������� ����� �������������� ������ ������� � ���������� ��� ������������ ����� ������
    
  //�������������� UART
   uart_init(CBR_9600);
               
  //������ ���������
  load_eeprom_params(&edat);
  
  //��������� ��������� ����������            
  __enable_interrupt();    
     
  while(1)
  {
    //��������� ����������/�������� ������ ����������������� �����
    process_uart_interface(&edat);  
   
    //���������� ����������� ��������
    save_param_if_need(&edat);                        
    
    calculate_instant_freq(&edat);                           

    update_buffer_freq(&edat);

    average_measured_values(&edat);        

    control_engine_units(&edat);
       
    //� ����������� �� �������� ���� ������� �������� ��������������� ����� ������             
    if (edat.sens.gas)
      edat.fn_dat = (__flash F_data*)&tables[edat.param.fn_gas];    //�� ����
    else  
      edat.fn_dat = (__flash F_data*)&tables[edat.param.fn_benzin];//�� �������
    
    /*
    //�� ��������� �������
    switch(mode)
    {
      case 0: //����� �����
        if (edat.sens.inst_frq > edat.param.smap_abandon)
        {                   
         mode=1;        
        }      
        edat.curr_angle=str_func(&edat);                //������� ��� - ������� ��� �����
        edat.airflow=0;
        break;            
      case 1: //����� ��������� ����
       if (edat.sens.carb)//������ ���� ������ - � ������� �����
       {
        mode=2;
       }      
        edat.curr_angle=idl_func(&edat);               //������� ��� - ������� ��� �� 
        edat.curr_angle+=tmp_func(&edat);              //��������� � ��� ������������� ���������
        edat.curr_angle+=idl_pregul(&edat);            //��������� �����������
        edat.airflow=0;
        break;            
      case 2: //������� ����� 
       if (edat.sens.carb)//������ ���� ��������� - � ����� ��
       {
        mode=1;
       }
       edat.curr_angle=wrk_func(&edat);                //������� ��� - ������� �������� ������
       edat.curr_angle+=tmp_func(&edat);               //��������� � ��� ������������� ���������
        break;     
      default:  //���������� �������� - ���� � ����       
        edat.curr_angle=0;
        break;     
    }
      
    //��������� � ��� �����-���������
    edat.curr_angle+=edat.param.angle_corr;
      
    //������������ ������������ ��� �������������� ���������
    if (edat.curr_angle > edat.param.max_angle)
               edat.curr_angle = edat.param.max_angle;  
    if (edat.curr_angle < edat.param.min_angle)
               edat.curr_angle = edat.param.min_angle; 
    */
    edat.curr_angle = 0; //TEST!!!

    //��������� ��� ��� ���������� � ��������� �� ������� ����� ���������       
    __disable_interrupt();    
     ignition_dwell_angle = edat.curr_angle;
    __enable_interrupt();                
  
  }
}
