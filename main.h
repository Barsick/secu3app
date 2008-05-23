
#define SETBIT(x,y)   (x |= (1<<y))    /* ��������� ���� y � ����� x*/
#define CLEARBIT(x,y) (x &= (~(1<<y))) /* ����� ���� y � ����� x*/
#define CHECKBIT(x,y) (x & (1<<y))     /* �������� ���� y � ����� x*/

#define ADC_DISCRETE       0.0025       //���� �������� ��� � �������

#define TSENS_SLOPP        0.01        //������ ������ ������� ����������� �����/������
#define TSENS_ZERO_POINT   2.73        //���������� �� ������ ������� ����������� ��� 0 �������� �������

#define EEPROM_PARAM_START 0x002       //����� ��������� ���������� � EEPROM 

#define LOW_ENDIAN_DATA_FORMAT        //Intel data format

#define SND_TIMECONST     250
#define PAR_SAVE_COUNTER  254


#define PAR_CRC_SIZE   sizeof(unsigned short) //������ ���������� ����������� ����� ���������� � ������
#define CODE_CRC_SIZE   sizeof(unsigned short) //������ ���������� ����������� ����� �������� � ������

//������ ���� ��������� ��� ����� ����������� �����
#define CODE_SIZE (BOOT_START-CODE_CRC_SIZE)


//���� ���������� ��� ���������� � ���������, ��������������� ������� ������� ����� ������ ������� ��������, �� 
//�������� �������� ���������� ���� ������ ���������� ��� ����� ������������� ������������ ���� (������� ������ 
//����������� �������� �� ����� ������).
#define TEETH_BEFORE_UP         20                                      //���������� ������ ����� ������ �� �.�.� (18...22)
#define SPEED_MEASURE_TEETH     10                                       //���-�� ������ ��� ��������� �������� �������� ���������
#define ANGLE_MULTIPLAYER       40                                      //����������� ��������������� ����� �������� ���������  
#define DEGREES_PER_TEETH       6                                       //���������� �������� ������������ �� ���� ��� �����
#define TEETH_BACK_SYNC         55                                      //��� ����� �������� �� ����� ��������� � ��������� ������ �����������
//��� ����������� ������������ ������������ �������� ������� ������ ���� 1/3, ��� ������������ ���������� � ������� ������� 
//�������� ����� ����������� �� �����. 
#define IGNITION_TEETH          10                                      //������������ �������� ��������� (� ������ �����)


#define IGNITION_PULSE_14       0x10                                    //����� ��� ������ �������� ��������� �� 1-4 ��������
#define IGNITION_PULSE_23       0x20                                    //����� ��� ������ �������� ��������� �� 2-3 ��������
#define IGNITION_PULSE_OFF      0xCF                                    //����� �������� ������ ��������� ��� ���� ��� ��������� ������������


#define ADC_VREF_TYPE           0xC0
//������ ������������ ������� ���
#define ADCI_MAP                2
#define ADCI_UBAT               1         
#define ADCI_TEMP               0
//���-�� �������� ��� ���������� ���������� ������� 
#define FRQ_AVERAGING           16                                          
#define MAP_AVERAGING           4   
#define BAT_AVERAGING           6   
#define TMP_AVERAGING           8  

#define T2_RELOAD_VALUE         100


//���������� ���������� ����� ������������ ��� ������ �������
#define F_WRK_POINTS_F         16
#define F_WRK_POINTS_L         16
#define F_TMP_POINTS_T         12
#define F_TMP_POINTS_L         8
#define F_STR_POINTS           16                            
#define F_IDL_POINTS           16     

#define F_NAME_SIZE            16

//��������� ����������� �� �������� ������� � �������� ���
#define T_TO_DADC(Tc) ((unsigned int)((TSENS_ZERO_POINT + (Tc*TSENS_SLOPP))/ADC_DISCRETE)) 


#define TSCALE_LO_VALUE     T_TO_DADC(-16)                      //-16 �������� ����� ������ ����� ����� ����������� (� �������� �������)
#define TSCALE_STEP      ((unsigned int)((11.0*TSENS_SLOPP)/ADC_DISCRETE)) // 11 �������� ����� ������ ������������ �� �������������� ��� (� ��������� ���)

#define TABLES_NUMBER          8                                      //���������� ������� ������������� �������� � ������ ��������


#ifndef _MAIN_
#define  _MAIN_

//��������� ���� ��������� �������������, �������� ��� = 0.5 ���� 
typedef struct 
{
  signed   char f_str[F_STR_POINTS];                       // ������� ��� �� ������
  signed   char f_idl[F_IDL_POINTS];                       // ������� ��� ��� ��
  signed   char f_wrk[F_WRK_POINTS_L][F_WRK_POINTS_F];     // �������� ������� ���
  signed   char f_tmp[F_TMP_POINTS_L][F_TMP_POINTS_T];     // ������� �������. ��� �� �����������
  unsigned char name[F_NAME_SIZE];                         // ��������������� ��� (��� ���������)
}F_data;


//��������� ��� ����� ������� - �� ����������� � ������������ ��������
typedef struct
{
 unsigned int map;                                          //�������� �� �������� ���������� (�����������)
 unsigned int voltage;                                      //���������� �������� ���� (�����������)
 unsigned int temperat;                                     //����������� ����������� �������� (�����������)
 unsigned int frequen;                                      //������� �������� ��������� (�����������)
 unsigned int inst_frq;                                     //���������� ������� ��������
 unsigned char carb;                                        //��������� ��������� ����������� 
 unsigned char gas;                                         //��������� �������� ������� 
}sensors;

