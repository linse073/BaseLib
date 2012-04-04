#include "Manager.h"
#include "AsynClient.h"
#include "Client.h"
#include "Log.h"
#include "Lock.h"
#include <process.h>
#include <ctime>

CManager::CManager(RecvFunc recvFunc, DisconFunc disconFunc)
:m_completePort(NULL), m_threadNum(0), 
m_threads(NULL), m_endEvent(NULL), m_timeThread(NULL),
m_recvFunc(recvFunc), m_disconFunc(disconFunc), m_clientIDSeed(0)
{
	InitializeCriticalSection(&m_criticalSection);
}

CManager::~CManager()
{
	Stop();
	DeleteCriticalSection(&m_criticalSection);
}

bool CManager::Start(int threadNum)
{
	do 
	{
		m_completePort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (m_completePort == NULL)
		{
			LOG("Manager fail to create completion port.");
			break;
		}

		if (threadNum == 0)
		{
			SYSTEM_INFO systemInfo;
			GetSystemInfo(&systemInfo);
			m_threadNum = 2*systemInfo.dwNumberOfProcessors + 1;
		}
		else
		{
			m_threadNum = threadNum;
		}
		m_threads = new HANDLE[m_threadNum];
		for (int i=0; i<m_threadNum; ++i)
			m_threads[i] = (HANDLE)_beginthreadex(NULL, 0, CompletionRoutine, this, 0, NULL);

		m_endEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (m_endEvent == NULL)
		{
			LOG("Manager fail to create end event.");
			break;
		}

		m_timeThread = (HANDLE)_beginthreadex(NULL, 0, TimeRoutine, this, 0, NULL);
		if (m_timeThread == NULL)
		{
			LOG("Manager fail to create time routine thread.");
			break;
		}

		return true;
	} while (false);

	Stop();
	return false;
}

void CManager::Stop()
{
	if (m_timeThread)
	{
		if (m_endEvent)
		{
			if (SetEvent(m_endEvent))
			{
				if (WaitForSingleObject(m_timeThread, INFINITE) != WAIT_OBJECT_0)
					LOG("Manager fail to end time routine.");
			}
			else
			{
				LOG("Manager fail to set end event.");
			}
		}
		CloseHandle(m_timeThread);
		m_timeThread = NULL;
	}
	if (m_endEvent)
	{
		CloseHandle(m_endEvent);
		m_endEvent = NULL;
	}

	if (m_completePort)
	{
		for (int i=0; i<m_threadNum; ++i)
			PostQueuedCompletionStatus(m_completePort, 0, 0, NULL);
		if (WaitForMultipleObjects(m_threadNum, m_threads, TRUE, INFINITE) != WAIT_OBJECT_0)
			LOG("Server fail to end completion routine.");
		for (int i=0; i<m_threadNum; ++i)
			CloseHandle(m_threads[i]);
		m_threads = NULL;
		m_threadNum = 0;
		CloseHandle(m_completePort);
		m_completePort = NULL;
	}

	{
		CLock lock(&m_criticalSection);
		for (ClientsIter iter=m_clients.begin(); iter!=m_clients.end(); ++iter)
			delete iter->second;
		m_clients.clear();
	}
	m_clientIDSeed = 0;
	m_server.Stop();
}

bool CManager::StartServer(const char* ip, int port)
{
	return m_server.Start(ip, port, GetIOCompletePort());
}

int CManager::AddClient(const char* ip, int port)
{
	CAsynClient* pClient = new CAsynClient(this);
	if (!pClient->Start(ip, port))
	{
		LOG("Manager fail to start client[%d].", pClient->GetID());
		delete pClient;
		return 0;
	}
	bool result = false;
	{
		CLock lock(&m_criticalSection);
		result = m_clients.insert(Clients::value_type(pClient->GetID(), pClient)).second;
	}
	if (!result)
	{
		LOG("Manager fail to add client[%d].", pClient->GetID());
		delete pClient;
		return 0;
	}
	return pClient->GetID();
}

bool CManager::Send(int clientID, const char* buf, int size)
{
	IClient* pClient = NULL;
	{
		CLock lock(&m_criticalSection);
		ClientsIter iter = m_clients.find(clientID);
		if (iter == m_clients.end())
		{
			LOG("Manager can't find client[%d].", clientID);
			return false;
		}
		pClient = iter->second;
	}
	return pClient->PreSend(buf, size);
}

HANDLE CManager::GetIOCompletePort() const
{
	return m_completePort;
}

CManager::DisconFunc CManager::GetDisconnectFunc() const
{
	return m_disconFunc;
}

unsigned WINAPI CManager::CompletionRoutine(void* param)
{
	CManager* pManager = (CManager*)param;
	while (true)
	{
		DWORD transferBytes = 0; 
		ULONG completeKey = 0;
		LPOVERLAPPED lpOverlapped = NULL;
		DWORD result = GetQueuedCompletionStatus(pManager->m_completePort, &transferBytes, &completeKey, &lpOverlapped, INFINITE);
		if (result == 0)
		{
			LOG("Completion routine fail to get completion status.");
			if (lpOverlapped)
			{
				NetData* pData = CONTAINING_RECORD(lpOverlapped, NetData, overlapped);
				switch (pData->operation)
				{
				case NETOP_Accept:
					{
						IClient* pClient = (IClient*)pData->extra;
						LOG("Client[%d] fail to accept connection, then restart it.", pClient->GetID());
						pClient->Restart(pData->operation);
					}
					break;
				case NETOP_Connect:
					{
						IClient* pClient = (IClient*)completeKey;
						LOG("Client[%d] fail to connect server, then restart it.", pClient->GetID());
						pClient->Restart(pData->operation);
					}
					break;
				case NETOP_Receive:
					{
						IClient* pClient = (IClient*)completeKey;
						LOG("Client[%d] fail to receive message, then restart it.", pClient->GetID());
						pClient->Restart(pData->operation);
					}
					break;
				case NETOP_Send:
					{
						IClient* pClient = (IClient*)completeKey;
						LOG("Client[%d] fail to send message, then restart it.", pClient->GetID());
						pClient->Restart(pData->operation);
					}
					break;
				case NETOP_Disconnect:
					{
						IClient* pClient = (IClient*)completeKey;
						LOG("Client[%d] fail to disconnect, then restart it.", pClient->GetID());
						pClient->Restart(pData->operation);
					}
					break;
				default:
					LOG("Completion routine get an unexpected operation[%s].", NETOPSTR[pData->operation]);
					break;
				}
			}
			else
			{
				LOG("Completion routine fail to get completion status and overlapped is null.");
			}
			continue;
		}
		if (lpOverlapped)
		{
			NetData* pData = CONTAINING_RECORD(lpOverlapped, NetData, overlapped);
			switch (pData->operation)
			{
			case NETOP_Accept:
				{
					IClient* pClient = (IClient*)pData->extra;
					if (pClient->PostConnect(pData))
					{
						if (transferBytes != 0)
							LOG("Client[%d] accept a connection successfully, but transferred bytes is not zero.", pClient->GetID());
						if (!pClient->AppendReceive())
						{
							LOG("Client[%d] fail to append receive, then restart it.", pClient->GetID());
							pClient->Restart(pData->operation);
						}
					}
					else
					{
						LOG("Client[%d] fail to call PostConnect.", pClient->GetID());
						pClient->Restart(pData->operation);
					}
				}
				break;
			case NETOP_Connect:
				{
					IClient* pClient = (IClient*)completeKey;
					if (pClient->PostConnect(pData))
					{
						if (transferBytes != 0)
							LOG("Client[%d] connect server successfully, but transferred bytes is not zero.", pClient->GetID());
						if (!pClient->AppendReceive())
						{
							LOG("Client[%d] fail to append receive, then restart it.", pClient->GetID());
							pClient->Restart(pData->operation);
						}
					}
					else
					{
						LOG("Client[%d] fail to call PostConnect.", pClient->GetID());
						pClient->Restart(pData->operation);
					}
				}
				break;
			case NETOP_Receive:
				{
					IClient* pClient = (IClient*)completeKey;
					if (transferBytes == 0)
					{
						if (!pClient->AppendDisconnect(false))
						{
							LOG("Client[%d] fail to append disconnect, then restart it.", pClient->GetID());
							pClient->Restart(pData->operation);
						}
					}
					else
					{
						pClient->PostReceive(transferBytes);
						const char* buf = NULL;
						int size = 0;
						pClient->PreRead(&buf, &size);
						pClient->PostRead(pManager->m_recvFunc(pClient->GetID(), buf, size));
						if (!pClient->AppendReceive())
						{
							LOG("Client[%d] fail to append receive, then restart it.", pClient->GetID());
							pClient->Restart(pData->operation);
						}
					}
				}
				break;
			case NETOP_Send:
				{
					IClient* pClient = (IClient*)completeKey;
					if (pClient->PostSend(transferBytes))
					{
						if (!pClient->AppendSend())
						{
							LOG("Client[%d] fail to append send, then restart it.", pClient->GetID());
							pClient->Restart(pData->operation);
						}
					}
				}
				break;
			case NETOP_Disconnect:
				{
					IClient* pClient = (IClient*)completeKey;
					pClient->PostDisconnect();
				}
				break;
			default:
				LOG("Completion routine get an unexpected operation[%s].", NETOPSTR[pData->operation]);
				break;
			}
		}
		else
		{
			LOG("Completion routine get a null overlapped, then exit.");
			break;
		}
	}
	return 0;
}

unsigned WINAPI CManager::TimeRoutine(void* param)
{
	CManager* pManager = (CManager*)param;
	int nextWait = (int)time(NULL) + WAIT_TIME_INTERVAL;
	while (true)
	{
		HANDLE addClientEvent = pManager->m_server.GetAddClientEvent();
		HANDLE events[] = {pManager->m_endEvent, addClientEvent};
		DWORD eventsNum = sizeof(events)/sizeof(events[0]);
		if (addClientEvent == NULL)
			eventsNum--;
		int now = (int)time(NULL);
		while (now > nextWait)
			nextWait += WAIT_TIME_INTERVAL;
		DWORD result = WSAWaitForMultipleEvents(eventsNum, events, FALSE, nextWait-now, FALSE);
		switch (result)
		{
		case WSA_WAIT_EVENT_0:
			return 0;
			break;
		case WSA_WAIT_EVENT_0+1:
			pManager->AddClient();
			break;
		case WSA_WAIT_TIMEOUT:
			pManager->Maintain();
			break;
		case WSA_WAIT_FAILED:
			LOGNET("Time routine fail to wait events.");
			break;
		default:
			LOG("Time routine get an unexpected result.");
			break;
		}
	}
	return 0;
}

void CManager::Maintain()
{
	int now = (int)time(NULL);
	Clients tempClients;
	{
		CLock lock(&m_criticalSection);
		tempClients = m_clients;
	}
	for (ClientsIter iter=tempClients.begin(); iter!=tempClients.end(); ++iter)
	{
		IClient* pClient = iter->second;
		pClient->TimeRoutine(now);
	}
}

int CManager::GenClientID()
{
	return InterlockedIncrement((LONG*)&m_clientIDSeed);
}

void CManager::AddClient()
{
	for (int i=0; i<ADD_CLIENT_NUM_PER_TIME; ++i)
	{
		CClient* pClient = new CClient(&m_server, this);
		if (!pClient->Start())
		{
			LOG("Manager fail to start client[%d].", pClient->GetID());
			delete pClient;
			continue;
		}
		bool result = false;
		{
			CLock lock(&m_criticalSection);
			result = m_clients.insert(Clients::value_type(pClient->GetID(), pClient)).second;
		}
		if (!result)
		{
			LOG("Manager fail to add client[%d].", pClient->GetID());
			delete pClient;
		}
	}
}