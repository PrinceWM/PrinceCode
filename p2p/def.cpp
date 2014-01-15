#include "stdafx.h"
#include "def.h"
#include <stdio.h>
#include <stdarg.h>
#if defined(MOHO_X86)
#include <unistd.h>
#include <pthread.h>
#endif

INT myGetLastError()
{
#if defined(MOHO_X86)
    return errno;
#elif defined(MOHO_WIN32)
    // return WSAmyGetLastError();
    return GetLastError();
#endif
}

VOID mySleep(INT ms)
{
#if defined(MOHO_X86)
    usleep(ms*1000);
#elif defined(MOHO_WIN32)
    Sleep(ms);
#endif
}

#if defined(MOHO_X86)
VOID probeThreadExit(INT sig)
{
    pthread_exit(NULL);
}
#endif
