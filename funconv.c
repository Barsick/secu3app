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

#define TSCALE_LO_VALUE     T_TO_DADC(-16)                      //-16 �������� ����� ������ ����� ����� ����������� (� �������� �������)
#define TSCALE_STEP      ((unsigned int)((11.0*TSENS_SLOPP)/ADC_DISCRETE)) // 11 �������� ����� ������ ������������ �� �������������� ��� (� ��������� ���)

__flash const int F_SlotsRanges[16] = {600,720,840,990,1170,1380,1650,1950,2310,2730,3210,3840,4530,5370,6360,7500}; 
__flash const int F_SlotsLength[15] = {120,120,150,180, 210, 270, 300, 360, 420, 480, 630, 690, 840, 990, 1140}; 


// ������� ���������� ������������ (�����������)
// n,p - ��������� ��������������� ��������������� (������� � ��������)
// a1,a2,a3,a4 - �������� ������� (���) � ����� ������������ (���� ����������������)
// n_s,p_s - ���������� ���� ������ (�� �������� � �������� ��������������)
// n_l,p_l - ������� ������� ������ (�� �������� � �������� ��������������)
// ���������� ����������������� �������� ������� * 16         
int func_i3d(int n,int p,int a1,int a2,int a3,int a4,int n_s,int p_s,int n_l,int p_l)
{
   int a23,a14;  
   a23 = ((a2*16)+(((long)(a3-a2)*16)*(n-n_s))/n_l);
   a14 = (a1*16)+(((long)(a4-a1)*16)*(n-n_s))/n_l;
   return (a14+((((long)(a23-a14))*(p-p_s))/p_l));
} 

// ������� �������� ������������
// n - ������� �������� ��������� (���-1)
// a1,a2 - �������� ������� (���) � ����� ������������
// n_s - �������� �������� � ��������� �����
// n_l - ����� ������� ����� �������
// ���������� ����������������� �������� ������� * 16                   
int func_i2d(int n,int a1,int a2,int n_s,int n_l)
{
  return ((a1*16)+(((long)(a2-a1)*16)*(n-n_s))/n_l);
}


// ��������� ������� ��� �� �������� ��� ��������� ����
// ���������� �������� ���� ���������� � ����� ���� * 32. 2 * 16 = 32.
int idl_func(ecudata* d)
{
  signed char i;
  int n = d->sens.inst_frq;

  //������� ���� ������������, ������ ����������� ���� ������� ������� �� �������
  for(i = 14; i >= 0; i--)
    if (d->sens.inst_frq >= F_SlotsRanges[i]) break;                        

  if (i < 0)  {i = 0; n = 600;}

  return func_i2d(n,
              d->fn_dat->f_idl[i],d->fn_dat->f_idl[i+1],
              F_SlotsRanges[i],F_SlotsLength[i]);
}


// ��������� ������� ��� �� �������� ��� ����� ���������
// ���������� �������� ���� ���������� � ����� ���� * 32, 2 * 16 = 32.
int str_func(ecudata* d)
{
  int i,i1,n = d->sens.inst_frq;                                           

  if (n < 200) n = 200; //200 - ����������� �������� ��������

  i = (n - 200) / 40;   //40 - ��� �� ��������

  if (i>=15) i = i1 = 15; 
  else i1 = i+1;

  return func_i2d(n,d->fn_dat->f_str[i],d->fn_dat->f_str[i1], (i * 40) + 200, 40);
}


// ��������� ������� ��� �� ��������(���-1) � ��������(���) ��� �������� ������ ���������
// ���������� �������� ���� ���������� � ����� ���� * 32, 2 * 16 = 32.
int wrk_func(ecudata* d)
{    
/*   int  j,i1,j1,prel,n=d->sens.inst_frq;
   signed char i;               
   for(i = 14; i >= 0; i--)   
     if (n >= F_SlotsRanges[i]) break;                           
   if (i<0)  {i = 0; n = 600;}
   prel = ((int)(d->atmos_press - d->param.press_swing))- d->sens.map;                  
   if (prel < 0) prel = 0;         
   j = (prel/d->param.map_grad);
   i1 = i+1;   j1 = j+1;
   if (j>=(F_WRK_POINTS_F-1))
   { j1 = j = F_WRK_POINTS_F-1;}
   d->airflow = 16-j;
   return func_i3d(n,prel,
	   d->fn_dat->f_wrk[j][i],
	   d->fn_dat->f_wrk[j1][i],
	   d->fn_dat->f_wrk[j1][i1],
	   d->fn_dat->f_wrk[j][i1],
	   F_SlotsRanges[i],
	   (d->param.map_grad*(int)j),
	   F_SlotsLength[i],
	   d->param.map_grad)*2;*/
 return 0;
}

/*
//�������������� �������������� �������� � ����������� � ���
// t - ����������� ����������� ��������
// p - �������� �� �������� ����������
// ���������� �������� ���� ���������� � ����� ���� * 40
int tmp_func(ecudata* d)
{  
   int i,j,i1,j1,prel,mg2,tm,t=d->sens.temperat;      
   if (!d->param.tmp_use) return 0;   //��� ���������, ���� ���� �� ������������� ����-��
   mg2 = ((int)d->param.map_grad)*2;
   prel = ((int)(d->atmos_press - d->param.press_swing))- d->sens.map;                  
   if (prel < 0) prel = 0;   
   if (t < TSCALE_LO_VALUE) t = TSCALE_LO_VALUE;
   tm = t-TSCALE_LO_VALUE;
   i  = tm/TSCALE_STEP;   j  = prel/mg2;
   if (i>=(F_TMP_POINTS_T-1))
   {  i1 = i = F_TMP_POINTS_T-1; }
   else   
      i1 = i+1;      
   if (j>=(F_TMP_POINTS_L-1))
   {  j1 = j = F_TMP_POINTS_L-1; }
   else   
      j1 = j+1;
   return func_i3d(tm,prel,
	   d->fn_dat->f_tmp[j][i],
	   d->fn_dat->f_tmp[j1][i],
	   d->fn_dat->f_tmp[j1][i1],
	   d->fn_dat->f_tmp[j][i1],
	   (TSCALE_STEP*((int)i)),
	   mg2*((int)j),
	   TSCALE_STEP,
	   mg2);
}
*/

//���������������� ��������� ��� ������������� �������� �� 
//���� ���������� ���������     
//  ���������� ��� * 40;
int idl_pregul(ecudata* d)
{
  int uoz,error,factor;      
  //���� ��������� �������������� ������������� �������� �� ��� �������
  //������ �� ��������, �� �������  � ������� ��������������        
  if ((!d->param.idl_regul)||(d->sens.inst_frq>1100))
      return 0;  
  error = d->param.idl_turns - d->sens.inst_frq;  
  //���� � ���� ������������������, �� ��� �������������
  if (error>500)  error = 500;
  if (error<-500) error = -500;
  if (abs(error)<d->param.MINEFR) 
     return 0;
  //�������� ����������� ����������� � ����������� ���������
  if (error>0)
     factor = d->param.ifac1;
  else    
     factor = d->param.ifac2;                         
  uoz = (factor*error)/10;
  if (uoz > (35*ANGLE_MULTIPLAYER))  //35 ����.
      return (35*ANGLE_MULTIPLAYER);
  else
      return uoz;  
}
