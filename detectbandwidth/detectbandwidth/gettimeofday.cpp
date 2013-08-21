


//#include "headers.h"
#include "gettimeofday.h"
//#include <WinDef.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#ifdef __cplusplus
extern "C" {
#endif

	int gettimeofday( struct timeval* tv, void* timezone ) {
		FILETIME time;
		double   timed;

		GetSystemTimeAsFileTime( &time );

		// Apparently Win32 has units of 1e-7 sec (tenths of microsecs)
		// 4294967296 is 2^32, to shift high word over
		// 11644473600 is the number of seconds between
		// the Win32 epoch 1601-Jan-01 and the Unix epoch 1970-Jan-01
		// Tests found floating point to be 10x faster than 64bit int math.

		timed = ((time.dwHighDateTime * 4294967296e-7) - 11644473600.0) +
			(time.dwLowDateTime  * 1e-7);

		tv->tv_sec  = (long) timed;
		tv->tv_usec = (long) ((timed - tv->tv_sec) * 1e6);

		return 0;
	}

#ifdef __cplusplus
} /* end extern "C" */
#endif


