#ifndef PROTOCOL_INCLUDED
#define PROTOCOL_INCLUDED

#include <wctype.h>
#include <wchar.h>
#include <stdbool.h>

#define MAX_NAME_LENGTH 255
#define MAX_ADDRESS_LENGTH 63

typedef enum {
    READ_SUCCESS = 0,
    READ_ERR_MALFUNCTIONING_PEER,
    READ_ERR_BROKEN_SOCKET,
    READ_ERR_PEER_CLOSED,
    READ_ERR_NOT_ENOUGH_MEMORY
} MessageReadStatus;

typedef enum {
    SEND_SUCCESS = 0,
    SEND_ERR_INTERRUPTED,
    SEND_ERR_NOT_ENOUGH_MEMORY
} MessageSendStatus;

typedef struct {
    wchar_t name[MAX_NAME_LENGTH + 1];
    wchar_t address[MAX_ADDRESS_LENGTH + 1];
    unsigned short port;
} SenderIdentity;

///////////////////////
///// CLIENT API //////
///////////////////////

typedef struct {
    wchar_t* text;
    SenderIdentity sender;
    bool senderIsYourself;
} client_ReceivedMessage;

void              client_setup();
void              client_teardown();
MessageReadStatus client_readMessageFromServer(int confd, client_ReceivedMessage* msgPtr);
void              client_freeReceivedMessage(client_ReceivedMessage* msgPtr);
MessageSendStatus client_sendMessageToServer(int confd, wchar_t const* const name, wchar_t const* const messageToSend);

///////////////////////
///// SERVER API //////
///////////////////////

typedef struct {
    wchar_t* text;
    int confd;
    wchar_t name[MAX_NAME_LENGTH + 1];
} server_MessageSentFromClient;

void              server_setup();
void              server_teardown();
MessageReadStatus server_readMessageFromClient(int confd, server_MessageSentFromClient* msgPtr);
void              server_freeMessageFromClient(server_MessageSentFromClient* msgPtr);
MessageSendStatus server_forwardMessageToClient(int confd, wchar_t const* text, SenderIdentity const* senderIdentity, bool senderIsHim);

#endif // PROTOCOL_INCLUDED
