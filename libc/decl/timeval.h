#ifndef _TIMEVAL_T_DECL
#define _TIMEVAL_T_DECL
struct timeval
{
	time_t tv_sec;
	suseconds_t tv_usec;
};
#endif
