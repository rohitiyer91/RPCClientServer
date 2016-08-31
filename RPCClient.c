/*
File Name 		: RPCClient.c
Project Name 	: COM S 554 Programming Project 1000
Author 			: Rohit Iyer(930363459)
Purpose			: This is a preliminary file for testing the Client part of a UDP client/server communication
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include<errno.h>

#define SIZE 1000

//Defining the server port Id to be unique as hinted in the problem statement
#define RECIPIENT_PORT 	IPPORT_RESERVED+3459

//Predefined strcuture declaration
typedef struct {
unsigned int length;
unsigned char data[SIZE];
} Message;

 typedef enum {
 OK,  /* operation successful */
 BAD,  /* unrecoverable error */
 WRONGLENGTH,
 DivZero  /* bad message length supplied */
 } Status;

typedef struct sockaddr_in SocketAddress ;

typedef struct {
 enum {Request, Reply} messageType;  /* same size as an unsigned int */
 unsigned int RPCId; /* unique identifier */
 unsigned int procedureId; /* e.g.(1,2,3,4) for (+, -, *, /) */
 int arg1; /* argument/ return parameter */
 int arg2; /* argument/ return parameter */
} RPCMessage; /* each int (and unsigned int) is 32 bits = 4 bytes */

int reqProcId;

//Structures from UDPSock.c
struct hostent *gethostbyname() ;
void printSA(struct sockaddr_in sa) ;
void makeDestSA(struct sockaddr_in * sa, char *hostname, int port) ;
void makeLocalSA(struct sockaddr_in *sa) ;

//Function declaration prototype from question
Status DoOperation (Message *message, Message *reply, int sock, SocketAddress serverSA);
Status  UDPsend(int sock, Message *msg, SocketAddress dest);
Status  UDPreceive(int sock, Message *msg, SocketAddress *orig);

//Marshal function declaration
void marshal(RPCMessage *rm, Message *message);
void unMarshal(RPCMessage *rm, Message *message);


//This is the client module
//The main function will trigger the client process and expect only one argument - "server address"
//Usage from cmd line : ./RPCClient <hostname>
void main(int argc,char **argv)
{
    int serverPort = RECIPIENT_PORT;
   int clientSock;
   char *serverName;
   SocketAddress mySocketAddress, yourSocketAddress;
   Message clientMsg,replyMsg;
   Status status; 

   //Step1 : Create socket.
   if((clientSock = socket(AF_INET, SOCK_DGRAM, 0))<0) {
	printf("EchoClient :: socket creation failed\n");
	exit(1);
   }

   //Step2: Check for server info in argument list.
   if(argc<=1){
	  printf("EchoClient :: server name not provided \n");
	  exit(1);
   }

   serverName = argv[1];

   //Step3: Create a SocketAddress struct for the server machine.
   makeDestSA(&yourSocketAddress,serverName, serverPort);
   
   //Keep taking user input to be sent to the user from the terminal
   while(1){
	  printf("\nEnter Message: ");
	  scanf("%s",clientMsg.data);
	  
	  status = DoOperation(&clientMsg, &replyMsg, clientSock, yourSocketAddress);
	  
	  //DoOperation is called and if it returns other than OK or DIV by Zero, it exits from the while loop         
	  if(status != OK && status != DivZero){
		 perror("EchoClient :: Failed to send message to server \n");
		 exit(1);           
	  }
	  
	  //Important to clear the string as the next user input might have content from previous input
	  memset(replyMsg.data,0,SIZE); 		
   }     
             
}

