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
int work_function(ecudata* d)
{    
   int  gradient, discharge, rpm = d->sens.inst_frq;
   signed char f,l,fp1,lp1;   

   //������� ���� ������������, ������ ����������� ���� ������� ������� �� �������            
   for(f = 14; f >= 0; f--)   
     if (rpm >= F_SlotsRanges[f]) break; 
                            
   //������� ����� �������� �� 600-� �������� � ����                                                        
   if (f < 0)  {f = 0; rpm = 600;}
   fp1 = f + 1;   
   
   discharge = (d->param.map_upper_pressure - d->sens.map);
   if (discharge < 0) discharge = 0;         
   
   //map_upper_pressure - ������� �������� ��������
   //map_lower_pressure - ������ �������� ��������
   gradient = (d->param.map_upper_pressure - d->param.map_lower_pressure) / 16; //����� �� ���������� ����� ������������ �� ��� ��������
   l = (discharge / gradient);
   
   lp1 = l + 1;      
   if (l >= (F_WRK_POINTS_F - 1))
   { 
     lp1 = l = F_WRK_POINTS_F - 1;
   }
   //��������� ���������� ������� �������
   d->airflow = 16 - l;
   
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


//���������������� ��������� ��� ������������� �������� �� ����� ���������� ���������     
// ���������� �������� ���� ���������� � ����� ���� * 32.
int idling_pregulator(ecudata* d)
{
  int correction,error,factor;      
  //���� ��������� �������������� ������������� �������� �� ��� �������
  //������ �� ��������, �� �������  � ������� ��������������        
  if (!d->param.idl_regul)
    return 0;  
  error = d->param.idling_rpm - d->sens.inst_frq;  
  
  //������������ ������, � ����� ���� �� � ���� ������������������, �� ��� �������������
  if (error > 500) error = 500;
  if (error <-500) error = -500;
  if (abs(error) < d->param.MINEFR) 
    return 0;
    
  //�������� ����������� �����������, � ����������� �� ����� ������
  if (error > 0)
     factor = d->param.ifac1;
  else    
     factor = d->param.ifac2;                         
     
  //��� ������������ ������ 1.0, �������� ��������� ��� ����� �������� ��������� ������,
  //������������ ������������ ����� ������������ ���!   
  correction = (factor * error);
  if (correction > ANGLE_MAGNITUDE(35))  //������� ������ �������������
      return ANGLE_MAGNITUDE(35);
  if (correction < ANGLE_MAGNITUDE(-35))  //������ ������ �������������
      return ANGLE_MAGNITUDE(-35);
      
  return correction;    
}


//���������� ��� ������������� ��� �� ���������� ������� ���������
//new_advance_angle - ����� �������� ���
//intstep - �������� ���� ��������������, ������������� �����
//is_enabled - ���� ����� 1, �� ������������� ���������, 0 - ���������
//���������� ����������������� ���
int transient_state_integrator(int new_advance_angle, unsigned int intstep, char is_enabled)
{
 static signed int old_advance_angle = 0;
 signed int difference;
 if (is_enabled)
 {
  difference = new_advance_angle - old_advance_angle;  
  if (abs(difference) > intstep)
  {
   if (difference > 0)
     old_advance_angle+=intstep;
   else    
     old_advance_angle-=intstep;
   return old_advance_angle;
  }
 }
 //������� ��� ����� ���������� � ��������� ���
 old_advance_angle = new_advance_angle;
 return old_advance_angle;
}
