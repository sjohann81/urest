# uREST - A simple protocol for REST operations over UDP

The protocol is based on a request/response model, and can be seen as a reduced version of the Constrained Application Protocol (CoAP).


## Initiator

An initiator is the client of a transaction (origin on a request, destination on a response). Verbs/methods similar to HTTP are used.


## Responder

A responder is the server of a transaction (destination on a request, origin on a response). Response codes similar to HTTP are used. The responder 'goes along' with requests from an initiator.


## Transmission parameters

- ACK_TIMEOUT: 2 s
- MAX_RETRANSMIT: 3

Transmission is performed by single messages (unsolicited / non-confimable) or request/response pairs. Retransmission is performed using timeout and exponential back-off.


## Packet format

Maximum UDP payload considered in this specification is 512 bytes. Multiple packets are needed on a transmission (series of REQs and ACKs) if the last transmitted packet is full (payload of 507 bytes). Unsolicited messages are limited to a single packet.

- Token / transaction ID (16 bits)
- Sequence (16 bits)
- Type (UNS, REQ, ACK, RST) (2 bits)
- Method / response (6 bits - 3 bits for major code, 3 bits for minor code)
- Options (5 bits)
- Content type (3 bits - type)
- Payload (507 bytes)


## Message types

- UNS - unsolicited - used for both non-confirmable or unsolicited requests / responses.
- REQ - request
- ACK - response (acknowledge)
- RST - reset

The initiator is responsible for the control of the flow of data and token generation. A responder must reply using the exactly the same token and a token must remain the same during the entire transaction (sequence of requests/responses).

Unsolicited messages use individual tokens and a sequence number of zero. On the other hand, requests and responses are considered pairs, so a response uses the same sequence number of a matching request.


## Transactions

A transaction is composed by a single unsolicited message, which can be either a request or a response, or a sequence of REQ and ACK messages. In a sequence of messages, it is up to the initiator to retry on cases where data loss occour (detected by ACK_TIMEOUT and exponential back-off).

If an initiator or a responder receive an unexpected sequence number, a RST message should be sent.


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


### Sequence


### Type


### Response codes

Success

- 2.00 - ok / content
- 2.01 - created
- 2.02 - accepted
- 2.04 - no content
- 2.05 - changed

Client error

- 4.00 - bad request
- 4.01 - unauthorized
- 4.03 - forbidden
- 4.04 - not found
- 4.05 - method not allowed
- 4.06 - not acceptable

Server error

- 5.00 - internal server error
- 5.01 - not implemented
- 5.02 - bad gateway
- 5.03 - service unavailable
- 5.04 - gateway timeout


### Options

Reserved for future use.


### Content-type

- 1 - json encoded
- 2 - raw base64 encoded
- 3 - raw


### Payload encoding

- Type 1 (json encoded) -  On a request method, data should start with a 'uri' field, containing the relative path to the resource in the server. For both requests and responses, data is encoded in aditional fields, containing the application specific data format. Traditional URL encoded fields (using something like '/urlencoded?firstname=sid&lastname=sloth') is not supported.
- Type 2 (raw base64 encoded)
- Type 3 (raw)

## Unicast / multicast