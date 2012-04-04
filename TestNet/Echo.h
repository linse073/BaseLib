#ifndef NETLIB_ECHO
#define NETLIB_ECHO

#include "Base.h"
#include <set>

class CEcho
{
private:
	typedef std::tr1::function<bool(int, const char*, int)> SendFunc;

	typedef std::set<int> Clients;
	typedef Clients::iterator ClientsIter;

public:
	CEcho(SendFunc sendFunc);
	~CEcho();
	void Process(int clientID, const char* pData, int size);
	void Send(int clientID, const char* pData, int size);
	void End(int clientID);
	void AddAsynClient(int clientID);

private:
	SendFunc m_sendFunc;

	Clients m_asynClients;
};

#endif