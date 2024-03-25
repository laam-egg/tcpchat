// Common stuff and ncurses
#define NCURSES_WIDECHAR 1
#include <locale.h>
#include <wchar.h>
#include <wctype.h>
#include <ncurses.h>
#include <ncursesw/curses.h>
#include <curses.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

// Sockets
#include <arpa/inet.h>
#include <sys/socket.h>

// Polling
#include <poll.h>

#include "protocol.h"

// DECLARATIONS

int main();

void setupApplication();
void setupConnection(wchar_t const* const SERVER_IP, unsigned short SERVER_PORT);
void setupChatUI();

void teardownChatUI();
void teardownConnection();
void teardownApplication();

void runApplication();

void fatalError(char const* errorMessage);

typedef enum {
	fRed_bBlack = 1, // f=foreground, b=background
	fGreen_bBlack,
	fYellow_bBlack,
	fBlue_bBlack,
	fMagenta_bBlack,
	fCyan_bBlack,
	fWhite_bBlack,
	ColorPair_UPPER_LIMIT
} ColorPair;
int const NUM_COLOR_PAIRS = ColorPair_UPPER_LIMIT - 1;

// IMPLEMENTATIONS

#define KEY_CTRL(x) ((x) & 0x1f)

int main() {
	setupApplication();
	runApplication();
	teardownApplication();
	return 0;
}

void string_insert_char(wchar_t* s, size_t pos, wchar_t c) {
	size_t size = wcslen(s);
	for (size_t i = size + 1; i > pos; --i) {
		s[i] = s[i - 1];
	}
	s[pos] = c;
}

void string_delete_char(wchar_t* s, size_t pos) {
	size_t size = wcslen(s);
	for (size_t i = pos; i < size; ++i) {
		s[i] = s[i + 1];
	}
}

WINDOW* chatHistoryWindow;
WINDOW* messageInputWindow;
int sockfd;
wchar_t username[MAX_NAME_LENGTH + 1] = { 0 };
bool chatUIStarted = false;

void setupChatUI() {
	if (!chatUIStarted) {
		initscr();

		start_color();
		init_pair(fRed_bBlack, COLOR_RED, COLOR_BLACK);
		init_pair(fGreen_bBlack, COLOR_GREEN, COLOR_BLACK);
		init_pair(fYellow_bBlack, COLOR_YELLOW, COLOR_BLACK);
		init_pair(fBlue_bBlack, COLOR_BLUE, COLOR_BLACK);
		init_pair(fMagenta_bBlack, COLOR_MAGENTA, COLOR_BLACK);
		init_pair(fCyan_bBlack, COLOR_CYAN, COLOR_BLACK);
		init_pair(fWhite_bBlack, COLOR_WHITE, COLOR_BLACK);

		raw();
		noecho();
		keypad(stdscr, true);

		attron(fWhite_bBlack);
		clear();

		int height = LINES - 4;
		int width = COLS;
		int startY = 0;
		int startX = 0;
		chatHistoryWindow = newwin(height, width, startY, startX);
		scrollok(chatHistoryWindow, true);

		waddwstr(chatHistoryWindow, L"(Press Ctrl-C to exit.)\n");
		wrefresh(chatHistoryWindow);

		height = 3;
		width = COLS;
		startY = LINES - 3;
		startX = 0;
		messageInputWindow = newwin(height, width, startY, startX);
		keypad(messageInputWindow, true);
		scrollok(messageInputWindow, true);
		wtimeout(messageInputWindow, 0);
		keypad(messageInputWindow, true);
		chatUIStarted = true;
	}
}

void teardownChatUI() {
	if (chatUIStarted) {
		delwin(chatHistoryWindow);
		delwin(messageInputWindow);
		endwin();
		chatUIStarted = false;
	}
}

void setupConnection(wchar_t const* const SERVER_IP, unsigned short SERVER_PORT) {
	client_setup();

	char SERVER_IP_ASCII[MAX_ADDRESS_LENGTH + 1] = { 0 };
	wcstombs(SERVER_IP_ASCII, SERVER_IP, MAX_ADDRESS_LENGTH);

	struct sockaddr_in addr;
	memset((void*)&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_IP_ASCII, &addr.sin_addr);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	wprintf(L"Connecting to server at %ls:%hu...\n", SERVER_IP, SERVER_PORT);
	int connectResult = connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));

	if (connectResult != 0) {
		fatalError("Could not connect to server");
	}
	wprintf(L"Connected.\n");
}

