#include <iostream>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>		// for the integer types
#include <wiringPi.h>
#include <wiringPiSPI.h>

#include<sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>   
#include <net/if.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>  

using namespace std;

#define MSG_SIZE 100;

class RTUlog{
    public unsigned int time_stamp;
    public char[24] time_stamp_c;
    public int RTU_num;
    public int switch1;
    public int switch2;
    public int button;
    public int LED1;
    public int LED2;
    public int LED3;
    public int ADC;
    public int change[6];
    public char log[MSG_SIZE];
    public string string_log;
    char s[1];
    char ss[2];
    char sss[3];
    time_t timep;
    
    

    public RTUlog(){
        switch1=0;
        switch2=0;
        button=0;
        RTU_num=1;
        LED1=0;
        LED2=0;
        LED3=0;
        ADC=0；
        change[0]=0；
        change[1]=0；
        change[2]=0；
        change[3]=0；
        change[4]=0；
        change[5]=0；
    }
    public string check_status(int i){
        if(i)
        return "ON";
        else
        return "OFF";
    }
    public string check_change(){
        string temp="";
        if(change[0])
        temp+="S1 Change ";
        if(change[1])
        temp+="S2 Change ";
        if(change[2])
        temp+="B Change ";
        if(change[3])
        temp+="L1 Change ";
        if(change[4])
        temp+="L2 Change ";
        if(change[5])
        temp+="L3 Change ";        
        return temp;
    }
    public char[] print_log(){
            bzero(log,MSG_SIZE);
            time(&timep);   
            printf("%s\n", ctime(&timep));
            string_log=ctime(&timep);
            string_log+="RTU #: 1";
            string_log+=" S1";
            string_log+=check_status(switch1);
            string_log+=" S2";
            string_log+=check_status(switch2);
            string_log+=" B";
            string_log+=check_status(button);
            string_log+=" L1";
            string_log+=check_status(LED1);
            string_log+=" L2";
            string_log+=check_status(LED2);
            string_log+=" L3";
            string_log+=check_status(LED3);
            string_log+=" Voltage: ";
            string_log+=to_string(ADC);
            string_log+=" Event: ";
            string_log+=check_change();
            for(int i=0;i<string_log.length();i++)
            log[i]=string_log[i];
            log[i]='\0';

            return log;
    }
}

int current=0;
unsigned int current_time;
RTUlog rtulog[1000];
struct timeval tv;
char buffer[MSG_SIZE]; //buffer
int log_num;

//socket defination
int sock, n;
struct sockaddr_in server, any;
socklen_t fromlen,length;
int status=0;
int boolval = 1;			// for a socket option

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

    //port
    if (argc != 2)
   {
	   printf("usage: %s port\n", argv[0]);
       exit(1);
   }
    
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


    sock = socket(AF_INET, SOCK_DGRAM, 0); // Creates socket
    if (sock < 0)
    error("socket");

    length = sizeof(server);		// size of structure
    fromlen = sizeof(any);
    bzero(&server,length);          //set to 0
    server.sin_family = AF_INET;		// symbol constant for Internet domain
    server.sin_port = htons(atoi(argv[1]));				// port field
    server.sin_addr.s_addr = INADDR_ANY;	// IP address

    if(bind(sock,(struct sockaddr *)&server,length)<0){      //bind
        printf("binding error");
        exit(-1);
    }

    // change socket permissions to allow broadcast
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &boolval, sizeof(boolval)) < 0)
    {
        printf("error setting socket options\n");
        exit(-1);
    }
    //main receive massage from socket
    //thread log information

    gettimeofday(&tv, NULL);
    current_time=1000*tv.tv_sec+tv.tv_usec/1000;

    pthread_t ptr1;
    int t1=pthread_create(&ptr1,NULL,(void *)&Thread_log,NULL);

    while(1){
        n = recvfrom(sock, buffer, MSG_SIZE, 0, (struct sockaddr *)&any, &fromlen);     //receive message
        if (n < 0)
            error("recvfrom");
 
        printf("%d: Received a datagram: %s\n",counter, buffer);
        //check ONLED
        if(strstr(buffer,"ONLED1")!=0){
            digitalWrite(8,1);
        }

        //check ONLED
        if(strstr(buffer,"ONLED2")!=0){
            digitalWrite(9,1);
        }

        //check ONLED
        if(strstr(buffer,"ONLED3")!=0){
            digitalWrite(7,1);
        }

        //check OFFLED
        if(strstr(buffer,"OFFLED1")!=0){
            digitalWrite(8,0);
        }

        //check OFFLED
        if(strstr(buffer,"OFFLED2")!=0){
            digitalWrite(9,0);
        }

        //check OFFLED
        if(strstr(buffer,"OFFLED3")!=0){
            digitalWrite(7,0);
        }

        //check LOG
        if(buffer[0]=='L' && buffer[1]=='O' && buffer[2]=='G'){
            if(buffer[3]=='0'){
                //print all log
            }
            else{
                log_num=atoi(buffer[3]);
                bzero(buffer,MSG_SIZE);
                strcpy(buffer,"time stamp");
            strcat(buffer,ip);
            strcat(buffer," is the master");
            sendto(sock,buffer,MSG_SIZE,0,(const struct sockaddr *)&any,fromlen);
            bzero(buffer,MSG_SIZE);


            }
        }



    }
  
   close(sock);						// close socket.




    return 0;
}
