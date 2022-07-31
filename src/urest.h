#define UREST_DEFAULT_PORT	4677
#define UREST_REQ_BUF_SIZE	4096
#define UREST_RETRIES		3

enum fragment_size {
	FRAG_SIZE_16 = 1,
	FRAG_SIZE_32,
	FRAG_SIZE_64,
	FRAG_SIZE_128,
	FRAG_SIZE_256,
	FRAG_SIZE_512,
	FRAG_SIZE_1024
};

enum message_type {
	UNS = 0,
	REQ,
	ACK,
	RST
};

enum content_type {
	JSON_ENC = 0,
	UREST_ENC,
	URI_ENC,
	FLAT_ENC
};

enum method_major {
	VERB = 0,
	INFO = 1,
	SUCCESS = 2,
	CLNT_ERROR = 4,
	SERV_ERROR = 5
};

enum method_minor_verb {
	GET = 1,
	POST,
	PUT,
	DELETE,
	PINGREQ = 31
};

enum method_minor_info {
	CONTINUE = 0,
	PROCESSING = 2,
	PINGACK = 31
};

enum method_minor_success {
	OK = 0,
	CREATED = 1,
	ACCEPTED = 2,
	DELETED = 4,
	RESET = 5
};

enum method_minor_clnt_error {
	BAD_REQUEST = 0,
	UNAUTHORIZED = 1,
	FORBIDDEN = 3,
	NOT_FOUND = 4,
	NOT_ALLOWED = 5,
	NOT_ACCEPTABLE = 6,
	REQ_TIMEOUT = 8,
	CONFLICT = 9,
	TOO_LONG = 14,
	TEAPOT = 18,
	UNPROCESSABLE = 22,
	LOCKED = 23,
	TOO_MANY = 29
};

enum method_minor_serv_error {
	INTERNAL_ERROR = 0,
	NOT_IMPLEMENTED,
	BAD_GATEWAY,
	SERVICE_UNAVAILABLE,
	GATEWAY_TIMEOUT
};

enum error_code {
	UNKNOWN_FRAGMENT_SIZE = -99,
	FRAGMENT_SIZE_MISMATCH,
	SEQUENCE_MISMATCH,
	WRONG_TOKEN,
	REQUEST_FAILED
};


struct urest_s {
	uint8_t frag_size : 3;
	uint8_t msg_type : 3;
	uint8_t cnt_type : 2;
	uint8_t mtd_major : 3;
	uint8_t mtd_minor : 5;
	uint16_t tkn;
	uint16_t seq;
};


/* server side */

struct serv_packet_s {
	void *packet_arg;
	char *packet;
	void (*packet_handler_recv)(void *, char *, uint16_t *);
	void (*packet_handler_send)(void *, char *, uint16_t);
};

struct resource_s {
	char *endpoint_name;
	char *endpoint_uri;
	void (*handler_get)(void *);
	void (*handler_post)(void *);
	void (*handler_put)(void *);
	void (*handler_delete)(void *);
};

struct resource_list_s {
	struct resource_list_s *next;
	struct resource_s *resource;
};

struct resource_list_s *urest_resource_list(void);
struct resource_s *urest_resource_endpoint(char *name, char *uri);
int urest_resource_handler(struct resource_s *resource, void (*handler)(void *), uint8_t method);
int urest_register_resource(struct resource_list_s *resource_list, struct resource_s *resource);
int urest_handle_request(struct serv_packet_s *serv_packet, struct resource_list_s *resource_list);


/* client side */

struct clnt_packet_s {
	void *packet_arg;
	char *packet;
	void (*packet_handler)(void *, char *, uint16_t, uint16_t *);
};

struct server_s {
	struct clnt_packet_s *packet_drv;
	char *ip;
	uint16_t port;
	uint8_t frag_size;
};

struct server_s *urest_link(struct clnt_packet_s *clnt_packet, char *ip, uint16_t port, uint8_t frag_size);
int urest_get(struct server_s *server, char *data, char *response, uint16_t buflen);
int urest_post(struct server_s *server, char *data, char *response, uint16_t buflen);
int urest_put(struct server_s *server, char *data, char *response, uint16_t buflen);
int urest_delete(struct server_s *server, char *data, char *response, uint16_t buflen);

int base32_encode(char *in, uint16_t len, char *out);
int base32_decode(char *in, uint16_t len, char *out);
