
    SECU-3 Application software. Distributed under GPL license

    Designed by Alexey A. Shabelnikov 2007. Ukraine, Gorlovka.
    Microprocessors systems - design & programming.
    http://secu-3.org e-mail: shabelnikov@secu-3.org


    How to compile the project
    ��� ������������� ������

    It is possible to compile project for ATMega16, ATMega32, ATMega64. Version
for ATMega64 compiles,but it will not work! You can compile the project using 
either IAR(MS Windows) or GCC(Linux, MS Windows).
    Under MS Windows: Run configure.bat with corresponding options (type of MCU
                      and type of compiler),it will create Makefile and start 
                      building.
    Under Linux:      Run configure.sh with option - type of MCU, it will create
                      Makefile and start building.

    ������ ����� �������������� ��� ATMega16, ATMega32, ATMega64. ��� ATMega64 
��� �������������, �� �������� �� �� �����! �� ������ ������������� ������ 
��������� IAR ��� GCC. ��������� configure.bat c ���������������� ������� (��� 
���������������� � ��� �����������), ����� ������ Makefile � �������� ������ 
�������.

    List of symbols which affects compilation:
    ������ �������� ����������� �����������:

    VPSEM                for using of starter blocking output for indication of 
                         idle economizer valve's state
                         ��� ��������� ��������� ������� ���� ������������ �����
                         ���������� ��������


    WHEEL_36_1           for using 36-1 crank (60-2 used by default)
                         ��� ������������� ��������� ����� 36-1 (�� ��������� 
                         60-2)


    INVERSE_IGN_OUTPUTS  use for to invert ignition outputs
                         ��� �������������� ������� ���������� ����������


    DWELL_CONTROL        for direct controlling of dwell
                         ��� ������� ���������� ����������� ������� � �������� 
                         ���������


    COOLINGFAN_PWM       use PWM for controlling of electric cooling fan
                         ������������ ��� ��� ��� ��� ���������� ��������� 
                         �����������

    REALTIME_TABLES      allow editing of tables in realtime (use RAM)
                         ��������� �������������� ������ � �������� �������

    DEBUG_VARIABLES      for watching and editing of some firmware variables 
                         (used for debug by developers)
                         ��������� ����� ������� ����������� ����������� � 
                         ������ ��������� ���������� ��������


Necessary symbols you can define in the preprocessor's options of compiler
(edit corresponding Makefile).
������ ��� ������� �� ������ ���������� � ������ ������������� ����������� 
(������������ ��������������� Makefile).
