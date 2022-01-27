/* Name: Abhimanyu Choudhary
Student_Id:00001537566*/

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

#define TIMEOUT_SECS    3       /* number of seconds between retransmits */
#define MAXTRIES        3       /* number of tries before giving up */


/*togenerate errors*/

typedef enum {
    ERROR_PAYLOAD_MISMATCH = 1,
    ERROR_NO_ERROR = 0,
    ERROR_END_OF_PACKET_MISMATCH = 2
} SIMULATE_ERROR_TYPE;

struct sockaddr_in server;
struct sockaddr_in from;
struct hostent *hp;
struct sigaction myAction;       /* For setting signal handler */
char buffer[MAX_DATA_MSG_SIZE];
header_data_packet *headerdatamsgptr=NULL;
int seg_num;
char payload[255];
int slen = sizeof(server);
int sockid;
char recvBuf[10];
int count1;
int ack_counter = 0;     /* Count of times sent - GLOBAL for signal-handler access */
int i;
server_response *response = NULL;
   
void CatchAlarm(int ignored);            /* Handler for SIGALRM */

/* Global Function send packet to the server */

int sendMsg(int seg_num, char *payload, SIMULATE_ERROR_TYPE error)
{  
     headerdatamsgptr= (header_data_packet*)buffer;
     headerdatamsgptr->start_packid = START_PACK_ID;
     headerdatamsgptr->client_id = 100;
     headerdatamsgptr->type = DATA;
     headerdatamsgptr->seg_no = seg_num;
     printf("seg no = %d \n",headerdatamsgptr->seg_no );
     headerdatamsgptr->len = strlen(payload) + 1;
     memcpy(&buffer[0] + sizeof(header_data_packet), payload, headerdatamsgptr->len);      /**/
     unsigned short* endOfPacket = (unsigned short*)( &buffer[0] + sizeof(header_data_packet) + headerdatamsgptr->len);   /**/
     *endOfPacket = END_PACK_ID;
     unsigned short msgSize = sizeof(header_data_packet) + headerdatamsgptr->len + 2;
     if (error == ERROR_PAYLOAD_MISMATCH)                          /* simulating error, send error data packet to server */
        headerdatamsgptr->len += 10;
     else if (error == ERROR_END_OF_PACKET_MISMATCH)                /* simulating error, send error data packet to server */
        *endOfPacket = 0x1234;
     int count = sendto(sockid, buffer, msgSize,0, (struct sockaddr *) &server, slen );   
        if (count!= -1)
            printf("msg sent size= %d \n",count);
     return 0;
}

/* Global Function receive response from the server */

int receiveMsg()
{ 
    count1 = recvfrom(sockid, recvBuf, sizeof(recvBuf), 0,(struct sockaddr *)&server, &slen);
    if (count1!=-1)
           {
                    alarm(0);
		    response = (server_response*)recvBuf;
		    if (response->a.type == ACK)                       /* Got ack response from server*/
		    {
		      handleAcknowledgement(response);
		    }
		    else if (response->a.type == REJECT)              /* Got reject response from server*/
		    {
		      handleReject(response);
		    }
		    else
		    {
		      printf("unknown msg \n");
		    }
             }
    return count1;
}

int handleAcknowledgement(server_response *response)
{          
   printf("ack received \n");   
   printf(" PacketId = %x \n",response->b.type );
   //printf("seg num = %x \n",response->b.rec_seg_no );
   //  printf("endofpackid = %x \n",response->b.end_pac_id);
   printf("\n");
   return 0;           
}

int handleReject(server_response *response)
{
  int seg_num1;
  printf("reject message received \n");    
  printf("Packet Id (type) = %x \n",response->c.type );
  printf("rejection sub_code = %x \n",response->c.reject_sub_code);
  printf("\n");
  // printf("seg num = %x \n",response->c.rec_seg_no );
  //  printf("endofpackid = %x \n",response->c.end_pac_id    
  return 0;
}

/*Main Function*/

int main(int argc , char *argv[])
{  

 if (argc !=3)
  {
    printf("usage: server port \n");
    exit(1);
  }
 /* Set signal handler for alarm signal */

 myAction.sa_handler = CatchAlarm;
 if (sigfillset(&myAction.sa_mask) < 0)       /* block everything in handler */
    printf("sigfillset() failed \n");
 myAction.sa_flags = 0;

 if (sigaction(SIGALRM, &myAction, 0) < 0)
        printf("sigaction() failed for SIGALRM \n");

 /* Construct the server address structure */

 server.sin_family= AF_INET;
 hp = gethostbyname(argv[1]);
 if (hp==0) 
   {
     printf("unknown host");
     exit(1);
   }
 bcopy((char *)hp->h_addr,(char *)&server.sin_addr,hp->h_length);
 server.sin_port=htons(atoi(argv[2]));

 sockid = socket(AF_INET,SOCK_DGRAM,0);
 if (sockid==-1)
    printf("interface is not created \n");
 else
    printf("interface created \n");

  /* File handling for I/P */

 FILE *fp;
 fp = fopen("data.txt","r");
 int payloadlen = 0;
 int error_info = 0;
     
 while(fscanf(fp, "%d%d%d", &seg_num, &payloadlen, &error_info) == 3)
      {
        memset(payload,'a', payloadlen);
        payload[payloadlen] = '\0';
           
        if(error_info == ERROR_NO_ERROR)
           {           
            // printf("%d\n", seg_num);
            // printf("%s\n", payload);
            //printf("%d\n", error_info);
             ack_counter = 0;
             do {
		  sendMsg(seg_num, payload, ERROR_NO_ERROR);           /*Sending packets with no error*/
		  alarm(TIMEOUT_SECS);
		  if(receiveMsg() == -1)                                        /*no response from server*/
		    {
		      if (errno == EINTR)    /*  Alarm went off  */          
		      printf("timed out, %d more tries...\n", MAXTRIES-ack_counter);
		    } 
		  else 
		     {
		      break;
		     }
		     
		  } while(ack_counter < 3);
	      if(ack_counter==3)
		  {
		     printf("Server doesnot respond \n");
		     printf("\n");
		  } 
             }
          else if (error_info == ERROR_PAYLOAD_MISMATCH)
             {
                 printf("error in data packet \n");
                // printf("%d\n", seg_num);
	        // printf("%s\n", payload);
                 //printf("%d\n", error_info);
			 
	         sendMsg(seg_num, payload, ERROR_PAYLOAD_MISMATCH);    /*sending pckets with error*/
                 receiveMsg();

               }
            else if (error_info == ERROR_END_OF_PACKET_MISMATCH) 
               {
		 printf("error in data packet \n");
                 //printf("%d\n", seg_num);
	         //printf("%s\n", payload);
                 //printf("%d\n", error_info);
			   
	         sendMsg(seg_num, payload, ERROR_END_OF_PACKET_MISMATCH);   /*sending pckets with error*/
                 receiveMsg();
               } 

        } 
 
  fclose(fp);
return 0;
}

void CatchAlarm(int ignored)     /* Handler for SIGALRM */
{
    ack_counter += 1;
}

