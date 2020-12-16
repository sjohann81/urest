# uREST - A simple protocol for REST operations over UDP

The protocol is based on a request/response model, and can be seen as a reduced and much simplified version of the Constrained Application Protocol (CoAP). Applications of this protocol include systems where memory is a limited resource, such as sensor networks, home appliances and machine-to-machine (M2M). In this protocol, a client/server model is used and entities are referenced as *initiator* and *responder* respectively.


## Initiator

An initiator is the client of a transaction (origin on a request, destination on a response). Verbs/methods similar to HTTP are used. A transaction may be composed of several messages and is coordinated by the initiator. The initiator defines the flow of data and retransmissions, acting as a master.


## Responder

A responder is the server of a transaction (destination on a request, origin on a response). Response codes similar to HTTP are used. The responder acts as a slave of the initiator, accepting requests or partial requests from it. A response may be composed of several packets, and the flow of data is coordinated by the initiator.


## Transmission parameters

- ACK_TIMEOUT: 2 s
- MAX_RETRANSMIT: 3

Transmission is performed by single messages (unsolicited / non-confimable) or request/response pairs. Retransmission is performed using timeout and exponential back-off. A transmission is controlled by the initiator following a two-message protocol:

- Initiator performs a request (REQ) operation (or partial request)
- Responder acknowledges the reception with an (ACK) operation (without any content) or acknowledges and returns data.
- Initiator continues transferring or retrieving additional data (REQ), followed responder confirmations or transfers (ACK).

Data should only be returned from the responder after a request is completed. For example, on a request composed of multiple messages, the reponder acknowledges each message with no content but the last message, where data starts to be transfered. On a response composed of multiple message, the initiator transmits messages with no content until completion of the transfer from the responder.

Unsolicited messages are used for non-confirmable requests or responses and no retransmission should be performed, as an acknowledge is not expected.


## Message format

A message is composed by an 8 byte header followed by a payload. A transaction can be split in multiple messages, and each message must be transmitted in a UDP datagram. Maximum UDP payload considered in this specification is 512 bytes. On large transactions, multiple messages are needed (series of REQs and ACKs) if the last transmitted message payload is full (504 bytes). Unsolicited messages are limited to a single, individual message without any confirmation.

### Message header

- Token / transaction ID (32 bits)
- Sequence (16 bits)
- Type (UNS, REQ, ACK, RST) (2 bits)
- Method / response (6 bits - 2 bits for major code, 4 bits for minor code)
- Options (5 bits)
- Content type (3 bits - type)
- Payload (504 bytes)


## Message types

- UNS - unsolicited - used for both non-confirmable or unsolicited requests / responses.
- REQ - request
- ACK - response (acknowledge)
- RST - reset

The initiator is responsible for the control of the flow of data, and must use the same token from the first acknowledge for the rest of a transaction. An initial token composed of zeroes is used in the first message. A responder must acknowledge the reception of data of a new transaction with a new token. This token must remain the same during the entire transaction (sequence of request/response packets). Request and response messages are considered pairs, so a response uses the same sequence number of a matching request.

Unsolicited messages use a token composed of zeroes as well as sequence number of zero.


## Transactions

A transaction is composed by a single unsolicited message, which can be either a request or a response, or a sequence of REQ and ACK messages. In a sequence of messages, it is up to the initiator to retry on cases where data loss occour (detected by ACK_TIMEOUT and exponential back-off). If an initiator or a responder receive an unexpected sequence number or token, a RST message should be sent and the transaction is finished.

	initiator               responder               initiator               responder
	|                       |                       |                       |
	|   REQ (seq 0, 504B)   | 0.01 (GET)            |   REQ (seq 0, 504B)   | 0.01 (GET)
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
	|   ACK (seq 2, 504B)   | 2.06 (continue)       |                       |
	|<----------------------| token: 0xaabbccdd     |   REQ (seq 0, 0B)     | 0.01 (GET)
	|                       |                       |---------------------->| token: 0x00000000
	|   REQ (seq 3, 0B)     | 0.01 (GET)            |                       |
	|---------------------->| token: 0xaabbccdd     |    (processing...)    |
	|                       |                       |                       |
	|   ACK (seq 3, 50B)    | 2.00 (ok)             |   ACK (seq 0, 356B)   | 2.00 (ok)
	|<----------------------| token: 0xaabbccdd     |<----------------------| token: 0x69696969
	          (a)                                              (c)

