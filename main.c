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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "funconv.h"
#include "main.h"
#include "uart.h"
#include "tables.h"
#include "boot.h"
#include "ufcodes.h"
#include "crc_lib.h"


#define BEGIN_MEASURE() { f1.sens_ready=0; ADMUX = ADCI_MAP|ADC_VREF_TYPE; SETBIT(ADCSRA,ADSC);} //��������� ��������� �������� � ��������
#define RESET_TCNT0()     TCNT0 = TCNT0_H = 0                           //����� ��������
#define GET_TCNT0()      (((TCNT0_H<<8)&0xFF00)|TCNT0)                  //��������� �������� ��������
#define SET_TCNT0(v)     {TCNT0_H = v >> 8,TCNT0 = 255-(v & 0xFF);}     //������������� ������� ��������� ���������
#define EE_START_WR_BYTE()  {EECR|= (1<<EEMWE);  EECR|= (1<<EEWE);}     //���������� ������� ������ ����� � EEPROM
#define EE_START_WR_DATA(e_a,r_a,cnt)  {eewd.eews=0,eewd.ee_addr=e_a,eewd.sram_addr=r_a,eewd.count=cnt;EECR|=0x08;}//��������� ������� ������ � EEPROM ���������� ����� ������ 

   
//--���������� ����������     
unsigned int  time_nt;                                                  //������ ��������� ��������� ������� ����������� n ������  
signed   int  goal_angle;                                               //��������� ��� * ANGLE_MULTIPLAYER
unsigned char ignmask;                                                  //������� ����� ��� ������� �������� ������ ���������
unsigned char ign_teeth;                                                //��� ������� ������������ �������� ������� ��������� �� ������
unsigned char stop_counter=0;
unsigned char pars_counter=0;

unsigned int map_abuf[MAP_AVERAGING];                                   //����� ���������� ����������� ��������
unsigned int bat_abuf[BAT_AVERAGING];                                   //����� ���������� ���������� �������� ����
unsigned int tmp_abuf[TMP_AVERAGING];                                   //����� ���������� ����������� ����������� ��������
unsigned int frq_abuf[FRQ_AVERAGING];                                   //����� ���������� ������� �������� ���������

unsigned char eeprom_buf[64];

//��������� ���������� ����������� ��� ���������� ������ � EEPROM
typedef struct 
{
  unsigned int ee_addr;                                                  //����� ��� ������ � EEPROM
  unsigned char* sram_addr;                                              //����� ������ � ��� 
  unsigned char count;                                                   //���������� ������
  unsigned char eews;                                                    //��������� �������� ������
}ee_wrdesc;

ee_wrdesc eewd;


//���������� ���������� �� EEPROM
//��� ���������� ������ �������� ������ ������� � ������� ������ ����� ������� ������
#pragma vector=EE_RDY_vect
__interrupt void ee_ready_isr(void)
{ 
  switch(eewd.eews)
  {
    case 0:
      EEAR = eewd.ee_addr;       
      EEDR = *eewd.sram_addr;
      EE_START_WR_BYTE();                          
      eewd.sram_addr++;
      eewd.ee_addr++;
      if (--eewd.count==0)
       eewd.eews = 1;   //��������� ���� ������� �� ������.
      else      
       eewd.eews = 0;   
      break;    
    case 1:
      EEAR=0x000;      
      CLEARBIT(EECR,EERIE); //��������� ���������� �� EEPROM        
      break;      
  }//switch  
}



//���������� �� ������������ �/C 0. �/� 0 ��������� �� 16-�� �������� � ���������� ��� ���������
//��������� ���������� ����� ������� (����� ��������), � ����� ��� ������� ����������� ������� ��� ���������� ���(����� �������)
#pragma vector=TIMER0_OVF_vect
__interrupt void timer0_ovf_isr(void)
{
  if (f1.t0mode==0)  
  {//����� 16-������� ��������
   if (TCNT0_H < 255) TCNT0_H++;
  }
  else
  {//����� 16-������� �������        
      if (TCNT0_H!=0)  //������� ���� �� �������� ?
      {
        TCNT0 = 0;
        TCNT0_H--;         
      }  
      else  
      {//������ ��������� �� ��������� ���� ���������    
       f1.t0mode=0;
       PORTD|=ignmask;  
       ign_teeth=0;
      }
  }
}


