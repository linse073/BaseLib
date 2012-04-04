#ifndef NETLIB_BASE
#define NETLIB_BASE

#include <WinSock2.h>
#include <functional>

#define RECV_BUFFER_SIZE	20*1024
#define SEND_BUFFER_SIZE	50*1024
#define DISCON_BUFFER_SIZE	32
#define ACCEPT_BUFFER_SIZE	128
#define CONNECT_BUFFER_SIZE 128

#define DENIAL_SERVICE_TIME 30
#define MAX_IDLE_TIME		60
#define MAX_DISCONNECT_TIME 30

#define ADD_CLIENT_NUM_PER_TIME 10
#define LISTEN_BACKLOG			5
#define WAIT_TIME_INTERVAL		1000

#define STR(op) #op

enum NETOP
{
	NETOP_Accept,
	NETOP_Receive,
	NETOP_Send,
	NETOP_Disconnect,
	NETOP_Connect
};

const char* NETOPSTR[];

enum NETSTATUS
{
	NETSTATUS_Dead,
	NETSTATUS_Connected,
	NETSTATUS_CompletionNotify,
	NETSTATUS_Disconnecting,
	NETSTATUS_Disconnected
};

const char* NETSTATUSSTR[];

struct NetData
{
	OVERLAPPED overlapped;
	WSABUF buffer;
	NETOP operation;
	void* extra;
};

void* GetExtensionFunction(SOCKET sock, GUID funcID);

class CNetInit
{
public:
	CNetInit();
	~CNetInit();
};

#endif