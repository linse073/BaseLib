#include "Server.h"
#include "Log.h"
#include "Manager.h"

CServer::CServer()
:m_listenSocket(INVALID_SOCKET), m_addClientEvent(NULL)
{

}

CServer::~CServer()
{
	Stop();
}

bool CServer::Start(const char* ip, int port, HANDLE completePort)
{
	do 
	{
		m_listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (m_listenSocket == INVALID_SOCKET)
		{
			LOGNET("Server fail to create listen socket.");
			break;
		}

		const BOOL trueOptVal = TRUE;
		if (setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&trueOptVal, sizeof(trueOptVal)) == SOCKET_ERROR)
		{
			LOGNET("Server fail to set listen socket to reuse address.");
			break;
		}

		if (setsockopt(m_listenSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&trueOptVal, sizeof(trueOptVal)) == SOCKET_ERROR)
		{
			LOGNET("Server fail to set listen socket to no delay.");
			break;
		}

		//NOTIFY: It's no need to set SIO_KEEPALIVE_VALS option.

		SOCKADDR_IN listenAddr;
		memset(&listenAddr, 0, sizeof(listenAddr));
		listenAddr.sin_family = AF_INET;
		listenAddr.sin_port = htons(port);
		listenAddr.sin_addr.s_addr = inet_addr(ip);

		if (bind(m_listenSocket, (SOCKADDR*)&listenAddr, sizeof(listenAddr)) == SOCKET_ERROR)
		{
			LOGNET("Server fail to bind listen socket.");
			break;
		}

		if (listen(m_listenSocket, LISTEN_BACKLOG) == SOCKET_ERROR)
		{
			LOGNET("Server fail to listen.");
			break;
		}

		if (CreateIoCompletionPort((HANDLE)m_listenSocket, completePort, 0, 0) == NULL)
		{
			LOG("Server fail to associate listen socket with completion port.");
			break;
		}

		m_addClientEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (m_addClientEvent == NULL)
		{
			LOG("Server fail to create adding client event.");
			break;
		}
		if (WSAEventSelect(m_listenSocket, m_addClientEvent, FD_ACCEPT) == SOCKET_ERROR)
		{
			LOGNET("Server fail to associate listen socket with adding client event.");
			break;
		}

		return true;
	} while (false);

	Stop();
	return false;
}

void CServer::Stop()
{
	if (m_addClientEvent)
	{
		CloseHandle(m_addClientEvent);
		m_addClientEvent = NULL;	
	}

	if (m_listenSocket)
	{
		closesocket(m_listenSocket);
		m_listenSocket = INVALID_SOCKET;
	}
}

SOCKET CServer::GetListenSocket() const
{
	return m_listenSocket;
}

HANDLE CServer::GetAddClientEvent() const
{
	return m_addClientEvent;
}