#include <locale.h>
#include <wctype.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <poll.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <stdio.h>

#include "protocol.h"
#include "lklist.h"

typedef struct {
    int confd;
    wchar_t address[MAX_ADDRESS_LENGTH + 1];
    unsigned short port;
} Client;

typedef struct {
    Client* sender;
    server_MessageSentFromClient message;
} Message;

LK_WANT_STRUCT_TYPE(Client, Client_, )
LK_WANT_STRUCT_TYPE(Message, Message_, )

struct pollfd* allocateAndListPollFds(LkClient_List const* clientList, int listeningSockFd, size_t* nfds) {
    *nfds = 1 + lkClient_Size(clientList);
    struct pollfd* fds = (struct pollfd*)malloc((*nfds) * sizeof(struct pollfd));
    if (!fds) return NULL;

    fds[0].fd = listeningSockFd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    if (*nfds > 1) {
        // Client list is not empty
        LkClient_Node* current = lkClient_Head(clientList);
        int i = 1;
        do {
            fds[i].fd = lkClient_GetNodeData(clientList, current)->confd;
            fds[i].events = POLLIN;
            fds[i].revents = 0;
            ++i;
        } while (lkClient_Next(&current));
    }

    return fds;
}

bool forwardMessageToAllClients(LkClient_List* clientList, Message const* message, SenderIdentity const* senderIdentity) {
    LkClient_Node* current = lkClient_Head(clientList);
    if (current != NULL) {
        do {
            Client* targetClient = lkClient_GetNodeData(clientList, current);
            MessageSendStatus sendStatus = server_forwardMessageToClient(
                targetClient->confd,
                message->message.text,
                senderIdentity,
                message->sender->confd == targetClient->confd
            );
            if (sendStatus != SEND_SUCCESS) {
                return false;
            }
        } while (lkClient_Next(&current));
    }
    return true;
}

int eventLoop(int sockfd) {
    LkClient_List* clientList = lkClient_Init();
    LkMessage_List* messages = lkMessage_Init();

    struct pollfd* fds = NULL;
    size_t nfds = 0;
    int retval = 0;

    for (;;) {
        free((void*)fds);
        fds = allocateAndListPollFds(clientList, sockfd, &nfds);
        wprintf(L"Current number of clients: %d\n", nfds - 1);

        bool needToUpdateFds = false;
        do {
            int numEvents = poll(fds, nfds, -1);
            if (numEvents == 0) continue;
            if (numEvents < 0) {
                wprintf(L"poll(): unexpected error\n");
                retval = 1; goto FINALIZE;
            }

            {
                /////// READING MESSAGES FROM ALL CLIENTS /////////
                LkClient_Node* current = lkClient_Head(clientList);
                for (int i = 1; i < nfds; ++i) {
                    if (current == NULL) {
                        wprintf(L"error: client list and fds mismatch\n");
                        retval = 1; goto FINALIZE;
                    }

                    Client* thisClient = lkClient_GetNodeData(clientList, current);
                    bool disconnectThisClient = false;

                    int revents = fds[i].revents;
                    fds[i].revents = 0;

                    if ((revents & POLLIN) == POLLIN) {
                        Message msg;
                        msg.sender = thisClient;
                        MessageReadStatus readStatus = server_readMessageFromClient(fds[i].fd, &msg.message);
                        if (readStatus != READ_SUCCESS) {
                            wprintf(L"read error: %d\n", readStatus);
                            disconnectThisClient = true;
                        } else {
                            if (!lkMessage_Insert(messages, NULL, &msg)) {
                                wprintf(L"error: out of memory\n");
                                retval = 1; goto FINALIZE;
                            }
                        }
                    }
                    if (((revents & POLLERR) == POLLERR) || ((revents & POLLHUP) == POLLHUP)) {
                        wprintf(L"POLLERR or POLLHUP occurred\n");
                        disconnectThisClient = true;
                    }

                    if (disconnectThisClient) {
                        wprintf(L"info: a client disconnected\n");
                        LkClient_Node* clientToRemove = current;
                        lkClient_Next(&current);
                        close(lkClient_GetNodeData(clientList, clientToRemove)->confd);
                        lkClient_Remove(clientList, clientToRemove);
                        needToUpdateFds = true;
                    } else {
                        lkClient_Next(&current);
                    }
                }
            }

            {
                //////////// DELIVERING MESSAGES TO ALL CLIENTS ////////////
                LkMessage_Node* current = lkMessage_Head(messages);
                if (current != NULL) {
                    do {
                        Message* msg = lkMessage_GetNodeData(messages, current);
                        SenderIdentity senderIdentity;
                        wcscpy(senderIdentity.address, msg->sender->address);
                        wcscpy(senderIdentity.name, msg->message.name);
                        senderIdentity.port = msg->sender->port;

                        if (!forwardMessageToAllClients(clientList, msg, &senderIdentity)) {
                            wprintf(L"error: could not forward a message to all clients\n");
                            retval = 1; goto FINALIZE;
                        }

                        server_freeMessageFromClient(&msg->message);
                    } while (lkMessage_Next(&current));
                    lkMessage_Clear(messages);
                }
            }

            // Check for incoming connections
            if ((fds[0].revents & POLLIN) == POLLIN) {
                wprintf(L"New client arrived, accepting connection...\n");
                Client client;
                struct sockaddr_storage addr;
                socklen_t sizeOfAddr = sizeof(addr);
                client.confd = accept(sockfd, (struct sockaddr*)&addr, &sizeOfAddr);
                if (client.confd >= 0) {
                    char addressString[MAX_ADDRESS_LENGTH + 1];
                    if (addr.ss_family == AF_INET) {
                        // IPv4
                        struct sockaddr_in* A = (struct sockaddr_in*)&addr;
                        client.port = ntohs(A->sin_port);
                        inet_ntop(AF_INET, (void*)(&A->sin_addr), addressString, MAX_ADDRESS_LENGTH);
                    } else {
                        // IPv6
                        struct sockaddr_in6* A = (struct sockaddr_in6*)&addr;
                        client.port = ntohs(A->sin6_port);
                        inet_ntop(AF_INET6, (void*)(&A->sin6_addr), addressString, MAX_ADDRESS_LENGTH);
                    }
                    mbstowcs(client.address, addressString, MAX_ADDRESS_LENGTH);

                    lkClient_Insert(clientList, NULL, &client);
                    needToUpdateFds = true;
                } else if (errno != EWOULDBLOCK) {
                    wprintf(L"error: accept() failed");
                    retval = 1; goto FINALIZE;
                }
            }
            fds[0].revents = 0;
        } while (!needToUpdateFds);
    }

FINALIZE:
    free((void*)fds);
    lkClient_Destroy(clientList);
    return retval;
}

int main() {
    setlocale(LC_ALL, "");
    
    char const* SERVER_IP = "0.0.0.0";
    unsigned short SERVER_PORT = 12345;

    struct sockaddr_in addr;
    memset((void*)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // fcntl(sockfd, F_SETFL, O_NONBLOCK);
    // ioctl(sockfd, FIONBIO, &opt);

    bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    listen(sockfd, 5);

    wprintf(L"Server listening at %s:%hu\n", SERVER_IP, SERVER_PORT);

    int retval = eventLoop(sockfd);

    close(sockfd);
    return 0;
}
