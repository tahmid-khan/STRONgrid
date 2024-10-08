/*
*  TcpClient.cpp
*
*  Copyright (C) 2017 Luigi Vanfretti
*
*  This file is part of StrongridDLL.
*
*  StrongridDLL is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  StrongridDLL is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with StrongridDLL.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#include <cstring>      // std::memset
#include <sstream>      // std::stringstream

#ifdef _WIN32
#	include <winerror.h>    // WSAETIMEDOUT
#	include <WinSock2.h>    // WSAGetLastError, SOCK_STREAM, SOL_SOCKET, SO_RCVTIMEO, connect, recv, send, setsockopt, socket, closesocket
#	include <ws2def.h>      // AF_UNSPEC, addrinfo
#	include <WS2tcpip.h>    // freeaddrinfo, getaddrinfo
#else
#	include <cerrno>        // EAGAIN, EWOULDBLOCK, errno
#	include <netdb.h>       // addrinfo, freeaddrinfo, getaddrinfo
#	include <sys/socket.h>  // AF_UNSPEC, SOCK_STREAM, SOL_SOCKET, SO_RCVTIMEO, connect, recv, send, setsockopt, socket
#	include <unistd.h>      // close
#	define closesocket  close
#endif // _WIN32

#include "../StrongridBase/common.h"
#include "TcpClient.h"

using namespace strongridbase;
using namespace strongridclientbase;

TcpClient::TcpClient(std::string ipAddress, int port)
{
	m_sockfd = 0;
	m_ipAddr = ipAddress;
	m_port = port;
	m_win32Initialized  = false;
	InitializeWindowsSocket();
}

TcpClient::~TcpClient()
{
	Close();
}

int TcpClient::GetSocketDescriptor() const
{
	return m_sockfd;
}

void TcpClient::Connect()
{
	int sockfd;
    addrinfo hints, *servinfo, *p;
    int rv;


    std::memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;


	std::stringstream ss; ss << m_port;
    if ((rv = getaddrinfo(m_ipAddr.c_str(), ss.str().c_str(), &hints, &servinfo)) != 0)
        throw Exception("Unable to get address information.");

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            closesocket(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL)
        throw Exception("Failed to connect to host");

    freeaddrinfo(servinfo); // all done with this structure

	m_sockfd = sockfd;
}

void TcpClient::Close()
{
	if( m_sockfd != 0 ) closesocket(m_sockfd);
	m_sockfd = 0;
}

int TcpClient::Send( const char* data, int length )
{
	int bytesSent = 0;
	while( bytesSent < length )
	{
		int retVal = send(m_sockfd, data + bytesSent, length - bytesSent, 0 );
		if( retVal <= 0 )
		{
			Close();
			throw Exception("Unable to send data!");
		}

		bytesSent += retVal;
	}
	return bytesSent;
}

int TcpClient::Recv( char* refData, int length, int timeoutMs )
{
	int bytesReceived = 0;
	while( bytesReceived < length )
	{

		// set timeout
#ifdef _WIN32
		DWORD dwto = timeoutMs;
		setsockopt(m_sockfd,SOL_SOCKET, SO_RCVTIMEO , (const char*)&dwto, sizeof(DWORD) );
#else
		timeval timeout {timeoutMs/1000, timeoutMs%1000 * 1000};  // millisec -> {sec, usec}
		setsockopt(m_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#endif // _WIN32

		// Perform read
		int retVal = recv(m_sockfd, refData + bytesReceived, length - bytesReceived, 0 );
		if( retVal <= 0 )
		{

#ifdef _WIN32
			if( WSAGetLastError() == WSAETIMEDOUT )
#else
			if (errno == EAGAIN || errno == EWOULDBLOCK)
#endif // _WIN32
				throw SocketTimeout("Unable to read within timeout");
			else {
				Close();
				throw SocketException("An error ocurred while attempting to read data");
			}
		}

		bytesReceived += retVal;
	}
	return bytesReceived;
}


void TcpClient::InitializeWindowsSocket()
{
	// Only initialize once
	if( m_win32Initialized == true ) return;
	m_win32Initialized = true;

#ifdef _WIN32	// no need to initialize on Unix
	// Initialize window32 socket
	 WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2,0), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
		throw Exception("Unable to initialize winsock2!");
    }
#endif // _WIN32
}
