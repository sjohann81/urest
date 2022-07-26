# uREST - A simple protocol for REST operations on resource constrained devices

The protocol presented in this document is based on a request/response model, and can be seen as a reduced and much simplified version of the Constrained Application Protocol (CoAP). Some concepts are similar to CoAP, although the protocol is not directly based on it. Applications of this protocol include systems where memory and communication bandwidth is are strictly limited resources, such as sensor networks, microcontrollers, home appliances and machine-to-machine (M2M) communication. In this protocol, a client/server model is used and entities are referenced as *initiator* and *responder* respectively.

Usage of an IP based stack as the underlying transport mechanism is not mandatory, and this can reduce even more the resources needed for an application. For example, some simple RF transducers already define a frame format consisting of source and destination addresses, port / channel and error checking for the payload. In the case of an IP network, a simple UDP datagram can used as the transport layer for uREST messages. 


## 1 - Transaction actors

A transaction is defined as a complete transfer of data between two parties, including data from a request operation and data from a corresponding response operation. In this protocol, a transaction occours between an *initiator* and a *responder*.

### 1.1 - Initiator

An initiator is the client of a transaction (origin on a request, destination on a response). Verbs/methods similar to HTTP are used. A transaction may be composed of several messages and is coordinated by the initiator. The initiator defines the flow of data and retransmissions, acting as a master and defining requests which may be composed by one or more messages.


### 1.2 - Responder

A responder is the server of a transaction (destination on a request, origin on a response). Response codes similar to HTTP are used. The responder acts as a slave of the initiator, accepting requests or partial requests from it. A response may be composed of several messages, and the flow of data is coordinated by the initiator.


## 2 - URIs and resources

### 2.1 - URI scheme

URIs are encoded with three main components: the protocol name, machine and port identification and resource path. For example: *'urest://machine:port/path/to/the/resource'*. The first two parts are used to identify the protocol and host acting as the responder. The last part is transferred by the initiator on the payload of the request.

### 2.2 - Resources

Resources in this protocol are identified by a URI format which locates the resource in a hierarchical structure of resources. The responder is identified by the underlying addressing scheme employed in the network layer, including an address or host name and a UDP port. A resource in this responder is identified with an absolute path to the resource, encoded as a parameter on the initiator payload, represented in JSON, uREST, URI or flat format.

On a request method, data should start with an individual object, composed by a field containing the relative path to the resource in the server, followed by additional data. For both requests and responses, data is encoded in aditional fields, containing the application specific format.

#### 2.2.1 - Examples:

Request:

- JSON encoded -> {{"uri": "/path/to/resource"}, { ... data ... }}
- uREST encoded -> {s:{/path/to/resource}, ... data ...}
- URI encoded -> /path/to/resource?data1=123&data2=abc
- Flat encoded -> /path/to/resource?data1:123:data2:abc

Response:

- { ... data ... }
- /path/to/resource?key:value ...


## 3 - Transmission parameters

A transaction is decomposed in multiple fragments called messages. Messages are the basic encapsulation of both protocol information, data payload and a mechanism used for flow control and retransmissions.

- ACK_TIMEOUT: 2 s
- MAX_RETRANSMIT: 3

Transmission is performed by single messages (unsolicited / non-confirmable) or request/response pairs. Retransmission is performed using timeout and exponential back-off. A transmission is controlled by the initiator following a two-message protocol:

- Initiator performs a request (**REQ**) operation (or partial request)
- Responder acknowledges the reception with an (**ACK**) operation (without any content) or acknowledges and returns data.
- Initiator continues transferring or retrieving additional data (**REQ**), followed responder confirmations or data transfers (**ACK**).

Data should only be returned from the responder after a transaction request is completed. For example, on a request composed of multiple messages, the reponder acknowledges each message with no content but the last message, where data starts to be transfered (piggybacked mode). On common mode, data starts to be transferred only after the request is completely transferred to the responder. On a response composed of multiple messages, the initiator transmits messages with no content until completion of the transfer from the responder using acknowlege messages.

