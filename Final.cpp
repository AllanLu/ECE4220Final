/* 	Name       : 	Final.cpp
	Author     : 	Yiwei Lu and Brent Schultez
	Description: 	ECE4220 Final Project */
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>	
#include <wiringPi.h>
#include <wiringPiSPI.h>

#include <sys/time.h>
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
#include <semaphore.h>

#define MSG_SIZE 50
#define SPI_CHANNEL	      0	// 0 or 1
#define SPI_SPEED 	2000000	// Max speed is 3.6 MHz when VDD = 5 V
#define ADC_CHANNEL       2	// Between 1 and 3
using namespace std;

sem_t my_semaphore;
uint16_t ADCvalue;
uint16_t get_ADC(int channel);
int pipe_c[2];
float voltage;


char buffer[MSG_SIZE]; //buffer used to receive message
char log_buffer[MSG_SIZE];//buffer used to send message

int status=0;
int sockfd, portno, n;
struct sockaddr_in serv_addr;
struct hostent *server;
char msg[MSG_SIZE];
pid_t p;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

//get ADC value
uint16_t get_ADC(int ADC_chan)
{
	uint8_t spiData[3];
	spiData[0] = 0b00000001; // Contains the Start Bit
	spiData[1] = 0b10000000 | (ADC_chan << 4);	// Mode and Channel: M0XX0000
												// M = 1 ==> single ended
									// XX: channel selection: 00, 01, 10 or 11
	spiData[2] = 0;	// "Don't care", doesn't matter the value.
	
	// The next function performs a simultaneous write/read transaction over the selected
	// SPI bus. Data that was in the spiData buffer is overwritten by data returned from
	// the SPI bus.
	wiringPiSPIDataRW(SPI_CHANNEL, spiData, 3);
	
	// spiData[1] and spiData[2] now have the result (2 bits and 8 bits, respectively)
	
	return ((spiData[1] << 8) | spiData[2]);
}

//RTU class
class RTUlog{
    public:
        RTUlog(){
        switch1=0;
        switch2=0;
        button=0;
        RTU_num=1;
        LED1=0;
        LED2=0;
        LED3=0;
        ADC=0;
        eventcount=0;
        for(int i=0;i<10;i++)
        eventlist[i]=0;

    }

    int RTU_num;//RTU number
    int switch1;//1 for ON and 0 for OFF
    int switch2;
    int button;
    int LED1;
    int LED2;
    int LED3;
    int ADC;
    float output_v;
    string string_log;//use to send message
    char log_temp[MSG_SIZE];//use to send message
    time_t timep;//record time
    int eventcount;//record event
    int eventlist[10];//0:s1 1:s2 2:b 3:L1 4:L2 5:L3 6:overload 7:no power
    char int2char[5];//use to convert from int to char[]
    
    //check status
    string check_status(int i){
        if(i==1){
        return "ON";}
        else  if(i==0){
        return "OFF";
        }
    }
    //check event
    string check_event(int i,string s){
        switch(eventlist[i]){
        case 0:s+="S1 Change ";break;
        case 1:s+="S2 Change ";break;
        case 2:s+="B Change ";break;
        case 3:s+="L1 Change ";break;
        case 4:s+="L2 Change ";break;
        case 5:s+="L3 Change ";break;
        case 6:s+="Overload ";break;
        case 7:s+="No power ";break;
        }       
        
        return s;
    }
    
