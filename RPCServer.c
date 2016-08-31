/*
File Name 		: RPCServer.c
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

int reqProcId , proc; //This will be used to proc Id of the incoming request.

//Structures from UDPSock.c
struct hostent *gethostbyname() ;
void printSA(struct sockaddr_in sa) ;
void makeDestSA(struct sockaddr_in * sa, char *hostname, int port) ;
void makeLocalSA(struct sockaddr_in *sa) ;
void makeReceiverSA(struct sockaddr_in *sa, int port);

//Function declaration prototype from question
Status GetRequest (Message *callMessage, int sock, SocketAddress *clientSA);
Status SendReply (Message *replyMessage, int sock, SocketAddress clientSA);
Status UDPsend(int sock, Message *msg, SocketAddress dest);
Status UDPreceive(int sock, Message *msg, SocketAddress *orig);

//Marshal function declaration
void marshal(RPCMessage *rm, Message *message);
void unMarshal(RPCMessage *rm, Message *message);

Status add(int ,int ,int *);
Status sub(int ,int ,int *);
Status mul(int ,int ,int *);
Status division(int ,int ,int *);

void calculate(RPCMessage rm,RPCMessage *output);


//This is the SERVER module
//The main function will trigger the SERVER process.
void main(int argc,char **argv)
{
	int sock;
	SocketAddress mySocketAddress,yourSocketAddress;
	Message callMessage,replyMessage;
	Status status;
	RPCMessage *rm;
	
	int port = RECIPIENT_PORT;

   //Make this the receiver socket
	makeReceiverSA(&mySocketAddress , port);
   
    //Step1 : Create socket.
    if((sock = socket(AF_INET, SOCK_DGRAM, 0))<0) {
		printf("EchoServer :: socket creation failed");
		exit(1);
    }

	 //Step2: Bind the socket to the local socket address
	 if( bind(sock, (struct sockaddr *)&mySocketAddress, sizeof(struct sockaddr_in))!= 0){
		perror("EchoServer :: Bind failed\n");
		close(sock);
		exit(1);
	}
		 
	 printf("\nEchoServer :: Server Initiated\n");
             
	 //Wait for a message from the client process
	  while(1){

		  //Get the message from the client.       
		 if((status=GetRequest (&callMessage,sock, &yourSocketAddress))!=OK){
			 perror("EchoServer :: Unable to receive message from client.");
			 exit(1);
		 }
		  
		 //Check if the client sent "Ping" or "Stop"
		if(strstr(callMessage.data,"Ping")!=NULL){
			 printf("\nEchoServer :: Client sent PING. Sending ACK\n");           
			 sprintf(replyMessage.data , "OK");
	  
			 if(SendReply (&replyMessage,sock, yourSocketAddress)!=OK){
				printf("\n EchoServer :: Failed to send reply to client.");
				exit(1);
			}
			
	    }else if(strstr(callMessage.data,"Stop")!=NULL){
		    printf("\n\nEchoServer :: Client sent PING. Sending ACK and quitting service\n"); 
			sprintf(replyMessage.data , "OK");

			if(SendReply (&replyMessage,sock, yourSocketAddress)!=OK){
				printf("\n EchoServer :: Failed to send reply to client.");
				exit(1);
			}
			exit(1);
			
		}else {
			printf("\n EchoServer :: Client sent an arithematic expression.\n");
			
			RPCMessage rm, result;
			
			//Unmarshalling the recieved message
			unMarshal(&rm, &callMessage);
			//Check if messaege type is REQUEST or not
			if(rm.messageType == Reply){
				printf("\n EchoServer :: Incorrect RPC message type.\n");
				exit(1);
			}

			Status calcStatus;
			//Calling the calculate function which performs the requested calculation
			 calculate(rm,&result);

		   //Extracting elements from the unmarshalled message.
		    reqProcId = rm.RPCId;
			proc = rm.procedureId;

		   //copying the result value and status value to replyMessage
			sprintf(replyMessage.data,"%d",result.arg1);
			strcat(replyMessage.data,";");
			char msg[10];
			sprintf(msg,"%d",result.arg2);
			strcat(replyMessage.data,msg);

		   //Sending a reply with value and status of the result		                
			if(SendReply (&replyMessage,sock, yourSocketAddress)!=OK){
				printf("\n sendreply failed");
				exit(1);
			}

		}
	}
}


/*
Function Name 	: GetRequest
Inputs 			: Message * , Message * , int , SocketAddress
Outputs 		: Status
Purpose 		: This will call UDPSend to send a message to the server and block the socket till it received a response from the server.
*/
Status GetRequest(Message *callMessage, int sock , SocketAddress *clientSA){
     Status status = UDPreceive(sock, callMessage,clientSA);
     return status;
}


