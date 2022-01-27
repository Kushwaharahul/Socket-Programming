/* Name: Abhimanyu Choudhary
Student_Id:00001537566*/

#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<string.h>
#include<sys/socket.h>
#include<netdb.h>
#include "file_db.h"
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

struct sockaddr_in server;
struct sockaddr_in from;
int length;
int slen = sizeof(from);
int sockid;

int respMsg(unsigned short respSubcode, subscriber_message *client_msg) {

    subscriber_message msg;

    msg.start_packid = START_PACKET_ID;
    msg.client_id = client_msg->client_id;
    msg.type = respSubcode;
    msg.seg_no = client_msg->seg_no;
    msg.len = 5;
    msg.technology = client_msg->technology;
    msg.number = client_msg->number;
    msg.end_packid = END_PACKET_ID;
    int count = sendto(sockid, &msg, sizeof(msg), 0, (struct sockaddr *)&from, slen);
    if (count != -1) {
     //   printf("response msg sent size= %d, sub code= %x\n", count, respSubcode);
    } else {
        printf("unable to send response msg\n");
    }
    
    return count;
}


int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf( "error, no port provided\n");
        return 0;
     }     
      
     sockid = socket(AF_INET,SOCK_DGRAM,0);  //family,type,protocol = IPv4, UDP, 0 is default protocol 
     if (sockid == -1)  //failure returns -1
         printf("interface is not created\n");
     else
         printf("interface created\n");
      
     length = sizeof(server);
     bzero(&server,length);
     server.sin_family= AF_INET;
     server.sin_addr.s_addr = htonl(INADDR_ANY);
     server.sin_port= htons(atoi(argv[1]));

     if(bind (sockid, (struct sockaddr *)&server, length) != -1)  //2nd parameter= struct sockaddr, the (IP) address and port of the machine
                                                                  //3rd parameter= size of sockaddr structure 
         printf("server binded\n");
 
     subscriber_message msg;
     while(1) {
         int count = recvfrom (sockid, &msg, sizeof(msg), 0, (struct sockaddr *)&from, &slen); 
         //int count = recvfrom(sockid, recvBuf, bufLen, flags,&clientAddr, addrlen);
         //recvBuf: void[], stores received bytes
         //bufLen: # bytes received
         //flags: integer, special options, usually just 0
         //count: # bytes transmitted (-1 if error)
         //clientAddr: struct sockaddr, address of the client
         //addrLen: sizeof(clientAddr)
         if (count != -1) {
           //  printf("message received = %d\n", count);
         } else {
             printf("unable to receive message\n");
             continue;
         }

        /* printf("startpacketid = %x\n", msg.start_packid );
         printf("clientid = %x\n", msg.client_id );
         printf("data type = %x\n", msg.type );
         printf("seg num = %x\n", msg.seg_no );
         printf("number = %u\n", msg.number );
         printf("technology = %d\n", msg.technology );*/

         if (msg.start_packid != START_PACKET_ID ||
             msg.type != ACCESS_PERM ||
             msg.end_packid != END_PACKET_ID) {
             printf("invalid message\n");
             continue;
         }

         SubscriberInfo_t info;
         if (getSubscriberInfo(msg.number, &info) == -1) {
             printf("unable to find subscriber information in database for %u\n", msg.number);
             respMsg(NOT_EXIST, &msg);
             continue;
         } else {
             printf("found subscriber information in database for %u\n", msg.number);
         }
         
         if (msg.technology != info.technology) {
             printf("technology mismatch %d %d\n", msg.technology, info.technology);
             respMsg(NOT_EXIST, &msg);
             continue;
         }

         if (info.payment_stat == 0) {
             printf("not paid\n");
             respMsg(NOT_PAID, &msg);
             continue;
         } 
 
         respMsg(ACCESS_OK, &msg);     
    }

    return 0;    
}