void teardownConnection() {
	close(sockfd);

	client_teardown();
}

void teardownApplication() {
	werase(messageInputWindow);
	wrefresh(messageInputWindow);
	waddwstr(chatHistoryWindow, L"YOU: <Disconnecting...>");
	wrefresh(chatHistoryWindow);

	teardownChatUI();
	teardownConnection();

	wprintf(L"Good bye.\n");
	exit(EXIT_SUCCESS);
}

void handleSigint(int sig) {
	teardownApplication();
}

void fatalError(char const* errorMessage) {
	teardownChatUI();
	fprintf(stderr, "ERROR: %s\n", errorMessage);
	exit(EXIT_FAILURE);
}

void setupApplication() {
	setlocale(LC_ALL, "");

	///// Basic Information /////
	size_t lastCharIndex;

	wchar_t SERVER_IP[MAX_ADDRESS_LENGTH + 1] = { 0 };
	wprintf(L"SERVER IP   : "); fgetws(SERVER_IP, MAX_ADDRESS_LENGTH + 1, stdin);
	lastCharIndex = wcslen(SERVER_IP) - 1;
	if (SERVER_IP[lastCharIndex] == L'\n') {
		SERVER_IP[lastCharIndex] = L'\0';
	}

#define SERVER_PORT_STRING_MAX_LENGTH 5
	wchar_t SERVER_PORT_STRING[SERVER_PORT_STRING_MAX_LENGTH + 2] = { 0 };
	wprintf(L"SERVER PORT : "); fgetws(SERVER_PORT_STRING, SERVER_PORT_STRING_MAX_LENGTH + 2, stdin);
	wchar_t* unused_endptr = NULL;
	unsigned short SERVER_PORT = (unsigned short)wcstoul(SERVER_PORT_STRING, &unused_endptr, 10);

	wprintf(L"YOUR NAME   : ");
	fgetws(username, MAX_NAME_LENGTH + 1, stdin);
	lastCharIndex = wcslen(username) - 1;
	if (username[lastCharIndex] == L'\n') {
		username[lastCharIndex] = L'\0';
	}

	///// Actual Work /////
	setupConnection(SERVER_IP, SERVER_PORT);

	setupChatUI();
	signal(SIGINT, SIG_IGN);
}

ColorPair getSenderColorPair(wchar_t const* senderName, wchar_t const* senderAddress, unsigned short senderPort) {
	// Hash the sender information
	wint_t sum = 0;
	for (size_t i = 0; ; ++i) {
		wint_t k = (wint_t)senderName[i];
		if (k == 0) break;
		sum += k;
	}
	for (size_t i = 0; ; ++i) {
		wint_t k = (wint_t)senderAddress[i];
		if (k == 0) break;
		sum += k * 2;
	}
	sum += senderPort;
	ColorPair colorPair = sum % NUM_COLOR_PAIRS;
	if (colorPair == fWhite_bBlack) {
		colorPair = fCyan_bBlack;
	}
	return colorPair;
}

void pushChatHistory(wchar_t const* senderName, wchar_t const* senderAddress, unsigned short senderPort, wchar_t const* message) {
	wattron(chatHistoryWindow, COLOR_PAIR(getSenderColorPair(senderName, senderAddress, senderPort)));
	waddwstr(chatHistoryWindow, senderName);
	waddwstr(chatHistoryWindow, L" <");
	waddwstr(chatHistoryWindow, senderAddress);
	waddwstr(chatHistoryWindow, L":");
#define MAX_NUM_DIGITS_OF_PORT 10
	wchar_t senderPortString[MAX_NUM_DIGITS_OF_PORT + 2];
	swprintf(senderPortString, MAX_NUM_DIGITS_OF_PORT + 1, L"%hu", senderPort);
	waddwstr(chatHistoryWindow, senderPortString);
	waddwstr(chatHistoryWindow, L"> ");

	wattron(chatHistoryWindow, COLOR_PAIR(fWhite_bBlack));
	waddwstr(chatHistoryWindow, message);
	waddwstr(chatHistoryWindow, L"\n");
	wrefresh(chatHistoryWindow);

	wrefresh(messageInputWindow);
}

