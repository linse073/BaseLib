#include "Util.h"
#include <memory.h>
#include <cstdio>

CKMP::CKMP(const char* pattern, int size)
:m_size(size)
{
	m_pattern = new char[m_size];
	memcpy(m_pattern, pattern, m_size);
	m_back = new int[m_size];
	m_back[0] = -1;
	for (int index=1; index<m_size; ++index)
	{
		int backIndex = m_back[index-1];
		while (m_pattern[index]!=m_pattern[backIndex+1] && backIndex>=0)
			backIndex = m_back[backIndex];
		if (m_pattern[index] == m_pattern[backIndex+1])
			m_back[index] = backIndex+1;
		else
			m_back[index] = 0;
	}
}

CKMP::~CKMP()
{
	delete[] m_pattern;
	delete[] m_back;
}

const char* CKMP::Search(const char* dest, int size)
{
	int patternIndex = 0;
	int index = 0;
	while (patternIndex<m_size && index<size)
	{
		if (m_pattern[patternIndex] == dest[index])
		{
			++patternIndex;
			++index;
		}
		else
		{
			if (patternIndex == 0)
				++index;
			else
				patternIndex = m_back[patternIndex-1]+1;
		}
	}
	if (patternIndex == m_size)
		return &dest[index-m_size];
	else
		return NULL;
}