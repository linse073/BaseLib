#include "AsynClient.h"
#include "Log.h"
#include "Lock.h"
#include "Manager.h"
#include <process.h>
#include <MSWSock.h>

CAsynClient::CAsynClient(CManager* pManager)
:m_socket(INVALID_SOCKET), m_status(NETSTATUS_Dead),
m_receive(RECV_BUFFER_SIZE, NETOP_Receive), m_send(SEND_BUFFER_SIZE, NETOP_Send),
m_connect(CONNECT_BUFFER_SIZE, NETOP_Connect), m_discon(DISCON_BUFFER_SIZE, NETOP_Disconnect),
m_serverIP(), m_serverPort(0), m_restart(false), m_pManager(pManager), m_ID(pManager->GenClientID())
{
	InitializeCriticalSection(&m_criticalSection);
}

CAsynClient::~CAsynClient()
{
	Stop();
	DeleteCriticalSection(&m_criticalSection);
}

bool CAsynClient::Start(const char* ip, int port)
{
	do 
	{
		m_serverIP = ip;
		m_serverPort = port;
		if (!Connect())
		{
			LOG("Asynchronous client[%d] fail to connect server.", GetID());
			break;
		}

		return true;
	} while (false);

	Stop();
	return false;
}

void CAsynClient::Stop()
{
	CLock lock(&m_criticalSection);
	if (m_socket != INVALID_SOCKET)
	{
		shutdown(m_socket, SD_SEND);
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}
	m_status = NETSTATUS_Dead;
}

bool CAsynClient::PreSend(const char* buf, int size)
{
	CLock lock(&m_criticalSection);
	if (m_status != NETSTATUS_Connected)
		return false;
	bool pendingSend = m_send.HasData();
	if (!m_send.Write(buf, size))
	{
		LOG("Asynchronous client[%d] fail to send message, then restart it.", GetID());
		Restart(NETOP_Send);
		return false;
	}
	if (!pendingSend)
		AppendSend();
	return true;
}

bool CAsynClient::Connect()
{
	if (m_socket == INVALID_SOCKET)
	{
		m_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (m_socket == INVALID_SOCKET)
		{
			LOGNET("Fail to create asynchronous client[%d] socket.", GetID());
			return false;
		}

		const BOOL trueOptVal = TRUE;
		if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&trueOptVal, sizeof(trueOptVal)) == SOCKET_ERROR)
		{
			LOGNET("Fail to set asynchronous client[%d] socket to reuse address.", GetID());
			return false;
		}

		if (setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&trueOptVal, sizeof(trueOptVal)) == SOCKET_ERROR)
		{
			LOGNET("Fail to set asynchronous client[%d] socket to no delay.", GetID());
			return false;
		}

		SOCKADDR_IN bindAddr;
		memset(&bindAddr, 0, sizeof(bindAddr));
		bindAddr.sin_family = AF_INET;
		bindAddr.sin_port = htons(0);
		bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		if (bind(m_socket, (SOCKADDR*)&bindAddr, sizeof(bindAddr)) == SOCKET_ERROR)
		{
			LOGNET("Fail to bind asynchronous client[%d] socket.", GetID());
			return false;
		}

		if (CreateIoCompletionPort((HANDLE)m_socket, m_pManager->GetIOCompletePort(), (ULONG_PTR)this, 0) == NULL)
		{
			LOG("Fail to associate asynchronous client[%d] socket with completion port.", GetID());
			return false;
		}
	}

	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(m_serverIP.c_str());
	serverAddr.sin_port = htons(m_serverPort);

	static GUID GuidConnectEx = WSAID_CONNECTEX;
	static LPFN_CONNECTEX ConnectEx = (LPFN_CONNECTEX)GetExtensionFunction(m_socket, GuidConnectEx);
	NetData* pData = m_connect.GetNetData();
	if (ConnectEx(m_socket, (const sockaddr*)&serverAddr, sizeof(serverAddr), NULL, 0, NULL, &pData->overlapped) == FALSE)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			LOGNET("Asynchronous client[%d] fail to call ConnectEx.", GetID());
			return false;
		}
	}
	return true;
}

bool CAsynClient::PostConnect(NetData* pData)
{
	CLock lock(&m_criticalSection);
	m_status = NETSTATUS_Connected;
	if (setsockopt(m_socket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0) == SOCKET_ERROR)
	{
		LOGNET("Asynchronous client[%d] fail to update connect context.", GetID());
		return false;
	}
	m_send.Clean();
	m_receive.Clean();
	m_discon.Clean();
	m_connect.Clean();
	return true;
}

bool CAsynClient::PostSend(int size)
{
	CLock lock(&m_criticalSection);
	m_send.MoveHead(size);
	return m_send.HasData();
}

