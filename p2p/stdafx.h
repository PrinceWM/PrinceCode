// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#pragma once

#if defined(MOHO_WIN32)

#define _CRT_SECURE_NO_WARNINGS 
#define _CRT_NON_CONFORMING_SWPRINTFS
#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // �� Windows ͷ���ų�����ʹ�õ�����
// Windows ͷ�ļ�:
#include <windows.h>
#include <WinBase.h>
#include <WinSock2.h>
#include <mswsock.h>

// C ����ʱͷ�ļ�
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <iostream> 
#include <io.h> 
#include <Mmsystem.h> 
#include <Windows.h>

#include <Wininet.h> 

#include <list>
#include <deque>

#elif defined(MOHO_X86)

#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>

#endif

// TODO: �ڴ˴����ó�����Ҫ������ͷ�ļ�
