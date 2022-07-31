#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "urest.h"


struct resource_list_s *urest_resource_list(void)
{
	struct resource_list_s *list;
	
	list = malloc(sizeof(struct resource_list_s));	

	if (!list)
		return 0;
		
	list->next = 0;
	
	return list;
}

struct resource_s *urest_resource_endpoint(char *name, char *uri)
{
	struct resource_s *resource;
	
	resource = malloc(sizeof(struct resource_s));
	
	if (!resource)
		return 0;
		
	resource->endpoint_name = malloc(strlen(name) + 2);
	
	if (!resource->endpoint_name) {
		free(resource);
		
		return 0;
	}
	
	resource->endpoint_uri = malloc(strlen(uri) + 2);
	
	if (!resource->endpoint_uri) {
		free(resource->endpoint_name);
		free(resource);
		
		return 0;
	}

	strcpy(resource->endpoint_name, name);
	strcpy(resource->endpoint_uri, uri);
	resource->handler_get = 0;
	resource->handler_post = 0;
	resource->handler_put = 0;
	resource->handler_delete = 0;
	
	return resource;
}

int urest_resource_handler(struct resource_s *resource, void (*handler)(void *), uint8_t method)
{
	switch (method) {
	case GET:
		resource->handler_get = handler;
		break;
	case POST:
		resource->handler_post = handler;
		break;
	case PUT:
		resource->handler_put = handler;
		break;
	case DELETE:
		resource->handler_delete = handler;
		break;
	default:
		return -1;
	}
	
	return 0;
}

int urest_register_resource(struct resource_list_s *resource_list, struct resource_s *resource)
{
	struct resource_list_s *node = resource_list, *new_node;
	
	new_node = malloc(sizeof(struct resource_list_s));
	
	if (!new_node)
		return -1;
		
	while (node->next)
		node = node->next;
		
	node->resource = resource;
	node->next = new_node;
	node->next->next = 0;
	
	return 0;
}


static void send_ack(struct serv_packet_s *serv_packet, uint8_t major, uint8_t minor, uint16_t size)
{
	struct urest_s *header = (struct urest_s *)serv_packet->packet;
	
	header->msg_type = ACK;
	header->mtd_major = major;
	header->mtd_minor = minor;
	serv_packet->packet_handler_send(serv_packet->packet_arg, serv_packet->packet, sizeof(struct urest_s) + size);
}

