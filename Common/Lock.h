#ifndef NETLIB_LOCK
#define NETLIB_LOCK

#include <WinSock2.h>

class CLock
{
public:
	CLock(PCRITICAL_SECTION criticalSection);
	~CLock();

private:
	PCRITICAL_SECTION m_criticalSection; 
};

#endif