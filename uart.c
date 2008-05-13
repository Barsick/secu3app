 /****************************************************************
 *       SECU-3  - An open source, free engine control unit
 *    Designed by Alexey A. Shabelnikov. Ukraine, Gorlovka 2007.
 *       Microprocessors systems - design & programming.
 *    contacts:
 *              http://secu-3.narod.ru
 *              ICQ: 405-791-931
 ****************************************************************/

#include <ina90.h>
#include <iom16.h>
#include "uart.h"
#include "ufcodes.h"

char snd_mode = SENSOR_DAT;
char rcv_mode = 0;

unsigned char r_data[64];                                                 //����� UART-a - ������ ����������� �������
unsigned char s_data[64];                                                 //����� UART-a - ������ ������������ �������
unsigned char *rcv_data=r_data;
unsigned char *snd_data=s_data;
const __flash char hdig[] = "0123456789ABCDEF";


#define HTOD(h) ((h<0x3A)?h-'0':h-'A'+10)



void USART_Init(unsigned int baud)
{
  // Set baud rate 
  UBRRH = (unsigned char)(baud>>8);
  UBRRL = (unsigned char)baud;
  UCSRA = 0;                                                  //�������� �� ���������� 
  UCSRB=(1<<RXCIE)|(1<<TXCIE)|(1<<RXEN)|(1<<TXEN);            //��������,���������� � �� ���������� ���������
  UCSRC=(1<<URSEL)/*|(1<<USBS)*/|(1<<UCSZ1)|(1<<UCSZ0);       //8 ���, 1 ����, ��� �������� ��������
  f1.snd_busy = 0;                                            //���������� �� ��� �� ��������
  f1.rcv_busy = 0;
}


unsigned char send_i8h(unsigned char i)
{
  static unsigned char state=0;
  static unsigned char ii;    
  switch(state)
  {
    case 0:
      ii=i;
      UDR = hdig[ii/16];    //�������� ������� ���� HEX �����
      state=1;
      return state;
    case 1:  
      UDR = hdig[ii%16];    //�������� ������� ���� HEX �����
      state=0;              //�� � �������� ���������
      return state;      
  }
  return 0;
}

unsigned char send_i16h(unsigned int i)
{
  static unsigned char state=0;
  static unsigned char il,ih;  
  switch(state)
  {
    case 0:
      il=GETBYTE(i,0);
      ih=GETBYTE(i,1);
      UDR = hdig[ih/16];    //�������� ������� ���� HEX ����� (������� ����)
      state=1;
      return state;
    case 1:  
      UDR = hdig[ih%16];    //�������� ������� ���� HEX ����� (������� ����)
      state=2;
      return state;      
    case 2:  
      UDR = hdig[il/16];    //�������� ������� ���� HEX ����� (������� ����)
      state=3;
      return state;      
    case 3:  
      UDR = hdig[il%16];    //�������� ������� ���� HEX ����� (������� ����)
      state=0;              //�� � �������� ���������
      return state;      
  }
  return 0;
}

unsigned char recv_i8h(unsigned char *s)
{
  static unsigned char state=0;
  static unsigned char ii;    
  unsigned char u=UDR;             //�� UDR ����� ��������� ������ ���� ���
  switch(state)
  {
    case 0:       
      ii=HTOD(u)<<4;    
      state++;
      return 0;
    case 1:  
      ii|=HTOD(u);          
      state=0;              //�� � �������� ���������
      (*s)++;
      return ii;      
  }
  return 0;
}

unsigned int recv_i16h(unsigned char *s)
{
  static unsigned char state=0;
  static unsigned int ii;    
  unsigned char u=UDR;
  switch(state)
  {
    case 0:      
      SETBYTE(ii,1) = (HTOD(u))<<4;          
      state++;
      return 0;
    case 1:  
      SETBYTE(ii,1)|=(HTOD(u));          
      state++;              
      return 0;      
    case 2:   
      SETBYTE(ii,0)=(HTOD(u))<<4;    
      state++;
      return 0;
    case 3:  
      SETBYTE(ii,0)|=(HTOD(u));          
      state=0;              //�� � �������� ���������
      (*s)++;
      return ii;      
  }
  return 0;
}



