# uREST - A simple protocol for REST operations over UDP

The protocol presented in this document is based on a request/response model, and can be seen as a reduced and much simplified version of the Constrained Application Protocol (CoAP). Some concepts are similar to CoAP, although the protocol is not directly based on it. Applications of this protocol include systems where memory and communication bandwidth is are strictly limited resources, such as sensor networks, home appliances and machine-to-machine (M2M) communication. In this protocol, a client/server model is used and entities are referenced as *initiator* and *responder* respectively.

Usage of an IP based stack as the underlying transport mechanism is not mandatory, and this can reduce even more the resources needed for an application. For example, some simple RF transducers already define a frame format consisting of source and destination addresses, along with error checking for the payload. In this case, an UDP header consisting of source and destination ports, payload size and CRC can be transferred directly over frames, and used as a transport layer for uREST messages.


## 1 - Transaction actors

A transaction is defined as a complete transfer of data between two parties, including data from a request operation and data from a corresponding response operation. In this protocol, a transaction occours between an *initiator* and a *responder*.

### 1.1 - Initiator

An initiator is the client of a transaction (origin on a request, destination on a response). Verbs/methods similar to HTTP are used. A transaction may be composed of several messages and is coordinated by the initiator. The initiator defines the flow of data and retransmissions, acting as a master and defining requests which may be composed by one or more messages.


### 1.2 - Responder

A responder is the server of a transaction (destination on a request, origin on a response). Response codes similar to HTTP are used. The responder acts as a slave of the initiator, accepting requests or partial requests from it. A response may be composed of several messages, and the flow of data is coordinated by the initiator.


## 2 - URIs and resources

### 2.1 - URI scheme

URIs are encoded with three main components: the protocol name, server and port identification and resource path. For example: *'urest://server:port/path/to/the/resource'*. The first two parts are used to identify the protocol and host acting as the responder. The last part is transferred by the initiator on the payload of the first request using the syntax *{"uri": "/path/to/the/resource"}*.

### 2.2 - Resources

Resources in this protocol are identified by a URI format which locates the resource in a hierarchical structure of resources. The responder is identified by the underlying addressing scheme employed in the network layer, including an address or host name and a UDP port. A resource in this responder is identified with an absolute path to the resource, encoded as a parameter on the initiator payload, represented in JSON or uREST format.

On a request method, data should start with an individual object, composed by a 'uri' field (JSON) or a string field (uREST) containing the relative path to the resource in the server, followed by additional data. For both requests and responses, data is encoded in aditional fields, containing the application specific format. Traditional URI encoded fields (using something like '/urlencoded?firstname=sid&lastname=sloth') are not supported.

#### 2.2.1 - Examples:

Request:

- JSON encoded -> {{"uri": "/path/to/resource"}, { ... data ... }}
- uREST enconded -> {{s:%%/path/to/resource%%},{ ... data ... }}

Response:

- { ... data ... }



## 3 - Transmission parameters

- ACK_TIMEOUT: 2 s
- MAX_RETRANSMIT: 3

Transmission is performed by single messages (unsolicited / non-confirmable) or request/response pairs. Retransmission is performed using timeout and exponential back-off. A transmission is controlled by the initiator following a two-message protocol:

- Initiator performs a request (REQ) operation (or partial request)
- Responder acknowledges the reception with an (ACK) operation (without any content) or acknowledges and returns data.
- Initiator continues transferring or retrieving additional data (REQ), followed responder confirmations or transfers (ACK).

Data should only be returned from the responder after a request is completed. For example, on a request composed of multiple messages, the reponder acknowledges each message with no content but the last message, where data starts to be transfered. On a response composed of multiple message, the initiator transmits messages with no content until completion of the transfer from the responder.

Unsolicited messages are used for non-confirmable requests or responses and no retransmission should be performed, as an acknowledge is not expected.


## 4 - Message format

A message is composed by an 8 byte header followed by a payload. A transaction can be split in multiple messages, and each message must be transmitted in a UDP datagram. Maximum UDP payload considered in this specification is 512 bytes. On large transactions, multiple messages are needed (series of REQs and ACKs) if the last transmitted message payload is full (506 bytes). Unsolicited messages are limited to a single, individual message without any confirmation.

### 4.1 - Message header

- Token / transaction ID (16 bits)
- Sequence number (16 bits)
- Type (UNS, REQ, ACK, RST) (2 bits)
- Method / response (6 bits - 2 bits for major code, 4 bits for minor code)
- Options (5 bits)
- Content type (3 bits - type)
- Payload (506 bytes)


## 5 - Message types, tokens and sequence numbers

### 5.1 - Message types

There are four different types of messages described in this specification. Unsolicited messages are used as a one way communication, and can be used as both non-confirmable requests or unsolicited responses. Request and response messages are used for coordinated transfers for both requests and responses. Reset messages are used on failures or abort operations.

- UNS - unsolicited - used for both non-confirmable or unsolicited requests / responses.
- REQ - request
- ACK - response (acknowledge)
- RST - reset

### 5.2 - Tokens and sequence numbers