//���������� �� ������� ������� 1 (���������� ��� ����������� ���������� ����)
//��������� ������� �������� ��������� ���������� ����� ����������� �����������(��� 1-4) �
//����� 30-�� ���� (��� 2-3) � ���������� ���������� �� ���������� ���. 
#pragma vector=TIMER1_CAPT_vect
__interrupt void timer1_capt_isr(void)
{  
  unsigned int t0; int diff;
  static unsigned char teeth=0;                                        //����� �������� ���� (1 ������ ���������)
  static signed int  curr_angle;                                       //������� ��� * ANGLE_MULTIPLAYER
  static unsigned int pptm=0x7FFF;                                     //������ �������� ����������� ������ ������� ����� �������
  static unsigned char measure_state=0;                                //������� ��������� ��������� �������� (��) 
          
  //��������� ���������� ����� ����� ������� � ���������� ������� ���� �� ��������� � ������ ��������  
  t0 = GET_TCNT0();    
  if (f1.t0mode==0) {  RESET_TCNT0();  }
                      
  //�������� ������� ��� �������������, ��������� �������� �������� ���������, ������� ��������� � ������ �����  
  switch(measure_state)
  {
   case 0:              //��������� ������������� (����� �����������)
       TCNT1 = 0;
       if (pptm < t0)
       {                //����������� �������
        teeth = 0;                                                      //�������� ������ ������
        curr_angle = ANGLE_MULTIPLAYER * DEGREES_PER_TEETH * (TEETH_BEFORE_UP-1);
        f1.released=0;                                                  //� ������ ������ ����� ��������� ���������� �������� ���� ���������� ���������
        ignmask=IGNITION_PULSE_14;                                      //����� ��������� ��������� �� 1-4 ���������
        ign_teeth+=2;                                                   //��������� ��� ���������� ����        
        f1.t1ovf=0;                                                     //���������� ���� ������������ ����� ����� �������
        f1.rotsync = 1;                                                 //������������� ������� �������� ������������� 
        BEGIN_MEASURE();                                                //������ �������� ��������� �������� ���������� ������
        measure_state = 1;
       }    
      goto inxt;
   case 1:               //��������� �������� 1-4 (�������� ����� ����������� ������)
      if (teeth==SPEED_MEASURE_TEETH)
      {                      
        time_nt=(f1.t1ovf)?0xFFFF:ICR1;                                 //���� ���� ������������ �� ������������� ����������� ��������� �����      
        measure_state = 2;   
      } 
      goto crel;
   case 2:              //������� ���� � �������� ������ �������� �������� 2-3 
      if (teeth==30)
      {
       TCNT1 = 0;
       f1.t0mode=0;                                                     //����� ����� �������� ����� ����� �������    
       curr_angle = ANGLE_MULTIPLAYER * DEGREES_PER_TEETH * (TEETH_BEFORE_UP-1);
       f1.released=0;
       ignmask=IGNITION_PULSE_23;                     
       f1.t1ovf=0;
       f1.rotsync = 1;
       BEGIN_MEASURE();                                                //������ �������� ��������� �������� ���������� ������       
       measure_state = 3;   
      }
      goto crel;
   case 3:              //��������� �������� 2-3 (�������� ����� ����������� ������)
      if (teeth==(30+SPEED_MEASURE_TEETH))
      {
        time_nt=(f1.t1ovf)?0xFFFF:ICR1;                                 //���� ���� ������������ �� ������������� ����������� ��������� �����                         
        measure_state = 4;                                              //�� �������� ������� �������� � ��������� ��������� (����� �����������)
      }           
      goto crel;
   case 4:              //������� ���� � �������� �� �������� � ��������� ������ �����������   
      if (teeth!=TEETH_BACK_SYNC)
          goto crel;
      f1.t0mode=0;    
      measure_state=0;                                                  //����� ��������� � ��������� �������������        
      goto inxt;          
  }
crel:                   //����� ���� ������� ����������� ���
   if (!f1.released)    //������ ���� ���
   {
     diff = curr_angle-goal_angle;
     if (diff <= (ANGLE_MULTIPLAYER * DEGREES_PER_TEETH))
     { //�������� ��������� ������ ������ ����. ���������� ��������� ����� ��������������� ����������� ����
      f1.t0mode = 1;
      f1.released=1;
      t0 = ((long)diff * t0)/(ANGLE_MULTIPLAYER * DEGREES_PER_TEETH);      
      SET_TCNT0(t0);                 
    }
   } 
inxt:                   //�������� ������������ �������� ������� ��������� � ���������� ��������� ������ � �������� ����
  if (ign_teeth >= (IGNITION_TEETH-1))
     PORTD&=IGNITION_PULSE_OFF;

   teeth++;  ign_teeth++;                                          
   curr_angle=curr_angle-(ANGLE_MULTIPLAYER * DEGREES_PER_TEETH);        //������ ��� - ���� �� �.�.�. ���������� �� 6 ����.        
   pptm = t0*2;                                                          //����������� ������ ��� �������� ����������� 
}