Unsolicited messages are used for non-confirmable requests or responses and no retransmission should be performed, as an acknowledge is not expected.


## 4 - Message format

A message is composed by a 6 byte header followed by a payload. A transaction can be split in multiple messages (or fragments), and each message must be transmitted in a single datagram / packet. Maximum message size considered in this specification is 1024 bytes. On large transactions, multiple messages are needed (series of REQs and ACKs) if the last transmitted message payload is full. Unsolicited messages are limited to a single, individual message without any confirmation.

### 4.1 - Message header

- Fragment size (3 bits)
- Type (UNS, REQ, ACK, RST) (3 bits) 
- Content type (2 bits)
- Method major code (3 bits)
- Method minor code (5 bits)
- Token / transaction ID (16 bits)
- Sequence number (16 bits)
- Payload


## 5 - Message types, tokens and sequence numbers

### 5.1 - Message types

There are four different types of messages described in this specification. Unsolicited messages are used as a one way communication, and can be used as both non-confirmable requests or unsolicited responses. Request and response messages are used for coordinated transfers for both requests and responses. Reset messages are used on failures or abort operations.

- UNS - unsolicited - used for both non-confirmable or unsolicited requests / responses.
- REQ - request
- ACK - response (acknowledge)
- RST - reset

### 5.2 - Tokens and sequence numbers

The initiator is responsible for the control of the flow of data, and must use the same token from the first acknowledge for the rest of a transaction. An initial token composed of zeroes is used in the first message from the initiator, and a responder must acknowledge the reception of data of a new transaction with a new token. This token must remain the same during the entire transaction (sequence of request/response packets). Request and response messages are considered pairs, so a response uses the same sequence number of a matching request.

Unsolicited messages use a token composed of zeroes as well as sequence number of zero.


## 6 - Transactions

A transaction is composed by a single unsolicited message, which can be either a request or a response, or a sequence of REQ and ACK messages. In a sequence of messages, it is up to the initiator to retry on cases where data loss occour (detected by ACK_TIMEOUT and exponential back-off). If an initiator or a responder receive an unexpected sequence number or token, a reset (RST) message should be sent and the transaction aborted.

	initiator               responder               initiator               responder
	|                       |                       |                       |
	|   REQ (seq 0, 506B)   | 0.01 (GET)            |   REQ (seq 0, 506B)   | 0.01 (GET)
	|---------------------->| token: 0x00000000     |---------------------->| token: 0x00000000
	|                       |                       |                       |
	|   ACK (seq 0, 0B)     | 1.00 (continue)       |   ACK (seq 0, 0B)     | 1.00 (continue)
	|<----------------------| token: 0xaabbccdd     |<----------------------| token: 0x55443322
	|                       |                       |                       |
	|   REQ (seq 1, 231B)   | 0.01 (GET)            |   REQ (seq 1, 231B)   | 0.01 (GET)
	|---------------------->| token: 0xaabbccdd     |---------------------->| token: 0x55443322
	|                       |                       |                       |
	|   ACK (seq 1, 0B)     | 1.02 (processing)     |    (processing...)    |
	|<----------------------| token: 0xaabbccdd     |                       |
	|                       |                       |   ACK (seq 1, 125B)   | 2.00 (ok)
	|    (processing...)    |                       |<----------------------| token: 0x55443322
	|                       |                                  (b)
	|   REQ (seq 2, 0B)     | 0.01 (GET)
	|---------------------->| token: 0xaabbccdd
	|                       |                       initiator               responder
	|   ACK (seq 2, 506B)   | 1.00 (continue)       |                       |
	|<----------------------| token: 0xaabbccdd     |   REQ (seq 0, 170B)   | 0.01 (GET)
	|                       |                       |---------------------->| token: 0x00000000
	|   REQ (seq 3, 0B)     | 0.01 (GET)            |                       |
	|---------------------->| token: 0xaabbccdd     |    (processing...)    |
	|                       |                       |                       |
	|   ACK (seq 3, 50B)    | 2.00 (ok)             |   ACK (seq 0, 356B)   | 2.00 (ok)
	|<----------------------| token: 0xaabbccdd     |<----------------------| token: 0x69696969
	          (a)                                              (c)

