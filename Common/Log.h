#ifndef NETLIB_LOG
#define NETLIB_LOG

#include <WinSock2.h>
#include <fstream>

#define MAX_LOG_BUFFER_SIZE 1024

#define LOGNET(msg, ...) CLog::GetInstance()->Log(__FILE__, __FUNCTION__, __LINE__, WSAGetLastError(), msg, __VA_ARGS__); WSASetLastError(0)
#define LOG(msg, ...) CLog::GetInstance()->Log(__FILE__, __FUNCTION__, __LINE__, GetLastError(), msg, __VA_ARGS__); SetLastError(0)

class CLog
{
public:
	static CLog* GetInstance();
	~CLog();
	void Log(const char* file, const char* func, int line, DWORD dwError, const char* format, ...);

private:
	CLog();
	
	std::ofstream m_logFile;
};

#endif