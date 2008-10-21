 /****************************************************************
 *       SECU-3  - An open source, free engine control unit
 *    Designed by Alexey A. Shabelnikov. Ukraine, Gorlovka 2007.
 *       Microprocessors systems - design & programming.
 *    contacts:
 *              http://secu-3.narod.ru
 *              ICQ: 405-791-931
 ****************************************************************/

#include <stdlib.h>
#include "funconv.h"
#include "adc.h"
#include "ckps.h"
#include "secu3.h"
#include "vstimer.h"
#include "magnitude.h"


//������ ������� �������� ������ ����� �� ��� ��������, ��� ������� ����� � ����� ��.
__flash const int F_SlotsRanges[16] = {600,720,840,990,1170,1380,1650,1950,2310,2730,3210,3840,4530,5370,6360,7500}; 
__flash const int F_SlotsLength[15] = {120,120,150,180, 210, 270, 300, 360, 420, 480, 630, 690, 840, 990, 1140}; 

// ������� ���������� ������������ (�����������)
// x, y - �������� ���������� ��������������� �������
// a1,a2,a3,a4 - �������� ������� � ����� ������������ (���� ����������������)
// x_s,y_s - �������� ���������� ������� ��������������� ������ ������������� �������
// x_l,y_l - ������� ������������� ������� (�� x � y ��������������)
// ���������� ����������������� �������� ������� * 16         
int bilinear_interpolation(int x,int y,int a1,int a2,int a3,int a4,int x_s,int y_s,int x_l,int y_l)
{
   int a23,a14;  
   a23 = ((a2 * 16) + (((long)(a3 - a2) * 16) * (x - x_s)) / x_l);
   a14 = (a1 * 16) + (((long)(a4 - a1) * 16) * (x - x_s)) / x_l;
   return (a14 + ((((long)(a23 - a14)) * (y - y_s)) / y_l));
} 

// ������� �������� ������������
// x - �������� ��������� ��������������� �������
// a1,a2 - �������� ������� � ����� ������������
// x_s - �������� ��������� ������� � ��������� �����
// x_l - ����� ������� ����� �������
// ���������� ����������������� �������� ������� * 16                   
int simple_interpolation(int x,int a1,int a2,int x_s,int x_l)
{
  return ((a1 * 16) + (((long)(a2 - a1) * 16) * (x - x_s)) / x_l);
}


// ��������� ������� ��� �� �������� ��� ��������� ����
// ���������� �������� ���� ���������� � ����� ���� * 32. 2 * 16 = 32.
int idling_function(ecudata* d)
{
  signed char i;
  int rpm = d->sens.inst_frq;

  //������� ���� ������������, ������ ����������� ���� ������� ������� �� �������
  for(i = 14; i >= 0; i--)
    if (d->sens.inst_frq >= F_SlotsRanges[i]) break;                        

  if (i < 0)  {i = 0; rpm = 600;}

  return simple_interpolation(rpm,
              d->fn_dat->f_idl[i],d->fn_dat->f_idl[i+1],
              F_SlotsRanges[i],F_SlotsLength[i]);
}


// ��������� ������� ��� �� �������� ��� ����� ���������
// ���������� �������� ���� ���������� � ����� ���� * 32, 2 * 16 = 32.
int start_function(ecudata* d)
{
  int i,i1,rpm = d->sens.inst_frq;                                           

  if (rpm < 200) rpm = 200; //200 - ����������� �������� ��������

  i = (rpm - 200) / 40;   //40 - ��� �� ��������

  if (i >= 15) i = i1 = 15; 
  else i1 = i + 1;

  return simple_interpolation(rpm,d->fn_dat->f_str[i],d->fn_dat->f_str[i1], (i * 40) + 200, 40);
}


// ��������� ������� ��� �� ��������(���-1) � ��������(���) ��� �������� ������ ���������
// ���������� �������� ���� ���������� � ����� ���� * 32, 2 * 16 = 32.
int work_function(ecudata* d, char i_update_airflow_only)
{    
   int  gradient, discharge, rpm = d->sens.inst_frq, l;
   signed char f,fp1,lp1;   

   discharge = (d->param.map_upper_pressure - d->sens.map);
   if (discharge < 0) discharge = 0;         
   
   //map_upper_pressure - ������� �������� ��������
   //map_lower_pressure - ������ �������� ��������
   gradient = (d->param.map_upper_pressure - d->param.map_lower_pressure) / 16; //����� �� ���������� ����� ������������ �� ��� ��������
   if (gradient < 1)
     gradient = 1;  //��������� ������� �� ���� � ������������� �������� ���� ������� �������� ������ �������
   l = (discharge / gradient);
   
   if (l >= (F_WRK_POINTS_F - 1))
     lp1 = l = F_WRK_POINTS_F - 1;
   else
     lp1 = l + 1;      

   //��������� ���������� ������� �������
   d->airflow = 16 - l;
   
   if (i_update_airflow_only)
     return 0; //������� ���� ��������� ������ ��� �� ������ �������� ������ ������ �������

   //������� ���� ������������, ������ ����������� ���� ������� ������� �� �������            
   for(f = 14; f >= 0; f--)   
     if (rpm >= F_SlotsRanges[f]) break; 
                            
   //������� ����� �������� �� 600-� �������� � ����                                                        
   if (f < 0)  {f = 0; rpm = 600;}
   fp1 = f + 1;   
      
   return bilinear_interpolation(rpm, discharge,
	   d->fn_dat->f_wrk[l][f],
	   d->fn_dat->f_wrk[lp1][f],
	   d->fn_dat->f_wrk[lp1][fp1],
	   d->fn_dat->f_wrk[l][fp1],
	   F_SlotsRanges[f],
	   (gradient * l),
	   F_SlotsLength[f],
	   gradient);
}


