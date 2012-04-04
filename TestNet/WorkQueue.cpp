#include "WorkQueue.h"
#include "Log.h"
#include <process.h>

CWorkQueue::CWorkQueue(WorkFunc workFunc, EndFunc endFunc)
:m_threadNum(0), m_completePort(NULL), m_threads(NULL),
m_workFunc(workFunc), m_endFunc(endFunc)
{

}

CWorkQueue::~CWorkQueue()
{
	Stop();
}

bool CWorkQueue::Start(int threadNum)
{
	do 
	{
		m_completePort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (m_completePort == NULL)
		{
			LOG("Work queue fail to create completion port.");
			break;
		}

		m_threadNum = threadNum;
		m_threads = new HANDLE[m_threadNum];
		for (int i=0; i<m_threadNum; ++i)
			m_threads[i] = (HANDLE)_beginthreadex(NULL, 0, CompletionRoutine, this, 0, NULL);

		return true;
	} while (false);

	Stop();
	return false;
}

void CWorkQueue::Stop()
{
	if (m_completePort)
	{
		for (int i=0; i<m_threadNum; ++i)
			PostQueuedCompletionStatus(m_completePort, 0, 0, NULL);
		if (WaitForMultipleObjects(m_threadNum, m_threads, TRUE, INFINITE) != WAIT_OBJECT_0)
			LOG("Work queue fail to end completion routine.");
		for (int i=0; i<m_threadNum; ++i)
			CloseHandle(m_threads[i]);
		m_threads = NULL;
		m_threadNum = 0;
		CloseHandle(m_completePort);
		m_completePort = NULL;
	}
}

void CWorkQueue::Post(int clientID, const char* pData, int size)
{
	PackQueue* workData = (PackQueue*)new char[sizeof(int)+size];
	workData->clientID = clientID;
	memcpy(workData->data, pData, size);
	if (PostQueuedCompletionStatus(m_completePort, size, (ULONG_PTR)workData, NULL) == FALSE)
		LOG("Work queue fail to call PostQueuedCompletionStatus.");
}

void CWorkQueue::End(int clientID)
{
	if (PostQueuedCompletionStatus(m_completePort, 0, (ULONG_PTR)clientID, NULL) == FALSE)
		LOG("Work queue fail to call PostQueuedCompletionStatus.");
}

unsigned WINAPI CWorkQueue::CompletionRoutine(void* param)
{
	CWorkQueue* pQueue = (CWorkQueue*)param;
	while (true)
	{
		DWORD transferBytes = 0; 
		ULONG completeKey = 0;
		LPOVERLAPPED lpOverlapped = NULL;
		DWORD result = GetQueuedCompletionStatus(pQueue->m_completePort, &transferBytes, &completeKey, &lpOverlapped, INFINITE);
		//NOTIFY: If there is only one work thread, it would be better to call GetQueuedCompletionStatusEx.
		if (result == 0)
		{
			LOG("Completion routine fail to get completion status.");
			if (transferBytes)
				delete[] (char*)completeKey;
			continue;
		}
		if (transferBytes)
		{
			PackQueue* workData = (PackQueue*)completeKey;
			pQueue->m_workFunc(workData->clientID, workData->data, (int)transferBytes);
			delete[] (char*)completeKey;
		}
		else if (completeKey)
		{
			pQueue->m_endFunc((int)completeKey);
		}
		else
		{
			break;
		}
	}
	return 0;
}