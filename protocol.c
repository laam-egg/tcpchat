/**
 * 
 */

#include "protocol.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>

///////////////////////
// UTILITY FUNCTIONS //
///////////////////////

size_t numDigitsOf(size_t n) {
    if (n == 0) return 1;
    size_t result = 0;
    while (n > 0) {
        n /= 10;
        ++result;
    }
    return result;
}

//////////////////////////////////////////////////
// LOW-LEVEL FUNCTIONALITY  (PRIVATE FUNCTIONS) //
//////////////////////////////////////////////////

#define CONTENT_LENGTH_STRING_BUFFER_LENGTH 22 // max(size_t) = 2^64 - 1, which has 20 digits

MessageReadStatus recvError(int numBytesRead) {
    if (numBytesRead < 0) {
        return READ_ERR_BROKEN_SOCKET;
    } else /* if (numBytesRead == 0) */ {
        return READ_ERR_PEER_CLOSED;
    }
}

/**
 * Raw message syntax:
 * <Message length>:<Message>
 * 
 * For example:
 * 11:Hello World
 * 1:.
 * 
 * This function tries to read the socket
 * till it encounters the delimiter ':',
 * so that it could know the message
 * length. Later, it will try to receive
 * that message in full.
 */
MessageReadStatus rawReadMessage(int confd, wchar_t** bufferPtr, size_t* currentBufferLengthPtr, size_t* actualMessageLength) {
    ///////////////////////////////////////////////////////
    // GET MESSAGE LENGTH/CONTENT LENGTH INTO tempBuffer //
    ///////////////////////////////////////////////////////

    size_t totalNumCharsRead = 0;

    size_t messageStartPos = 0;
    wchar_t tempBuffer[CONTENT_LENGTH_STRING_BUFFER_LENGTH + 1];

    bool foundDelimiter = false;
    do {
        size_t numBytesRead = recv(confd, (void*)(tempBuffer + totalNumCharsRead), (CONTENT_LENGTH_STRING_BUFFER_LENGTH - totalNumCharsRead) * sizeof(tempBuffer[0]), 0);
        if (numBytesRead <= 0) {
            return recvError(numBytesRead);
        }

        size_t numCharsRead = numBytesRead / sizeof(tempBuffer[0]);
        size_t previousTotalNumCharsRead = totalNumCharsRead;
        totalNumCharsRead += numCharsRead;
        size_t i;
        for (i = previousTotalNumCharsRead; i < totalNumCharsRead; ++i) {
            if (tempBuffer[i] == L':') {
                foundDelimiter = true;
                tempBuffer[i] = L'\0';
                messageStartPos = i + 1;
                break;
            }
        }
    } while (!foundDelimiter);

    wchar_t* endptr_unused;
    size_t messageLength = (size_t)wcstoul(tempBuffer, &endptr_unused, 10);
    if (messageLength == 0) {
        return READ_ERR_MALFUNCTIONING_PEER;
    }

    ////////////////////////////////////////////////////////
    // CHECK MESSAGE BUFFER SIZE AND REALLOCATE IF NEEDED //
    ////////////////////////////////////////////////////////

    wchar_t* buffer = *bufferPtr;
    if (messageLength + 1 > currentBufferLengthPtr[0]) {
        size_t newBufferLength = messageLength + 1;
        free((void*)buffer);
        buffer = bufferPtr[0] = (wchar_t*)malloc(newBufferLength * sizeof(buffer[0]));
        if (buffer == NULL) {
            return READ_ERR_NOT_ENOUGH_MEMORY;
        }
        currentBufferLengthPtr[0] = newBufferLength;
    }

    /////////////////////////////////////////////////////////
    // Copy initial message segment that is accidentally   //
    // received during message length retrieval (the XXX   //
    // part in MessageLength:XXX...) into the right buffer //
    // and at the right position.                          //
    /////////////////////////////////////////////////////////

    for (size_t i = messageStartPos; i < totalNumCharsRead; ++i) {
        buffer[i - messageStartPos] = tempBuffer[i];
    }
    totalNumCharsRead = totalNumCharsRead - messageStartPos;

    //////////////////////////////////////
    // GET THE ACTUAL MESSAGE (CONTENT) //
    //////////////////////////////////////

    while (totalNumCharsRead < messageLength) {
        size_t numBytesRead = recv(confd, (void*)(buffer + totalNumCharsRead), (messageLength - totalNumCharsRead) * sizeof(buffer[0]), 0);
        if (numBytesRead <= 0) {
            return recvError(numBytesRead);
        }

        size_t numCharsRead = numBytesRead / sizeof(buffer[0]);
        totalNumCharsRead += numCharsRead;
    };
    buffer[totalNumCharsRead] = L'\0';

    return READ_SUCCESS;
}

