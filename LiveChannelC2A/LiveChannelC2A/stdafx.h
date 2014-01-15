// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#if defined(MOHO_WIN32)

#define _CRT_SECURE_NO_WARNINGS 
#define _CRT_NON_CONFORMING_SWPRINTFS
#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头中排除极少使用的资料
// Windows 头文件:
#include <windows.h>
#include <WinBase.h>
#include <WinSock2.h>
#include <mswsock.h>

// C 运行时头文件
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

// TODO: 在此处引用程序需要的其他头文件
