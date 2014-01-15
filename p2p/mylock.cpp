// stdafx.cpp : 只包括标准包含文件的源文件
// LiveChannelC2A.pch 将作为预编译头
// stdafx.obj 将包含预编译类型信息

#include "stdafx.h"
#include "mylock.h"

CMyLock::CMyLock()
{

}
CMyLock::~CMyLock()
{

}
void CMyLock::init()
{
#if defined(MOHO_X86)
	pthread_mutex_init(&m_lock, NULL);
#elif defined(MOHO_WIN32)
	InitializeCriticalSection(&m_lock);
#endif
}
void CMyLock::destroy()
{
#if defined(MOHO_X86)
	pthread_mutex_destroy(&m_lock);
#elif defined(MOHO_WIN32)

#endif
}
void CMyLock::lock()
{
#if defined(MOHO_X86)
	pthread_mutex_lock(&m_lock);
#elif defined(MOHO_WIN32)
	EnterCriticalSection(&m_lock);
#endif
}
void CMyLock::unlock()
{
#if defined(MOHO_X86)
	pthread_mutex_unlock(&m_lock);
#elif defined(MOHO_WIN32)
	LeaveCriticalSection(&m_lock);
#endif
}