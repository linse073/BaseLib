#include "Packet.h"
#include "BaseType.h"
#include "Log.h"

CPacket::CPacket()
:m_kmp(PACK_COMPART, PACK_COMPART_SIZE)
{

}

CPacket::~CPacket()
{

}

void CPacket::Init(PacketFunc packetFunc, SendFunc sendFunc)
{
	m_packetFunc = packetFunc;
	m_sendFunc = sendFunc;
}

int CPacket::Process(int clientID, const char* pData, int size)
{
	int pos = 0;
	while (true)
	{
		int leave = size-pos;
		if (leave < int(PACK_COMPART_SIZE+PACK_LENGTH_SIZE))
			break;
		const char* compart = m_kmp.Search(&pData[pos], leave);
		if (compart == NULL)
		{
			LOG("Can't find packet compart literal, then discard it.");
			return size;
		}
		else
		{
			if (compart != &pData[pos])
				LOG("The head of data don't contain packet compart literal.");
			Packet* packet = (Packet*)&compart[PACK_COMPART_SIZE];
			if (leave < int(PACK_COMPART_SIZE+PACK_LENGTH_SIZE+packet->length))
				break;
			m_packetFunc(clientID, packet->data, packet->length);
			pos += PACK_COMPART_SIZE+PACK_LENGTH_SIZE+packet->length;
		}
	}
	return pos;
}

bool CPacket::Send(int clientID, const char* buf, int size)
{
	int totalSize = size+PACK_COMPART_SIZE+PACK_LENGTH_SIZE;
	char* pData = new char[totalSize];
	memcpy(pData, PACK_COMPART, PACK_COMPART_SIZE);
	Packet* pPacket = (Packet*)&pData[PACK_COMPART_SIZE];
	pPacket->length = size;
	memcpy(pPacket->data, buf, size);
	bool result = m_sendFunc(clientID, pData, totalSize);
	delete[] pData;
	return result;
}