//��������� ��������� �������
typedef struct
{
  unsigned char tmp_use;                                    //������� ������������ ����-��
  unsigned char carb_invers;                                //�������� ��������� �� �����������
  unsigned char idl_regul;                                  //������������ �������� ������� �� �������������� ���
  unsigned char fn_benzin;                                  //����� ������ ������������� ������������ ��� �������
  unsigned char fn_gas;                                     //����� ������ ������������� ������������ ��� ����
  unsigned char map_grad;                                   //������ ����� ������� �������
  unsigned int  ephh_lot;                                   //������ ����� ����
  unsigned int  ephh_hit;                                   //������� ����� ����
  unsigned int  starter_off;                                //����� ���������� �������� (�������)
  signed   int  press_swing;                                //������� �������� ��� ��������� �������� ��������   
  unsigned int  smap_abandon;                               //������� �������� � �������� ����� �� �������   
  signed   int  max_angle;                                  //����������� ������������� ���
  signed   int  min_angle;                                  //����������� ������������ ���
  signed   int  angle_corr;                                 //�����-��������� ���    
  unsigned int  idl_turns;                                  //�������� ������� �� ��� ����������� �������������� ���   
  signed   int  ifac1;                                      //������������ �-���������� �������� ��, ��� ������������� �
  signed   int  ifac2;                                      //������������� ������ ��������������, 1...100 
  signed   int  MINEFR;                                     //���� ������������������ ���������� (�������)
  signed   int  vent_on;                                    //����������� ��������� �����������
  signed   int  vent_off;                                   //����������� ���������� �����������  
  unsigned short crc;                                       //����������� ����� ������ ���� ��������� (��� �������� ������������ ������ ����� ���������� �� EEPROM)  
}params;

//��� ��c�������� 't' (������������� ������ � ������ UART-a)
typedef struct
{
  unsigned char tmp_use;
  signed   int  vent_on;                                    
  signed   int  vent_off;                                   
}ud_t;

//��� ��c�������� 'c' (������������� ������ � ������ UART-a)
typedef struct
{
  unsigned int  ephh_lot;                                   
  unsigned int  ephh_hit;                                   
  unsigned char carb_invers;                                
}ud_c;


//��� ��c�������� 'r' (������������� ������ � ������ UART-a)
typedef struct
{
  unsigned char idl_regul;                                  
  signed   int  ifac1;                                      
  signed   int  ifac2;                                      
  signed   int  MINEFR;      //����� signed??? (�������� ���� ������� unsigned)                              
  unsigned int  idl_turns;                                  
}ud_r;


//��� ��c�������� 'a' (������������� ������ � ������ UART-a)
typedef struct
{
  signed   int  max_angle;                                  
  signed   int  min_angle;                                  
  signed   int  angle_corr;                                 
}ud_a;

//��� ��c�������� 'm' (������������� ������ � ������ UART-a)
typedef struct
{
  unsigned char fn_benzin;                
  unsigned char fn_gas;            
  unsigned char map_grad;         
  signed   int  press_swing;   //����� signed??? (�������� ���� ������� unsigned)
}ud_m;

//��� ��c�������� 'p' (������������� ������ � ������ UART-a)
typedef struct
{
   unsigned int  starter_off;       
   unsigned int  smap_abandon;             
}ud_p;

//��� ��c�������� 'n' (������������� ������ � ������ UART-a)
typedef struct
{
   char snd_mode;
}ud_n;


//��� ��c�������� 'f' (������������� ������ � ������ UART-a)
typedef struct
{
  unsigned char tables_num;                                  //���������� �������� �������������
  unsigned char index;                                       //����� ������
  char          name[F_NAME_SIZE];                           //��������������� ��� (��� ���������)  
}ud_f;

//��������� ������ �������, ������������ ������ ��������� ������
typedef struct
{
 params           param;                                      //--���������
 sensors          sens;                                       //--�������
 unsigned char    ephh_valve;                                 //��������� ������� ����
 int              atmos_press;                                //����������� ��������
 unsigned char    airflow;                                    //������ �������
 signed int       curr_angle;                                 //������� ���� ����������
 __flash F_data*  fn_dat;                                     //��������� �� ����� �������������
}ecudata;



//��� ��c�������� 's' (������������� ������ � ������ UART-a)
typedef struct 
{
  sensors sens;
  unsigned char    ephh_valve;                                 
  unsigned char    airflow;                                    
  signed int       curr_angle;                                 
}ud_s;



#ifdef LOW_ENDIAN_DATA_FORMAT  //low endian data store format (Intel)
#define GETBYTE(src,rel) *(((unsigned char*)&src+rel))
#define SETBYTE(des,rel) *(((unsigned char*)&des+rel))
#else                          //big endian data store format (Motorola) 
#define GETBYTE(src,rel) *(((unsigned char*)&src+sizeof(src)-1-rel))
#define SETBYTE(des,rel) *(((unsigned char*)&des+sizeof(des)-1-rel))
#endif  

typedef struct
{
 unsigned char   t0mode:1;                                              //0 - �������, 1 - ������
 unsigned char released:1;                                              //���� ���������� ������ �������� ������� ���������
 unsigned char    t1ovf:1;                                              //���� ������������ ������� 1
 unsigned char  rotsync:1;                                              //���� ������������� � ���������
 unsigned char  snd_busy:1;                                             //���� ������������� ����������� (�����)
 unsigned char  rcv_busy:1;                                             //���� ������������� ��������� (�����)
 unsigned char  sens_ready:1;                                           //������� ���������� � �������� ������ � ����������           
}bitfield1;

#endif

//�������� ����������� ���������� � ����� ���������  � ��������� � ��������� ��������� �����/������
__no_init volatile bitfield1 f1@0x22;
__no_init __regvar unsigned char TCNT0_H@15;                            //��� ���������� �������/�������� 0 �� 16 ��������

