#include "Client.h"
#include "Server.h"
#include "Log.h"
#include "Lock.h"
#include "Manager.h"
#include <MSWSock.h>
#include <ctime>

CClient::CClient(CServer* pServer, CManager* pManager)
:m_socket(INVALID_SOCKET), m_pServer(pServer), m_pManager(pManager), m_ID(pManager->GenClientID()),
m_lastActionTime(0), m_completePort(false), m_status(NETSTATUS_Dead),
m_recv(RECV_BUFFER_SIZE, NETOP_Receive), m_send(SEND_BUFFER_SIZE, NETOP_Send), 
m_discon(DISCON_BUFFER_SIZE, NETOP_Disconnect), m_accept(ACCEPT_BUFFER_SIZE, NETOP_Accept)
{
	InitializeCriticalSection(&m_criticalSection);
}

CClient::~CClient()
{
	Stop();
	DeleteCriticalSection(&m_criticalSection);
}

bool CClient::Start()
{
	do 
	{
		if (m_socket == INVALID_SOCKET)
		{
			m_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			if (m_socket == INVALID_SOCKET)
			{
				LOGNET("Fail to create client[%d] socket.", GetID());
				break;
			}
		}

		static GUID GuidAcceptEx = WSAID_ACCEPTEX;
		static LPFN_ACCEPTEX AcceptEx = (LPFN_ACCEPTEX)GetExtensionFunction(m_pServer->GetListenSocket(), GuidAcceptEx);
		NetData* pData = m_accept.GetNetData();
		pData->extra = this;
		DWORD recvLen = 0;
		if (AcceptEx(m_pServer->GetListenSocket(), m_socket, pData->buffer.buf, 0, 
			sizeof(SOCKADDR_IN)+16, sizeof(SOCKADDR_IN)+16, &recvLen, &pData->overlapped) == FALSE)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				LOGNET("Client[%d] fail to call AcceptEx.", GetID());
				break;
			}
		}

		return true;
	} while (false);

	Stop();
	return false;
}

void CClient::Stop()
{
	CLock lock(&m_criticalSection);
	if (m_socket != INVALID_SOCKET)
	{
		shutdown(m_socket, SD_SEND);
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}
	m_completePort = false;
	m_status = NETSTATUS_Dead;
}

bool CClient::PostConnect(NetData* pData)
{
	CLock lock(&m_criticalSection);
	m_status = NETSTATUS_Connected;
	SOCKADDR* localAddr = NULL;
	SOCKADDR* remoteAddr = NULL;
	int localAddrLen = 0;
	int remoteAddrLen = 0;
	GetAcceptExSockaddrs(pData->buffer.buf, 0,
		sizeof(SOCKADDR_IN)+16, sizeof(SOCKADDR_IN)+16, 
		&localAddr, &localAddrLen, &remoteAddr, &remoteAddrLen);

	SOCKET listenSocket = m_pServer->GetListenSocket();
	if (setsockopt(m_socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (const char*)&listenSocket, sizeof(listenSocket)) == SOCKET_ERROR)
	{
		LOGNET("Client[%d] fail to update accept context.", GetID());
		return false;
	}

	if (!m_completePort)
	{
		if (CreateIoCompletionPort((HANDLE)m_socket, m_pManager->GetIOCompletePort(), (ULONG_PTR)this, 0) == NULL)
		{
			LOG("Fail to associate client[%d] socket with completion port.", GetID());
			return false;
		}
		m_completePort = true;
	}
	m_send.Clean();
	m_recv.Clean();
	m_discon.Clean();
	m_accept.Clean();
	m_lastActionTime = 0;
	m_status = NETSTATUS_CompletionNotify;
	return true;
}

bool CClient::PreSend(const char* buf, int size)
{
	CLock lock(&m_criticalSection);
	if (m_status != NETSTATUS_CompletionNotify)
		return false;
	bool pendingSend = m_send.HasData();
	if (!m_send.Write(buf, size))
	{
		LOG("Client[%d] fail to send message, then restart it.", GetID());
		Restart(NETOP_Send);
		return false;
	}
	if (!pendingSend)
		AppendSend();
	return true;
}

bool CClient::PostSend(int size)
{
	CLock lock(&m_criticalSection);
	m_send.MoveHead(size);
	return m_send.HasData();
}

bool CClient::AppendSend()
{
	CLock lock(&m_criticalSection);
	if (m_status != NETSTATUS_CompletionNotify)
		return true;
	m_send.Tidy();
	NetData* pData = m_send.GetNetData();
	DWORD sendBytes = 0;
	if (WSASend(m_socket, &pData->buffer, 1, &sendBytes, 0, &pData->overlapped, NULL) == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			LOGNET("Client[%d] fail to call WSASend.", GetID());
			return false;
		}
	}
	return true;
}

void CClient::PostRead(int size)
{
	m_recv.MoveHead(size);
	m_recv.Tidy();
	if (!m_recv.HasSpace())
	{
		LOG("There is no space to receive in client[%d].", GetID());
		m_recv.Clean();
	}
}