//���������� ���������� �� �������� ������ ����� UART
#pragma vector=USART_TXC_vect
__interrupt void usart_tx_isr(void)
{       
static unsigned char state=0;
static unsigned char i;
 __enable_interrupt();
 switch(snd_mode)
 {     
   case TEMPER_PAR:  
     switch(state)
     {
      case 0:
        UDR = snd_mode;   //�������� ���������� �������
        state++;
        break;
      case 1:
        UDR = ((ud_t*)snd_data)->tmp_use+0x30;        
          state++;
        break;  
      case 2:
        if (!send_i16h(((ud_t*)snd_data)->vent_on))       
          state++;
        break;  
      case 3:
        if (!send_i16h(((ud_t*)snd_data)->vent_off))       
          state++;
        break;  
      case 4: 
        UDR = '\r';         //�������� ������ - ������� ����� ������
        state++;
        break;
      case 5: 
        state=0;          //���������� �� � �������� ��������� 
        f1.snd_busy=0;   //���������� ����� � �������� ����� ������
        break;            
     }
     break; 
     
  case CARBUR_PAR:  
     switch(state)
     {
      case 0:
        UDR = snd_mode;   //�������� ���������� �������
        state++;
        break;
      case 1:
        if (!send_i16h(((ud_c*)snd_data)->ephh_lot))       
          state++;
        break;  
      case 2:
        if (!send_i16h(((ud_c*)snd_data)->ephh_hit))       
          state++;
        break;  
      case 3:
        UDR = ((ud_c*)snd_data)->carb_invers+0x30;        
          state++;
        break;  
      case 4: 
        UDR = '\r';         //�������� ������ - ������� ����� ������
        state++;
        break;
      case 5: 
        state=0;          //���������� �� � �������� ��������� 
        f1.snd_busy=0;   //���������� ����� � �������� ����� ������
        break;            
     }
     break;      
     
  case IDLREG_PAR:  
     switch(state)
     {
      case 0:
        UDR = snd_mode;   //�������� ���������� �������
        state++;
        break;
      case 1:
        UDR = ((ud_r*)snd_data)->idl_regul+0x30;         
        state++;
        break;  
      case 2:
        if (!send_i16h(((ud_r*)snd_data)->ifac1))       
          state++;
        break;  
      case 3:
        if (!send_i16h(((ud_r*)snd_data)->ifac2))       
          state++;
        break;  
      case 4:
        if (!send_i16h(((ud_r*)snd_data)->MINEFR))       
          state++;
        break;  
      case 5:
        if (!send_i16h(((ud_r*)snd_data)->idl_turns))       
          state++;
        break;  
      case 6: 
        UDR = '\r';         //�������� ������ - ������� ����� ������
        state++;
        break;
      case 7: 
        state=0;          //���������� �� � �������� ��������� 
        f1.snd_busy=0;   //���������� ����� � �������� ����� ������
        break;            
     }
     break;      

    case ANGLES_PAR:  
     switch(state)
     {
      case 0:
        UDR = snd_mode;   //�������� ���������� �������
        state++;
        break;
      case 1:
        if (!send_i16h(((ud_a*)snd_data)->max_angle))       
          state++;
        break;  
      case 2:
        if (!send_i16h(((ud_a*)snd_data)->min_angle))       
          state++;
        break;  
      case 3:
        if (!send_i16h(((ud_a*)snd_data)->angle_corr))       
          state++;
        break;  
      case 4: 
        UDR = '\r';         //�������� ������ - ������� ����� ������
        state++;
        break;
      case 5: 
        state=0;          //���������� �� � �������� ��������� 
        f1.snd_busy=0;   //���������� ����� � �������� ����� ������
        break;            
     }
     break;      

   case FUNSET_PAR:  
     switch(state)
     {
      case 0:
        UDR = snd_mode;   //�������� ���������� �������
        state++;
        break;
      case 1:
        if (!send_i8h(((ud_m*)snd_data)->fn_benzin))       
          state++;
        break;  
      case 2:
        if (!send_i8h(((ud_m*)snd_data)->fn_gas))       
          state++;
        break;  
      case 3:
        if (!send_i8h(((ud_m*)snd_data)->map_grad))       
          state++;
        break;  
      case 4:
        if (!send_i16h(((ud_m*)snd_data)->press_swing))       
          state++;
        break;  
      case 5: 
        UDR = '\r';         //�������� ������ - ������� ����� ������
        state++;
        break;
      case 6: 
        state=0;          //���������� �� � �������� ��������� 
        f1.snd_busy=0;   //���������� ����� � �������� ����� ������
        break;            
     }
     break;      
     
     
  case STARTR_PAR:  
     switch(state)
     {
      case 0:
        UDR = snd_mode;   //�������� ���������� �������
        state++;
        break;
      case 1:
        if (!send_i16h(((ud_p*)snd_data)->starter_off))       
          state++;
        break;  
      case 2:
        if (!send_i16h(((ud_p*)snd_data)->smap_abandon))       
          state++;
        break;  
      case 3: 
        UDR = '\r';         //�������� ������ - ������� ����� ������
        state++;
        break;
      case 4: 
        state=0;          //���������� �� � �������� ��������� 
        f1.snd_busy=0;   //���������� ����� � �������� ����� ������
        break;            
     }
     break;         
     
     
  case FNNAME_DAT:
     switch(state)
     {
      case 0:
        UDR = snd_mode;   //�������� ���������� �������
        state++;
        break;
      case 1:
        if (!send_i8h(((ud_f*)snd_data)->tables_num))       
          state++;
        break;  
      case 2:
        if (!send_i8h(((ud_f*)snd_data)->index))       
          state++;
        i=0;  
        break;  
      case 3: //�������� ������� ����� ���������
          UDR=((ud_f*)snd_data)->name[i++];
          if (i>=F_NAME_SIZE)
             state++;             
        break;          
      case 4: 
        UDR = '\r';         //�������� ������ - ������� ����� ������
        state++;
        break;
      case 5: 
        state=0;          //���������� �� � �������� ��������� 
        f1.snd_busy=0;   //���������� ����� � �������� ����� ������
        break;            
     }     
     break;     
     
     
  case SENSOR_DAT:
     switch(state)
     {
      case 0:
        UDR = snd_mode;   //�������� ���������� �������
        state++;
        break;
      case 1:  
        if (!send_i16h(((ud_s*)snd_data)->sens.frequen))   //������� �������� ���������
          state++;
        break;    
      case 2:  
        if (!send_i16h(((ud_s*)snd_data)->sens.map))       //���������� �������� �� �������� ����������
          state++;
        break;    
      case 3:  
        if (!send_i16h(((ud_s*)snd_data)->sens.voltage))   //���������� � �������� ����
          state++;
        break;    
      case 4:  
        if (!send_i16h(((ud_s*)snd_data)->sens.temperat)) //����������� ����������� ��������
          state++;
        break;    
      case 5:  
        if (!send_i16h(((ud_s*)snd_data)->curr_angle))   //������� ���
          state++;
        break;            
      case 6:  
        if (!send_i8h(((ud_s*)snd_data)->airflow))       //��� ������� - ������ �������
          state++;
        break;            
      case 7:  
        UDR = ((ud_s*)snd_data)->ephh_valve+0x30;       //���� ������ - ��������� ������� ����
        state++;
        break;            
      case 8:  
        UDR = ((ud_s*)snd_data)->sens.carb+0x30;        //���� ������ - ��������� ����������� ��������
        state++;
        break;    
      case 9:  
        UDR = ((ud_s*)snd_data)->sens.gas+0x30;         //��������� �������� �������
        state++;
        break;                        
      case 10: 
        UDR = '\r';         //�������� ������ - ������� ����� ������
        state++;
        break;
      case 11: 
        state=0;          //���������� �� � �������� ��������� 
        f1.snd_busy=0;   //���������� ����� � �������� ����� ������
        break;    
     }       
     break;      
 }
}


