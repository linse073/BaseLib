#include "Stream.h"
#include "Log.h"

CStream::CStream(int bufSize, NETOP op)
:m_bufSize(bufSize), m_head(0), m_tail(0), m_operation(op)
{
	m_buf = new char[m_bufSize];
}

CStream::~CStream()
{
	delete[] m_buf;
}

NetData* CStream::GetNetData()
{
	memset(&m_netData, 0, sizeof(m_netData));
	switch (m_operation)
	{
	case NETOP_Send:
		m_netData.buffer.buf = &m_buf[m_head];
		if (m_head <= m_tail)
			m_netData.buffer.len = m_tail-m_head;
		else
			m_netData.buffer.len = m_bufSize-m_head;
		break;
	case NETOP_Receive:
	case NETOP_Accept:
	case NETOP_Disconnect:
	case NETOP_Connect:
		m_netData.buffer.buf = &m_buf[m_tail];
		if (m_head <= m_tail)
		{
			if (m_head != 0)
				m_netData.buffer.len = m_bufSize-m_tail;
			else
				m_netData.buffer.len = m_bufSize-m_tail-1;
		}
		else
		{
			m_netData.buffer.len = m_head-m_tail-1;
		}
		break;
	default:
		LOG("Stream get an unexpected operation.");
		break;
	}
	m_netData.operation = m_operation;
	return &m_netData;
}

bool CStream::Write(const char* buf, int size)
{
	int free = (m_head<=m_tail)?(m_bufSize-m_tail+m_head-1):(m_tail-m_head-1);
	if (size > free)
	{
		LOG("Writing message exceed max buffer size.");
		return false;
	}
	int tailFree = m_bufSize-m_tail;
	if (size <= tailFree)
	{
		memcpy(&m_buf[m_tail], buf, size);
	}
	else
	{
		memcpy(&m_buf[m_tail], buf, tailFree);
		memcpy(m_buf, &buf[tailFree], size-tailFree);
	}
	m_tail = (m_tail+size)%m_bufSize;
	return true;
}

void CStream::MoveHead(int offset)
{
	if (m_head <= m_tail)
	{
		if (m_head+offset > m_tail)
		{
			LOG("Moving head exceed tail.");
			return;
		}
	}
	else
	{
		if (m_head+offset > m_bufSize)
		{
			LOG("Moving head exceed buffer size");
			return;
		}
	}
	m_head = (m_head+offset)%m_bufSize;
}

void CStream::MoveTail(int offset)
{
	if (m_head <= m_tail)
	{
		if (m_head != 0)
		{
			if (m_tail+offset > m_bufSize)
			{
				LOG("Moving tail exceed buffer size");
				return;
			}
		}
		else
		{
			if (m_tail+offset >= m_bufSize)
			{
				LOG("Moving tail exceed max size.");
				return;
			}
		}
	}
	else
	{
		if (m_tail+offset >= m_head)
		{
			LOG("Move tail exceed tail.");
			return;
		}
	}
	m_tail = (m_tail+offset)%m_bufSize;
}

void CStream::Tidy()
{
	if (m_head == 0)
		return;
	if (m_head < m_tail)
	{
		memmove(m_buf, &m_buf[m_head], m_tail-m_head);
		m_tail -= m_head;
	}
	else if (m_head > m_tail)
	{
		int tailSize = m_bufSize-m_head;
		if (m_head-m_tail >= tailSize)
		{
			memmove(&m_buf[tailSize], m_buf, m_tail);
			memmove(m_buf, &m_buf[m_head], tailSize);
		}
		else
		{
			char* tempBuf = new char[tailSize];
			memcpy(tempBuf, &m_buf[m_head], tailSize);
			memmove(&m_buf[tailSize], m_buf, m_tail);
			memcpy(m_buf, tempBuf, tailSize);
			delete[] tempBuf;
		}
		m_tail += tailSize;
	}
	else
	{
		m_tail = 0;
	}
	m_head = 0;
}

void CStream::Clean()
{
	m_tail = m_head = 0;
}

void CStream::PreRead(const char** buf, int* size) const
{
	*buf = &m_buf[m_head];
	if (m_head <= m_tail)
		*size = m_tail-m_head;
	else
		*size = m_bufSize-m_head;
}

bool CStream::HasData() const
{
	return m_tail!=m_head;
}

bool CStream::HasSpace() const
{
	return ((m_head<=m_tail)?(m_bufSize-m_tail+m_head-1):(m_tail-m_head-1))>0;
}