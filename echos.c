/*
Program Name: echos.c
Author Name : Sushrut Kaul , Ishan Tyagi
Department of Electrical and Computer Engineering , Texas A&M University
*/


/*
Include the header files 
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include<sys/wait.h>
#include<signal.h>

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
	 printf("echoed %d bytes \n",n);
	 return n;
}



void sigchld_handler(int s)
{
	 int saved_errno= errno;
	 while(waitpid(-1,NULL,WNOHANG )>0);
	 errno =saved_errno;
}

void *get_in_addr(struct sockaddr *sa)
{
	if(sa->sa_family == AF_INET) {
	 return &(((struct sockaddr_in*)sa)->sin_addr);
	 return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
}

void err_sys(const char* x) 
{ 
	    perror(x); 
	        exit(1); 
}

#define BACKLOG 10  // Backlog is the maximum number of the clients that can connect
#define MAXDATASIZE 100
int main(int argc,char *argv[]) {

//Variables definitions begin
int sockfd,new_fd,numbytes,bytes_sent;
 char buf[MAXDATASIZE];
struct addrinfo hints, *servinfo , *p;
 struct sockaddr_storage their_addr;//connector's address
 socklen_t sin_size;
 struct sigaction sa;
 int yes=1;
 char s[INET6_ADDRSTRLEN];
 int rv;
// Variables definitions end
 

memset(&hints,0,sizeof hints);
hints.ai_family =AF_UNSPEC;// We are not specifying the ADDRESS TYPE	
hints.ai_socktype =SOCK_STREAM;// Using the Stream Socket
hints.ai_flags =AI_PASSIVE ; // AI_PASSIVE REFERS TO "MY-IP ADDRESS"




if((rv =getaddrinfo(NULL,argv[1],&hints,&servinfo )) !=0) {
	fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(rv));						
	return 1;								
}


/*ai_next refers to the pointer to the next node
in the linked-list that we have obtained
*/


for(p=servinfo ;p!=NULL ; p->ai_next ) {
	if((sockfd = socket(p->ai_family , p->ai_socktype, p->ai_protocol))==-1){				
		err_sys("server :socket");		
		continue;
	}
												
if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int))==-1) {
		err_sys("setsockopt");										
		exit(1);									 
}												
if(bind(sockfd,p->ai_addr,p->ai_addrlen)==-1) {
		close(sockfd);											
		err_sys("server:bind");										
		continue;									 
}
 break    ; // break when you get a usable configuration
}
	freeaddrinfo(servinfo) ;//servinfo is no longer required								
if(p==NULL) {
	fprintf(stderr,"server:failed to bind\n") ;							
exit(1);
 }
if(listen(sockfd,BACKLOG) ==-1) {
		err_sys("listen");
		exit(1);																				
}
sa.sa_handler =sigchld_handler;	
sigemptyset(&sa.sa_mask);
sa.sa_flags= SA_RESTART ;									
if(sigaction(SIGCHLD ,&sa,NULL)==-1) {								
err_sys("sigaction");
exit(1);
}

 printf("server waiting for connections...\n");
while(1) {												
sin_size =sizeof their_addr;
new_fd=accept(sockfd,(struct sockaddr *)&their_addr,&sin_size);	 //new_fd returns a new socket descriptor


if(new_fd ==-1) {
	err_sys("accept");
	continue;
  }
inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s,sizeof s);
printf("server got connection from %s\n",s);
if(!fork()){ 
	while(1) {	
    	   //close(sockfd);
	    numbytes = read(new_fd,buf,MAXDATASIZE-1);						        
		if(numbytes==-1) {	
			err_sys("recv");
			exit(1);                                                                             
  }

/*If the client issues an EOF from the standard input,
    the read command here will return zero. We use that 
    command to tell the child process to exit.
*/
		if(numbytes==0)  
		{
			
			printf("Child process exiting\n");
			printf("Client Disconnected \n");
			close(new_fd);
			exit(1);
		}
buf[numbytes]='\0'; //  Null terminate the string	

printf("server  :received--> %s\n",buf);	
bytes_sent=writen(new_fd,buf,strlen(buf));
	 
       }
      }	
    }
																									
close(new_fd);	
return(0);
  }	
