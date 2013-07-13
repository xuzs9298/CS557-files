#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main()
{
	char * ip = "192.168.278.1";
	unsigned long ip_new = inet_addr	(ip);
	printf("%lu\n",ip_new);	
}
