#include "Base.h"
#include "Log.h"

const char* NETOPSTR[] = 
{
	STR(NETOP_Accept),
	STR(NETOP_Receive),
	STR(NETOP_Send),
	STR(NETOP_Disconnect),
	STR(NETOP_Connect)
};

const char* NETSTATUSSTR[] =
{
	STR(NETSTATUS_Dead),
	STR(NETSTATUS_Connected),
	STR(NETSTATUS_CompletionNotify),
	STR(NETSTATUS_Disconnecting),
	STR(NETSTATUS_Disconnected)
};

void* GetExtensionFunction(SOCKET sock, GUID funcID)
{
	DWORD bytes = 0;
	void* funcAddr = NULL;
	if (WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &funcID, sizeof(funcID),
		&funcAddr, sizeof(funcAddr), &bytes, NULL, NULL) == SOCKET_ERROR)
	{
		LOGNET("Fail to get extension function.");
		return NULL;
	}
	return funcAddr;
}

CNetInit::CNetInit()
{
	WSAData data;
	if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
		LOGNET("Fail to initialize net.");
}

CNetInit::~CNetInit()
{
	if (WSACleanup() == SOCKET_ERROR)
		LOGNET("Fail to end net.");
}