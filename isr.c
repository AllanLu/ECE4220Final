
#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>		// for the integer types
#include <wiringPi.h>
#include <wiringPiSPI.h>

void button123(){

    printf("123\n");
}
int main(int argc, char *argv[]){
    wiringPiSetup();
    
    pinMode(27,INPUT);
    pinMode(0,INPUT);
    pinMode(1,INPUT);
    pinMode(26,INPUT);//24
    pinMode(23,INPUT);//28

    pullUpDnControl(27,PUD_DOWN);
    pullUpDnControl(0,PUD_DOWN);
    pullUpDnControl(1,PUD_DOWN);
    pullUpDnControl(26,PUD_DOWN);
    pullUpDnControl(23,PUD_DOWN);
    
    if(0>wiringPiISR(27,INT_EDGE_FALLING,button123))
    {
        printf("interrupt function register failure");
        exit(-1);
    }

    while(1){
        printf("button1 %d button2 %d button3 %d button4 %d button5 %d \n",digitalRead(27),digitalRead(0),digitalRead(1),digitalRead(26),digitalRead(23));                
        if(digitalRead(27)==LOW)
        printf("button press\n");
        delay(2);
        sleep(2);
    }
    return 0;
}