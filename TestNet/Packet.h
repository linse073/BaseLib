#ifndef NETLIB_PACKET
#define NETLIB_PACKET

#include "Base.h"
#include "Util.h"

class CPacket
{
private:
	typedef std::tr1::function<void(int, const char*, int)> PacketFunc;
	typedef std::tr1::function<bool(int, const char*, int)> SendFunc;

public:
	CPacket();
	~CPacket();
	void Init(PacketFunc packetFunc, SendFunc sendFunc);
	int Process(int clientID, const char* pData, int size);
	bool Send(int clientID, const char* buf, int size);

private:
	PacketFunc m_packetFunc;
	SendFunc m_sendFunc;
	CKMP m_kmp;
};

#endif