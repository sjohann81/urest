#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "urest.h"
 
#define BUFLEN			1024
#define UDP_TIMEOUT_SEC		0			/* socket timeout (in sec) */
#define UDP_TIMEOUT_USEC	500000			/* socket timeout (in usec) */

struct socket_ctx_s {
	struct sockaddr_in si_other;
	int s, slen, recv_len;
	struct timeval tv;
};

void clnt_packet_handler(void *arg, char *data, uint16_t send_size, uint16_t *recv_size)
{
	struct socket_ctx_s *sock = (struct socket_ctx_s *)arg;
	
	/* send the message */
	if (sendto(sock->s, data, send_size, 0, (struct sockaddr *)&sock->si_other, sizeof(struct sockaddr_in)) == -1)
		printf("error sending data.\n");

	memset(data,'\0', BUFLEN);
	/* try to receive some data, this is a blocking call (with timeout!) */
	if ((sock->recv_len = recvfrom(sock->s, data, BUFLEN, 0, (struct sockaddr *)&sock->si_other, (socklen_t *)&sock->slen)) == -1) {
		*recv_size = 0;
		
		return;
	} else {
		*recv_size = sock->recv_len;
	}
}

int main(int argc, char **argv)
{
	struct socket_ctx_s sock;
	char req[BUFLEN], resp[BUFLEN];
	char packet[BUFLEN];
	int err;
	
	if (argc != 3) {
		printf("Usage: %s <server ip> <port>\n", argv[0]);
		
		return -1;
	}
	
	sock.tv.tv_sec = UDP_TIMEOUT_SEC;
	sock.tv.tv_usec = UDP_TIMEOUT_USEC;
 
	if ((sock.s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		printf("error creating socket.\n");
		
		return -1;
	}

	memset((char *) &sock.si_other, 0, sizeof(sock.si_other));
	sock.si_other.sin_family = AF_INET;
	sock.si_other.sin_port = htons(atoi(argv[2]));
	
	if (setsockopt(sock.s, SOL_SOCKET, SO_RCVTIMEO, &sock.tv, sizeof(sock.tv)) < 0) {
		printf("error setting socket.\n");
		
		return -1;
	}

	if (inet_aton(argv[1], &sock.si_other.sin_addr) == 0) {
		printf("error converting network address.\n");
		
		return -1;
	}


	struct clnt_packet_s socket;
	
	socket.packet_arg = &sock;
	socket.packet = packet;
	socket.packet_handler = clnt_packet_handler;
	
	struct server_s *server1, *server2;
	
	server1 = urest_link(&socket, argv[1], atoi(argv[2]), FRAG_SIZE_128);
	server2 = urest_link(&socket, argv[1], atoi(argv[2]), FRAG_SIZE_32);
 
	while (1) {
		strcpy(req, "/lights/light1");
		err = urest_get(server1, req, resp, BUFLEN);
		if (err) {
			printf("status: %d, resp: %s\n", err, resp);
		}
		sleep(1);
		
		strcpy(req, "/lights/light1");
		err = urest_put(server1, req, resp, BUFLEN);
		if (err) {
			printf("status: %d, resp: %s\n", err, resp);
		}
		sleep(1);
		
		strcpy(req, "/lights/light2?value:1");
		err = urest_get(server2, req, resp, BUFLEN);
		if (err) {
			printf("status: %d, resp: %s\n", err, resp);
		}
		sleep(1);
		
		strcpy(req, "/lights/light2?value:1:status:444");
		err = urest_get(server2, req, resp, BUFLEN);
		if (err) {
			printf("status: %d, resp: %s\n", err, resp);
		}
		sleep(1);
		
		strcpy(req, "/lights/light2?value:NBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQNBSWY3DPEB3W64TMMQQQ");
		err = urest_get(server2, req, resp, BUFLEN);
		if (err) {
			printf("status: %d, resp: %s\n", err, resp);
		}
		sleep(1);
	}

	close(sock.s);
	
	return 0;
}
