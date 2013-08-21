#ifndef GETTIMEOFDAY_H
#define GETTIMEOFDAY_H

#ifndef HAVE_GETTIMEOFDAY

#ifdef __cplusplus
extern "C" {
#endif

	int gettimeofday( struct timeval* tv, void* timezone );

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* HAVE_GETTIMEOFDAY */
#endif /* GETTIMEOFDAY_H */
