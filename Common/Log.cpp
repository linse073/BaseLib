#include "Log.h"
#include "Lock.h"
#include <cstdio>
#include <ctime>
#include <Psapi.h>

CLog* CLog::GetInstance()
{
	static CLog s_log;
	return &s_log;
}

CLog::CLog()
{
	InitializeCriticalSection(&m_criticalSection);
	char logFileName[512] = "Log/";
	GetModuleBaseNameA(GetCurrentProcess(), NULL, PTAIL(logFileName));
	char* dotPos = strchr(logFileName, '.');
	if (dotPos != NULL)
		*dotPos = 0;
	time_t lt = time(NULL); 
	struct tm st;
	localtime_s(&st, &lt); 
	strftime(PTAIL(logFileName), "_%y%m%d.log", &st);
	m_logFile.open(logFileName, std::ios::app);
}

CLog::~CLog()
{
	DeleteCriticalSection(&m_criticalSection);
	if (m_logFile.is_open())
		m_logFile.close();
}

void CLog::Log(const char* file, const char* func, int line, DWORD dwError, const char* format, ...)
{
	char buf[MAX_LOG_BUFFER_SIZE] = "";
	const char* fileName = strrchr(file, '\\');
	if (fileName == NULL)
		fileName = file;
	else
		fileName += 1;
	sprintf_s(PTAIL(buf), "[%s][%s][%d]:", fileName, func, line);
	va_list list;
	va_start(list, format);
	vsnprintf_s(PTAIL(buf), _TRUNCATE, format, list);
	va_end(list);
	sprintf_s(PTAIL(buf), "\n");

	if (dwError)
	{
		LPCSTR msg = NULL;
		DWORD systemLocal = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
		DWORD msgLen = FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwError, systemLocal, (LPSTR)&msg, 0, NULL);
		if (msgLen)
		{
			sprintf_s(PTAIL(buf), "LastError:%s", msg);
			LocalFree((HLOCAL)msg);
			msg = NULL;
		}
		else
		{
			sprintf_s(PTAIL(buf), "Fail to format message, error code is 0x%x\n", GetLastError());
		}
	}

	CLock lock(&m_criticalSection);
	printf("%s", buf);
	if (m_logFile.is_open())
		m_logFile << buf << std::endl;
}