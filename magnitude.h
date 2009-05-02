
#ifndef _MAGNITUDE_H_
#define _MAGNITUDE_H_

//��������� ��� ���������� ��� �������������� �� ����� � ��������� �������
//� ����� ����� 
#define ROUND(x) ((int16_t)( (x) + 0.5 - ((x) < 0) ))

//������ ������� ���������� ��� �������������� �����-��������� � ��������� �������
//� ����� �����. �������� ���������� ������� �������� � ����� ������.
#define ANGLE_MAGNITUDE(a) ROUND ((a) * ANGLE_MULTIPLAYER)
#define TEMPERATURE_MAGNITUDE(t) ROUND ((t) * TEMP_PHYSICAL_MAGNITUDE_MULTIPLAYER)
#define VOLTAGE_MAGNITUDE(t) ROUND ((t) * UBAT_PHYSICAL_MAGNITUDE_MULTIPLAYER)
#define PRESSURE_MAGNITUDE(t) ROUND ((t) * MAP_PHYSICAL_MAGNITUDE_MULTIPLAYER)

#endif //_MAGNITUDE_H_
