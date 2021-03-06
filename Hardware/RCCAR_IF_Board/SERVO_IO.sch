EESchema Schematic File Version 2
LIBS:RCCAR_IF_Board-rescue
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:crf_1
LIBS:dips-s
LIBS:RCCAR_IF_Board-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 6 8
Title "Servo IO"
Date "2016-02-11"
Rev "A"
Comp "SP"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L CONN_01X03 P11
U 1 1 56C05596
P 5350 3650
F 0 "P11" H 5350 3850 50  0000 C CNN
F 1 "OUT_1" V 5450 3650 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Straight_1x03" H 5350 3650 60  0001 C CNN
F 3 "" H 5350 3650 60  0000 C CNN
	1    5350 3650
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X03 P12
U 1 1 56C0560F
P 5350 4400
F 0 "P12" H 5350 4600 50  0000 C CNN
F 1 "OUT_2" V 5450 4400 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Straight_1x03" H 5350 4400 60  0001 C CNN
F 3 "" H 5350 4400 60  0000 C CNN
	1    5350 4400
	1    0    0    -1  
$EndComp
Text HLabel 4750 3550 0    60   Input ~ 0
OUT_1
Text HLabel 4750 4300 0    60   Input ~ 0
OUT_2
Wire Wire Line
	3700 3650 5150 3650
Wire Wire Line
	4250 3650 4250 4400
Wire Wire Line
	4250 4400 5150 4400
Wire Wire Line
	5150 4500 4150 4500
Wire Wire Line
	4150 4500 4150 3750
Wire Wire Line
	3700 3750 5150 3750
$Comp
L CONN_01X02 P10
U 1 1 56C056C8
P 3500 3700
F 0 "P10" H 3500 3850 50  0000 C CNN
F 1 "SERVO_PWR" V 3600 3700 50  0000 C CNN
F 2 "w_conn_screw:mors_2p" H 3500 3700 60  0001 C CNN
F 3 "" H 3500 3700 60  0000 C CNN
	1    3500 3700
	-1   0    0    1   
$EndComp
Connection ~ 4250 3650
Connection ~ 4150 3750
$Comp
L R R16
U 1 1 56C057DB
P 4150 3200
F 0 "R16" V 4230 3200 50  0000 C CNN
F 1 "0R" V 4150 3200 50  0000 C CNN
F 2 "Resistors_SMD:R_0603" V 4080 3200 30  0001 C CNN
F 3 "" H 4150 3200 30  0000 C CNN
	1    4150 3200
	1    0    0    -1  
$EndComp
$Comp
L R R15
U 1 1 56C0582C
P 3950 3200
F 0 "R15" V 4030 3200 50  0000 C CNN
F 1 "0R" V 3950 3200 50  0000 C CNN
F 2 "Resistors_SMD:R_0603" V 3880 3200 30  0001 C CNN
F 3 "" H 3950 3200 30  0000 C CNN
	1    3950 3200
	1    0    0    -1  
$EndComp
Wire Wire Line
	4150 3350 4150 3650
Connection ~ 4150 3650
Wire Wire Line
	3950 3350 3950 3750
Connection ~ 3950 3750
$Comp
L GND #PWR094
U 1 1 56C058BF
P 3750 3150
F 0 "#PWR094" H 3750 2900 50  0001 C CNN
F 1 "GND" H 3750 3000 50  0000 C CNN
F 2 "" H 3750 3150 60  0000 C CNN
F 3 "" H 3750 3150 60  0000 C CNN
	1    3750 3150
	1    0    0    -1  
$EndComp
Wire Wire Line
	3750 3150 3750 2950
Wire Wire Line
	3750 2950 3950 2950
Wire Wire Line
	3950 2950 3950 3050
Wire Wire Line
	4150 3050 4150 2850
Text Notes 4600 3300 0    60   ~ 0
Bridge options\nfor internal supply
$Comp
L CONN_01X03 P13
U 1 1 56C059CA
P 7400 3650
F 0 "P13" H 7400 3850 50  0000 C CNN
F 1 "IN1" V 7500 3650 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Straight_1x03" H 7400 3650 60  0001 C CNN
F 3 "" H 7400 3650 60  0000 C CNN
	1    7400 3650
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X03 P14
U 1 1 56C05A47
P 7400 4400
F 0 "P14" H 7400 4600 50  0000 C CNN
F 1 "IN2" V 7500 4400 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Straight_1x03" H 7400 4400 60  0001 C CNN
F 3 "" H 7400 4400 60  0000 C CNN
	1    7400 4400
	1    0    0    -1  
$EndComp
$Comp
L R R17
U 1 1 56C05D27
P 6950 3550
F 0 "R17" V 6850 3550 50  0000 C CNN
F 1 "220R" V 6950 3550 50  0000 C CNN
F 2 "Resistors_SMD:R_0603" V 6880 3550 30  0001 C CNN
F 3 "" H 6950 3550 30  0000 C CNN
	1    6950 3550
	0    1    1    0   
