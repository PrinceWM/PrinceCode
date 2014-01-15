// stdafx.cpp : ֻ������׼�����ļ���Դ�ļ�
// LiveChannelC2A.pch ����ΪԤ����ͷ
// stdafx.obj ������Ԥ����������Ϣ

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
    //�������ز���߳�
    pthread_attr_t a; //�߳�����
    pthread_attr_init(&a); //��ʼ���߳�����
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED); //�����߳�����

    int err = pthread_create(&m_threadID, &a, threadFun,usrdata);
    if (err || m_threadID == 0)
    {
        return 0;
    }
#elif defined(MOHO_WIN32)
    DWORD threadID;
    m_threadHandle = CreateThread(
            NULL,							//��ȫ����ʹ��ȱʡ��
            0,								//�̵߳Ķ�ջ��С��
            threadFun,         //�߳����к�����ַ��
            usrdata,						//�����̺߳����Ĳ�����
            0,								//������־��
            &threadID);	//�ɹ���������̱߳�ʶ�롣
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

