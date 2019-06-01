#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <stdbool.h>
#include <string.h>


#define BUFFERSIZE 500
#define TIMEOUT_INTERVAL 0.03
#define TIMEOUT_OK 0.1
#define WINDOWSIZE 5
#define SLEEPTIME 0.1
double getCurrentTime();
void sendOK();
int createUDPSocket (int port,char* ipaddr);
void gotoxy(int x,int y);
struct sockaddr_in serverAddress;
struct stat buffer;
int sockLength;
int socketN;
int no_packets;
off_t file_size;
int file;
int d,hours,minutes,seconds;

struct packet{
int seqN;
int datSize;
char data[BUFFERSIZE];
};

int main (int argc, char**argv){
    
    socketN=createUDPSocket(atoi(argv[2]), argv[1]);
    sockLength = sizeof(struct sockaddr_in);

    if ((file = open(argv[3],O_RDONLY))==-1){
		perror("open fail");
		return EXIT_FAILURE;
	}

    if (stat(argv[3],&buffer)==-1){
		perror("stat fail");
		return EXIT_FAILURE;
	}
	else
		file_size=buffer.st_size;
	no_packets=((int)file_size/BUFFERSIZE) + 1;

    struct packet packetArr[WINDOWSIZE]; //array to store packets
    struct packet * ackPacket = malloc(sizeof(struct packet)); // packet to receive
    bool ackArray[WINDOWSIZE] = {false,false,false,false,false}; //boolean ack array
    int totalAcks = 0; 
    int duplicatePackets = 0;
    int dataRead;
    int totalPackets = 0;  //keeps count of all packets sent
    int totalPacketsSent = 0; //keeps count of good packets
    int windowPackets = 0;
    int totalAcksReceived = 0;
    double globalStartTime = getCurrentTime();
    double start_time;
    while(totalPackets <= no_packets){
        windowPackets=0;
        for(int i = 0; i < WINDOWSIZE ; i++){
            //reading packets from file
            dataRead = read(file,packetArr[i].data,BUFFERSIZE);
            if (dataRead != 0){
                packetArr[i].datSize = dataRead;
                packetArr[i].seqN = i;
                totalPackets++;
                windowPackets++;
            }
            if (totalPackets == no_packets){
                    if (i+1<WINDOWSIZE){
                    packetArr[i+1].seqN = i+1;
                    packetArr[i+1].datSize = 3;
                    strncpy(packetArr[i+1].data,"EOF",BUFFERSIZE);
                    windowPackets++;
                    totalPackets++;
                    break;
                }
            }    
        }
        //sending the packets
        for(int u=0;u<windowPackets;u++){
            int x = sendto(socketN,&packetArr[u],sizeof(struct packet),0,(struct sockaddr*)&serverAddress,sockLength);
            totalPacketsSent++;
            if(x ==-1){
                perror("Critical Error");
                return EXIT_FAILURE;}
        }
        start_time = getCurrentTime();
        int transitionFlag = 1;
        while(transitionFlag){
            if (getCurrentTime() - start_time <= TIMEOUT_INTERVAL){
                int y = recvfrom(socketN,ackPacket,sizeof(*ackPacket),MSG_DONTWAIT,(struct sockaddr*)&serverAddress,&sockLength);
                if (y != -1 && ackPacket->seqN != -10 ){
                    if(ackArray[ackPacket->seqN]!=true){
                        //not a duplicateACK
                        totalAcks++;
                        totalAcksReceived++;
                    }
                    ackArray[ackPacket->seqN] = true;
                }
                //all acks received
                if(totalAcks==WINDOWSIZE){
                    //sending OK message to transition between packet windows
                    sendOK();
                    double start_time2 = getCurrentTime();
                    while(1){
                        if (getCurrentTime() - start_time2 <= TIMEOUT_OK){
                            //waiting for OKACK
                            int q = recvfrom(socketN,ackPacket,sizeof(*ackPacket),MSG_DONTWAIT,(struct sockaddr*)&serverAddress,&sockLength);
                            if (q != -1 && ackPacket!=NULL) {
                                if (strncmp(ackPacket->data, "OKACK",BUFFERSIZE) == 0){
                                    totalAcks = 0;
                                    transitionFlag = 0;
                                    break;
                                }
                            }
                        }
                        else{
                            //OK timeout
                            sendOK();
                            start_time2 = getCurrentTime();
                        }
                    }
                }
                //last window
                if(totalAcksReceived >= no_packets){
                    totalAcks = 0;
                    //clear ackArray
                    for(int i = 0; i< WINDOWSIZE;i++){
                        ackArray[i]=false;
                    }
                    break;
                }
            }
            else{
                //timeout
                printf("\n");
                for(int u=0;u < windowPackets;u++){
                    if (ackArray[u]==false){
                        int x = sendto(socketN,&packetArr[u],sizeof(struct packet),0,(struct sockaddr*)&serverAddress,sockLength);
                        sleep(SLEEPTIME);
                        duplicatePackets++;
                        totalPacketsSent++;
                        if(x ==-1){
                            perror("Critical Error!");
                            return EXIT_FAILURE;
                        }
                    }
                }
                start_time = getCurrentTime();
            }
            
        }
        //clear ackArray
        for(int i = 0; i< WINDOWSIZE;i++){
            ackArray[i]=false;
        }
        gotoxy(0,4);
        printf("Packets Sent %d/%d \n",totalPackets,no_packets);
        int progress = ((totalPackets*100)/no_packets);
        printf("Progress = %d percent \n",progress);
        //printf("Total Acks Received %d\n",totalAcksReceived);
        double speed = (((totalPacketsSent*500)/1024)/1024)/(getCurrentTime() - globalStartTime);
        printf("Speed=%f MBps\n",speed);
        d =(int) (getCurrentTime() - globalStartTime);
        hours = (int)(d/3600);
        minutes = (int)((d%3600)/60);
        seconds = (int)(d % 60);
        printf("Time Elapsed: Minutes Time %dh %dm %ds\n",hours,minutes,seconds);
    }
    printf("Total Duplicate Sent %d\n",duplicatePackets);
    d =(int) (getCurrentTime() - globalStartTime);
    hours = (int)(d/3600);
    minutes = (int)((d%3600)/60);
    seconds = (int)(d % 60);
    printf("Time Taken: Minutes Time %dh %dm %ds\n",hours,minutes,seconds);
    printf("END");
    close(socketN);
    close(file);
    

}

