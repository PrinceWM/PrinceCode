#ifndef MYPTHREAD_H
#define MYPTHREAD_H

#include "def.h"
#if defined(MOHO_X86)
#include <pthread.h>
#include <signal.h>
#endif


#if defined(MOHO_X86)
typedef LPVOID LPTHREADFUN(LPVOID para);
#elif defined(MOHO_WIN32)
typedef DWORD (WINAPI *LPTHREADFUN)(LPVOID lpThreadParameter);
#endif

class CMyThread
{
public:
    CMyThread();

    ~CMyThread();

    int isCreat();

    BOOL creat(LPTHREADFUN threadFun,LPVOID usrdata);

    void terminate();

private:
#if defined(MOHO_X86)
    pthread_t m_threadID;
#elif defined(MOHO_WIN32)
    HANDLE m_threadHandle;
#endif

    BOOL m_isCreat;

};
#endif // MYPTHREAD_H