int urest_handle_request(struct serv_packet_s *serv_packet, struct resource_list_s *resource_list)
{
	char buf[sizeof(struct urest_s) + UREST_REQ_BUF_SIZE];
	struct urest_s *header = (struct urest_s *)serv_packet->packet;
	struct urest_s *request = (struct urest_s *)buf;
	struct resource_list_s *node = resource_list;
	uint16_t pkt_len, data_len, payload_size, seq = 0, seq_ack = 0, retries = 0;
	char *uri_param;
	
	do {
		/* try to receive some data, this is a blocking call */
		serv_packet->packet_handler_recv(serv_packet->packet_arg, serv_packet->packet, &pkt_len);
		data_len = strnlen(serv_packet->packet + sizeof(struct urest_s), pkt_len);
		
		/* no data (socket timeout) */
		if (data_len == 0)
			return 0;

		/* decode payload size for fragments in this message */
		if (seq == 0) {
			switch (header->frag_size) {
			case FRAG_SIZE_16: payload_size = 16 - sizeof(struct urest_s); break;
			case FRAG_SIZE_32: payload_size = 32 - sizeof(struct urest_s); break;
			case FRAG_SIZE_64: payload_size = 64 - sizeof(struct urest_s); break;
			case FRAG_SIZE_128: payload_size = 128 - sizeof(struct urest_s); break;
			case FRAG_SIZE_256: payload_size = 256 - sizeof(struct urest_s); break;
			case FRAG_SIZE_512: payload_size = 512 - sizeof(struct urest_s); break;
			case FRAG_SIZE_1024: payload_size = 1024 - sizeof(struct urest_s); break;
			default:
				return UNKNOWN_FRAGMENT_SIZE;
			}
		}
			
		if (ntohs(header->seq) != seq) {
			if (++retries < UREST_RETRIES) {
				/* status 503 */
				/* send_ack(serv_packet, SERV_ERROR, SERVICE_UNAVAILABLE); */
				continue;
			} else
				return SEQUENCE_MISMATCH;
		}
			
		if (seq > 0 && header->tkn != request->tkn)
			return WRONG_TOKEN;

		/* copy data to processing buffer */
		if (sizeof(struct urest_s) + (seq + 1) * payload_size < sizeof(struct urest_s) + UREST_REQ_BUF_SIZE) {
			if (seq == 0) {
				header->tkn = htons(random() % 0xffff);
				memcpy(buf, serv_packet->packet, payload_size + sizeof(struct urest_s));
			} else {
				if (header->frag_size != request->frag_size)
					return FRAGMENT_SIZE_MISMATCH;
				
				header->tkn = request->tkn;
				memcpy(buf + sizeof(struct urest_s) + seq * payload_size, serv_packet->packet + sizeof(struct urest_s), payload_size);
			}
		} else {
			/* status 414 */
			send_ack(serv_packet, CLNT_ERROR, TOO_LONG, 0);
			
			return 0;
		}

/*		printf("[rcv] seq %d data %zu\n", seq, strnlen(serv_packet->packet + sizeof(struct urest_s), payload_size)); */

		/* send an ACK (continue or accepted) */
		if (data_len == payload_size)
			send_ack(serv_packet, INFO, CONTINUE, 0);

		seq++;
	} while (data_len == payload_size);
		
	if (request->msg_type == REQ) {
		if (request->mtd_major == VERB && request->cnt_type == FLAT_ENC) {
			while (node->next) {
				uri_param = strstr((const char *)request + sizeof(struct urest_s), "?");
				if (uri_param) {
					*uri_param = '\0';
					if (strcmp(node->resource->endpoint_uri, (const char *)request + sizeof(struct urest_s)) == 0) {
						*uri_param = '?';
						
						break;
					}
					*uri_param = '?';
				} else {
					if (strcmp(node->resource->endpoint_uri, (const char *)request + sizeof(struct urest_s)) == 0)
						break;
				}
				
				node = node->next;
			}
			
			/* status 404 */
			if (!node->next) {
				send_ack(serv_packet, CLNT_ERROR, NOT_FOUND, 0);
			
				return 0;
			}
			
			switch (request->mtd_minor) {
			case GET:
				if (node->resource->handler_get) {
					send_ack(serv_packet, INFO, PROCESSING, 0);
					node->resource->handler_get((char *)request + sizeof(struct urest_s));
					break;
				} else {
					/* status 405 */
					send_ack(serv_packet, CLNT_ERROR, NOT_ALLOWED, 0);
			
					return 0;
				}
			case POST:
				if (node->resource->handler_post) {
					send_ack(serv_packet, INFO, PROCESSING, 0);
					node->resource->handler_post((char *)request + sizeof(struct urest_s));
					break;
				} else {
					/* status 405 */
					send_ack(serv_packet, CLNT_ERROR, NOT_ALLOWED, 0);
			
					return 0;
				}
			case PUT:
				if (node->resource->handler_put) {
					send_ack(serv_packet, INFO, PROCESSING, 0);
					node->resource->handler_put((char *)request + sizeof(struct urest_s));
					break;
				} else {
					/* status 405 */
					send_ack(serv_packet, CLNT_ERROR, NOT_ALLOWED, 0);
			
					return 0;
				}
			case DELETE:
				if (node->resource->handler_get) {
					send_ack(serv_packet, INFO, PROCESSING, 0);
					node->resource->handler_delete((char *)request + sizeof(struct urest_s));
					break;
				} else {
					/* status 405 */
					send_ack(serv_packet, CLNT_ERROR, NOT_ALLOWED, 0);
			
					return 0;
				}
			case PINGREQ:
				break;
			default:
				/* status 501 */
				send_ack(serv_packet, CLNT_ERROR, NOT_ALLOWED, 0);
			
				return 0;
			}
		} else {
			/* status 406 */
			send_ack(serv_packet, CLNT_ERROR, NOT_ACCEPTABLE, 0);
		
			return 0;
		}
	} else {
		/* status 400 */
		send_ack(serv_packet, CLNT_ERROR, BAD_REQUEST, 0);
			
		return 0;
	}
	
	data_len = strnlen(buf + sizeof(struct urest_s), sizeof(struct urest_s) + UREST_REQ_BUF_SIZE);
	
	do {
		/* try to receive some data, this is a blocking call */
		serv_packet->packet_handler_recv(serv_packet->packet_arg, serv_packet->packet, &pkt_len);
			
		if (ntohs(header->seq) != seq) {
			if (++retries < UREST_RETRIES) {
				/* status 503 */
				/* send_ack(serv_packet, SERV_ERROR, SERVICE_UNAVAILABLE); */
				continue;
			} else
				return SEQUENCE_MISMATCH;
		}

		if (header->tkn != request->tkn)
			return WRONG_TOKEN;

		if (header->frag_size != request->frag_size)
			return FRAGMENT_SIZE_MISMATCH;
				
		header->tkn = request->tkn;
		memcpy(serv_packet->packet + sizeof(struct urest_s), buf + sizeof(struct urest_s) + seq_ack * payload_size, payload_size);
		pkt_len = strnlen(buf + sizeof(struct urest_s) + seq_ack * payload_size, payload_size);
		
		/* send an ACK (continue or accepted) */
		if (pkt_len == payload_size)
			send_ack(serv_packet, INFO, CONTINUE, pkt_len);

		seq++;
		seq_ack++;
	} while (seq_ack - 1 < data_len / payload_size);
	
	send_ack(serv_packet, SUCCESS, OK, pkt_len);
	
	return 0;
}



