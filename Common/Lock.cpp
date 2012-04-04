#include "Lock.h"

CLock::CLock(PCRITICAL_SECTION criticalSection)
:m_criticalSection(criticalSection)
{
	EnterCriticalSection(m_criticalSection);
}

CLock::~CLock()
{
	LeaveCriticalSection(m_criticalSection);
}