bool CAsynClient::AppendSend()
{
	CLock lock(&m_criticalSection);
	if (m_status != NETSTATUS_Connected)
		return true;
	m_send.Tidy();
	NetData* pData = m_send.GetNetData();
	DWORD sendBytes = 0;
	if (WSASend(m_socket, &pData->buffer, 1, &sendBytes, 0, &pData->overlapped, NULL) == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			LOGNET("Asynchronous client[%d] fail to call WSASend.", GetID());
			return false;
		}
	}
	return true;
}

void CAsynClient::PostRead(int size)
{
	m_receive.MoveHead(size);
	m_receive.Tidy();
	if (!m_receive.HasSpace())
	{
		LOG("There is no space to receive in asynchronous client[%d].", GetID());
		m_receive.Clean();
	}
}

void CAsynClient::PostReceive(int size)
{
	m_receive.MoveTail(size);
}

bool CAsynClient::AppendReceive()
{
	CLock lock(&m_criticalSection);
	if (m_status != NETSTATUS_Connected)
		return true;
	NetData* pData = m_receive.GetNetData();
	DWORD recvBytes = 0;
	DWORD flag = 0;
	if (WSARecv(m_socket, &pData->buffer, 1, &recvBytes, &flag, &pData->overlapped, NULL) == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			LOGNET("Asynchronous client[%d] fail to call WSARecv.", GetID());
			return false;
		}
	}
	return true;
}

bool CAsynClient::AppendDisconnect(bool restart)
{
	static GUID GuidDisconnectEx = WSAID_DISCONNECTEX;
	static LPFN_DISCONNECTEX DisconnectEx = (LPFN_DISCONNECTEX)GetExtensionFunction(m_socket, GuidDisconnectEx);
	CLock lock(&m_criticalSection);
	NetData* pData = m_discon.GetNetData();
	if (DisconnectEx(m_socket, &pData->overlapped, TF_REUSE_SOCKET, 0) == FALSE)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			LOGNET("Asynchronous Client[%d] fail to call DisconnectEx.", GetID());
			return false;
		}
	}
	m_status = NETSTATUS_Disconnecting;
	m_restart = restart;
	return true;
}

void CAsynClient::PostDisconnect()
{
	CLock lock(&m_criticalSection);
	m_status = NETSTATUS_Disconnected;
	(m_pManager->GetDisconnectFunc())(GetID());
	if (m_restart)
	{
		if (!Connect())
		{
			LOG("Fail to start asynchronous client[%d].", GetID());
			Restart(NETOP_Disconnect);
		}
	}
}

void CAsynClient::Restart(NETOP curOp)
{
	CLock lock(&m_criticalSection);
	LOG("Restart asynchronous client[%d], current status[%s] and operation[%s].", GetID(), NETSTATUSSTR[m_status], NETOPSTR[curOp]);
	switch (m_status)
	{
	case NETSTATUS_Dead:
		{
			if (curOp == NETOP_Connect)
				LOG("Cann't start asynchronous client[%d].", GetID());
		}
		break;
	case NETSTATUS_Connected:
		{
			if (CancelIoEx((HANDLE)m_socket, NULL) == 0)
			{
				if (GetLastError() != ERROR_NOT_FOUND)
					LOG("Asynchronous client[%d] fail to call CancelIoEx.", GetID());
			}
			if (!AppendDisconnect(true))
			{
				LOG("Asynchronous client[%d] fail to reconnect server.", GetID());
				Stop();
				if (!Connect())
					LOG("Fail to start asynchronous client[%d].", GetID());
			}
		}
		break;
	case NETSTATUS_Disconnecting:
		{
			if (curOp == NETOP_Disconnect)
			{
				Stop();
				if (m_restart)
				{
					if (!Connect())
						LOG("Fail to start asynchronous client[%d].", GetID());
				}
			}
		}
		break;
	case NETSTATUS_Disconnected:
		{
			if (curOp==NETOP_Disconnect || curOp==NETOP_Connect)
			{
				Stop();
				if (!Connect())
					LOG("Fail to start asynchronous client[%d].", GetID());
			}
		}
		break;
	default:
		LOG("Asynchronous client[%d] get an unexpected status[%s].", GetID(), NETSTATUSSTR[m_status]);
		break;
	}
}

void CAsynClient::TimeRoutine(int now)
{
	CLock lock(&m_criticalSection);
	switch (m_status)
	{
	case NETSTATUS_Dead:
		{
			if (!Connect())
				LOG("Fail to start asynchronous client[%d].", GetID());
		}
		break;
	default:
		break;
	}
}

int CAsynClient::GetID() const
{
	return m_ID;
}

void CAsynClient::PreRead(const char** buf, int* size) const
{
	m_receive.PreRead(buf, size);
}