
/*
 * Program Name: Client.c
 * Authors: Sushrut Kaul, Ishan Tyagi
 * Department of Electrical and Computer Engineering
 * Texas A&M University,College Station
 * 
*/

/*
 * Standard header file declarations 
 * 
*/

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<netdb.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>

#define DATA_SIZE_LIMIT 100 // set limit to the Data size to be handled

/* Step 1 : Create static variables to maintain the state across calls */

static char my_buffer[DATA_SIZE_LIMIT];
static char *buffer_pointer;
static int  buffer_count; 


void err_sys(const char* x)
{
	perror(x);
	exit(1);
}


/*The following function is used to write n bytes.
  This function offers an advantage over the standard
  write method call. It sends 'n' bytes while the 
  standard write can sometimes send less and that 
  condition is not an error.
  */ 

int writen(int sock_d,char *str,int n)
{
	ssize_t bytes_sent;
	size_t bytes_remaining=n;
	while(bytes_remaining!=0) {
		bytes_sent = write(sock_d,str,bytes_remaining);
		if(bytes_sent<=0)  {
			if(errno== EINTR && bytes_sent <0)
				bytes_sent=0;
			else
				return -1;
		}
		str=str+bytes_sent;
		bytes_remaining=bytes_remaining-bytes_sent;
	}
	printf("wrote %d bytes \n",n);
	return n;
}


/*
   my_read function takes the socket descriptor
   and pointer to a character where the read char 
   will be stored. This function is used by our
   readline function
   */

int my_read(int sock_d,char *ptr)
{

	//Initially , the buffer_count value is zero.
	if(buffer_count<=0) {
up:		//label

		buffer_count=	read(sock_d,my_buffer,sizeof(my_buffer));
		/*buffer_count is updated in the previous statement.
		  It can be positive,negative or zero. Each value corresponds
		  to a specific meaning checked below */


		//CASE A: BUFFER_COUNT is less than zero.
		//Caution: Indicates error condition

		if(buffer_count < 0) {
			//EINTR requires us to call read again .
			//So, we jump back to label 'up'
			if(errno == EINTR) {
				goto up;
			}
			else {
				return -1;
			}
		} 


		//CASE B: Buffer_count==0 
		//Indicates an end of file condition

		else if (buffer_count==0)
			return 0;


		//CASE C:When Buffer_count>0,we set set the buffer pointer 
		//equal to a pointer to the string from which we are reading
		buffer_pointer= my_buffer;
	}

	buffer_count=buffer_count-1; //one element read from the buffer
	*ptr=*buffer_pointer;        //assign pointer to the character pointer
	buffer_pointer=buffer_pointer+1; // Increment pointer
	return 1;
}


int readline(int sock_d,char *ptr,int max)
{
	//Readline will call our my_read function for better control
	int bytes_received;
	int i=1;
	char c,*ptr1;
	ptr1=ptr;
	while(i<max)
	{	
		bytes_received =my_read(sock_d,&c); //read a character from buffer into c
		
		if(bytes_received== 1)
		{
			*ptr1=c; 
			ptr1=ptr1+1;
			if(c == '\n') //null termination
				break;
		}
		else if (bytes_received == 0) {
			*ptr1=0; //this is the End of file situation
			return (i-1);
		}	
		else {
			return -1;
		}
	
             
	        /*	
		switch(bytes_received) {
			//case 1:If bytes_received is 1, we got one char from buffer
			case 1:
				*ptr1=c; 
				ptr1=ptr1+1;
				if(c=='\n') // New line character read
					break;     //line read complete
				break;
			case 0: 
				//End of file situation
				*ptr1=0; 
				return (i-1);
			default: 
				return -1;
		}
		*/

		i=i+1; //loop counter
	}
	*ptr1=0; // null termination
	return i; // return the number of bytes read
}



void *get_in_addr(struct sockaddr *sa)
{
	if(sa->sa_family==AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



int main(int argc,char *argv[])
{
	int sockfd,numbytes;
	char buf[DATA_SIZE_LIMIT];
	struct addrinfo hints , *server_info, *p;
	char input_stdin[100];
	int rv;
	int bytes_written;
	char s[INET6_ADDRSTRLEN];

	if(argc!=3) {
		fprintf(stderr,"usage :client hostname port-number\n");
		exit(1);
	}


	memset(&hints ,0,sizeof hints);
	hints.ai_family =AF_UNSPEC;     // can be IpV4/IpV6
	hints.ai_socktype =SOCK_STREAM; // Stream Socket


	if((rv=getaddrinfo(argv[1],argv[2],&hints,&server_info)) !=0) {
		fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(rv));
		return 1;
	}

	for(p=server_info; p!= NULL; p=p->ai_next) { // Find and use the first working configuration
		//ai_next is the pointer to next node in the linked list
		if((sockfd=socket(p->ai_family,p->ai_socktype,p->ai_protocol))==-1) {
			err_sys("client:socket");
			continue;
		}

		if(connect(sockfd,p->ai_addr,p->ai_addrlen)==-1) {
			close(sockfd);
			err_sys("client : connect");
			continue;
		}
		break;
	}


	if(p==NULL) {
		fprintf(stderr,"client:failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s,sizeof s);
	printf("client:connecting to %s\n",s);
	freeaddrinfo(server_info);//server_info is not longer required



	while(1) // Infinite loop
	{
		bzero(input_stdin,100); // zero off input_stdin string
		if((fgets(input_stdin,100,stdin)==NULL)|| feof(stdin))  
			/* fgets() returns a NULL when we type the END-OF-FILE(CNTRL-D)
			   However, this does not take care of the case when we input
			   some text followed by the END-OF-FILE. For that, we use the 
			   feof() function. feof will return true if it encounters an 
			   end of file.*/

		{
			printf("\nEnd of file detected\n");
			printf("\nclosing the socket on client side\n");
			printf("Client disconnected\n");
			close(sockfd);
			exit(1);
		}

		//Now to echo the received string back, we use the writen function
		bytes_written=writen(sockfd,input_stdin,strlen(input_stdin));

		if(bytes_written==-1)
		{
			err_sys("writen");
			exit(1);
		}

		numbytes=readline(sockfd,buf,DATA_SIZE_LIMIT);
		printf("%s",buf);
		printf("%d bytes recieved\n",numbytes);

		if(numbytes== -1)
		{
			err_sys("readline");
			exit(1);
		}


	}

	return 0;

}