void readIncomingMessage() {
	client_ReceivedMessage message;
	MessageReadStatus readStatus = client_readMessageFromServer(sockfd, &message);
	if (readStatus != READ_SUCCESS) {
		char error[32];
		snprintf(error, 32, "READ_ERR: %d", readStatus);
		fatalError(error);
	}

	pushChatHistory(message.sender.name, message.sender.address, message.sender.port, message.text);

	client_freeReceivedMessage(&message);
}

void runApplication() {
#define INPUT_MESSAGE_MAX_LENGTH 1023
	wchar_t inputMessage[INPUT_MESSAGE_MAX_LENGTH + 1];
	bool stop;

	struct pollfd fds[] = {
		{ sockfd, POLLIN, 0 }
	};
	int nfds = sizeof(fds) / sizeof(fds[0]);

	while (1) {
		inputMessage[0] = L'\0';
		stop = false;

		size_t inputMessageCurrentPos = 0;
		wint_t c;
		while (true) {
			bool haveKeystroke = (wget_wch(messageInputWindow, &c) != ERR);
			if (poll(fds, nfds, 0) > 0) {
				short revents = fds[0].revents;
				fds[0].revents = 0;

				if ((revents & POLLHUP) == POLLHUP) {
					fatalError("Socket closed on this party");
				} else if ((revents & POLLERR) == POLLERR) {
					fatalError("Socket error");
				} else if ((revents & POLLIN) == POLLIN) {
					readIncomingMessage();
				}
			}
			if (!haveKeystroke) continue;

			if (c == '\n') break;
			switch (c) {
				case KEY_BACKSPACE:
				case '\b':
				case 127:
				{
					size_t inputMessageLength = wcslen(inputMessage);
					if (inputMessageCurrentPos >= inputMessageLength) {
						inputMessageCurrentPos = inputMessageLength;
					}
					if (inputMessageCurrentPos > 0) {
						--inputMessageCurrentPos;
						string_delete_char(inputMessage, inputMessageCurrentPos);
					}
				}
				break;

				case KEY_LEFT:
				{
					if (inputMessageCurrentPos > 0) {
						--inputMessageCurrentPos;
					}
				}
				break;

				case KEY_RIGHT:
				{
					if (inputMessageCurrentPos <= wcslen(inputMessage)) {
						++inputMessageCurrentPos;
					}
				}
				break;

				case KEY_CTRL('c'):
				{
					handleSigint(SIGINT);
				}
				break;

				default:
				{
					size_t inputMessageLength = wcslen(inputMessage);
					if (inputMessageLength < INPUT_MESSAGE_MAX_LENGTH) {
						string_insert_char(inputMessage, inputMessageCurrentPos, c);
						++inputMessageCurrentPos;
					}
				}
				break;
			}

			// Print currently-entered message onto the input window,
			// and move the cursor to the correct position.
			werase(messageInputWindow);
			wchar_t k = inputMessage[inputMessageCurrentPos];
			inputMessage[inputMessageCurrentPos] = 0;
			mvwaddwstr(messageInputWindow, 0, 0, inputMessage);
			int curX, curY;
			getyx(messageInputWindow, curY, curX);
			
			inputMessage[inputMessageCurrentPos] = k;
			werase(messageInputWindow);
			mvwaddwstr(messageInputWindow, 0, 0, inputMessage);
			wmove(messageInputWindow, curY, curX);
			wrefresh(messageInputWindow);
		}

		if (stop) {
			teardownApplication();
		}
		if (wcslen(inputMessage) == 0) continue;
		MessageSendStatus sendStatus = client_sendMessageToServer(sockfd, username, inputMessage);
		if (sendStatus != SEND_SUCCESS) {
			fatalError("SEND ERROR");
		}
		werase(messageInputWindow);
		wrefresh(messageInputWindow);
	}
}
