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
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "project.h" 
#include <signal.h>

#define SERVERPORT 20000
#define MAXDATASIZE 1024
#define MAXCONN_NUM 10



using namespace std;

int new_fd,sockfd;
bool signal_t;
bool has_connection;
char SERVERIP[128];
char ip[128];
char asn[128];
char hostname[1024];
char port[128];
char buf[ MAXDATASIZE] ; 

void catch_int(int sig_num)
{
	close(sockfd);
	close(new_fd);
	exit(-1);
}

int recv_num(int sockfd, char* buf, int length,int flags)
{
	while(length != 0)
	{
		int num = recv ( sockfd, buf, length, flags); 
		if(num < 0 )
			return -1;
		length -= num;
		buf = buf+num;
	}
	return 1;
}
//check whether the xmlNode contains the correct src_ip and src_as_number
//It returns a value which has two bits represents whether they have the two properties
int check_element(xmlNode * a_node, char* src_ip, char* src_asn)
{
    xmlNode *cur_node = NULL;
	bool flag1 = false,flag2 = false;
	char buffer[MAXDATASIZE],buffer2[MAXDATASIZE];
	
    for (cur_node = a_node; cur_node; cur_node = cur_node->next) 
	{
		if (cur_node->type == XML_ELEMENT_NODE) 
		{
			if(xmlStrcmp(cur_node->name,(const xmlChar*)"ADDRESS")==0 && xmlStrcmp(cur_node->children->content,(const xmlChar*)src_ip)==0 && xmlStrcmp(cur_node->parent->name,(const xmlChar*)"SRC_ADDR")==0)
			{
					flag1 = true;
			}
			else if(xmlStrcmp(cur_node->name,(const xmlChar*)"SRC_AS")==0 && xmlStrcmp(cur_node->children->content,(const xmlChar*)src_asn)==0)
			{
				flag2 = true;
			}
				
		}
		int returned = check_element(cur_node->children,src_ip,src_asn);
		flag1 = flag1 || returned&1;
		flag2 = flag2 || returned&2;
		
	}
	int result = 0;
	if(flag1)
		result |=1;
	if(flag2)
		result |=2;
	return result;
}

bool check_values(xmlNode * a_node, char* src_ip, char* src_asn)
{
	int result = check_element(a_node,src_ip,src_asn);
	/*if the result contains both the correct source ip address and AS number*/
	if(result == 3)
		return true;
	else 
		return false;
}

int parser(char* xml_file_name, char* ip, char* asn)
{
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;

	//parse the file and get the DOM 
	if ((doc = xmlReadFile(xml_file_name, NULL, 0)) == NULL){
		printf("error: could not parse file %s\n", xml_file_name);
	}

	//Get the root element node 
	root_element = xmlDocGetRootElement(doc);

	/*If it contains the correct source ip and as number then the TIMESTAMP and OCTET will be printed*/
	//if(check_values(root_element,argv[2],argv[3]))
		//print_element(root_element);
	
	
	if(check_values(root_element,ip,asn)){
		return 1;
	}
	else{
		return 0;
	}
	xmlFreeDoc(doc);       // free document
	xmlCleanupParser();    // Free globals

}

void * connect(void * pm) // the thread to connect to the BGPMon and send the data out to specified costumer
{
	int portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	char xml_data_filename[32] = "xml_data";
	char template_string[16] = "</BGP_MESSAGE>";

	int matching_status = 0;
	int message_count = 0;
	
	char buffer[MAXDATASIZE];

	portno = atoi(port);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) 
	{
		printf("ERROR opening socket\n");
		exit(-1);
	}
	server = gethostbyname(hostname);
	if (server == NULL) {
		printf("ERROR, no such host \"%s\"\n",hostname);
		exit(-1);
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, 
		 (char *)&serv_addr.sin_addr.s_addr,
		 server->h_length);
	serv_addr.sin_port = htons(portno);
	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
		printf("ERROR connecting");

	char num_buff[MAXDATASIZE];
	bzero(num_buff,MAXDATASIZE);
	sprintf(num_buff,"%s%d",xml_data_filename,message_count);
	ofstream outfile(num_buff);

	bzero(buffer,MAXDATASIZE);
	int template_length = strlen(template_string);

	n = recv(sockfd,buffer,MAXDATASIZE-1,0);
	while(1)
	{
		n = recv(sockfd,buffer,MAXDATASIZE-1,0);
		//printf("%s\n",buffer);
		if (n < 0) 
		{
			    printf("ERROR reading from socket\n");
			 	exit(-1);
		}
		for(int i=0;i<n;i++)
		{
			if(buffer[i] == template_string[matching_status])
				matching_status++;
			else
				matching_status=0;

			outfile << buffer[i];
			if(matching_status == template_length)
			{
				message_count = (message_count+1)%100;
				matching_status=0;
				
				outfile.close();
				
				bzero(num_buff,MAXDATASIZE);
				sprintf(num_buff,"%s%d",xml_data_filename,message_count);
				//printf("%s\n",num_buff);
				outfile.open(num_buff);

				sprintf(num_buff,"%s%d",xml_data_filename,(message_count+99)%100);
				int ret = parser(num_buff,ip,asn);
				//printf("%d\n",ret);
				if(ret == 1 && has_connection)
				{
					while(signal_t == false);
					char buf[MAXDATASIZE];
					ifstream infile;
					infile.open(num_buff);
					if(infile.is_open())          
					{
						 while(infile.good() && !infile.eof())
						 {
							   memset(buf,0,MAXDATASIZE);	
							   infile.read(buf,MAXDATASIZE-1);
								//int count_num;
								////if(!infile)
								//	count_num = infile.gcount();
								//else
								//	count_num = MAXDATASIZE;
								if ( send ( new_fd, buf , MAXDATASIZE-1 , MSG_NOSIGNAL) == -1) { 
									//printf("Sending to client ERROR\n");
									break;
								}
						 }
						 infile.close();
					}
				}
			}
		}
	}
    close(sockfd);
	return (void *)0; 
}