The initiator is responsible for the control of the flow of data, and must use the same token from the first acknowledge for the rest of a transaction. An initial token composed of zeroes is used in the first message from the initiator, and a responder must acknowledge the reception of data of a new transaction with a new token. This token must remain the same during the entire transaction (sequence of request/response packets). Request and response messages are considered pairs, so a response uses the same sequence number of a matching request, except the first message.

Unsolicited messages use a token composed of zeroes as well as sequence number of zero.


## 6 - Transactions

A transaction is composed by a single unsolicited message, which can be either a request or a response, or a sequence of REQ and ACK messages. In a sequence of messages, it is up to the initiator to retry on cases where data loss occour (detected by ACK_TIMEOUT and exponential back-off). If an initiator or a responder receive an unexpected sequence number or token, a reset (RST) message should be sent and the transaction aborted.

	initiator               responder               initiator               responder
	|                       |                       |                       |
	|   REQ (seq 0, 506B)   | 0.01 (GET)            |   REQ (seq 0, 506B)   | 0.01 (GET)
	|---------------------->| token: 0x00000000     |---------------------->| token: 0x00000000
	|                       |                       |                       |
	|   ACK (seq 0, 0B)     | 2.06 (continue)       |   ACK (seq 0, 0B)     | 2.06 (continue)
	|<----------------------| token: 0xaabbccdd     |<----------------------| token: 0x55443322
	|                       |                       |                       |
	|   REQ (seq 1, 231B)   | 0.01 (GET)            |   REQ (seq 1, 231B)   | 0.01 (GET)
	|---------------------->| token: 0xaabbccdd     |---------------------->| token: 0x55443322
	|                       |                       |                       |
	|   ACK (seq 1, 0B)     | 2.02 (accepted)       |    (processing...)    |
	|<----------------------| token: 0xaabbccdd     |                       |
	|                       |                       |   ACK (seq 1, 125B)   | 2.00 (ok)
	|    (processing...)    |                       |<----------------------| token: 0x55443322
	|                       |                                  (b)
	|   REQ (seq 2, 0B)     | 0.01 (GET)
	|---------------------->| token: 0xaabbccdd
	|                       |                       initiator               responder
	|   ACK (seq 2, 506B)   | 2.06 (continue)       |                       |
	|<----------------------| token: 0xaabbccdd     |   REQ (seq 0, 170B)   | 0.01 (GET)
	|                       |                       |---------------------->| token: 0x00000000
	|   REQ (seq 3, 0B)     | 0.01 (GET)            |                       |
	|---------------------->| token: 0xaabbccdd     |    (processing...)    |
	|                       |                       |                       |
	|   ACK (seq 3, 50B)    | 2.00 (ok)             |   ACK (seq 0, 356B)   | 2.00 (ok)
	|<----------------------| token: 0xaabbccdd     |<----------------------| token: 0x69696969
	          (a)                                              (c)

### 6.1 - Piggybacked mode

Transfers can happen in piggybacked mode, where the responder includes the result of the processing (or at least, part of it) in an acknowledge message immediatelly following a request (examples 'b' and 'c'). The responder answers with a code 2.00 (ok) when no more content should be transferred or with a code 2.06 (continue), when more content should be transferred from the responder. Piggybacked mode can be used if processing time is small (under 1 second, or ACK_TIMEOUT/2), respecting the transmission parameters for the acknowledge timeout.

### 6.2 - Common mode

When processing time is greater than one second (or ACK_TIMEOUT/2) or no piggybacked mode is used, the last message from the initiator is confirmed with a code 2.02 (accepted), and it is up to the initiator to poll the responder periodically for data (example 'a'). If not ready, the responder must answer with a code 2.02 (accepted) and return no data, until it is ready. Then, it answers with a code 2.00 (ok) when no more content needs be transferred or with a code 2.06 (continue) when more content is available. If no data is returned and the response code is 2.02 (accepted), the initiator assumes the responder is processing, and in this case the sequence number must stay the same until data starts to be transfered from the responder. If only a single message is returned as a response (code 2.00), then the sequence number doesn't change.

### 6.3 - Concurrent transactions

Transactions can happen concurrently, and it is up to the responder to keep track of multiple transactions from different initiators. If a responder is resource constrained and can only keep track of a single transaction, or is currently overloaded, it should just ignore a request of a new transaction from the initiator. It is up to the initiator to interpret this as a message loss and perform a retransmission.


## 7 - Verbs / methods

- 0.01 - GET
	- success: 2.00
	- URI must be included in the request
- 0.02 - POST
	- success: 2.01 or 2.05
	- URI of the created object must be returned on 2.01 but not on 2.05 (not created, but changed)
- 0.03 - PUT
	- success: 2.05
- 0.04 - DELETE
	- success: 2.04


## 8 - Message fields

### 8.1 - Token

Tokens are generated on the responder side, as a confirmation of a new transaction. A token is a 32 bit integer, generated using a pseudo-random number generator with good distribution. This token should be used for the rest of the transaction, and can be used on the responder side to keep track of multiple transactions.

### 8.2 - Sequence number