$EndComp
$Comp
L R R18
U 1 1 56C05DC4
P 6950 4300
F 0 "R18" V 6850 4300 50  0000 C CNN
F 1 "220R" V 6950 4300 50  0000 C CNN
F 2 "Resistors_SMD:R_0603" V 6880 4300 30  0001 C CNN
F 3 "" H 6950 4300 30  0000 C CNN
	1    6950 4300
	0    1    1    0   
$EndComp
$Comp
L C C61
U 1 1 56C05E2B
P 6350 3750
F 0 "C61" H 6375 3850 50  0000 L CNN
F 1 "100n" H 6375 3650 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 6388 3600 30  0001 C CNN
F 3 "" H 6350 3750 60  0000 C CNN
	1    6350 3750
	1    0    0    -1  
$EndComp
Wire Wire Line
	7200 3650 6650 3650
Wire Wire Line
	6650 3400 6650 4400
Wire Wire Line
	6650 4400 7200 4400
Wire Wire Line
	7200 3750 6750 3750
Wire Wire Line
	6750 3750 6750 4750
Wire Wire Line
	6750 4500 7200 4500
$Comp
L C C62
U 1 1 56C06053
P 6350 4500
F 0 "C62" H 6375 4600 50  0000 L CNN
F 1 "100n" H 6375 4400 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 6388 4350 30  0001 C CNN
F 3 "" H 6350 4500 60  0000 C CNN
	1    6350 4500
	1    0    0    -1  
$EndComp
Wire Wire Line
	6200 4300 6800 4300
Wire Wire Line
	6350 4300 6350 4350
Wire Wire Line
	6200 3550 6800 3550
Wire Wire Line
	6350 3550 6350 3600
$Comp
L GND #PWR095
U 1 1 56C0610E
P 6350 4000
F 0 "#PWR095" H 6350 3750 50  0001 C CNN
F 1 "GND" H 6350 3850 50  0000 C CNN
F 2 "" H 6350 4000 60  0000 C CNN
F 3 "" H 6350 4000 60  0000 C CNN
	1    6350 4000
	1    0    0    -1  
$EndComp
Wire Wire Line
	6350 3900 6350 4000
$Comp
L GND #PWR096
U 1 1 56C06182
P 6350 4750
F 0 "#PWR096" H 6350 4500 50  0001 C CNN
F 1 "GND" H 6350 4600 50  0000 C CNN
F 2 "" H 6350 4750 60  0000 C CNN
F 3 "" H 6350 4750 60  0000 C CNN
	1    6350 4750
	1    0    0    -1  
$EndComp
Wire Wire Line
	6350 4650 6350 4750
Wire Wire Line
	7100 4300 7200 4300
Wire Wire Line
	7100 3550 7200 3550
Text HLabel 6200 3550 0    60   Input ~ 0
IN_1
Text HLabel 6200 4300 0    60   Input ~ 0
IN_2
Connection ~ 6350 4300
Connection ~ 6350 3550
$Comp
L GND #PWR097
U 1 1 56C06564
P 6750 4750
F 0 "#PWR097" H 6750 4500 50  0001 C CNN
F 1 "GND" H 6750 4600 50  0000 C CNN
F 2 "" H 6750 4750 60  0000 C CNN
F 3 "" H 6750 4750 60  0000 C CNN
	1    6750 4750
	1    0    0    -1  
$EndComp
Connection ~ 6750 4500
$Comp
L +5V #PWR098
U 1 1 56C0673F
P 4150 2850
F 0 "#PWR098" H 4150 2700 50  0001 C CNN
F 1 "+5V" H 4150 2990 50  0000 C CNN
F 2 "" H 4150 2850 60  0000 C CNN
F 3 "" H 4150 2850 60  0000 C CNN
	1    4150 2850
	1    0    0    -1  
$EndComp
$Comp
L VCC #PWR099
U 1 1 56C06AA8
P 6650 3400
F 0 "#PWR099" H 6650 3250 50  0001 C CNN
F 1 "VCC" H 6650 3550 50  0000 C CNN
F 2 "" H 6650 3400 60  0000 C CNN
F 3 "" H 6650 3400 60  0000 C CNN
	1    6650 3400
	1    0    0    -1  
$EndComp
Connection ~ 6650 3650
$Comp
L R R33
U 1 1 56C1A2B5
P 4950 3550
F 0 "R33" V 4850 3550 50  0000 C CNN
F 1 "220R" V 4950 3550 50  0000 C CNN
F 2 "Resistors_SMD:R_0603" V 4880 3550 30  0001 C CNN
F 3 "" H 4950 3550 30  0000 C CNN
	1    4950 3550
	0    1    1    0   
$EndComp
Wire Wire Line
	4750 3550 4800 3550
Wire Wire Line
	5100 3550 5150 3550
$Comp
L R R34
U 1 1 56C1A788
P 4950 4300
F 0 "R34" V 4850 4300 50  0000 C CNN
F 1 "220R" V 4950 4300 50  0000 C CNN
F 2 "Resistors_SMD:R_0603" V 4880 4300 30  0001 C CNN
F 3 "" H 4950 4300 30  0000 C CNN
	1    4950 4300
	0    1    1    0   
$EndComp
Wire Wire Line
	5100 4300 5150 4300
Wire Wire Line
	4750 4300 4800 4300
$EndSCHEMATC