//���������� �� ������������ �/� 1. ������������ � ������� ����� ��������� ������� �������� ���������
//��� ������������ ����� ������������. ������������ ���������� ��������� ��� ��������� ������� ��������,
//����� ��� ��������� ��������� ��������� �������� ������������ ���������, � ������������� �� ������� ��
//������� ������.
#pragma vector=TIMER1_OVF_vect
__interrupt void timer1_ovf_isr(void)
{ 
 f1.t1ovf=1;
}
 
  
//���������� �� ���������� �������������� ���. ��������� �������� ���� ���������� ��������. ����� �������
//��������� ��� ���������� ����� ��������� ��� ������� �����, �� ��� ��� ���� ��� ����� �� ����� ����������.
#pragma vector=ADC_vect
__interrupt void ADC_isr(void)
{
 static unsigned char  map_ai=MAP_AVERAGING-1;
 static unsigned char  bat_ai=BAT_AVERAGING-1;
 static unsigned char  tmp_ai=TMP_AVERAGING-1;;      
 __enable_interrupt(); 

 switch(ADMUX&0x07)
 {
   case ADCI_MAP: //��������� ��������� ����������� ��������
      map_abuf[map_ai] = ADC;      
      //��������� �������� ������� ������ ����������
      (map_ai==0) ? (map_ai = MAP_AVERAGING - 1): map_ai--;            
      ADMUX = ADCI_UBAT|ADC_VREF_TYPE;   
      SETBIT(ADCSRA,ADSC);
      break;
   case ADCI_UBAT://��������� ��������� ���������� �������� ����
      bat_abuf[bat_ai] = ADC;      
      //��������� �������� ������� ������ ����������
      (bat_ai==0) ? (bat_ai = BAT_AVERAGING - 1): bat_ai--;            
      ADMUX = ADCI_TEMP|ADC_VREF_TYPE;   
      SETBIT(ADCSRA,ADSC);
      break;
   case ADCI_TEMP://��������� ��������� ����������� ����������� ��������
      tmp_abuf[tmp_ai] = ADC;      
      //���������  �������� ������� ������ ����������
      (tmp_ai==0) ? (tmp_ai = TMP_AVERAGING - 1): tmp_ai--;               
      ADMUX = ADCI_MAP|ADC_VREF_TYPE;    
      f1.sens_ready = 1;                
      break; 
 } 
}


//���������� �� ����������� �/� 2 - ��� ������� ��������� ���������� � �������. ���������� ������ 10��
#pragma vector=TIMER2_OVF_vect
__interrupt void timer2_ovf_isr(void)
{ 
 TCNT2=T2_RELOAD_VALUE; 
 if (stop_counter > 12)
  {//������������ ������������� ������� �������� ������������� � ��������� ���������
    f1.rotsync=1;
    BEGIN_MEASURE();
    stop_counter=0;
  }
   else
    stop_counter++;
    
  __enable_interrupt();       
  if (pars_counter < PAR_SAVE_COUNTER)
    pars_counter++; 
}
  
//������������ ������� �������� ��������� �� ����������� ������� ����������� SPEED_MEASURE_TEETH ������ �����.
//time_nt - ����� � ��������� ������� (���� �������� = 4���), � ����� ������ 60 ���, ���� �������� 60 ������,
//� ����� ������� 1000000 ���, ������:
//   N(min-1) = 60/((time*(60/SPEED_MEASURE_TEETH)*4)/1000000)
void rotation_frq(ecudata* d)
{
  unsigned int time;
  __disable_interrupt();
   time=time_nt;           //������������ ��������� ������ � ����������
  __enable_interrupt();      
  d->sens.inst_frq=((15000000L*SPEED_MEASURE_TEETH)/60)/(time);
}

