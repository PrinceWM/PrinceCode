#ifndef GETTIMEOFDAY_H
#define GETTIMEOFDAY_H
//#include "common.h"
//#ifdef WIN32_BANDTEST

#ifdef __cplusplus
extern "C" {
#endif

	int gettimeofday( struct timeval* tv, void* timezone );

#ifdef __cplusplus
} /* end extern "C" */
#endif

//#endif /* WIN32_BANDTEST */
#endif /* GETTIMEOFDAY_H */
