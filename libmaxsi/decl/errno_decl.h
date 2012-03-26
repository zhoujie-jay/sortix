#ifndef _ERRNO_DECL
#define _ERRNO_DECL

/* Returns the address of the errno variable for this thread. */
extern int* get_errno_location(void);

/* get_errno_location will forward the request to a user-specified function if
   specified, or if NULL, will return the global thread-shared errno value. */
typedef int* (*errno_location_func_t)(void);
extern void set_errno_location_func(errno_location_func_t func);

#define errno (*get_errno_location())

#endif
