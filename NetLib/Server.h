#ifndef NETLIB_SERVER
#define NETLIB_SERVER

#include "Base.h"

class CServer
{
public:
	CServer();
	~CServer();
	bool Start(const char* ip, int port, HANDLE completePort);
	void Stop();
	bool Send(int clientID, const char* buf, int size);

	SOCKET GetListenSocket() const;
	HANDLE GetAddClientEvent() const;

private:
	SOCKET m_listenSocket;
	HANDLE m_addClientEvent;
};

#endif