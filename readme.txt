
    SECU-3 Application software. Distributed under GPL license

    Designed by Alexey A. Shabelnikov 2007. Ukraine, Kiev.
    Microprocessor systems - design & programming.
    http://secu-3.org e-mail: shabelnikov@secu-3.org


    How to compile the project
    ��� ������������� ������

    It is possible to compile the project for ATMega16, ATMega32, ATMega64,
ATMega644. Version for ATMega64 compiles, but it will not work! You can compile
the project using either IAR(MS Windows) or GCC(Linux, MS Windows).
    Under MS Windows: Run configure.bat with corresponding options (type of MCU
                      and type of compiler),it will create Makefile and start
                      building.
    Under Linux:      Run configure.sh with option - type of MCU, it will create
                      Makefile and start building.

    ������ ����� �������������� ��� ATMega16, ATMega32, ATMega64. ��� ATMega64
��� �������������, �� �������� �� �� �����! �� ������ ������������� ������
��������� IAR ��� GCC. ��������� configure.bat c ���������������� ������� (���
���������������� � ��� �����������), ����� ������ Makefile � �������� ������
�������. ���� ����������� ������ ��������� ����� ����������. ���������� ��������
����� �������� ��� ��������� ������ � ������� ��.

    List of symbols which affects compilation:
    ������ �������� ����������� �����������:

    VPSEM                For using of starter blocking output for indication of
                         idle cut off valve's state
                         ��� ��������� ��������� ������� ���� ������������ �����
                         ���������� ��������

    DWELL_CONTROL        For direct controlling of dwell
                         ��� ������� ���������� ����������� ������� � ��������
                         ���������


    COOLINGFAN_PWM       Use PWM for controlling of electric cooling fan
                         ������������ ��� ��� ��� ��� ���������� ���������
                         �����������

    REALTIME_TABLES      Allow editing of tables in realtime (use RAM)
                         ��������� �������������� ������ � �������� �������

    DEBUG_VARIABLES      For watching and editing of some firmware variables
                         (used for debug by developers)
                         ��������� ����� ������� ����������� ����������� �
                         ������ ��������� ���������� ��������

    PHASE_SENSOR         Use of phase (cam) sensor
                         (��������� ������������� ������� ���)


    PHASED_IGNITION      Use phased ignition. PHASE_SENSOR must be also used.
                         (��������� ������������ ���������)

    FUEL_PUMP            Electric fuel pump control
                         (���������� �������������������)


    THERMISTOR_CS        Use a resistive temperature sensor
                         (������������ ������ ����������� ����������� ��������
                         ������������ ����)

    REV9_BOARD           Build for SECU-3T boards of revision 9 and greater.
                         (������ ��� ���� SECU-3T ������� 9 � ����)

    DIAGNOSTICS          Include hardware diagnostics functionality
                         (�������� ��������� ����������� ���������� �����)

    HALL_OUTPUT          Include Hall sensor emulation functionality. Separate
                         output will be used.
                         (�������� ��������� �������� ������� � ������� �����)

    STROBOSCOPE          Include stroboscope functionality
                         (�������� ��������� �����������)

    SM_CONTROL           Enable stepper motor and choke control functionality
                         (�������� ���������������� �� ���������� �������
                         ���������� � ��������� ���������)

    VREF_5V              Use 5V ADC reference voltage. In this case divider
                         bottom resistors are not necessary. So, input impedance
                         will be high.
                         (������������ ������� ���������� ��� ��� 5�)

    HALL_SYNC            Use synchronization from Hall sensor (connected to PS
                         input) instead of CKP sensor
                         ������������ ������������� �� �� ������ ����

    CKPS_2CHIGN          Build firmware for use 2 channel igniters (driven by
                         both edges)
                         ������� �������� � ���������� 2-� ���������
                         ������������ (����������� 2-�� ��������)

    UART_BINARY          Use binary mode for UART instead of ASCII
                         ������������ �������� ����� ��� �������� ������ �����
                         UART ������ ASCII

    FUEL_INJECT          Include support of fuel injection
                         �������� ��������� ���������� �������� �������

    BL_BAUD_RATE   *     Baud rate for boot loader. Can be set to 9600, 14400,
                         19200, 28800, 38400, 57600. Note! Will not take effect
                         without reprogramming using ISP programmator.
                         (�������� �������� ������ ��� ����������)

    SPEED_SENSOR   *     Include speed sensor support
                         �������� ��������� ������� ��������

    INTK_HEATING   *     Include support of intake manifold heating control
                         �������� ��������� ���������� ���������� ���������
                         ����������

    AIRTEMP_SENS   *     Include support of intake air temperature sensor
                         �������� ��������� ������� ����������� �������

    BLUETOOTH_SUPP *     Include functionality for working with Bluetooth
                         �������� ��������� ������ � Bluetooth

    IMMOBILIZER    *     Include immobilizer and iButton functionality
                         �������� ��������� ������������� � iButton

    UNI_OUTPUT     *     Include support of an universal programmable output
                         �������� ��������� �������������� ����������������
                         ������

* means that option is internal and not displayed in the list of options in the
  SECU-3 Manager
  �������� ��� ����� �������� ���������� � �� ������������ � ������ ����� �
  SECU-3 Manager

Necessary symbols you can define in the preprocessor's options of the compiler
(edit corresponding Makefile).
������ ��� ������� �� ������ ���������� � ������ ������������� ����������� 
(������������ ��������������� Makefile).