void gotoxy(int x,int y)
 {
 printf("%c[%d;%df",0x1B,y,x);
 }

void sendOK(){
    struct packet *ack = malloc(sizeof(struct packet));
    ack->seqN = -1;
    ack->datSize = 0;
    memcpy(ack->data,(char *)"OK",BUFFERSIZE);
    int n = sendto(socketN,ack,sizeof(struct packet),0,(struct sockaddr*)&serverAddress,sockLength);
    sleep(SLEEPTIME);
}

double getCurrentTime(){
    struct timeval time;
    double secs = 0;
    gettimeofday(&time, NULL);
    secs = (double)(time.tv_usec) / 1000000 + (double)(time.tv_sec);
    return secs;
}


void TestSocketWithPacket(int socket,int l){
    int n;
    struct packet packetArr[1];
    printf("Enter Data Now: ");
    fgets(packetArr[0].data,BUFFERSIZE,stdin);
    packetArr[0].seqN = 23;
    n = sendto(socket,&packetArr[0],sizeof(struct packet),0,(struct sockaddr*)&serverAddress,l);
    printf("n=%d\n",n);
}


int createUDPSocket (int portNo,char* ip){
    int socketNo;
    if ( (socketNo = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    }
    memset(&serverAddress, 0, sizeof(serverAddress)); 
    bzero(&serverAddress,sizeof(serverAddress));
    
    serverAddress.sin_family    = AF_INET; // IPv4 
    serverAddress.sin_addr.s_addr = INADDR_ANY; 
    serverAddress.sin_port = htons(portNo);

    if (inet_pton(AF_INET,ip,&serverAddress.sin_addr)==0){
		printf("Invalid IP adress\n");
		return EXIT_FAILURE;
	}
    return socketNo;

}