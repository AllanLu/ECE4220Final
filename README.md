## Things to be done ##

1.Test

2.Add ADC check


## File description ##

Final.cpp is the main program

Final_server is a tcp server

#### Files below is additional ####

Final_k.c is for kernel space to detect switch(button) and control LEDs

Final_u.c is for user space to receive message from workstation to control LEDs and read log

Final_WS is Lab6_client_WS used in class

isr.c is to test the board whether the button is good(I found a broken board on Monday)

## About circuit ## 

R

22k,100k

C

0.01uf,1uf

## time stamp ##

Fri Dec  1 23:46:59 2017

Only use "23:46:59"

## Example ##

23:46:59 RTU1  (time and RTU number)

S1:ON S2:OFF B:OFF LED1:ON LED2:ON LED3:OFF  (status)

Voltage:1.5V  (voltage)

LED1 change  (a list of events)

S1 change

Voltage overload



