#ifndef DHS_CLIENT_SOCKET_H
#define DHS_CLIENT_SOCKET_H

#ifdef __WIN32__
#define _WIN32_WINNT  0x501
#include <winsock2.h>
#include <ws2tcpip.h>

#define socketStartup() {WSADATA wsaData;\
	WSAStartup(0x202, &wsaData);\
}

#define socketCleanup()  WSACleanup()
#define socketGetError() WSAGetLastError()

#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#define socketStartup()
#define socketCleanup()
#define socketGetError() errno

#define INVALID_SOCKET	-1
#define SOCKET_ERROR	-1
#endif

#endif /*DHS_CLIENT_SOCKET_H*/