/*
Function Name 	: UDPreceive
Inputs 			: int , Message * , SocketAddress *
Outputs 		: Status
Purpose 		: This will accept a message from the origin of the message
*/
Status SendReply (Message *replyMessage, int s, SocketAddress clientSA){
    Status status=UDPsend(s, replyMessage, clientSA);
    return status;
}


/*
Function Name 	: UDPsend
Inputs 			: int , Message *  , SocketAddress
Outputs 		: Status
Purpose 		: This will call call sendto to send a message to the destination and send back response of the call
*/
Status  UDPsend(int sock, Message *msg, SocketAddress dest){
	Status status=OK;
	int n;

	//If the assigned reply message is OK, send back message, else first Marshal the reply and then send the reply.
	if(strstr(msg->data,"OK") != NULL){
		//Send back data if the message is OK.
		 if( (n = sendto(sock, msg->data, SIZE, 0, (struct sockaddr *)&dest , sizeof(dest))) < 0){
				perror("Sending failed\n");
				status=BAD;
			}
	}else {
	RPCMessage rm;
	//Marshall and send back the message.
	marshal(&rm,msg);
	if( (n = sendto(sock, &rm, sizeof(rm), 0, (struct sockaddr *)&dest,
			sizeof(dest))) < 0){
			perror("Sending failed\n");
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

	//Extract message and convert it back to the Network byte order.
	char *result;
	
	//Assign the messageType as "Reply" and assign all rest of the values in the return message.
	rm->messageType=htonl(Reply);
	rm->procedureId=htonl(proc);
	rm->RPCId=htonl(reqProcId);
	
	result=strtok(message->data,";");
	rm->arg1=htonl(atoi(result));
	
	//Populate the original expression in the arg 2
	result=strtok(NULL,"\r");
	rm->arg2=htonl(atoi(result));
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


//Utility : Calculate , to call the corresponding arithematic function and pass back the result of the function.
void calculate(RPCMessage rm , RPCMessage *output){
	//Based on the procedureId corresponding calculation is performed
	switch(rm.procedureId){

	case 1:	output->arg2 = add(rm.arg1,rm.arg2,&output->arg1);
			break;

	case 2:	output->arg2 = sub(rm.arg1,rm.arg2,&output->arg1);                        
			break;

	case 3:	output->arg2 = mul(rm.arg1,rm.arg2,&output->arg1);
			break;

	case 4:	output->arg2 = division(rm.arg1,rm.arg2,&output->arg1);                       
			break;
	}
}


//Utility : Addition
Status add(int a,int b,int *c){
	int sum=a+b;
	*c=sum;
	return OK;
}

//Utility : Subtraction
Status sub(int a,int b,int *c){
	int sub=a-b;
	*c=sub;
	return OK;
}

//Utility : Division
Status division(int a,int b,int *c){
	if(b==0)
	return DivZero;
	
	int div=a/b;
	*c=div;
	return OK;
}

//Utility : Mutliplication
Status mul(int a,int b,int *c){
	int mul=a*b;
	*c=mul;
	return OK;
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

/* make a socket* address using any of the addressses of this computer
for a local socket on any port */
void makeLocalSA(struct sockaddr_in *sa)
{
	sa->sin_family = AF_INET;
	sa->sin_port = htons(RECIPIENT_PORT);
	sa-> sin_addr.s_addr = htonl(INADDR_ANY);
}

/* make a socket address using any of the addressses of this computer
for a local socket on given port */
void makeReceiverSA(struct sockaddr_in *sa, int port)
{
	sa->sin_family = AF_INET;
	sa->sin_port = htons(port);
	sa-> sin_addr.s_addr = htonl(INADDR_ANY);
}