//��������� ������� ��������� ��� �� �����������(����. �������) ����������� ��������
// ���������� �������� ���� ���������� � ����� ���� * 32, 2 * 16 = 32.
int coolant_function(ecudata* d)
{ 
  int i,i1,t = d->sens.temperat;                                           

  if (!d->param.tmp_use) 
    return 0;   //��� ���������, ���� ���� ��������������� ����-��
    
  //-30 - ����������� �������� �����������
  if (t < TEMPERATURE_MAGNITUDE(-30)) 
    t = TEMPERATURE_MAGNITUDE(-30);   

  //10 - ��� ����� ������ ������������ �� �����������
  i = (t - TEMPERATURE_MAGNITUDE(-30)) / TEMPERATURE_MAGNITUDE(10);   

  if (i >= 15) i = i1 = 15; 
  else i1 = i + 1;

  return simple_interpolation(t,d->fn_dat->f_tmp[i],d->fn_dat->f_tmp[i1], 
  (i * TEMPERATURE_MAGNITUDE(10)) + TEMPERATURE_MAGNITUDE(-30), TEMPERATURE_MAGNITUDE(10));   
}

//��������� ��������� ���� ���
typedef struct
{
  //������ ���������� ��� �������� ���������� �������� ������������ ����������� (���������)
  int output_state;
}IDLREGULSTATE;

IDLREGULSTATE idl_prstate;

//����� ��������� ���
void idling_regulator_init(void)
{
 idl_prstate.output_state = 0;
}

//���������������� ��������� ��� ������������� �������� �� ����� ���������� ���������     
// ���������� �������� ���� ���������� � ����� ���� * 32.
int idling_pregulator(ecudata* d, s_timer8* io_timer)
{
  int error,factor;
  //���� "��������" ���������� ��� ���������� ��������� �� �������� ������ � ��
  unsigned int capture_range = 200; 
    
  //���� PXX �������� ��� ������� ����������� ���� �� ���������� �������� ��������  
  // ��� ��������� �� ������� �� �������  � ������� ��������������        
  if (!d->param.idl_regul || (d->sens.frequen >(d->param.idling_rpm + capture_range))
       || (d->sens.temperat < TEMPERATURE_MAGNITUDE(70) && d->param.tmp_use))
    return 0;  
    
  //��������� �������� ������, ������������ ������ (���� �����), � �����, ���� �� � ���� 
  //������������������, �� ���������� ����������� ����� ���������.     
  error = d->param.idling_rpm - d->sens.frequen;   
  restrict_value_to(&error, -200, 200);
  if (abs(error) <= d->param.MINEFR) 
    return idl_prstate.output_state;
  
  //�������� ����������� ����������� ����������, � ����������� �� ����� ������
  if (error > 0)
    factor = d->param.ifac1;
  else
    factor = d->param.ifac2;                         
  
  //�������� �������� ��������� ������ �� ������� idle_period_time_counter
  if (s_timer_is_action(*io_timer))
  { 
    s_timer_set(*io_timer,IDLE_PERIOD_TIME_VALUE);
    idl_prstate.output_state = idl_prstate.output_state + (error * factor) / 4;
  }
  //������������ ��������� ������ � ������� ��������� �������������      
  restrict_value_to(&idl_prstate.output_state, ANGLE_MAGNITUDE(-12), ANGLE_MAGNITUDE(12));    
      
  return idl_prstate.output_state;    
}

//���������� ������ �������������� �������� ��������� ��� �� ���������� ������� ���������
//new_advance_angle - ����� �������� ���
//ip_prev_state - �������� ��� � ���������� �����
//intstep_p,intstep_m - �������� �������������� � �������������� ����� ��������������, ������������� �����
//���������� ����������������� ���
int advance_angle_inhibitor(int new_advance_angle, int* ip_prev_state, signed int intstep_p, signed int intstep_m)
{
 signed int difference;
  
 difference = new_advance_angle - *ip_prev_state;  
  
 if (difference > intstep_p)
 {
  *ip_prev_state+=intstep_p;
  return *ip_prev_state;
 }
  
 if (difference < -intstep_m)
 {
  *ip_prev_state-=intstep_m;
  return *ip_prev_state;
 }
  
 //������� ��� ����� ���������� � ��������� ���
 *ip_prev_state = new_advance_angle;
 return *ip_prev_state;
}

//������������ ��������� �������� ���������� ���������
void restrict_value_to(int *io_value, int i_bottom_limit, int i_top_limit)
{
 if (*io_value > i_top_limit)
  *io_value = i_top_limit;  
 if (*io_value < i_bottom_limit)
  *io_value = i_bottom_limit;  
}
