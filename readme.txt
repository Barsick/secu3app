
    SECU-3 Application software. Distributed under GPL license
    
    Designed by Alexey A. Shabelnikov 2007. Ukraine, Gorlovka.
    Microprocessors systems - design & programming.
    http://secu-3.org e-mail: shabelnikov@secu-3.org


      How to compile the project
      ��� ������������� ������

    It is possible to compile project for ATMega16, ATMega32, ATMega64. Version for ATMega64 compiles,
but it will not work! You can compile the project using either IAR or WinAvr. Run configure.bat with 
corresponding options (type of MCU and type of compiler), it will create Makefile and start building.
    ������ ����� �������������� ��� ATMega16, ATMega32, ATMega64. ��� ATMega64 ��� �������������, ��
�������� �� �� �����! �� ������ ������������� ������ ��������� IAR ��� WinAvr. ��������� configure.bat
c ���������������� ������� (��� ���������������� � ��� �����������), ����� ������ Makefile � ��������
������ �������.

    List of symbols which affects compilation:
    ������ �������� ����������� �����������:

    VPSEM - for using of starter blocking output for indication of idle economizer valve's state
            ��� ��������� ��������� ������� ���� ������������ ����� ���������� ��������


    WHEEL_36_1 - for using 36-1 crank (60-2 by default)
                 ��� ������������� ��������� ����� 36-1 (�� ��������� 60-2)


    INVERSE_IGN_OUTPUTS - use for to invert ignition outputs
                          ��� �������������� ������� ���������� ����������


    COIL_REGULATION - for direct controlling of coil regulation
                      ��� ������� ���������� ����������� ������� � �������� ���������


    COOLINGFAN_PWM - use PWM for controlling of electric cooling fan
                     ������������ ��� ��� ��� ��� ���������� ��������� �����������

    REALTIME_TABLES - allow editing of tables in realtime (use RAM)


Necessary symbols you can define in the preprocessor's options of compiler
(edit corresponding Makefile).
������ ��� ������� �� ������ ���������� � ������ ������������� ����������� 
(������������ ��������������� Makefile).
