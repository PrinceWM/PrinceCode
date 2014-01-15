#ifndef DEF_H
#define DEF_H

#if defined(MOHO_X86)

#define TRUE 1
#define FALSE 0

#define mysprintf snprintf
#define SOCKET_ERROR (-1)
#define INVALID_SOCKET (-1)

typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef unsigned short USHORT;
typedef unsigned char UCHAR;
typedef int BOOL;
typedef unsigned char BYTE;
typedef short SHORT;
typedef long LONG;
typedef char CHAR;
typedef int INT;
typedef unsigned long ULONG;
typedef void *LPVOID;
typedef void VOID;

typedef signed char         INT8;
typedef signed short        INT16;
typedef signed int          INT32;
typedef signed long long    INT64;
typedef unsigned char       UINT8;
typedef unsigned short      UINT16;
typedef unsigned int        UINT32;
typedef unsigned long long  UINT64;

typedef sockaddr_in SOCKADDR_IN;
typedef int SOCKET;
typedef struct sockaddr SOCKADDR;

VOID probeThreadExit(INT sig);

#elif defined(MOHO_WIN32)

#define mysprintf sprintf_s
typedef int socklen_t;

#endif

INT myGetLastError();
VOID mySleep(INT ms);

#endif // DEF_H