//���������� ������ ���������� ��� ������� ��������
void fillfrq(ecudata* d)
{
  static unsigned char frq_ai=FRQ_AVERAGING-1;
  //��������� ���������� ������ ����������  � �������� ��� �������
  frq_abuf[frq_ai] = d->sens.inst_frq;      
  (frq_ai==0) ? (frq_ai = FRQ_AVERAGING - 1): frq_ai--;     
}

//���������� ���������� ������ ��������� � ���������� ������ � ��������� 
//��������� �����������, �������� �������, ������� ����
void units_control(ecudata *d)
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
void average_values(ecudata* d)
{     
      unsigned char i;unsigned long sum;unsigned long s;       
      ADCSRA&=0xE7;                                //��������� ���������� �� ���
            
      for (sum=0,i = 0; i < MAP_AVERAGING; i++)
       sum+=map_abuf[i];      
      d->sens.map=(sum/MAP_AVERAGING)*2; 
          
      for (sum=0,i = 0; i < BAT_AVERAGING; i++)   //��������� ���������� �������� ����
       sum+=bat_abuf[i];      
      d->sens.voltage=(sum/BAT_AVERAGING)*6; 
       
      if (d->param.tmp_use) 
      {       
       for (sum=0,i = 0; i < TMP_AVERAGING; i++) //��������� ����������� (����)
        sum+=tmp_abuf[i];      
       d->sens.temperat=((sum/TMP_AVERAGING)*5)/3; 
      }  
      else             //���� �� ������������
       d->sens.temperat=0;
    
      ADCSRA=(ADCSRA&0xEF)|(1<<ADIE);            //��������� ���������� �� ��� �� ��������� ���� ����������
           
      for (s=0,i = 0; i < FRQ_AVERAGING; i++)    //��������� ������� �������� ���������
       s+=frq_abuf[i];      
      d->sens.frequen=(s/FRQ_AVERAGING);           
}


