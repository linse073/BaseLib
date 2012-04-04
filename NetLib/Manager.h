#ifndef NETLIB_MANAGER
#define NETLIB_MANAGER

#include "IClient.h"
#include "Server.h"
#include <map>

class CManager
{
private:
	typedef std::map<int, IClient*> Clients;
	typedef Clients::iterator ClientsIter;

	typedef std::tr1::function<int(int, const char*, int)> RecvFunc;
	typedef std::tr1::function<void(int)> DisconFunc;

public:
	CManager(RecvFunc recvFunc, DisconFunc disconFunc);
	~CManager();

	bool Start(int threadNum=0);
	void Stop();

	bool StartServer(const char* ip, int port);
	int AddClient(const char* ip, int port);
	bool Send(int clientID, const char* buf, int size);
	int GenClientID();

	HANDLE GetIOCompletePort() const;
	DisconFunc GetDisconnectFunc() const;

private:
	static unsigned WINAPI CompletionRoutine(void* param);
	static unsigned WINAPI TimeRoutine(void* param);

	void Maintain();
	void AddClient();

	HANDLE m_completePort;
	HANDLE m_endEvent;
	HANDLE m_timeThread;

	Clients m_clients;
	int m_clientIDSeed;

	int m_threadNum;
	HANDLE* m_threads;

	RecvFunc m_recvFunc;
	DisconFunc m_disconFunc;

	CServer m_server;

	CRITICAL_SECTION m_criticalSection;
};

#endif