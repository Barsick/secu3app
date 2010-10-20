 /****************************************************************
 *       SECU-3  - An open source, free engine control unit
 *    Designed by Alexey A. Shabelnikov. Ukraine, Gorlovka 2007.
 *       Microprocessors systems - design & programming.
 *    contacts:
 *              http://secu-3.narod.ru
 *              ICQ: 405-791-931
 ****************************************************************/

#include "ignlogic.h"
#include "funconv.h"
#include "secu3.h"

void advance_angle_state_machine(int16_t* padvance_angle_inhibitor_state, struct ecudata_t* d)
{
 switch(d->engine_mode)
 {
  case EM_START: //����� �����
   if (d->sens.inst_frq > d->param.smap_abandon)
   {                   
    d->engine_mode = EM_IDLE;    
    idling_regulator_init();    
   }      
   d->curr_angle=start_function(d);               //������� ��� - ������� ��� �����
   d->airflow = 0;                                //� ������ ����� ��� �������
   *padvance_angle_inhibitor_state = d->curr_angle;//� ������ ����� ������ ��������
   break;     
              
  case EM_IDLE: //����� ��������� ����
   if (d->sens.carb)//������ ���� ������ - � ������� �����
   {
    d->engine_mode = EM_WORK;
   }             
   work_function(d, 1);                           //��������� �������� ������� ������� 
   d->curr_angle = idling_function(d);            //������� ��� - ������� ��� �� 
   d->curr_angle+=coolant_function(d);            //��������� � ��� ������������� ���������
   d->curr_angle+=idling_pregulator(d,&idle_period_time_counter);//��������� �����������
   break;            
                                             
  case EM_WORK: //������� ����� 
   if (!d->sens.carb)//������ ���� ��������� - � ���������� ����� ��
   {
    d->engine_mode = EM_IDLE;
    idling_regulator_init();    
   }
   d->curr_angle=work_function(d, 0);           //������� ��� - ������� �������� ������
   d->curr_angle+=coolant_function(d);          //��������� � ��� ������������� ���������  
   //�������� �������� ���������� �� ���������� �� ���������
   d->curr_angle-=d->knock_retard;       
   break;     
       
  default:  //���������� �������� - ���� � ����       
   d->curr_angle = 0;
   break;     
 }
}
