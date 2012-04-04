#ifndef NETLIB_WORKQUEUE
#define NETLIB_WORKQUEUE

#include <WinSock2.h>
#include <functional>

class CWorkQueue
{
private:
	typedef std::tr1::function<void(int, const char*, int)>  WorkFunc;
	typedef std::tr1::function<void(int)> EndFunc;

public:
	CWorkQueue(WorkFunc workFunc, EndFunc endFunc);
	~CWorkQueue();
	bool Start(int threadNum);
	void Stop();
	void Post(int clientID, const char* pData, int size);
	void End(int clientID);

private:
	static unsigned WINAPI CompletionRoutine(void* param);

	struct PackQueue
	{
		int clientID;
		char data[1];
	};

	HANDLE m_completePort;
	int m_threadNum;
	HANDLE* m_threads;

	WorkFunc m_workFunc;
	EndFunc m_endFunc;
};

#endif