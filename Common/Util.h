#ifndef NETLIB_UTIL
#define NETLIB_UTIL

class CKMP
{
public:
	CKMP(const char* pattern, int size);
	~CKMP();
	const char* Search(const char* dest, int size);

private:
	char* m_pattern;
	int* m_back;
	int m_size;
};

#endif