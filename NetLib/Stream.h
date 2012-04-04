#ifndef NETLIB_STREAM
#define NETLIB_STREAM

#include "Base.h"

class CStream
{
public:
	CStream(int bufSize, NETOP op);
	~CStream();
	NetData* GetNetData();
	bool Write(const char* buf, int size);
	void MoveHead(int offset);
	void MoveTail(int offset);
	void Tidy();
	void Clean();

	void PreRead(const char** buf, int* size) const;
	bool HasData() const;
	bool HasSpace() const;

private:
	char* m_buf;
	int m_bufSize;
	int m_head;
	int m_tail;
	NetData m_netData;
	NETOP m_operation;
};

#endif