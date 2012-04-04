#ifndef NETLIB_ICLIENT
#define NETLIB_ICLIENT

#include "Base.h"

class IClient
{
public:
	IClient(){};
	virtual ~IClient(){};

	virtual bool PostConnect(NetData* pData) = 0;

	virtual bool PreSend(const char* buf, int size) = 0;
	virtual bool PostSend(int size) = 0;
	virtual bool AppendSend() = 0;

	virtual void PostRead(int size) = 0;
	virtual void PostReceive(int size) = 0;
	virtual bool AppendReceive() = 0;

	virtual bool AppendDisconnect(bool restart) = 0;
	virtual void PostDisconnect() = 0;
	virtual void Restart(NETOP curOp) = 0;
	virtual void TimeRoutine(int now) = 0;

	virtual int GetID() const = 0;
	virtual void PreRead(const char** buf, int* size) const = 0;
};

#endif