/*
Function Name 	: DoOperation
Inputs 			: Message * , Message * , int , SocketAddress
Outputs 		: Status
Purpose 		: This will call UDPSend to send a message to the server and block the socket till it received a response from the server.
*/
Status DoOperation (Message *message, Message *reply, int sock, SocketAddress serverSA){
     Status status;
    
	//Call UDPSend and pass the client socket, server address and the message to be sent.
	 //if the call succeeds, wait for reply, else send back error response
	 message->length = strlen(message->data)+1;
	 if((status=UDPsend(sock, message, serverSA))!=OK){
			printf("DoOperation :: Failed to send message to server.");
			return status;
	  }
      
      //TIMEOUT : We now check for the TIMEOUT condition
      int times=1;
      int x = anyThingThere(sock);
      while(x==0){
		  
		  //If x=0, that means no response. Send the message again and see if the server gets the message.
		  //If it does, great, else increase the counter by 1. If the couter =3, exit.
          if(UDPsend(sock, message, serverSA)!= OK){
			printf("DoOperation :: Timeout no. : %d", times);  
		  }
          
          printf("\n DoOperation:: Sending message again for timeout condition.");
		  
          if(times >= 3){
             printf("\n 3 Timouts occurred without server acknowledgement. Exiting client.\n");
             exit(1);
          }
          times++;
          x = anyThingThere(sock);
      }

      if((status = UDPreceive(sock, reply, &serverSA))!=OK){
		printf("DoOperation :: Failed to recieve data from server.\n");
		return status;
     }

      //checking if the replied message is OK.
	  //If it is, that means the client either originally sent either "Ping" or "Stop"
	  //If the client had sent "Stop", the server would have exited by now and so must the client.
	  //If the client did not send "Stop", then the client must continue as usual.
      if(strcmp(reply->data,"OK")==0){
		  printf("DoOperation :: Server replied OK.");
	  
		  if(strcmp(message->data,"Stop")==0){
			printf("\nDoOperation :: Stopping the client \n");
			exit(1);
		  }
		  memset(message->data,0,SIZE);
		  
	 }else{
		 
		 //If the client message was not "Stop" or "Ping", it must have been a mathmatical equation and we now need to unmarshall the RPC message.
        RPCMessage rm;
        unMarshal(&rm,reply);

		//Now that we have unmarshalled, we check the contents of the response.
		//We must also check if the response is the one the client originally sent and its of type "Reply"
       if(rm.RPCId == reqProcId && rm.messageType == Reply){
            printf("\nGot back response for Proc Id %d",reqProcId);
			
			//Check for Divide by Zero condition.
            if(rm.arg2 == DivZero){
				printf("\nDoOperation :: Divide by Zero Error\n");
				return DivZero;
			}
            else
				printf("\nDoOperation :: RESULT %d", rm.arg1);
       }else if (rm.RPCId != reqProcId){
		   perror("\nDoOperation :: Sent Proc Id  [%d], resposne Proc Id : [%d] are different. Exiting.");
		   return BAD;
	   }else{
		   perror("\nDoOperation :: Message type is not RESPONSE. Exiting.");
	   }
   } 

   status=OK;
   return status;
}


/*
Function Name 	: UDPsend
Inputs 			: int , Message *  , SocketAddress
Outputs 		: Status
Purpose 		: This will call call sendto to send a message to the destination and send back response of the call 
				  and marshall data before sending,if so required.
*/
Status  UDPsend(int sock, Message *msg, SocketAddress dest){

      Status status = OK; //being optimistic
	  int n;
	  RPCMessage rm;
	  char ping[]="Ping";
	   
	  //If the sent message is "Ping" or "Stop", then we do not need to marshall the message. 
	  //In such cases, we can directly send a message on the socket.
	  //If not, then we have a mathematical equation and we need to marshall the data.
	  if(strcmp(msg->data,"Ping")==0 || strcmp(msg->data,"Stop")==0){
		  if( (n = sendto(sock, msg->data,sizeof(msg), 0, (struct sockaddr *)&dest, sizeof(dest))) < 0){
			  perror("\nUDPsend :: failed to send to destination");
			  status=BAD;
		  }
	 }else{ 
		//Prepare to Marshall.
		RPCMessage rm; 
		marshal(&rm , msg);

		if( (n = sendto(sock , &rm, sizeof(rm), 0, (struct sockaddr *)&dest, sizeof(dest))) < 0){
			 perror("\nUDPsend :: failed to send to destination");
			 status=BAD;
		 }
	}

	return status;
}