void* receive_requirments(void* pm)
{
	int numbytes = 0;
	while((numbytes = recv_num ( new_fd, buf, sizeof(packet), 0) ) > 0)
	{
		
		sprintf(asn,"%d\0",ntohs(((packet *)buf)->asn));
		uint32_t tmp = ((packet *)buf)->ip;
		ntohl(tmp);
		sprintf(ip,"%s\0",inet_ntoa(*(in_addr *) (& tmp)));
		printf("Received %s %s\n",asn,ip);	
		
		if ( send ( new_fd, "<xml>" , 5, MSG_NOSIGNAL) == -1) { 
			continue;
		}
		has_connection = true;
		signal_t = true; 
	} 
	printf ( "Interupted\n" );
	close(new_fd); 
	return (void *)0;
} 

void * listen(void * pm)
// the thread listens who connects to the relay, record its ip address and the ip and asn it requests
{
	
	int sockfd, numbytes; 
	struct sockaddr_in server_addr; 
	struct sockaddr_in client_addr; 
	int sin_size; 
	if ( ( sockfd = socket ( AF_INET , SOCK_STREAM , 0) ) == - 1) { 
		printf ( "socket error\n" ) ;
	} 
	memset ( & client_addr, 0, sizeof ( struct sockaddr ) ) ; 
	server_addr. sin_family = AF_INET ; 
	server_addr. sin_port = htons ( SERVERPORT) ; 
	server_addr. sin_addr. s_addr = INADDR_ANY ; 
	if ( bind ( sockfd, ( struct sockaddr * ) & server_addr, sizeof ( struct sockaddr ) ) == - 1) { 
		printf ( "bind error\n" ) ; 
	} 
	if ( listen ( sockfd, MAXCONN_NUM) == - 1) { 
		printf ( "listen error\n" ) ; 
	} 
	int ret=0;
	pthread_t id1=0;
	while ( 1) { 
		sin_size = sizeof ( struct sockaddr_in ) ; 
		if ( ( new_fd = accept ( sockfd, ( struct sockaddr * ) &client_addr, (socklen_t*)&sin_size) ) == -1) { 
			printf ( "accept error\n" ) ; 
			continue ; 
		}
		printf ( "server: got connection from %s\n" , inet_ntoa( client_addr.sin_addr) ) ;
		
		signal_t = false;
		sprintf(SERVERIP,"%s",inet_ntoa( client_addr.sin_addr));
		SERVERIP[strlen(inet_ntoa( client_addr.sin_addr))] = '\0';

		if(id1!=0)
			pthread_cancel(id1);
		ret = pthread_create(&id1, NULL, receive_requirments, NULL);
		if (ret)
		{
			printf("Create pthread error!/n");
			exit(-1);
		}
	} 
	return (void *)0; 
}


void initialize()
{
	signal_t = false;
	has_connection = false;
	memset(SERVERIP,0,128);
	memset(ip,0,128);
	memset(asn,0,128);
	memset(hostname,0,128);
	memset(port,0,128);
}

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
				if (isprint (optopt))
					printf ("Unknown option `-%c'.\n", optopt);
				else
					printf ("Unknown option character `\\x%x'.\n",
							optopt);
			 	exit(-1);
			case ':':
				printf("Missing argument for '-%c'.\n",optopt);
				break;
				exit(-1);
			default:
			 	abort ();
		}
	}
}

void create_threads()
{
	int i=0, ret=0;
	pthread_t id1,id2;
	
	ret = pthread_create(&id1, NULL, connect, NULL);
	if (ret)
	{
		printf("Create pthread error!/n");
		exit(-1);
	}

	ret = pthread_create(&id2, NULL, listen, NULL);
	if (ret)
	{
		printf("Create pthread error!/n");
		exit(-1);
	}
	
	pthread_join(id1, NULL);
	pthread_join(id2, NULL);
}
int main(int argc, char *argv[])
{
	
	signal(SIGINT,catch_int);
	initialize();
	get_opt(argc,argv);
	create_threads();
	return 0;
}

