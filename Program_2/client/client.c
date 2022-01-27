/* Name: Abhimanyu Choudhary
Student_Id:00001537566*/

#include <stdio.h>
#include<stdio.h>
#include<sys/types.h>
#include <netinet/in.h>
#include<string.h>
#include<sys/socket.h>
#include<netdb.h>
#include<stdlib.h>
#include <unistd.h>     /* for close() and alarm() */
#include <errno.h>      /* for errno and EINTR */
#include <signal.h>     /* for sigaction() */
#ifndef _MSG_H_
#define _MSG_H_

#include <stdint.h>

#define START_PACKET_ID  (0xffff)
#define END_PACKET_ID    (0xffff)
#define ACCESS_PERM      (0xfff8)
#define NOT_PAID         (0xfff9)
#define NOT_EXIST        (0xfffa)
#define ACCESS_OK        (0xfffb)

#define TECHNOLOGY_2G    (02)
#define TECHNOLOGY_3G    (03)
#define TECHNOLOGY_4G    (04)
#define TECHNOLOGY_5G    (05)


typedef struct __attribute__((packed)) 
{
    unsigned short start_packid;
    unsigned char client_id;
    unsigned short type;
    unsigned char seg_no;
    unsigned char len;
    unsigned char technology;
    unsigned int number;
    unsigned end_packid; 
}subscriber_message;


typedef struct {
    unsigned int number;
    unsigned char technology;
    unsigned char payment_stat;
} SubscriberInfo_t;

#endif

#define TIMEOUT_SECS    3       /* Seconds between retransmits */
#define MAXTRIES        3       /* number of tries before giving up */

struct sockaddr_in server;
struct sockaddr_in from;
struct hostent *hp;
struct sigaction myAction;       /* For setting signal handler */
int slen = sizeof(server);
int sockid;
int ack_counter = 0;     /* Count of times sent - GLOBAL for signal-handler access */

void CatchAlarm(int ignored);


int handleAccessOk(subscriber_message *msg)
{
    printf("subscriber %u permitted to access\n", msg->number);
    return 0;
}

int handleNotPaid(subscriber_message* msg)
{
    printf("subscriber %u not paid\n", msg->number);
    return 0;
}

int handleNotExist(subscriber_message* msg)
{
    printf("subscriber %u does not exist\n", msg->number);
    return 0;
}

int sendMsg(unsigned int number, unsigned char technology)
{
    subscriber_message msg;

    msg.start_packid = START_PACKET_ID;
    msg.client_id = 100;
    msg.type = ACCESS_PERM;
    msg.seg_no = 0;
    msg.len = 5;
    msg.technology = technology;
    msg.number = number;
    msg.end_packid = END_PACKET_ID;

    int count = sendto(sockid, &msg, sizeof(msg), 0, (struct sockaddr *) &server, slen );
    
    if (count != -1) {
       // printf("msg sent size = %d \n",count);
    } else {
        printf("Unable to send message");
    }

   return 0;
}

int receiveMsg()
{
    subscriber_message msg;

    int count = recvfrom(sockid, &msg, sizeof(msg), 0,(struct sockaddr *)&server, &slen);
    if (count != -1) {
        alarm(0);
        if (msg.type == ACCESS_OK) {
            handleAccessOk(&msg);
        } else if (msg.type == NOT_PAID) {
            handleNotPaid(&msg);
        } else if (msg.type == NOT_EXIST) {
            handleNotExist(&msg);
        } else {
            printf("Unknown response from server");
        }
    }
    return count;
}


int main(int argc , char *argv[])
{
    if (argc != 3) {
        printf("usage: server port \n");
        exit(1);
    }

    /* Set signal handler for alarm signal */
    myAction.sa_handler = CatchAlarm;
    if (sigfillset(&myAction.sa_mask) < 0) /* block everything in handler */
        printf("sigfillset() failed \n");
    myAction.sa_flags = 0;

    if (sigaction(SIGALRM, &myAction, 0) < 0)
        printf("sigaction() failed for SIGALRM \n");

   /* Construct the server address structure */
     server.sin_family= AF_INET;
     hp = gethostbyname(argv[1]);
     if (hp == 0) {
         printf("unknown host");
         exit(1);
     }
     bcopy((char *)hp->h_addr,(char *)&server.sin_addr,hp->h_length);
     server.sin_port=htons(atoi(argv[2]));
     sockid = socket(AF_INET,SOCK_DGRAM,0);
     if (sockid == -1)
         printf("interface is not created \n");
     else
         printf("interface created \n");

/*Reading data from input file*/

    FILE *ptr = NULL;
    ptr = fopen("data.txt", "r");
    if (ptr == NULL) {
        printf("unable to open data.txt file\n");
        return -1;
    }

    unsigned int number = 0;
    unsigned int technology = 0;

    while (fscanf(ptr, "%u%d", &number, &technology) == 2) {

        ack_counter = 0;
        do {
            sendMsg(number, (unsigned char)technology);
            alarm(TIMEOUT_SECS);

            if(receiveMsg() == -1) {
                if (errno == EINTR)     /* Alarm went off  */
                    printf("timed out, %d more tries...\n", MAXTRIES-ack_counter);
                else
                    break;
            } else {
                break;
            }
        } while(ack_counter < MAXTRIES);

        if (ack_counter == MAXTRIES) {
            printf("Server doesnot respond \n");
            printf("\n");
        }
    }
fclose(ptr);
    return 0;
}
void CatchAlarm(int ignored)     /* Handler for SIGALRM */
{
    ack_counter += 1;
}
