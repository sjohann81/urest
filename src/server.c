#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "urest.h"
 
#define BUFLEN			1024
#define UDP_TIMEOUT_SEC		0			/* socket timeout (in sec) */
#define UDP_TIMEOUT_USEC	100000			/* socket timeout (in usec) */

struct socket_ctx_s {
	struct sockaddr_in si_me, si_other;
	int s, slen, recv_len;
	struct timeval tv;
};

void serv_packet_recv(void *arg, char *data, uint16_t *size)
{
	struct socket_ctx_s *sock = (struct socket_ctx_s *)arg;
	
	memset(data, 0, BUFLEN);
 
	/* try to receive some data, this is a blocking call */
	if ((sock->recv_len = recvfrom(sock->s, data, BUFLEN, 0, (struct sockaddr *)&sock->si_other, (socklen_t *)&sock->slen)) == -1) {
		*size = 0;
		
		return;
	} else {
		*size = sock->recv_len;
	}

	/* print details of the client/peer and the data received */
/*	printf("Received data from %s:%d\n", inet_ntoa(sock->si_other.sin_addr), ntohs(sock->si_other.sin_port));*/
}

void serv_packet_send(void *arg, char *data, uint16_t size)
{
	struct socket_ctx_s *sock = (struct socket_ctx_s *)arg;
	
	if (sendto(sock->s, data, size, 0, (struct sockaddr *) &sock->si_other, sock->slen) == -1)
		printf("error sending data.\n");
		
}


void light1_get(void *arg)
{
	int len;
	
	len = strlen((char *)arg);
	
	printf("light 1 GET len %d: %s\n", len, (char *)arg);
}

void light1_put(void *arg)
{
	int len;
	
	len = strlen((char *)arg);
	
	printf("light 1 PUT len %d: %s\n", len, (char *)arg);
	strcpy((char *)arg, "value updated!");
}

void light2_get(void *arg)
{
	int len;
	
	len = strlen((char *)arg);
	
	printf("light 2 GET len %d: %s\n", len, (char *)arg);
}

void light2_put(void *arg)
{
	int len;
	
	len = strlen((char *)arg);
	
	printf("light 2 PUT len %d: %s\n", len, (char *)arg);
}


int main(int argc, char **argv)
{
	struct socket_ctx_s sock;
	char packet[BUFLEN];
	int err;
	
	if (argc != 2) {
		printf("Usage: %s <port>\n", argv[0]);
		
		return -1;
	}

	sock.tv.tv_sec = UDP_TIMEOUT_SEC;
	sock.tv.tv_usec = UDP_TIMEOUT_USEC;

	/* create a UDP socket */
	if ((sock.s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		printf("error creating socket.\n");
		
		return -1;
	}
	
	if (setsockopt(sock.s, SOL_SOCKET, SO_RCVTIMEO, &sock.tv, sizeof(sock.tv)) < 0) {
		printf("error setting socket.\n");
		
		return -1;
	}

	sock.slen = sizeof(sock.si_other);
	
	/* zero out the structure */
	memset((char *) &sock.si_me, 0, sizeof(sock.si_me));
     
	sock.si_me.sin_family = AF_INET;
	sock.si_me.sin_port = htons(atoi(argv[1]));
	sock.si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
	/* bind socket to port */
	if (bind(sock.s, (struct sockaddr *)&sock.si_me, sizeof(sock.si_me)) == -1) {
		printf("error binding to socket.\n");
		
		return -1;
	}


	struct serv_packet_s socket;
	
	socket.packet_arg = &sock;
	socket.packet = packet;
	socket.packet_handler_recv = serv_packet_recv;
	socket.packet_handler_send = serv_packet_send;


	struct resource_list_s *list;
	
	list = urest_resource_list();

	
	struct resource_s *resource1, *resource2;
	resource1 = urest_resource_endpoint("light 1", "/lights/light1");
	resource2 = urest_resource_endpoint("light 2", "/lights/light2");

	urest_resource_handler(resource1, light1_get, GET);
	urest_resource_handler(resource1, light1_put, PUT);
	urest_resource_handler(resource2, light2_get, GET);
	urest_resource_handler(resource2, light2_put, PUT);
	
	urest_register_resource(list, resource1);
	urest_register_resource(list, resource2);
	
	/* keep listening for data */
	while (1) {
		err = urest_handle_request(&socket, list);
		if (err) {
			printf("WARNING: urest_handle_request() exited with error code %d\n", err);
		}
	}

	close(sock.s);
	
	return 0;
}