A sequence number is a 16 bit integer generated on the initiator side, starting as zero. The sequence of requests and responses is matched in pairs, using the same sequence number. This field is used to control the sequence of operations on both sides, as well as to identify data loss, retransmissions and transaction reset on failures.

### 8.3 - Type

Type is a two bit field, encoding the four possible message types: '00' - unsolicited/non-confirmable (UNS), '01' - request (REQ), '10' - acknowledge (ACK) and '11' - reset (RST).

### 8.4 - Methods and response codes

Methods and response codes are enconded in 6 bits, decomposed in two fields: first two bits encode the verb/method/response major opcode and the last four encode the minor opcode. Minor opcode is encoded in the second field as four bits, following the equivalent binary representation. The meaning of first field two bits is: '00' - verb/method, '01' - success (codes 2.xx), '10' - client error (codes 4.xx) and '11' - server error (codes 5.xx).

#### 8.4.1 - Success

- 2.00 - ok / content
- 2.01 - created
- 2.02 - accepted
- 2.04 - no content / deleted
- 2.05 - changed
- 2.06 - continue

#### 8.4.2 - Client error

- 4.00 - bad request
- 4.01 - unauthorized
- 4.03 - forbidden
- 4.04 - not found
- 4.05 - method not allowed
- 4.06 - not acceptable

#### 8.4.3 - Server error

- 5.00 - internal server error
- 5.01 - not implemented
- 5.02 - bad gateway
- 5.03 - service unavailable
- 5.04 - gateway timeout


### 8.5 - Options

Reserved for future use.

### 8.6 - Content-type

The content-type field is used to describe the actual format of data encoded in the payload. Only three formats are presented in this specification, and other formats may be used in the future. This field is represented as three bits, and the meaning of these bits is: '001' - JSON encoded (type 1), '010' - urest encoded (type 2), '011' - raw base64 encoded (type 3) and '100' - raw (type 4). Other values ('000' and '101' to '111') are reserved for future use.

### 8.7 - Payload encoding

Payload encoding follows the type of content specified in the content-type header field. The most common encoding used is type 1 (json encoded) and this encoding scheme should be used at least on the first request, as a URI representing a resource must be present. The application of different formats is specified as:

- Type 1 (JSON encoded) - Common serialization format. If binary data is transfered in a field, it should be represented in base64 encoding.
- Type 2 (uREST encoded) - Basic serialization format, suitable for easy parsing of data.
- Type 3 (raw base64 encoded) - Binary data, plain text, represented using base64 encoding.
- Type 4 (raw) - Binary data, optimized for less overhead and used on large transfers.


## 9 - uREST encoding and data serialization

uREST encoding is defined by a simple serialization format, suitable for compact data representation. The format is organized for easy parsing of data, supporting several features which make it ideal for use on resource constrained systems. On applications where only the memory data layout is important, data structure delimiters can be simply ignored. Some features:

- Easy parsing of data
- Easy syntax, no scape codes
- Suitable for compact data representation
- Supports generic data types or alternatives for strongly typed representation of data
- Arrays / lists. Full records are separated by a new line (0x0a) character as a delimiter
- Supports data structures

### 9.1 - Basic types

- 'i' - integer
- 'f' - float
- 's' - string
- 'b' - blob / base64 encoded

### 9.2 - Delimiters

- '{ .. }' - data structure
- ':' - data type and value separator
- ',' - data item saparator
- ';' - list / array item separator
- '%% .. %%' - string

### 9.3 - Example

	struct s {
		int a;
		float b;
		char c[30];
		struct s_s {
			float d, e;
		};
		int f[3];
	};

	{i:1234,f:55.123,s:%%hello world%%,{f:75.5555,f:9.5},i:5093;-6467;253}

### 9.4 - Extended data types

Additional types, suitable for strongly typed applications:

- 'Sc' - Signed 8 bit integer (char type, int8_t)
- 'Uc' - Unsigned 8 bit integer (unsigned char type, uint8_t)
- 'Ss' - Signed 16 bit integer (short type, int16_t)
- 'Us' - Unsigned 16 bit integer (unsigned short type, uint16_t)
- 'Si' - Signed 32 bit integer (int type, int32_t)
- 'Ui' - Unsigned 32 bit integer (unsigned int type, uint32_t)
- 'Sl' - Signed 64 bit integer (long long type, int64_t)
- 'Ul' - Unsigned 64 bit integer (unsigned long long type, uint64_t)
- 'Sf' - Single precision floating-point (float type, float32_t)
- 'Df' - Double precision floating-point (double type, float64_t)

In this case, the exact representation of the third field presented in the example from the last structure (*char c[30]*) would become an array *Sc:104;101;108;108;111 ...* instead of *s:%%hello world%%*. If only part of an array is used, the remaining items should be filled with zeroes.


## 10 - Unicast / multicast

Transactions in this specification follow the client/server model, and are viewed as unicast operations. Multicast operations can be implemented only with unsolicited messages and as such, are unconfirmed by the recipients. Multicast delivery is a responsability of the lower layers of the network stack.
