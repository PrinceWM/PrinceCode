// stdafx.cpp : 只包括标准包含文件的源文件
// LiveChannelC2A.pch 将作为预编译头
// stdafx.obj 将包含预编译类型信息

#include "stdafx.h"
#include "mypthread.h"

CMyThread::CMyThread()
{
#if defined(MOHO_X86)
    m_threadID = 0;
#elif defined(MOHO_WIN32)
    m_threadHandle = NULL;
#endif
}

CMyThread::~CMyThread()
{

}

int CMyThread::isCreat()
{
#if defined(MOHO_X86)
    if(m_threadID == 0)
    {
        return 0;
    }
    if(0 == pthread_kill(m_threadID,0))
    {
        return 1;
    }
    else
    {
        m_threadID = 0;
        return 0;
    }
#elif defined(MOHO_WIN32)
    if (!m_threadHandle)
    {
        return 0;
    }
    DWORD threadExitCode;
    GetExitCodeThread(m_threadHandle, &threadExitCode);
    if(threadExitCode == STILL_ACTIVE)
    {
        return 1;
    }
    else
    {
        m_threadHandle = NULL;
        return 0;
    }
#endif
}

BOOL CMyThread::creat(LPTHREADFUN threadFun,LPVOID usrdata)
{
    if(isCreat() == 1)
    {
        return 0;
    }
#if defined(MOHO_X86)
    //启动下载插件线程
    pthread_attr_t a; //线程属性
    pthread_attr_init(&a); //初始化线程属性
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED); //设置线程属性

    int err = pthread_create(&m_threadID, &a, threadFun,usrdata);
    if (err || m_threadID == 0)
    {
        return 0;
    }
#elif defined(MOHO_WIN32)
    DWORD threadID;
    m_threadHandle = CreateThread(
            NULL,							//安全属性使用缺省。
            0,								//线程的堆栈大小。
            threadFun,         //线程运行函数地址。
            usrdata,						//传给线程函数的参数。
            0,								//创建标志。
            &threadID);	//成功创建后的线程标识码。
    if(m_threadHandle == NULL)
    {
        return 0;
    }
#endif
    return 1;
}

void CMyThread::terminate()
{
    if(isCreat() == 0)
    {
        return;
    }
#if defined(MOHO_X86)
    int ret = pthread_kill(m_threadID,0);
    if(ret == 0)
    {
        pthread_kill(m_threadID, SIGUSR1);
        m_threadID = 0;
    }
#elif defined(MOHO_WIN32)
    DWORD threadExitCode;
    GetExitCodeThread(m_threadHandle, &threadExitCode);
    if(threadExitCode == STILL_ACTIVE)
    {
        TerminateThread(m_threadHandle, threadExitCode);
        m_threadHandle = NULL;
    }
#endif
}

