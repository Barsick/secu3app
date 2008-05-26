
#define ADC_VREF_TYPE           0xC0

//������ ������������ ������� ���
#define ADCI_MAP                2
#define ADCI_UBAT               1         
#define ADCI_TEMP               0

//������ ������� ���������� �� ������� �������
#define MAP_AVERAGING           4   
#define BAT_AVERAGING           4   
#define TMP_AVERAGING           8  


//��� ������� ���������� ������� �������� �� ������� ����������
unsigned int adc_get_map_value(unsigned char index);
unsigned int adc_get_ubat_value(unsigned char index);
unsigned int adc_get_temp_value(unsigned char index);

//��������� ��������� �������� � ��������, �� ������ ���� ����������  
//��������� ���������.
void adc_begin_measure(void);

//���������� �� 0 ���� ��������� ������ (��� �� ������)
char adc_is_measure_ready(void); 

//������������� ��� � ��� ���������� ���������
void adc_init(void);
