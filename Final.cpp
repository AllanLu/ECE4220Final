#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>		// for the integer types
#include <wiringPi.h>
#include <wiringPiSPI.h>

using namespace std;

class RTUlog{
    public unsigned int time_stamp;
    public int RTU_num;
    public int switch1;
    public int switch2;
    public int button;
    public int ADC;
    public int change[3];

    public RTUlog(){
        switch1=0;
        switch2=0;
        button=0;
        RTU_num=1;
        ADC=0；
        change[0]=0；
        change[1]=0；
        change[2]=0；
    }
}

int current=0;
RTUlog rtulog[1000];

void switch1(){
    rtulog[current].change[0]=1;
}
void switch2(){
    rtulog[current].change[1]=1;
}
void button(){
    rtulog[current].change[2]=1;
}

int main(int argc, char *argv[])
{
    if(wiringPiSetup()<0)
    return -1;
    
    //LED
    pinMode(8,OUTPUT);//red
    pinMode(9,OUTPUT);//yellow
    pinMode(7,OUTPUT);//green

    //switch
    pinMode(26,INPUT);//switch1
    pinMode(23,INPUT);//switch2
    pinMode(27,INPUT);//button1
    
    //turn off LED
    digitalWrite(8,0);
    digitalWrite(9,0);
    digitalWrite(7,0);

    pullUpDnControl(26,PUD_UP);
    pullUpDnControl(23,PUD_UP);
    pullUpDnControl(27,PUD_UP);

    if(wiringPiISR(26,INT_EDGE_RISING,&switch1) < 0)
    printf("Unable to setup ISR on switch1 \n");
    if(wiringPiISR(23,INT_EDGE_RISING,&switch2) < 0)
    printf("Unable to setup ISR on switch2 \n");
    if(wiringPiISR(27,INT_EDGE_RISING,&button) < 0)
    printf("Unable to setup ISR on button \n");

    //main receive massage from socket
    //thread log information





    return 0;
}
