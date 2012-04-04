#ifndef NETLIB_CLIENT
#define NETLIB_CLIENT

#include "Stream.h"
#include "IClient.h"

class CServer;
class CManager;
class CClient
	:public IClient
{
public:
	CClient(CServer* pServer, CManager* pManager);
	~CClient();
	bool Start();
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
	CServer* m_pServer;
	CManager* m_pManager;
	int m_ID;
	
	SOCKET m_socket;

	CStream m_send;
	CStream m_recv;
	CStream m_discon;
	CStream m_accept;

	CRITICAL_SECTION m_criticalSection;

	int m_lastActionTime;
	bool m_completePort;
	NETSTATUS m_status;
};

#endif