#ifndef MYLOCK_H
#define MYLOCK_H

#if defined(MOHO_X86)
#include <pthread.h>
#elif defined(MOHO_WIN32)
#include <windows.h>
#endif

class CMyLock
{
public:
	CMyLock();
	~CMyLock();
	void init();
	void destroy();
	void lock();
	void unlock();
private:

#if defined(MOHO_X86)
	pthread_mutex_t m_lock;
#elif defined(MOHO_WIN32)
	CRITICAL_SECTION m_lock;
#endif
};


#endif // MYLOCK_H
