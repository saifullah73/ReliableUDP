#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <stdbool.h>
#include <string.h>


#define BUFFERSIZE 500
#define WINDOWSIZE 5
struct sockaddr_in serverAddress,clt;
int createUDPSocket (int port);
void sendOKACK();
double getCurrentTime();
void sendAck(int ackNo);
void gotoxy(int x,int y);
int socketN,socketLength;
int d,minutes,seconds,hours;

struct packet{
    int seqN;
    int datSize;
    int data[BUFFERSIZE];
};


int main (int argc, char**argv){
    socketLength = sizeof(struct sockaddr_in);
    socketN = createUDPSocket(atoi(argv[1]));
    int file;

    if((file=open("received.mp4",O_CREAT|O_WRONLY|O_TRUNC,0600))==-1){
	 	perror("open fail");
	 	return EXIT_FAILURE;
	};
  

    struct packet * recvPacket = malloc(sizeof(struct packet));
    bool packBool[WINDOWSIZE];
    int totalGoodPackets = 0;
    int totalPackets = 0;
    int duplicatePacket = 0;
    int flag = 1;
    int windowPackets = 0;
    struct packet *packetArr[WINDOWSIZE];
    int trans = 1;
    double globalStartTime = getCurrentTime();
    printf("Ready To Receive:\n");
    while(flag){
        windowPackets=0;
        //clearing packBool
        for(int i = 0;i < WINDOWSIZE;i++){
            packBool[i]=false;
        }
        trans = 1;
        while(trans){
            //receiving data
            struct packet *pack = malloc(sizeof(struct packet));
            recvfrom(socketN,recvPacket,sizeof(*recvPacket),0,(struct sockaddr *)&serverAddress,&socketLength);
            if (recvPacket != NULL && strncmp(recvPacket->data, "EOF",BUFFERSIZE) != 0 && recvPacket->seqN != -1){
                //good data
                if (packBool[recvPacket->seqN] == false){
                    pack->seqN = recvPacket->seqN;
                    memcpy(pack->data,recvPacket->data,recvPacket->datSize);
                    pack->datSize = recvPacket->datSize;
                    packBool[recvPacket->seqN] = true;
                    packetArr[recvPacket->seqN] = pack;
                    windowPackets++;
                }
                else{
                    //duplicate received
                    pack -> seqN = recvPacket ->seqN;
                    memcpy(pack->data,recvPacket->data,recvPacket->datSize);
                    pack->datSize = recvPacket->datSize;
                    packetArr[recvPacket->seqN] = pack;
                    packBool[recvPacket->seqN] = true;
                    duplicatePacket++;
                }
                sendAck(recvPacket->seqN);
                totalPackets++;
            }
            //sendOKACK received unexpectedly
            if (recvPacket->seqN == -1 && windowPackets <= WINDOWSIZE){
                sendOKACK();
            }
            //EOF received 
            if (strncmp(recvPacket->data, "EOF",BUFFERSIZE) == 0){
                flag = 0;
                sendAck(recvPacket->seqN);
                sendOKACK();
                break;
            }
            //receiving OK for window transition
            if (windowPackets == WINDOWSIZE && recvPacket->seqN == -1){
                //sendOKACK();
                trans = 0;
                break;   
            }
            
        } 
        for(int x=0;x<windowPackets;x++){
            //writing file
            write(file,packetArr[x]->data,packetArr[x]->datSize);
            totalGoodPackets++;
        }
        gotoxy(0,4);
        printf("PacketReceived=%d\n",totalGoodPackets);
        double speed = (((totalPackets*500)/1024)/1024)/(getCurrentTime() - globalStartTime);
        printf("Speed=%f MBps\n",speed);
    };

    printf("Total packets received:%d\n",totalPackets);
    printf("Total packets accepted:%d\n",totalGoodPackets);
    printf("Total Duplicates Received %d\n",duplicatePacket);
    int d =(int) (getCurrentTime() - globalStartTime);
    int hours = (int)(d/3600);
    int minutes = (int)((d%3600)/60);
    int seconds = (int)(d % 60);
    printf("Time Taken: Minutes Time %dh %dm %ds\n",hours,minutes,seconds);
    close(socketN);
    close(file);

}

void gotoxy(int x,int y)
 {
 printf("%c[%d;%df",0x1B,y,x);
 }

double getCurrentTime(){
    struct timeval time;
    double secs = 0;
    gettimeofday(&time, NULL);
    secs = (double)(time.tv_usec) / 1000000 + (double)(time.tv_sec);
    return secs;
}


void sendOKACK(){
    struct packet *ack = malloc(sizeof(struct packet));
    ack->seqN = -10;
    ack->datSize = 0;
    memcpy(ack->data,(char *)"OKACK",BUFFERSIZE);
    int n = sendto(socketN,ack,sizeof(struct packet),0,(struct sockaddr*)&serverAddress,socketLength);
    sleep(0.01);
}

void sendAck(int ackNo){
    struct packet *ack = malloc(sizeof(struct packet));
    ack->seqN = ackNo;
    ack->datSize = 0;
    memcpy(ack->data,(char *)"ACK",BUFFERSIZE);
    int n = sendto(socketN,ack,sizeof(struct packet),0,(struct sockaddr*)&serverAddress,socketLength);
}


void testSocketWithPacket(int socketN,int socketLength){
    int n0;
    struct packet * recvPacket = malloc(sizeof(struct packet));
    n0 = recvfrom(socketN,recvPacket,sizeof(*recvPacket),0,(struct sockaddr *)&serverAddress,&socketLength);
    printf("SeqN: %d\n",recvPacket->seqN);
    printf("n0=%d\n",n0);
}

int createUDPSocket (int portNo){
    int l;
    int socketNo;
    if ( (socketNo = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE);
    }
    //preparation of the address of the destination socket
	l=sizeof(struct sockaddr_in);
	bzero(&serverAddress,l);
	serverAddress.sin_family=AF_INET;
	serverAddress.sin_port=htons(portNo);
	//INADDR_ANY binds it all interfaces not just localhost
	serverAddress.sin_addr.s_addr=htonl(INADDR_ANY);
	//Assign an identity to the socket
	if(bind(socketNo,(struct sockaddr*)&serverAddress,l)==-1){
		perror("bind fail");
		return EXIT_FAILURE;
	}
    return socketNo;
}