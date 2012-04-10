#include "Echo.h"
#include "Log.h"
#include "winConsole.h"
#include <cstdio>

CEcho::CEcho(SendFunc sendFunc)
:m_sendFunc(sendFunc)
{

}

CEcho::~CEcho()
{

}

void CEcho::Process(int clientID, const char* pData, int size)
{
	if (m_asynClients.find(clientID) == m_asynClients.end())
		Send(clientID, pData, size);
	else
		CONSOLE->printf("Client[%d] receive: %s\n", clientID, pData);
}

void CEcho::Send(int clientID, const char* pData, int size)
{
	if (!m_sendFunc(clientID, pData, size))
		LOG("Client[%d] fail to send message.", clientID);
}

void CEcho::End(int clientID)
{
	CONSOLE->printf("Client[%d] disconnect.\n", clientID);
	m_asynClients.erase(clientID);
}

void CEcho::AddAsynClient(int clientID)
{
	if (!m_asynClients.insert(Clients::value_type(clientID)).second)
		LOG("Client[%d] already exist.", clientID);
}