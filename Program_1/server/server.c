/* Name: Abhimanyu Choudhary
Student_Id:00001537566*/

#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<string.h>
#include<sys/socket.h>
#include<netdb.h>
#include <stdint.h>
#define START_PACK_ID 0XFFFF
#define END_PACK_ID 0XFFFF
#define CLIENT_ID_MAX 255
#define DATA_LEN_MAX 255
#define DATA 0XFFF1
#define ACK 0XFFF2
#define REJECT 0XFFF3
#define REJ_OUT_SEQ 0XFFF4
#define REJ_LEN_MISMATCH 0XFFF5
#define REJ_END_MISS 0XFFF6
#define REJ_DUP_PACK 0XFFF7


typedef struct __attribute__((packed)) 
{
    unsigned short start_packid;
    unsigned char client_id;
    unsigned short type;
    unsigned char seg_no;
    unsigned char len;   
}header_data_packet ;

typedef struct __attribute__((packed))
{
    unsigned short start_packid;
    unsigned char client_id;
    unsigned short type;
    unsigned char rec_seg_no;
    unsigned short end_pac_id;   
}header_ack_packet;

typedef struct __attribute__((packed))
{
    unsigned short start_packid;
    unsigned char client_id;
    unsigned short type;
    unsigned short reject_sub_code;
    unsigned char rec_seg_no;
    unsigned short end_pac_id;   
}header_reject_packet;

typedef struct __attribute__((packed))
{
    unsigned short start_packid;
    unsigned char client_id;
    unsigned short type;
}header_common;

typedef union
{
    header_common a;
    header_ack_packet b;
    header_reject_packet c; 
}server_response;

#define MAX_DATA_MSG_SIZE (sizeof(header_data_packet) + DATA_LEN_MAX + 2)
#define MAX_DATA_ACK_MSG_SIZE (sizeof(header_ack_packet))
#define MAX_DATA_REJECT_MSG_SIZE (sizeof(header_reject_packet))

int expected_segment_id = 0;   
struct sockaddr_in server;
struct sockaddr_in from;
int length;
char payload[256];
int slen = sizeof(from);
char recvBuf[MAX_DATA_MSG_SIZE];
header_data_packet *headerdatamsgptr;
char sendbufferAck[MAX_DATA_ACK_MSG_SIZE];
header_ack_packet *headerackmsgptr;
char sendbufferRej[MAX_DATA_REJECT_MSG_SIZE];
header_reject_packet *headerrejmsgptr;
int sockid;
    
/*Global function, send reject response to client*/

int rejectMsg(unsigned short rejectsubcode)
{
   headerrejmsgptr= (header_reject_packet*)sendbufferRej; //sendbufferRej is a buffer in order to push the data from header_reject_packet typcasting it
   headerrejmsgptr->start_packid = START_PACK_ID;
   headerrejmsgptr->client_id = 100;
   headerrejmsgptr->type = REJECT;
   headerrejmsgptr->reject_sub_code = rejectsubcode;
   headerrejmsgptr->rec_seg_no = headerdatamsgptr->seg_no;
   //printf("seg no %x \n",headerrejmsgptr->rec_seg_no);
   headerrejmsgptr->end_pac_id = END_PACK_ID;

   int count2 = sendto(sockid, sendbufferRej,sizeof(header_reject_packet),0, (struct sockaddr *) &from, slen );

   if (count2!= -1)
      printf("reject msg sent \n reject sub code= %x \n" ,headerrejmsgptr-> reject_sub_code);
                       
   else
      printf("reject msg not sent");

   return 0;
}

/* Main Function */

int main(int argc, char *argv[])
{
      if (argc < 2)
     {
        printf( "error, no port provided\n");
        exit(0);
     }
   
sockid = socket(AF_INET,SOCK_DGRAM,0);
if (sockid==-1)
   printf("interface is not created \n");
else
   printf("interface created \n");
      
length = sizeof(server);
bzero(&server,length);             //
server.sin_family= AF_INET;
server.sin_addr.s_addr = htonl(INADDR_ANY);
server.sin_port= htons(atoi(argv[1]));

if(bind(sockid, (struct sockaddr *) &server,length )!= -1)
   printf("server binded \n");
      
while(1)
    {
     // printf("expected segment id= %d \n" , expected_segment_id);
      int count = recvfrom(sockid, recvBuf, sizeof(recvBuf), 0,(struct sockaddr *)&from, &slen);
      if (count!=-1)
          printf("message received = %d \n", count);
      else
          printf("message not received \n");

     headerdatamsgptr= (header_data_packet*)recvBuf;
     printf("Packet Id (type) = %x \n",headerdatamsgptr->type );
    // printf("seg num = %x \n",headerdatamsgptr->seg_no );
     if (headerdatamsgptr->seg_no == expected_segment_id)
        printf("segment number ok %x \n",headerdatamsgptr->seg_no);
     else
        { 
         if(headerdatamsgptr->seg_no < expected_segment_id)
            {
	       rejectMsg(REJ_DUP_PACK);
              // printf("error : expected segment id is %d \n", expected_segment_id);
               printf("\n");
            }
         else
            {
               rejectMsg(REJ_OUT_SEQ);
              // printf("error : expected segment id is %d \n", expected_segment_id);
               printf("\n");
            }
             continue;
        }
             
    // printf("data length = %x \n",headerdatamsgptr->len );
     int payloadLength = count - sizeof(header_data_packet) - 2;
     if (payloadLength == headerdatamsgptr->len)
         memcpy(payload, &recvBuf[0] + sizeof(header_data_packet),headerdatamsgptr->len );
     else
         {
         rejectMsg(REJ_LEN_MISMATCH);
         printf("\n");
         continue;
         
         }
     
    // printf("the payload is %s \n",payload);
    // printf("size of payload is %d \n",strlen(payload)+1);
     unsigned short *endOfPacket =(unsigned short*)( &recvBuf[0] + sizeof(header_data_packet) + headerdatamsgptr->len);
     //printf("%x \n", *endOfPacket);
     
     if (*endOfPacket==END_PACK_ID)
        printf("end of packet ok \n");
     else
        {
         rejectMsg(REJ_END_MISS);
         printf("\n");
         continue;
         }  
     headerackmsgptr= (header_ack_packet*)sendbufferAck;
     headerackmsgptr->start_packid = START_PACK_ID;
     headerackmsgptr->client_id = 100;
     headerackmsgptr->type = ACK;
     headerackmsgptr->rec_seg_no = headerdatamsgptr->seg_no;
     headerackmsgptr->end_pac_id = END_PACK_ID;
     int count1 = sendto(sockid, sendbufferAck,sizeof(header_ack_packet),0, (struct sockaddr *) &from, slen );
     if (count1!= -1)
        {
          printf("ack sent \n");
          printf("\n");
          if (++expected_segment_id == 5)
              expected_segment_id = 0;
        }
     else
         printf("ack not sent");

     }
return 0;    
}