### 6.1 - Piggybacked mode

Transfers can happen in piggybacked mode, where the responder includes the result of the processing (or at least, part of it) in an acknowledge message immediatelly following a request (examples 'b' and 'c'). The responder answers with a code 2.00 (ok) when no more content should be transferred or with a code 1.00 (continue), when more content should be transferred to the initiator. Piggybacked mode can be used if processing time is small (under 1 second, or ACK_TIMEOUT/2), respecting the transmission parameters for the acknowledge timeout.

### 6.2 - Common mode

When processing time is greater than one second (or ACK_TIMEOUT/2) or no piggybacked mode is used, the last message from the initiator is confirmed with a code 1.02 (processing), and it is up to the initiator to poll the responder periodically for data (example 'a'). If not ready, the responder must answer with a code 2.02 (accepted) and return no data, until it is ready. Then, it answers with a code 2.00 (ok) when no more content needs be transferred or with a code 1.00 (continue) when more content is available. If no data is returned and the response code is 1.02 (processing), the initiator assumes the responder is processing, and in this case the sequence number must stay the same until data starts to be transfered from the responder. If only a single message is returned as a response (code 2.00), then the sequence number doesn't change.

### 6.3 - Concurrent transactions

Transactions can happen concurrently, and it is up to the responder to keep track of multiple transactions from different initiators. If a responder is resource constrained and can only keep track of a single transaction or  it is currently overloaded, an answer with a code 5.03 (service unavailable) should be sent as a reply to the request of a new transaction from the initiator. It is up to the initiator to perform a retransmission in the future.

## 7 - Message fields


### 7.1 - Fragment size

Fragment size is a three bit field, encoding the maximum size of a message (or fragment). Fragments can be 32, 64, 128, 256, 512 or 1024 bytes in size. There are seven defined fragment sizes: '001' - 16 bytes, '010' - 32 bytes, '011' - 64 bytes, '100' - 128 bytes, '101' - 256 bytes, '110' - 512 bytes and '111' - 1024 bytes. Fragment sizes of 512 and 1024 bytes are recommended for data transfers over UDP/IP. Other values may be useful for data transfers over smaller frames (such slow modems or RF).

### 7.2 - Type

Type is a three bit field, encoding the eight possible message types: '000' - unsolicited/non-confirmable (UNS), '001' - request (REQ), '010' - acknowledge (ACK) and '011' - reset (RST). Message types '100' to '111' are reserved.


### 7.3 - Content-type

The content-type field is used to describe the actual format of data encoded in the payload. Only four formats are presented in this specification, and other formats may be used in the future. This field is represented as two bits, and the meaning of these bits is: '00' - JSON encoded (type 0), '01' - urest encoded (type 1), '10' - URI encoded (type 2) and '11' - flat encoded (type 3).


### 7.3.1 - Payload encoding

Payload encoding follows the type of content specified in the content-type header field. The application of different formats is specified as:

- Type 0 (JSON encoded) - Common serialization format. If binary data is transfered in a field, it should be represented in base32 encoding, without padding.
- Type 1 (uREST encoded) - Basic serialization format, suitable for easy parsing of data.
- Type 2 (URI encoded) - Direct URI encoding.
- Type 3 (Flat encoded) - Flat encoding (key/value with separator, without hierarchy). If binary data is transfered in a field, it should be represented in base32 encoding, without padding.


### 7.4 - Methods and response codes