struct server_s *urest_link(struct clnt_packet_s *clnt_packet, char *ip, uint16_t port, uint8_t frag_size)
{
	struct server_s *server;
	
	server = malloc(sizeof(struct server_s));
	
	if (!server)
		return 0;
		
	server->ip = malloc(strlen(ip) + 2);
	
	if (!server->ip) {
		free(server);
		
		return 0;
	}

	strcpy(server->ip, ip);
	server->port = port;
	server->packet_drv = clnt_packet;
	server->frag_size = frag_size;
	
	return server;
}


static int send_data(struct server_s *server, uint8_t method, char *data, uint16_t *seq_val, uint16_t *token)
{
	char buf[sizeof(struct urest_s)];
	struct urest_s *header = (struct urest_s *)server->packet_drv->packet;
	struct urest_s *request = (struct urest_s *)buf;
	uint16_t seq = 0;
	uint16_t pkt_len, data_len, payload_size, size;
	
	data_len = strlen(data) + 1;
	*token = 0;
	
	switch (server->frag_size) {
	case FRAG_SIZE_16: payload_size = 16 - sizeof(struct urest_s); break;
	case FRAG_SIZE_32: payload_size = 32 - sizeof(struct urest_s); break;
	case FRAG_SIZE_64: payload_size = 64 - sizeof(struct urest_s); break;
	case FRAG_SIZE_128: payload_size = 128 - sizeof(struct urest_s); break;
	case FRAG_SIZE_256: payload_size = 256 - sizeof(struct urest_s); break;
	case FRAG_SIZE_512: payload_size = 512 - sizeof(struct urest_s); break;
	case FRAG_SIZE_1024: payload_size = 1024 - sizeof(struct urest_s); break;
	default:
		return UNKNOWN_FRAGMENT_SIZE;
	}
	
	do {
		header->frag_size = server->frag_size;
		header->msg_type = REQ;
		header->cnt_type = FLAT_ENC;
		header->mtd_major = VERB;
		header->mtd_minor = method;
		header->seq = htons(seq);

		if (seq == 0) {
			header->tkn = htons(0);
			*request = *header;
		}
		
		if (seq == 1) {
			request->tkn = header->tkn;
		}

		if (seq == data_len / payload_size)
			size = data_len % payload_size;
		else
			size = payload_size;

		memcpy(server->packet_drv->packet + sizeof(struct urest_s), data + seq * payload_size, size);

		/* send a REQ packet and wait for an ACK... */
		server->packet_drv->packet_handler(server->packet_drv->packet_arg, server->packet_drv->packet, sizeof(struct urest_s) + size, &pkt_len);
		
		if (pkt_len == 0)
			return REQUEST_FAILED;

		if (header->frag_size != request->frag_size)
			return FRAGMENT_SIZE_MISMATCH;

		if (ntohs(header->seq) != seq)
			return SEQUENCE_MISMATCH;

		if (seq > 0 && header->tkn != request->tkn)
			return WRONG_TOKEN;

		if (header->mtd_major == INFO) {
/*			if (header->mtd_minor == CONTINUE)
				printf("CONTINUE, token 0x%04x\n", ntohs(header->tkn));
			else {
				printf("PROCESSING, token 0x%04x\n", ntohs(header->tkn));
				break;
			}
*/
			if (header->mtd_minor == PROCESSING)
				break;
		} else {
			return header->mtd_major * 100 + header->mtd_minor;
		}

		seq++;
	} while (seq - 1 < data_len / payload_size);
	
	if (header->mtd_major == INFO) {
		if (header->mtd_minor != PROCESSING)
			return header->mtd_major * 100 + header->mtd_minor;
	} else {
		return header->mtd_major * 100 + header->mtd_minor;
	}
	
	*seq_val = seq + 1;
	*token = request->tkn;
	
	return 0;
}