    //print RTU number and time
    void print_log1(char bu[]){
            
        bzero(log_temp,MSG_SIZE);
        string_log="RTU1@";
        time(&timep);   
        string s_temp=ctime(&timep);
        for(int i=11;i<=18;i++)
        string_log+=s_temp[i];
        //convert from string to char[]
        for(int i=0;i<string_log.length();i++)
        log_temp[i]=string_log[i];
        log_temp[string_log.length()]='\0';
        strcpy(bu,log_temp);
    }
    //print switch and LED status
    void print_log2(char bu[]){
        bzero(log_temp,MSG_SIZE);
        string_log="";
        string_log+="S1:";
        string_log+=check_status(switch1);
        string_log+=" S2:";
        string_log+=check_status(switch2);
        string_log+=" B:";
        string_log+=check_status(button);
        string_log+=" L1:";
        string_log+=check_status(LED1);
        string_log+=" L2:";
        string_log+=check_status(LED2);
        string_log+=" L3:";
        string_log+=check_status(LED3);
        string_log+=" V:";
        
        bzero(int2char,5);
        printf("inside %.2f",output_v);
        sprintf(int2char,"%.2f",output_v);
        string_log+=int2char;
        string_log+='\n';
        
        //printf("after char r0 %d\n",eventcount);
        //convert from string to char[]
        for(int j=0;j<string_log.length();j++)
        log_temp[j]=string_log[j];
        //printf("before copy r0 %d\n",eventcount);
        log_temp[string_log.length()]='\0';
        strcpy(bu,log_temp);
        

    }
    //print event
    void print_log4(char bu[]){
        bzero(log_temp,MSG_SIZE);
        string_log="      Event: ";
        for(int i=0;i<eventcount;i++)
        //printf("inside function %d\n",eventcount);
        string_log=check_event(i,string_log);
        //convert from string to char[]
        for(int i=0;i<string_log.length();i++)
        log_temp[i]=string_log[i];
        log_temp[string_log.length()]='\0';
        strcpy(bu,log_temp);

    }

    void init(){
        switch1=0;
        switch2=0;
        button=0;
        RTU_num=1;
        LED1=0;
        LED2=0;
        LED3=0;
        ADC=0;
        eventcount=0;
        for(int i=0;i<10;i++)
        eventlist[i]=0;
    }
};
RTUlog rtulog[2];//0 for the past 1s, 1 for current 1s


//use ISR to detect
void switch1(){
    sem_wait(&my_semaphore);
    rtulog[1].eventlist[rtulog[1].eventcount]=0;
    rtulog[1].eventcount++;
    sem_post(&my_semaphore);
    cout<<"switch1 ISR"<<endl;
}
void switch2(){
    sem_wait(&my_semaphore);
    rtulog[1].eventlist[rtulog[1].eventcount]=1;
    rtulog[1].eventcount++;
    sem_post(&my_semaphore);
    cout<<"switch2 ISR"<<endl;
}
void button(){
    sem_wait(&my_semaphore);
    rtulog[1].eventlist[rtulog[1].eventcount]=2;
    rtulog[1].eventcount++;
    sem_post(&my_semaphore);
    cout<<"button ISR"<<endl;
}

//read ADC
void* Thread_ADC(void *arg){ 
        while(1){
        
        ADCvalue = get_ADC(ADC_CHANNEL);
        printf("ADC Value: %d\n", ADCvalue);
        fflush(stdout);


        sem_wait(&my_semaphore);
        rtulog[1].ADC=ADCvalue;


        rtulog[1].output_v=((3.300/1023)*ADCvalue)/2.0;
        int rand_num=rand();
        if(rand_num%9==0 || rand_num %7==0)
        rtulog[1].output_v+=1;
        //rtulog[1].output_v=(9.00/1023)*ADCvalue;
        printf("adc %.2f",rtulog[1].output_v);
        if(rtulog[1].output_v>2 ||(rtulog[1].output_v<1 && rtulog[1].output_v>0)){
        rtulog[1].eventlist[rtulog[1].eventcount]=6;
        rtulog[1].eventcount++;}
        sem_post(&my_semaphore);
		sleep(1);// delay 1s
        
        }
        pthread_exit(0);
    }