//������������ ������ ����� ��������� ������� � ������� UART-a
void swap_domains(ecudata* edat)
{ 
 static unsigned char uscount=0;
 static unsigned char index=0;
 unsigned char i;

 if (f1.rcv_busy)//������� ����� ����� ?
 { 
  switch(rcv_mode)  //�������������� ������ ��������� ������ � ����������� �� �����������
  {
    case CHANGEMODE:       
        if (f1.snd_busy)    //���������� �����. ���������� ��������� ��� ������������ � ������ ����� ������ ���������� ������ 
            goto remitted;  //���������� ������� �������������
        snd_mode = ((ud_n*)rcv_data)->snd_mode;
        goto completed;     
    case BOOTLOADER:
        if (f1.snd_busy)    //���������� �����. ���������� ��������� ��� ������������ � ������ ����� ��������� ���������
            goto remitted;  //���������� ������� �������������
        __disable_interrupt(); //���� � ���������� ���� ������� "cli", �� ��� ������� ����� ������
        BOOT_JMP();         //������� �� ��������� ����� �������� ���������            
        goto completed;     
    case TEMPER_PAR:            
       edat->param.tmp_use   = ((ud_t*)rcv_data)->tmp_use;
       edat->param.vent_on   = ((ud_t*)rcv_data)->vent_on;
       edat->param.vent_off  = ((ud_t*)rcv_data)->vent_off; 
       goto param_changed;     
    case CARBUR_PAR:   
       edat->param.ephh_lot  = ((ud_c*)rcv_data)->ephh_lot;
       edat->param.ephh_hit  = ((ud_c*)rcv_data)->ephh_hit;
       edat->param.carb_invers=((ud_c*)rcv_data)->carb_invers;
       goto param_changed;     
    case IDLREG_PAR:   
       edat->param.idl_regul = ((ud_r*)rcv_data)->idl_regul;
       edat->param.ifac1     = ((ud_r*)rcv_data)->ifac1;        
       edat->param.ifac2     = ((ud_r*)rcv_data)->ifac2;       
       edat->param.MINEFR    = ((ud_r*)rcv_data)->MINEFR;       
       edat->param.idl_turns = ((ud_r*)rcv_data)->idl_turns;    
       goto param_changed;     
    case ANGLES_PAR:   
       edat->param.max_angle = ((ud_a*)rcv_data)->max_angle;    
       edat->param.min_angle = ((ud_a*)rcv_data)->min_angle;    
       edat->param.angle_corr= ((ud_a*)rcv_data)->angle_corr;   
       goto param_changed;     
    case FUNSET_PAR:   
       if (((ud_m*)rcv_data)->fn_benzin < TABLES_NUMBER)
          edat->param.fn_benzin = ((ud_m*)rcv_data)->fn_benzin;    
       if (((ud_m*)rcv_data)->fn_gas < TABLES_NUMBER)    
          edat->param.fn_gas    = ((ud_m*)rcv_data)->fn_gas;              
       edat->param.map_grad  = ((ud_m*)rcv_data)->map_grad;     
       edat->param.press_swing=((ud_m*)rcv_data)->press_swing;  
       goto param_changed;     
    case STARTR_PAR:   
       edat->param.starter_off=((ud_p*)rcv_data)->starter_off;  
       edat->param.smap_abandon=((ud_p*)rcv_data)->smap_abandon;
       goto param_changed;     
  }//switch     
param_changed:                   //���� ���� �������� ��������� �� ���������� ������� �������
  pars_counter=0;
completed:    
  f1.rcv_busy=0;   //�� ��������� �������� ������ - �������� ����� ������ �� ��������       
 }

 //������������ �������� ������ � �������
 if (uscount >= SND_TIMECONST)
 {
  if (!f1.snd_busy)
  {                
  //���������� ����� �� �������� - ������ ����� ���������� ������ 
  //� ����������� �� �������� ����������� ���������� ������� ������������ ��������������� ������
  switch(snd_mode)
  {
    case TEMPER_PAR:   
       ((ud_t*)snd_data)->tmp_use     = edat->param.tmp_use;
       ((ud_t*)snd_data)->vent_on     = edat->param.vent_on;
       ((ud_t*)snd_data)->vent_off    = edat->param.vent_off;
       break;
    case CARBUR_PAR:   
       ((ud_c*)snd_data)->ephh_lot    = edat->param.ephh_lot;
       ((ud_c*)snd_data)->ephh_hit    = edat->param.ephh_hit;
       ((ud_c*)snd_data)->carb_invers = edat->param.carb_invers;
       break;
    case IDLREG_PAR:   
       ((ud_r*)snd_data)->idl_regul   = edat->param.idl_regul;
       ((ud_r*)snd_data)->ifac1       = edat->param.ifac1;
       ((ud_r*)snd_data)->ifac2       = edat->param.ifac2;
       ((ud_r*)snd_data)->MINEFR      = edat->param.MINEFR;
       ((ud_r*)snd_data)->idl_turns   = edat->param.idl_turns;
       break;
    case ANGLES_PAR:   
       ((ud_a*)snd_data)->max_angle   = edat->param.max_angle;
       ((ud_a*)snd_data)->min_angle   = edat->param.min_angle;
       ((ud_a*)snd_data)->angle_corr  = edat->param.angle_corr;
       break;
   case FUNSET_PAR:   
       ((ud_m*)snd_data)->fn_benzin   = edat->param.fn_benzin;
       ((ud_m*)snd_data)->fn_gas      = edat->param.fn_gas;
       ((ud_m*)snd_data)->map_grad    = edat->param.map_grad;
       ((ud_m*)snd_data)->press_swing = edat->param.press_swing;
       break;
   case STARTR_PAR:   
       ((ud_p*)snd_data)->starter_off = edat->param.starter_off;
       ((ud_p*)snd_data)->smap_abandon = edat->param.smap_abandon;
       break;
    case FNNAME_DAT:
       for(i = 0; i < F_NAME_SIZE; i++ )
          ((ud_f*)snd_data)->name[i]=tables[index].name[i];
       ((ud_f*)snd_data)->tables_num = TABLES_NUMBER;
       ((ud_f*)snd_data)->index      = index++;       
       if (index>=TABLES_NUMBER) index=0;              
       break;
    case SENSOR_DAT:
       memcpy(&((ud_s*)snd_data)->sens,&edat->sens,sizeof(sensors));
       ((ud_s*)snd_data)->ephh_valve  = edat->ephh_valve;
       ((ud_s*)snd_data)->airflow     = edat->airflow;
       ((ud_s*)snd_data)->curr_angle  = edat->curr_angle;       
       break;
  }//switch
  UDR='@';                       //�������� �������� ����� ������� ������ ���� ��������� �������� ����������
  f1.snd_busy=1;                 //������ ���������� �������� ��������� ������
  uscount=0;
  }
 }
remitted: 
  if (uscount < SND_TIMECONST) uscount++; 
}