Transfers can happen in piggybacked mode, where the responder includes the result of the processing (or at least, part of it) in an acknowledge message immediatelly following a request (examples 'b' and 'c'). The responder answers with a code 2.00 (ok) when no more content should be transferred or with a code 2.06 (continue), when more content should be transferred from the responder. Piggybacked mode can be used if processing time is small (under 1 second, or ACK_TIMEOUT/2), respecting the transmission parameters for the acknowledge timeout.

When processing time is greater than one second (or ACK_TIMEOUT/2) or no piggybacked mode is used, the last message from the initiator is confirmed with a code 2.02 (accepted), and it is up to the initiator to poll the responder periodically for data (example 'a'). If not ready, the responder must answer with a code 2.02 (accepted) and return no data, until it is ready. Then, it answers with a code 2.00 (ok) when no more content needs be transferred or with a code 2.06 (continue) when more content is available. If no data is returned and the response code is 2.02 (accepted), the initiator assumes the responder is processing, and in this case the sequence number must stay the same.

### Concurrent transactions

Transactions can happen concurrently, and it is up to the responder to keep track of multiple transactions from different initiators. If a responder is resource constrained and can only keep track of a single transaction, or is currently overloaded, it should just ignore a request of a new transaction from the initiator. It is up to the initiator to interpret this as a message loss and perform a retransmission.


## Verbs / methods

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


## Message fields

### Token

Tokens are generated on the responder side, as a confirmation of a new transaction. A token is a 32 bit integer, generated using a pseudo-random number generator with good distribution. This token should be used for the rest of the transaction, and can be used on the responder side to keep track of multiple transactions.

### Sequence

A sequence number is a 16 bit integer generated on the initiator side, starting as zero. The sequence of requests and responses is matched in pairs, using the same sequence number. This field is used to control the sequence of operations on both sides, as well as to identify data loss, retransmissions and transaction reset on failures.

### Type

Type is a two bit field, encoding the four possible message types: '00' - unsolicited/non-confirmable (UNS), '01' - request (REQ), '10' - acknowledge (ACK) and '11' - reset (RST).

### Methods and response codes

Methods and response codes are enconded in 6 bits, decomposed in two fields: first two bits encode the verb/method/response major opcode and the last four encode the minor opcode. Minor opcode is encoded in the second field as four bits, following the equivalent binary representation. The meaning of first field two bits are: '00' - verb/method, '01' - success (codes 2.xx), '10' - client error (codes 4.xx) and '11' - server error (codes 5.xx).

#### Success

- 2.00 - ok / content
- 2.01 - created
- 2.02 - accepted
- 2.04 - no content / deleted
- 2.05 - changed
- 2.06 - continue

#### Client error

- 4.00 - bad request
- 4.01 - unauthorized
- 4.03 - forbidden
- 4.04 - not found
- 4.05 - method not allowed
- 4.06 - not acceptable

#### Server error

- 5.00 - internal server error
- 5.01 - not implemented
- 5.02 - bad gateway
- 5.03 - service unavailable
- 5.04 - gateway timeout


### Options

Reserved for future use.

### Content-type

- 0 - reserved
- 1 - json encoded
- 2 - raw base64 encoded
- 3 - raw

### Payload encoding

- Type 1 (json encoded) -  On a request method, data should start with a 'uri' field, containing the relative path to the resource in the server. For both requests and responses, data is encoded in aditional fields, containing the application specific data format. Traditional URL encoded fields (using something like '/urlencoded?firstname=sid&lastname=sloth') are not supported.
- Type 2 (raw base64 encoded)
- Type 3 (raw)


## Unicast / multicast

Transactions in this specification follow the client/server model, and are viewed as unicast operations. Multicast operations can be implemented only with unsolicited messages and as such, are unconfirmed by the recipients. Multicast delivery is a responsability of the lower layers of the network stack.