MessageSendStatus rawSendMessage(int confd, wchar_t const* const messageToSend) {
    size_t messageLength = wcslen(messageToSend);
    size_t headerLength = numDigitsOf(messageLength) + 1 /* the delimiter L':' */;
    wchar_t headerBuffer[CONTENT_LENGTH_STRING_BUFFER_LENGTH + 2];
    swprintf(headerBuffer, headerLength + 1, L"%zu:", messageLength);
    size_t numBytesSent = send(confd, (void*)headerBuffer, headerLength * sizeof(headerBuffer[0]), 0);
    if (numBytesSent < headerLength) {
        return SEND_ERR_INTERRUPTED;
    }
    
    size_t expectedNumBytesSent = wcslen(messageToSend) * sizeof(messageToSend[0]);
    numBytesSent = send(confd, (void*)messageToSend, expectedNumBytesSent, 0);
    if (numBytesSent < expectedNumBytesSent) {
        return SEND_ERR_INTERRUPTED;
    }

    return SEND_SUCCESS;
}

/////////////////////////////////////////////////
// HIGH-LEVEL FUNCTIONALITY (PUBLIC FUNCTIONS) //
/////////////////////////////////////////////////

/*
 * From a client's perspective:
 *
 * FORMAT OF A MESSAGE SENT TO OTHER PEOPLE BY ME (FORMAT 1):
 * (technically, a message sent from a client to server)
 * Line 1     Sender Name
 * Line >= 2  Actual Message
 * 
 * FORMAT OF A MESSAGE SENT TO ME BY ANOTHER PERSON (FORMAT 2):
 * (technically, a message sent from server to a client)
 * Line 1     Sender Address
 * Line 2     Sender Port
 * Line 3     Sender Name
 * Line 4
 *     If Sender is Yourself:
 *          "Yourself"
 *     Else:
 *          "Else"
 * Line >= 5  Actual Message
 */

void client_setup() {}

void client_teardown() {}

MessageReadStatus client_readMessageFromServer(int confd, client_ReceivedMessage* msgPtr) {
    msgPtr->text = NULL;

    wchar_t* buffer = NULL;
    size_t bufferLength = 0;
    size_t actualMessageLength = 0;
    MessageReadStatus _returnValue_ = READ_SUCCESS;
    {
#define FAIL(readStatus) { _returnValue_ = readStatus; goto FINALIZE; }

        MessageReadStatus readStatus = rawReadMessage(confd, &buffer, &bufferLength, &actualMessageLength);
        if (readStatus != READ_SUCCESS) FAIL(readStatus)

        wchar_t* tokenizerInternalData;

        // Using FORMAT 2
        {
            // Line 1
            wchar_t* senderAddress = wcstok(buffer, L"\n", &tokenizerInternalData);
            size_t senderAddressLength = wcslen(senderAddress);
            if (senderAddressLength == 0 || senderAddressLength > MAX_ADDRESS_LENGTH) FAIL(READ_ERR_MALFUNCTIONING_PEER)
            wcscpy(msgPtr->sender.address, senderAddress);
        } {
            // Line 2
            wchar_t* senderPortString = wcstok(NULL, L"\n", &tokenizerInternalData);
            wchar_t* endptr_unused;
            unsigned short senderPort = (unsigned short)wcstoul(senderPortString, &endptr_unused, 10);
            msgPtr->sender.port = senderPort;
        } {
            // Line 3
            wchar_t* senderName = wcstok(NULL, L"\n", &tokenizerInternalData);
            size_t senderNameLength = wcslen(senderName);
            if (senderNameLength > MAX_NAME_LENGTH) FAIL(READ_ERR_MALFUNCTIONING_PEER)
            wcscpy(msgPtr->sender.name, senderName);
        } {
            // Line 4
            wchar_t* isYourself = wcstok(NULL, L"\n", &tokenizerInternalData);
            msgPtr->senderIsYourself = (wcscmp(isYourself, L"Yourself") == 0);
        } {
            // Line >= 5
            wchar_t* actualMessage = wcstok(NULL, L"", &tokenizerInternalData);

            size_t actualMessageLength = wcslen(actualMessage);
            msgPtr->text = (wchar_t*)malloc((actualMessageLength + 1) * sizeof(msgPtr->text[0]));
            if (msgPtr->text == NULL) FAIL(READ_ERR_NOT_ENOUGH_MEMORY)
            wcscpy(msgPtr->text, actualMessage);
        }
    }

FINALIZE:
    free((void*)buffer);
    if (_returnValue_ != READ_SUCCESS) {
        client_freeReceivedMessage(msgPtr);
    }
    return _returnValue_;
}

