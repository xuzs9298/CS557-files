#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <cstring>
#include <iostream>
#include <fstream>
#include <arpa/inet.h>
#include <pthread.h>
#include "project.h"
#include <signal.h>

# define MAXDATASIZE 1024


char hostname[1024];
char port[128];
char ip[128];
char asn[128];
char ip_tmp[128];
char asn_tmp[128];
bool new_requirment = false;

int sockfd =0;
char buf[ MAXDATASIZE] ; 
int  numbytes; 

void catch_int(int sig_num)
{
	close(sockfd);
	exit(-1);
}

//receive messages and print it
void * receive(void *pm)
{
	while ( ( numbytes = recv( sockfd, buf, MAXDATASIZE-1, 0) ) != -1) { 
		if ( numbytes) { 
			buf[numbytes] = '\0' ; 
			printf ( "received: %s\n" , buf) ; 
		} 
		if(numbytes == -1)
		{
			printf("ERROR in reading data from relay");
			break;
		}
	}
	close ( sockfd) ; 
		
}

//read new inputs from the user and send to the relay
void* send_new(void* pm)
{
	while(true)
	{
		int asn_test = -1;
		unsigned long ip_test = (in_addr_t)(-1);
		memset(asn_tmp,0,128);
		memset(ip_tmp,0,128);
		printf("Input ASN: \n");
		scanf("%s",asn_tmp);
		asn_test = atoi(asn_tmp);
		if(asn_test <= 0 || asn_test >= 65535)
		{
			printf("Invalid asn number\n");
			continue;
		}
		printf("Input IP: \n");
		scanf("%s",ip_tmp);
		ip_test = inet_addr(ip_tmp);
		if(ip_test == ( in_addr_t)(-1))
		{
			printf("Invalid IP address\n");
			continue;
		}
		memset(asn,0,128);
		memset(ip,0,128);
		strcpy(asn,asn_tmp);
		strcpy(ip,ip_tmp);
		packet pkt;
		pkt.asn = htons(atoi(asn));
		pkt.ip = inet_addr(ip);
		
		if( send ( sockfd, &pkt, sizeof(packet), MSG_NOSIGNAL) == -1)
		{
			printf("ERROR in sending message");
			continue;
		}
		else
		{
			printf ( "Sent the ip address and asn wanted\n" ); 
		}
		continue;
	}
}

//get options and initialize the parameters
void get_opt(int argc, char* argv[])
{
	int c;
	int port_test = 0;
	while ((c = getopt (argc, argv, "h:p:")) != -1)
	{
			switch (c)
			{
				case 'h':
					strcpy(hostname,optarg);
					break;
				case 'p':
					port_test = atoi(optarg);
					if(port_test < 1024 || port_test > 65535)
					{
						printf("Invalid port number\n");
						exit(-1);
					}
					strcpy(port,optarg);
					break;
				case '?':
					if (optopt == 'h')
						fprintf (stderr, "Option -%c requires an argument.\n", optopt);
					else if (isprint (optopt))
						fprintf (stderr, "Unknown option `-%c'.\n", optopt);
					else
						fprintf (stderr,
								"Unknown option character `\\x%x'.\n",
								optopt);
				 	exit(-1);
				default:
				 	abort ();
			}
	}
}

//Creates a connection to the relay
void create_connection()
{
	struct sockaddr_in server_addr; 
	if ( ( sockfd = socket ( AF_INET , SOCK_STREAM , 0) ) == -1) { 
		perror ( "socket error" ) ; 
		exit(-1); 
	} 
	memset ( & server_addr, 0, sizeof ( struct sockaddr ) ) ; 
	server_addr. sin_family = AF_INET ; 
	server_addr. sin_port = htons ( atoi(port) ) ; 
	server_addr. sin_addr. s_addr = inet_addr( hostname ) ; 
	if ( connect ( sockfd, ( struct sockaddr * ) &server_addr, sizeof ( struct sockaddr ) ) == -1) { 
		printf ( "connect error\n" ) ; 
		exit(-1); 
	} 

	
}

//This function creates two threads for sending requests and receiving xml messages
void create_threads()
{
	int ret = 0;
	
	pthread_t id1,id2;
	
	ret = pthread_create(&id1, NULL, receive, NULL);
	if (ret)
	{
		printf("Create pthread error!/n");
		exit (-1);
	}

	ret = pthread_create(&id2, NULL, send_new, NULL);
	if (ret)
	{
		printf("Create pthread error!/n");
		exit (-1);
	}

	pthread_join(id1, NULL);
	pthread_join(id2, NULL);
}

int main( int argc, char * argv[]) 
{ 
	get_opt(argc,argv);
	create_connection();
	create_threads();
	
	return 0; 
}