//��������������� ��������� ����� ������ ���������
void InitialMeasure(ecudata* e)
{ 
  unsigned char i=16;
  __enable_interrupt();
  do
  {
      BEGIN_MEASURE();                                                     
      while(!f1.sens_ready); 
  }while(--i);  
  __disable_interrupt();
  average_values(e);  
  e->atmos_press = e->sens.map;      //��������� ����������� ��������
}

//�������� ��������� ���� ������ �� flash � SRAM
void memcpy_f(unsigned char* sram,unsigned char* fl,int size)
{
   int count;
   for(count = 0; count < size; count++)
    sram[count] = ((__flash unsigned char*)fl)[count];
}


//������ ������ � EEPROM - ������� ����� ���������. �� ����� ��������� ����������� � ����������� ���������,
//� ��� ����������� ����������� �������� ������ � ��������� ����� � �� ���� �� ����� ����� � EEPROM.
//���������� ������ � EEPROM ���������� ������ ���� �� �������� ����� �� ��������� �� ����� �������� ������ ����������
//�� UART-a � ����������� ��������� ���������� �� �������.        
void save_param_if_need(ecudata* pd)
{
  if (pars_counter==PAR_SAVE_COUNTER) //��������� �� ���������� �� �������� �����
 {
  if (memcmp(eeprom_buf,&pd->param,sizeof(params)-PAR_CRC_SIZE)) //������� � ����������� ��������� ����������?
  {
    memcpy(eeprom_buf,&pd->param,sizeof(params));  
    ((params*)eeprom_buf)->crc=crc16(eeprom_buf,sizeof(params)-PAR_CRC_SIZE); //������� ����������� �����
    EE_START_WR_DATA(EEPROM_PARAM_START,eeprom_buf,sizeof(params));
  }
   pars_counter=0;
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
  memcpy_f((unsigned char*)&d->param,(unsigned char*)&def_param,sizeof(params));
 }    
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

  //������������� ���, ���������: f = 125.000 kHz, ���������� �������� �������� ���������� - 2.56V, ���������� ��������� 
  ADMUX=ADC_VREF_TYPE;
  ADCSRA=(1<<ADEN)|(1<<ADIE)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);     

  //��������� ���������� - �� ��� �� �����  
  ACSR=(1<ACD);             

  InitialMeasure(&edat);   //�������� ��������� ������ ��������� �������� ��� ������������� ������

  PORTD_Bit7 = 0;     //������� ���������� ��������
  
  //������������� ������� T0 � T1
  TCCR0  = (1<<CS01)|(1<<CS00);                             //clock = 250kHz
  TCCR2  = (1<<CS22)|(1<<CS21)|(1<<CS20);                   //clock = 15.625kHz
  TCCR1B = (1<<ICNC1)|(1<<ICES1)|(1<<CS11)|(1<<CS10);       //���������� ����, �������� ����� �������, clock = 250kHz
  TIMSK  = (1<<TICIE1)|(1<<TOIE0)|(1<<TOIE1)|(1<<TOIE2);    //��������� ���������� �� ������� � ������������ �/C 1, ������������ T/C 0, ������������ T/C 2
    
  //�������������� UART
   USART_Init(CBR_9600);

  //������������� ����� ������ ����������� � �������������� ����������                                       
  f1.t0mode=0;   
  time_nt=0xFFFF;           
  goal_angle=0;       
    
  //������ ���������
  load_eeprom_params(&edat);
  
  //��������� ��������� ����������            
  __enable_interrupt();    
     
  while(1)
  {
    rotation_frq(&edat);                      
    if (f1.rotsync)
    {//������ ����� ��������� ������������������ �������� 
      fillfrq(&edat);    
      f1.rotsync=0;
      stop_counter=0;
    }    
      
    average_values(&edat);        
    units_control(&edat);
    
    //� ����������� �� �������� ���� ������� �������� ��������������� ����� ������             
    if (edat.sens.gas)
      edat.fn_dat = (__flash F_data*)&tables[edat.param.fn_gas];    //�� ����
    else  
      edat.fn_dat = (__flash F_data*)&tables[edat.param.fn_benzin];//�� �������
    
    
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
    
    //��������� ��� ��� ���������� � ��������� �� ������� ����� ���������        
    __disable_interrupt();
     goal_angle = edat.curr_angle;
    __enable_interrupt();                
  
   swap_domains(&edat);  //����� ������� ����� ��������  
   
   save_param_if_need(&edat);    //���������� ����������� ��������                    
  }
}