void client_freeReceivedMessage(client_ReceivedMessage* msgPtr) {
    free((void*)msgPtr->text);
    msgPtr->text = NULL;
}

MessageSendStatus client_sendMessageToServer(int confd, wchar_t const* const name, wchar_t const* const messageToSend) {
    // Using FORMAT 1
    size_t bufferLength = wcslen(name) + 1 /*the newline L'\n'*/ + wcslen(messageToSend);
    wchar_t* buffer = (wchar_t*)malloc((bufferLength + 2) * sizeof(buffer[0]));
    if (buffer == NULL) return SEND_ERR_NOT_ENOUGH_MEMORY;
    swprintf(buffer, bufferLength + 1, L"%ls\n%ls", name, messageToSend);
    MessageSendStatus sendStatus = rawSendMessage(confd, buffer);
    free((void*)buffer);
    return sendStatus;
}

void server_setup() {}
void server_teardown() {}

MessageReadStatus server_readMessageFromClient(int confd, server_MessageSentFromClient* msgPtr) {
    MessageReadStatus _returnValue_ = READ_SUCCESS;

    msgPtr->text = NULL;

    wchar_t* buffer = NULL;
    size_t bufferLength = 0;
    size_t actualMessageLength;
    MessageReadStatus readStatus = rawReadMessage(confd, &buffer, &bufferLength, &actualMessageLength);
    if (readStatus != READ_SUCCESS) FAIL(readStatus)

    // Using FORMAT 1
    wchar_t* tokenizerInternalData;
    {
        // Line 1
        wchar_t* senderName = wcstok(buffer, L"\n", &tokenizerInternalData);
        size_t senderNameLength = wcslen(senderName);
        if (senderNameLength > MAX_NAME_LENGTH) FAIL(READ_ERR_MALFUNCTIONING_PEER)
        wcscpy(msgPtr->name, senderName);
    } {
        // Line >= 2
        wchar_t* message = wcstok(NULL, L"", &tokenizerInternalData);
        size_t messageLength = wcslen(message);
        msgPtr->text = (wchar_t*)malloc((messageLength + 1) * sizeof(message[0]));
        wcscpy(msgPtr->text, message);
    }

FINALIZE:
    free((void*)buffer);
    msgPtr->confd = confd;
    return _returnValue_;
}

void server_freeMessageFromClient(server_MessageSentFromClient* msgPtr) {
    free((void*)msgPtr->text);
    msgPtr->text = NULL;
}

MessageSendStatus server_forwardMessageToClient(int confd, wchar_t const* text, SenderIdentity const* senderIdentity, bool senderIsHim) {
    // Using FORMAT 2
    size_t payloadLength = wcslen(senderIdentity->address)                   // Sender Address
        + 1 + numDigitsOf(senderIdentity->port)           // Sender Port
        + 1 + wcslen(senderIdentity->name)                // Sender Name
        + 1 + (senderIsHim ? wcslen(L"Yourself") : wcslen(L"Else"))  // Sender Is Yourself
        + 1 + wcslen(text)                                // Actual Message
        + 1;

    wchar_t* payload = (wchar_t*)malloc(
        (payloadLength + 1) * sizeof(wchar_t)
    );
    if (payload == NULL) return SEND_ERR_NOT_ENOUGH_MEMORY;

    swprintf(payload, payloadLength, L"%ls\n%hu\n%ls\n%ls\n%ls",
        senderIdentity->address,
        senderIdentity->port,
        senderIdentity->name,
        (senderIsHim ? L"Yourself" : L"Else"),
        text
    );

    MessageSendStatus sendStatus = rawSendMessage(confd, payload);
    free((void*)payload);
    return sendStatus;
}
