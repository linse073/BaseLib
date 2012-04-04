#ifndef NETLIB_BASETYPE
#define NETLIB_BASETYPE

#define PACK_COMPART "$-$"
#define PACK_COMPART_SIZE strlen(PACK_COMPART)
#define PACK_LENGTH_TYPE int
#define PACK_LENGTH_SIZE sizeof(int)

struct Packet
{
	PACK_LENGTH_TYPE length;
	char data[1];
};

#endif