static int recv_data(struct server_s *server, uint8_t method, char *response, uint16_t buflen, uint16_t *seq_val, uint16_t *token)
{
	struct urest_s *header = (struct urest_s *)server->packet_drv->packet;
	uint16_t seq = *seq_val, seq_ack = 0;
	uint16_t pkt_len, payload_size;
	
	do {
		header->frag_size = server->frag_size;
		header->msg_type = REQ;
		header->cnt_type = FLAT_ENC;
		header->mtd_major = VERB;
		header->mtd_minor = method;
		header->seq = htons(seq);

		/* send a REQ packet and wait for an ACK... */
		server->packet_drv->packet_handler(server->packet_drv->packet_arg, server->packet_drv->packet, sizeof(struct urest_s), &pkt_len);
		
		switch (server->frag_size) {
		case FRAG_SIZE_16: payload_size = 16 - sizeof(struct urest_s); break;
		case FRAG_SIZE_32: payload_size = 32 - sizeof(struct urest_s); break;
		case FRAG_SIZE_64: payload_size = 64 - sizeof(struct urest_s); break;
		case FRAG_SIZE_128: payload_size = 128 - sizeof(struct urest_s); break;
		case FRAG_SIZE_256: payload_size = 256 - sizeof(struct urest_s); break;
		case FRAG_SIZE_512: payload_size = 512 - sizeof(struct urest_s); break;
		case FRAG_SIZE_1024: payload_size = 1024 - sizeof(struct urest_s); break;
		default:
			return UNKNOWN_FRAGMENT_SIZE;
		}
		
		/* copy data to processing buffer */
		if ((seq_ack + 1) * payload_size < buflen) {
			memcpy(response + seq_ack * payload_size, server->packet_drv->packet + sizeof(struct urest_s), payload_size);	
		}

		if (ntohs(header->seq) != seq)
			return SEQUENCE_MISMATCH;

/*		if (header->mtd_major == INFO) {
			if (header->mtd_minor == CONTINUE)
				printf("CONTINUE, token 0x%04x\n", ntohs(header->tkn));
		}
*/		
		seq++;
		seq_ack++;
	} while (pkt_len - sizeof(struct urest_s) == payload_size);
	
	return header->mtd_major * 100 + header->mtd_minor;
}


int urest_get(struct server_s *server, char *data, char *response, uint16_t buflen)
{
	int status;
	uint16_t seq = 0, token = 0;
	
	status = send_data(server, GET, data, &seq, &token);

	if (status)	
		return status;
		
	status = recv_data(server, GET, response, buflen, &seq, &token);
	
	return status;
}

int urest_post(struct server_s *server, char *data, char *response, uint16_t buflen)
{
	int status;
	uint16_t seq = 0, token = 0;
	
	status = send_data(server, POST, data, &seq, &token);

	if (status)	
		return status;
		
	status = recv_data(server, POST, response, buflen, &seq, &token);
	
	return status;
}

int urest_put(struct server_s *server, char *data, char *response, uint16_t buflen)
{
	int status;
	uint16_t seq = 0, token = 0;
	
	status = send_data(server, PUT, data, &seq, &token);

	if (status)	
		return status;
		
	status = recv_data(server, PUT, response, buflen, &seq, &token);
	
	return status;
}

int urest_delete(struct server_s *server, char *data, char *response, uint16_t buflen)
{
	int status;
	uint16_t seq = 0, token = 0;
	
	status = send_data(server, DELETE, data, &seq, &token);

	if (status)	
		return status;
		
	status = recv_data(server, DELETE, response, buflen, &seq, &token);
	
	return status;
}