void CClient::PostReceive(int size)
{
	m_recv.MoveTail(size);
	m_lastActionTime = (int)time(NULL);
}

bool CClient::AppendReceive()
{
	CLock lock(&m_criticalSection);
	if (m_status != NETSTATUS_CompletionNotify)
		return true;
	NetData* pData = m_recv.GetNetData();
	DWORD recvBytes = 0;
	DWORD flag = 0;
	if (WSARecv(m_socket, &pData->buffer, 1, &recvBytes, &flag, &pData->overlapped, NULL) == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			LOGNET("Client[%d] fail to call WSARecv.", GetID());
			return false;
		}
	}
	return true;
}

bool CClient::AppendDisconnect(bool restart)
{
	static GUID GuidDisconnectEx = WSAID_DISCONNECTEX;
	static LPFN_DISCONNECTEX DisconnectEx = (LPFN_DISCONNECTEX)GetExtensionFunction(m_socket, GuidDisconnectEx);
	CLock lock(&m_criticalSection);
	NetData* pData = m_discon.GetNetData();
	if (DisconnectEx(m_socket, &pData->overlapped, TF_REUSE_SOCKET, 0) == FALSE)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			LOGNET("Client[%d] to call DisconnectEx.", GetID());
			return false;
		}
	}
	m_status = NETSTATUS_Disconnecting;
	m_lastActionTime = (int)time(NULL);
	return true;
}

void CClient::PostDisconnect()
{
	CLock lock(&m_criticalSection);
	m_status = NETSTATUS_Disconnected;
	(m_pManager->GetDisconnectFunc())(GetID());
	if (!Start())
	{
		LOG("Fail to start client[%d].", GetID());
		Restart(NETOP_Disconnect);
	}
}

void CClient::Restart(NETOP curOp)
{
	CLock lock(&m_criticalSection);
	LOG("Restart client[%d], current status[%s] and operation[%s].", GetID(), NETSTATUSSTR[m_status], NETOPSTR[curOp]);
	switch (m_status)
	{
	case NETSTATUS_Dead:
		{
			if (curOp == NETOP_Accept)
				LOG("Cann't start client[%d].", GetID());
		}
		break;
	case NETSTATUS_Connected:
		{
			if (curOp == NETOP_Accept)
			{
				Stop();
				if (!Start())
					LOG("Fail to start client[%d].", GetID());
			}
		}
		break;
	case NETSTATUS_CompletionNotify:
		{
			if (CancelIoEx((HANDLE)m_socket, NULL) == 0)
			{
				if (GetLastError() != ERROR_NOT_FOUND)
					LOG("Client[%d] fail to call CancelIoEx.", GetID());
			}
			if (!AppendDisconnect(true))
			{
				LOG("Client[%d] fail to append disconnect.", GetID());
				Stop();
				if (!Start())
					LOG("Fail to start client[%d].", GetID());
			}
		}
		break;
	case NETSTATUS_Disconnecting:
	case NETSTATUS_Disconnected:
		{
			if (curOp==NETOP_Disconnect || curOp==NETOP_Accept)
			{
				Stop();
				if (!Start())
					LOG("Fail to start client[%d].", GetID());
			}
		}
		break;
	default:
		LOG("Client[%d] get an unexpected status[%s].", GetID(), NETSTATUSSTR[m_status]);
		break;
	}
}

void CClient::TimeRoutine(int now)
{
	CLock lock(&m_criticalSection);
	switch (m_status)
	{
	case NETSTATUS_CompletionNotify:
		{
			if (m_lastActionTime)
			{
				if (now-m_lastActionTime > MAX_IDLE_TIME)
				{
					LOG("Client[%d] has been idled for a long time.", GetID());
					Restart(NETOP_Receive);
				}
			}
			else
			{
				DWORD connectTime = 0;
				int optSize = sizeof(connectTime);
				if (getsockopt(m_socket, SOL_SOCKET, SO_CONNECT_TIME, (char*)&connectTime, &optSize) == SOCKET_ERROR)
				{
					LOGNET("Client[%d] fail to get socket connection time.", GetID());
				}
				else
				{
					if (connectTime!=0xFFFFFFFF && connectTime>DENIAL_SERVICE_TIME)
					{
						LOG("Client[%d] may be a malicious application.", GetID());
						Restart(NETOP_Receive);
					}
				}
			}
		}
		break;
	case NETSTATUS_Disconnecting:
		{
			if (m_lastActionTime)
			{
				if (now-m_lastActionTime > MAX_DISCONNECT_TIME)
				{
					LOG("Client[%d] has been in disconnecting for a long time.", GetID());
					Restart(NETOP_Disconnect);
				}
			}
			else
			{
				LOG("Client[%d] has been in disconnecting, but last action time is zero.", GetID());
			}
		}
		break;
	default:
		break;
	}
}

int CClient::GetID() const
{
	return m_ID;
}

void CClient::PreRead(const char** buf, int* size) const
{
	m_recv.PreRead(buf, size);
}