#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

#pragma comment(lib, "Ws2_32.lib")

/*
	STEPS TO CREATE SERVER:
		1. Initialize Winsock.
		2. Create a socket.
		3. Bind the socket.
		4. Listen on the socket for client.
		5. Accept a connection from a client.
		6. Recieve and send data.
		7. Disconnect.
*/

int __cdecl main(void)
{
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	//Initialize Winsock
	/*
		WSAStartup(): Initiates the use of the winsock DLL by a process

		Arguments:
			WORD wVersionRequired	:	sets the version of the socket to be created.
			LPWSADATA lpWSAData		:	gets the feedback of the windows sockets implementation.
	*/
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));	//Fills the memory with zeroes
	
	//make the IP address details
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	//Resolve the server address and ports
	/*
		getaddrinfo(): returns protocol independant translation from an ANSI hostname to ip addr
		In simple words, converts the IP address to machine readable language and gets info about host.

		Arguments :
			PCSTR pNodeName			:	contains the host name or numeric host address.
			PCSTR pServiceName		:	contains the service name or the port number.
			const ADDRINFOA *pHints	:	provides hints about the type of socket.
			PADDRINFOA *ppResult	:	gets the response information about the host.
	*/
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0)
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	//Create a SOCKET for listening to incoming requests
	/*
		socket(): creates an unbound socket and returns the file descriptor that can be later used to operate on the sockets.

		Arguments:
			int af			:	specifies the communication in which the SOCKET is to be created.
			int type		:	specifies the type of SOCKET to be created.
			int protocol	:	specifies the protocol to be used in SOCKET.
	*/
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	//Bind the SOCKET for TCP connection
	/*
		bind(): associates a local address with a SOCKET.

		Arguments:
			SOCKET s				:	descriptor of an unbound SOCKET.
			const sockaddr *addr	:	address of host.
			int namelen				:	length, in bytes, of the value.
	*/
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with the error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		return 1;
	}

	/*
		freeaddrinfo(): frees address information that the getaddrinfo dynamically allocates.
	*/
	freeaddrinfo(result);

	//Listen for incoming connection for the client socket
	/*
		listen(): function places a socket in a state of listening for incoming connection.

		Arguments:
			SOCKET s	:	descriptor of unbound, unconnected SOCKET.
			int backlog	:	maximum length of queue for pending connection.
	*/
	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	//Accept a client socket
	/*
		accept(): permits an incoming connection attempt on a SOCKET.

		Arguments:
			SOCKET s		:	descriptor of SOCKET in listening state.
			sockaddr *addr	:	optional pointer to address of connecting entity.
			int *addrlen	:	optional pointer to length of structure of addr parameter.
	*/
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET)
	{
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	//Server socket no longer needed
	closesocket(ListenSocket);

	do
	{
		/*
			recv(): recieves the data from the connected socket or a bound connectionless socket.

			Arguments:
				SOCKET s	:	descriptor for the SOCKET.
				char *buf	:	buffer to recieve the incoming data.
				int len		:	length of the buffer.
				int flags	:	for the modes of function.
		*/
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
		{
			printf("Bytes recieved: %d\n", iResult);

			//Echo the buffer back to sender
			/*
				send(): sends data to a connected SOCKET.

				Arguments:
					SOCKET s		:	descriptor identifying a connected SOCKET.
					const char *buf	:	buffer containing the data to be transmitted.
					int len			:	length of data in the buffer.
					int flags		:	specifies the way in which the call is made.
			*/
			iSendResult = send(ClientSocket, recvbuf, iResult, 0);
			if (iSendResult == SOCKET_ERROR)
			{
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}
			printf("Bytes sent: %d\n", iSendResult);
		}
		else if (iResult == 0)
		{
			printf("connection closing...\n");
		}
		else
		{
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}
	} while (iResult > 0);

	//shutdown the connection
	/*
		shutdown(): disables sends or recieves on a SOCKET.

		Arguments;
			SOCKET s	:	descriptor identifying a SOCKET.
			int how		:	flag that describes what types of operation will be stopped.
	*/
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR)
	{
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	//Cleanup
	closesocket(ClientSocket);
	WSACleanup();

	getchar();
	return 0;
}