int main(int argc, char *argv[])
{
    
    if (argc < 3)	// not enough arguments
    {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }

    if(wiringPiSetup()<0)
    return -1;

	
	if(wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED) < 0) {
		printf("wiringPiSPISetup failed\n");
		return -1 ;
	}

	if(pipe(pipe_c) < 0)
	{
		printf("pipe creation error\n");
		exit(-1);
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

    //set switch and button
    pullUpDnControl(26,PUD_DOWN);
    pullUpDnControl(23,PUD_DOWN);
    pullUpDnControl(27,PUD_DOWN);

    //considering use both to detect rising and falling
    if(wiringPiISR(26,INT_EDGE_FALLING,&switch1) < 0)
    printf("Unable to setup ISR on switch1 \n");
    if(wiringPiISR(23,INT_EDGE_FALLING,&switch2) < 0)
    printf("Unable to setup ISR on switch2 \n");
    if(wiringPiISR(27,INT_EDGE_FALLING,&button) < 0)
    printf("Unable to setup ISR on button \n");

    //init semaphore
    sem_init(&my_semaphore, 0, 1);

    //these below are from client_tcp.c to init tcp
    portno = atoi(argv[2]);		// port # was an input.
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // Creates socket. Connection based.
    if (sockfd < 0)
        error("ERROR opening socket");

    server = gethostbyname(argv[1]);  // converts hostname input (e.g. 10.3.52.255)
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    // fill in fields of serv_addr
    bzero((char *) &serv_addr, sizeof(serv_addr));	// sets all values to zero
    serv_addr.sin_family = AF_INET;		// symbol constant for Internet domain

    // copy to serv_addr.sin_addr.s_addr. Function memcpy could be used instead.
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    serv_addr.sin_port = htons(portno);		// fill sin_port field

    // establish connection to the server
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");
    
    //use child process
    p=fork();

    pthread_t ptr1,ptr2;
    
    //father
    if(p != 0){
        int t1=pthread_create(&ptr1,NULL,Thread_ADC,NULL);
	    close(pipe_c[0]);//plan to use pipe to communicate between father and child process
        
        while(1){
        sem_wait(&my_semaphore);
        //switch status
        rtulog[1].switch1=digitalRead(26);
        rtulog[1].switch2=digitalRead(23);
        rtulog[1].button=digitalRead(27);
        //LED status
        rtulog[1].LED1=digitalRead(8);
        rtulog[1].LED2=digitalRead(9);
        rtulog[1].LED3=digitalRead(7);
        //copy from current to past
        rtulog[0].switch1=rtulog[1].switch1;
        rtulog[0].switch2=rtulog[1].switch2;
        rtulog[0].button=rtulog[1].button;
        
        //detect LED change because failing to use pipe
        if(rtulog[0].LED1!=rtulog[1].LED1){
            rtulog[1].eventlist[rtulog[1].eventcount]=3;
            rtulog[1].eventcount++;
        }
        if(rtulog[0].LED2!=rtulog[1].LED2){
            rtulog[1].eventlist[rtulog[1].eventcount]=4;
            rtulog[1].eventcount++;
        }
        if(rtulog[0].LED3!=rtulog[1].LED3){
            rtulog[1].eventlist[rtulog[1].eventcount]=5;
            rtulog[1].eventcount++;
        }
        //detect no power
        if(rtulog[0].output_v==rtulog[1].output_v){
            rtulog[1].eventlist[rtulog[1].eventcount]=7;
            rtulog[1].eventcount++;
        }
        //copy from current to past
        rtulog[0].LED1=rtulog[1].LED1;
        rtulog[0].LED2=rtulog[1].LED2;
        rtulog[0].LED3=rtulog[1].LED3;
        rtulog[0].ADC=rtulog[1].ADC;
        rtulog[0].output_v=rtulog[1].output_v;
        rtulog[0].eventcount=rtulog[1].eventcount;
        for(int i=0;i<rtulog[1].eventcount;i++)
        rtulog[0].eventlist[i]=rtulog[1].eventlist[i];
        //init rtulog[1]
        rtulog[1].init();
        sem_post(&my_semaphore);
        //printf("r1 %d\n",rtulog[1].eventcount);
        //printf("r0 %d\n",rtulog[0].eventcount);
        int t=rtulog[0].eventcount;
        
        //send RTU number and time
        
        bzero(log_buffer,MSG_SIZE);
        rtulog[0].print_log1(log_buffer);
        n = write(sockfd,log_buffer,strlen(log_buffer));
        if (n < 0)
            error("ERROR writing to socket");
        //printf("after 1 r0 %d\n",rtulog[0].eventcount);
        //send current status
        bzero(log_buffer,MSG_SIZE);
        rtulog[0].print_log2(log_buffer);
        n = write(sockfd,log_buffer,strlen(log_buffer));
        if (n < 0)
         error("ERROR writing to socket");
        //send events
        rtulog[0].eventcount=t;
        //printf("after 2 r0 %d\n",rtulog[0].eventcount);
        bzero(log_buffer,MSG_SIZE);
        rtulog[0].print_log4(log_buffer);
        n = write(sockfd,log_buffer,strlen(log_buffer));
        if (n < 0)
        error("ERROR writing to socket");
        
        //delay 1 s, TODO change to real time
        sleep(1);
        }

    }
    
    //child process
    else{
	
    close(pipe_c[1]);//pipe
    //detect ADC
    
    int x;
    while(1){
        bzero(buffer,MSG_SIZE);
        //receive command from server
        n = read(sockfd,buffer,MSG_SIZE-1);	
        if (n < 0)
            error("ERROR reading from socket");
        
        printf("Command received: %s\n",buffer);
        
        //check ONLED
        if(strstr(buffer,"ONLED1")!=0){
            digitalWrite(8,1);
            sem_wait(&my_semaphore);
            rtulog[0].LED1=1;
            rtulog[1].LED1=1;
            rtulog[1].eventlist[rtulog[1].eventcount]=3;
            rtulog[1].eventcount++;
            sem_post(&my_semaphore);
        }

        //check ONLED
        if(strstr(buffer,"ONLED2")!=0){
            digitalWrite(9,1);
            sem_wait(&my_semaphore);
            rtulog[0].LED2=1;
            rtulog[1].LED2=1;
            rtulog[1].eventlist[rtulog[1].eventcount]=4;
            rtulog[1].eventcount++;
            sem_post(&my_semaphore);
        }

        //check ONLED
        if(strstr(buffer,"ONLED3")!=0){
            digitalWrite(7,1);
            sem_wait(&my_semaphore);
            rtulog[0].LED3=1;
            rtulog[1].LED3=1;
            rtulog[1].eventlist[rtulog[1].eventcount]=5;
            rtulog[1].eventcount++;
            sem_post(&my_semaphore);
        }

        //check OFFLED
        if(strstr(buffer,"OFFLED1")!=0){
            digitalWrite(8,0);
            sem_wait(&my_semaphore);
            rtulog[0].LED1=0;
            rtulog[1].LED1=0;
            rtulog[1].eventlist[rtulog[1].eventcount]=3;
            rtulog[1].eventcount++;
            sem_post(&my_semaphore);
        }

        //check OFFLED
        if(strstr(buffer,"OFFLED2")!=0){
            digitalWrite(9,0);
            sem_wait(&my_semaphore);
            rtulog[0].LED2=0;
            rtulog[1].LED2=0;
            rtulog[1].eventlist[rtulog[1].eventcount]=4;
            rtulog[1].eventcount++;
            sem_post(&my_semaphore);
        }

        //check OFFLED
        if(strstr(buffer,"OFFLED3")!=0){
            digitalWrite(7,0);
            sem_wait(&my_semaphore);
            rtulog[0].LED3=0;
            rtulog[1].LED3=0;
            rtulog[1].eventlist[rtulog[1].eventcount]=5;
            rtulog[1].eventcount++;
            sem_post(&my_semaphore);
        }
    }
    }
    
    close(sockfd);	
    return 0;
}
