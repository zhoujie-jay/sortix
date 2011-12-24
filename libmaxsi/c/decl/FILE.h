#ifndef _FILE_DECL
#define _FILE_DECL
#define BUFSIZ 8192UL
#define _FILE_REGISTERED (1<<0)
#define _FILE_NO_BUFFER (1<<1)
typedef struct _FILE
{
	/* This is non-standard, but useful. If you allocate your own FILE and
	   register it with fregister, feel free to use modify the following members
	   to customize how it works. Do not call or use these data structures,
	   though, as the standard library library may do various kinds of buffering
	   and locale/encoding conversion. */
	size_t buffersize;
	char* buffer;
	void* user;
	size_t (*read_func)(void* ptr, size_t size, size_t nmemb, void* user);
	size_t (*write_func)(const void* ptr, size_t size, size_t nmemb, void* user);
	int (*seek_func)(void* user, long offset, int whence);
	long (*tell_func)(void* user);
	void (*clearerr_func)(void* user);
	int (*eof_func)(void* user);
	int (*error_func)(void* user);
	int (*fileno_func)(void* user);
	int (*close_func)(void* user);
	void (*free_func)(struct _FILE* fp);
	/* Application writers shouldn't use anything beyond this point. */
	struct _FILE* prev;
	struct _FILE* next;
	int flags;
	size_t bufferused;
} FILE;
#endif