Methods and response codes are enconded in 8 bits, decomposed in two fields: first three bits encode the verb/method or response major opcode and the last five bits encode the minor opcode. Both major and minor opcodes are encoded in their respective fields following the equivalent numeric binary representation.

#### 7.4.1 - Verb / method

- 0.01 - GET (read)
	- success: 2.00
	- URI must be included in the request
- 0.02 - POST (create)
	- success: 2.01 or 2.00. Sometimes 2.02 (asynchronous calls)
	- URI of the created object must be returned on 2.01 but not on 2.05 (not created, but changed)
- 0.03 - PUT (update)
	- success: 2.00. Sometimes 2.02 (asynchronous calls)
- 0.04 - DELETE
	- success: 2.04
- 0.31 - PINGREQ
    - information: 1.31

#### 7.4.2 - Information

- 1.00 - continue
- 1.02 - processing
- 1.31 - PINGACK

#### 7.4.3 - Success

- 2.00 - ok / content
- 2.01 - created
- 2.02 - accepted
- 2.04 - no content / deleted
- 2.05 - reset content

#### 7.4.4 - Client error

- 4.00 - bad request
- 4.01 - unauthorized
- 4.03 - forbidden
- 4.04 - not found
- 4.05 - method not allowed
- 4.06 - not acceptable
- 4.08 - request timeout
- 4.09 - conflict
- 4.14 - URI too long
- 4.18 - i'm a teapot
- 4.22 - unprocessable entity
- 4.23 - locked
- 4.29 - too many requests

#### 7.4.5 - Server error

- 5.00 - internal server error
- 5.01 - not implemented
- 5.02 - bad gateway
- 5.03 - service unavailable
- 5.04 - gateway timeout


### 7.5 - Token

Tokens are generated on the responder side, as a confirmation of a new transaction. A token is a 16 bit integer, generated using a pseudo-random number generator with good distribution. This token should be used for the rest of the transaction, and can be used on the responder side to keep track of multiple transactions.


### 7.6 - Sequence number

A sequence number is a 16 bit integer generated on the initiator side, starting as zero. The sequence of requests and responses is matched in pairs, using the same sequence number. This field is used to control the sequence of operations on both sides, as well as to identify data loss, retransmissions and transaction reset on failures.


## 8 - uREST encoding and data serialization

uREST encoding is defined by a simple serialization format, suitable for compact data representation. The format is organized for easy parsing of data, supporting several features which make it ideal for use on resource constrained systems. On applications where only the memory data layout is important, data structure delimiters can be simply ignored. Some features:

- Easy parsing of data
- Easy syntax, no scape codes
- Suitable for compact data representation
- Supports generic data types or alternatives for strongly typed representation of data
- Arrays / lists
- Supports data structures


### 8.1 - Basic types

- 'i' - integer
- 'f' - float
- 's' - string
- 'b' - blob / base64 encoded


### 8.2 - Delimiters

- '{ .. }' - data structure / string
- ':' - data type and value separator
- ',' - data item saparator
- ';' - list / array item separator


### 8.3 - Example

	struct s {
		int a;
		float b;
		char c[30];
		struct s_s {
			float d, e;
		};
		int f[3];
	};

	{i:1234,f:55.123,s:{hello world},{f:75.5555,f:9.5},i:5093;-6467;253}


## 9 - Flat encoding and data representation

The flat encoding data format represents data as key value pairs. A pair is encoded as two successive fields and there is only one field delimiter (:). The field delimiter can be escaped with a backslash (\). Odd fields represent keys and even fields represent data. For example, to encode one integer, a float and a string, the following syntax is used:

:num1:555:num2:123.456:strval:hello world

In this format, binary data should be encoded in base32, without any padding.


## 10 - Unicast / multicast

Transactions in this specification follow the client/server model, and are viewed as unicast operations. Multicast operations can be implemented only with unsolicited messages and as such, are unconfirmed by the recipients. Multicast delivery is a responsability of the lower layers of the network stack.
