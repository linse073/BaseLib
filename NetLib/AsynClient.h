#ifndef NETLIB_ASYNCLIENT
#define NETLIB_ASYNCLIENT

#include "Stream.h"
#include "IClient.h"

class CManager;
class CAsynClient
	:public IClient
{
public:
	CAsynClient(CManager* pManager);
	virtual ~CAsynClient();

	bool Start(const char* ip, int port);
	void Stop();

	virtual bool PostConnect(NetData* pData);

	virtual bool PreSend(const char* buf, int size);
	virtual bool PostSend(int size);
	virtual bool AppendSend();

	virtual void PostRead(int size);
	virtual void PostReceive(int size);
	virtual bool AppendReceive();

	virtual bool AppendDisconnect(bool restart);
	virtual void PostDisconnect();
	virtual void Restart(NETOP curOp);
	virtual void TimeRoutine(int now);

	virtual int GetID() const;
	virtual void PreRead(const char** buf, int* size) const;

private:
	bool Connect();

	SOCKET m_socket;
	CStream m_receive;
	CStream m_send;
	CStream m_connect;
	CStream m_discon;

	CRITICAL_SECTION m_criticalSection;
	NETSTATUS m_status;

	std::string m_serverIP;
	int m_serverPort;
	bool m_restart;

	CManager* m_pManager;
	int m_ID;
};

#endif