/*
Function Name 	: UDPreceive
Inputs 			: int , Message * , SocketAddress *
Outputs 		: Status
Purpose 		: This will accept a message from the origin of the message
*/
Status  UDPreceive(int sock, Message *msg, SocketAddress *orig){

    Status status = OK; //being optimistic
    int n,origLen;
	
    origLen = sizeof(orig);
    
	//Call recvFrom to check for response from the origin.
	//If response comes back, send back OK ,else send back error
	if((n = recvfrom(sock, msg->data, SIZE, 0, (struct sockaddr *)orig, &origLen))<0){
		status = BAD;
		printf("\n UDPreceive :: Failed to recieve from origin");
		return status;
    }

    return status;
}


/*
Function Name 	: marshal
Inputs 			: RPCMessage * ,  Message * 
Outputs 		: void
Purpose 		: This will take an input message and marshal the message before sending to the server.
*/
void marshal(RPCMessage *rm, Message *message){

	char *str;

	//We now need to split the message into its constituent components
	//We then marashall individual components and assign them to the elements in the RPC message.
	char store[50];
	strcpy(store,message->data);

	if(strstr(message->data,"+")!=NULL){
		rm->procedureId=htonl(1);
		str=strtok(message->data,"+");
		
	}else if(strstr(message->data,"-")!=NULL){
		rm->procedureId=htonl(2);
		str=strtok(message->data,"-");
		
	}else if(strstr(message->data,"*")!=NULL){
		rm->procedureId=htonl(3);
		str=strtok(message->data,"*");
		
	}else if(strstr(message->data,"/")!=NULL){
		rm->procedureId=htonl(4);
		str=strtok(message->data,"/");
		
	}else{
		printf("Enter a valid message\n");
		exit(1);
	}
	
	//We now put the subsequen elements of the equation into the message.
	rm->arg1=htonl(atoi(str));
	str=strtok(NULL,"\r");
	rm->arg2=htonl(atoi(str));
	
	//We generate a procedure ID and put that into the message to track the message.
	srand(time(NULL));
	reqProcId = rand();
	rm->RPCId=htonl(reqProcId);
	rm->messageType=htonl(Request);

	//Putting the storage from temp back to the origianal message since we need to send both of them back.
	strcpy(message->data,store);
}


/*
Function Name 	: unMarshal
Inputs 			: RPCMessage *,Message *
Outputs 		: void
Purpose 		: This will unmarshall the message.
*/
void unMarshal(RPCMessage *rm, Message *message){
	//We now extract bytes from the message and unmarshall one by one.
	//We directly extract the RPC message by explicity typecasting
	RPCMessage *read=(RPCMessage *)message->data;
	
	//Extract elements
	rm->messageType = ntohl(read->messageType);
	rm->procedureId = ntohl(read->procedureId);
	rm->RPCId = ntohl(read->RPCId);
	
	rm->arg1 = ntohl(read->arg1);
	rm->arg2 = ntohl(read->arg2);
}


/* make a socket address for a destination whose machine and port
    are given as arguments */
void makeDestSA(struct sockaddr_in * sa, char *hostname, int port)
{
    struct hostent *host;

    sa->sin_family = AF_INET;
    if((host = gethostbyname(hostname))== NULL){
        printf("Unknown host name\n");
        exit(-1);
    }
    sa-> sin_addr = *(struct in_addr *) (host->h_addr);
    sa->sin_port = htons(port);
}

/* use select to test whether there is any input on descriptor s*/
int anyThingThere(int s)
{
	unsigned long read_mask;
	struct timeval timeout;
	int n;

	timeout.tv_sec = 10; /*seconds wait*/
	timeout.tv_usec = 0;        /* micro seconds*/
	read_mask = (1<<s);
	if((n = select(32, (fd_set *)&read_mask, 0, 0, &timeout))<0)
		perror("Select fail:\n");
	return n;
}