//���������� ���������� �� ������ ������ ����� UART
#pragma vector=USART_RXC_vect
__interrupt void usart_rx_isr()
{
  static unsigned char cstate=0,state=0;  

  switch(cstate)
  {
    case 0:            //��������� (������� ������ ������ �������)
      if (UDR=='!')      
      {
         cstate++;               
      }
      break;
    case 1:           //��������� ���������� �������
      rcv_mode=UDR;
      cstate++;
      state=0;      
      break;  
    case 2:           //����� ������ �������
      switch(rcv_mode)
      {
       case CHANGEMODE:
         switch(state)
         {
           case 0:     //������� �������� ������ �����������
             ((ud_n*)rcv_data)->snd_mode = UDR;
             cstate=3;
             break;         
         }       
         break;   
      
      
       case BOOTLOADER:     //������ ����������
         switch(state)
         {
           case 0:     
             if (UDR=='l')             
               cstate=3;
             else
               cstate=0;    //������                                        
             break;         
         }       
         break;   
                          
       case TEMPER_PAR:
         switch(state)
         {
           case 0:
             ((ud_t*)rcv_data)->tmp_use = UDR - 0x30;                
             state++;
             break;      
           case 1:  
             ((ud_t*)rcv_data)->vent_on=recv_i16h(&state);                
             break;     
           case 2:  
             ((ud_t*)rcv_data)->vent_off=recv_i16h(&cstate);  
             break;                                                                                                
         }       
         break;   
                  
                  
       case CARBUR_PAR:  
         switch(state)
         {
           case 0:  
             ((ud_c*)rcv_data)->ephh_lot=recv_i16h(&state);                
             break;     
           case 1:  
             ((ud_c*)rcv_data)->ephh_hit=recv_i16h(&state);  
             break;                                                                                                
           case 2:
             ((ud_c*)rcv_data)->carb_invers = UDR - 0x30;                
             cstate=3;              
             break;      
         }            
         break;               


       case IDLREG_PAR:  
         switch(state)
         {
           case 0:
             ((ud_r*)rcv_data)->idl_regul = UDR - 0x30;                
             state++;
             break;      
           case 1:  
             ((ud_r*)rcv_data)->ifac1=recv_i16h(&state);                
             break;     
           case 2:  
             ((ud_r*)rcv_data)->ifac2=recv_i16h(&state);  
             break;                                                                                                
           case 3:  
             ((ud_r*)rcv_data)->MINEFR=recv_i16h(&state);  
             break;                                                                                                
           case 4:  
             ((ud_r*)rcv_data)->idl_turns=recv_i16h(&cstate);  
             break;                                                                                                
         }         
         break;               


       case ANGLES_PAR: 
         switch(state)
         {
            case 0:  
             ((ud_a*)rcv_data)->max_angle=recv_i16h(&state);                
             break;     
           case 1:  
             ((ud_a*)rcv_data)->min_angle=recv_i16h(&state);  
             break;                                                                                                
           case 2:  
             ((ud_a*)rcv_data)->angle_corr=recv_i16h(&cstate);  
             break;                                                                                                
         }        
         break;               

         
       case FUNSET_PAR:  
         switch(state)
         {
           case 0:
             ((ud_m*)rcv_data)->fn_benzin=recv_i8h(&state);                
             state++;
             break;      
           case 1:  
             ((ud_m*)rcv_data)->fn_gas=recv_i8h(&state);                
             break;     
           case 2:  
             ((ud_m*)rcv_data)->map_grad=recv_i8h(&state);  
             break;                                                                                                
           case 3:  
             ((ud_m*)rcv_data)->press_swing=recv_i16h(&cstate);  
             break;                                                                                                
         }         
         break;               


       case STARTR_PAR:  
         switch(state)
         {
           case 0:  
             ((ud_p*)rcv_data)->starter_off=recv_i16h(&state);                
             break;     
           case 1:  
             ((ud_p*)rcv_data)->smap_abandon=recv_i16h(&cstate);  
             break;                                                                                                
         }               
         break;                        
         
      }   
      break;   
      
    case 3:  
       if (UDR=='\r')
       {             
         cstate=0;       //�� � �������� ���������      
         f1.rcv_busy=1;  //���������� ������� ���������� ������
       }    
      break;                   
  }
  
  
}
