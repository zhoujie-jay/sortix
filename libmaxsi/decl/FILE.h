#ifndef _FILE_DECL
#define _FILE_DECL
#define BUFSIZ 8192UL
#define _FILE_REGISTERED (1<<0)
#define _FILE_NO_BUFFER (1<<1)
#define _FILE_LAST_WRITE (1<<2)
#define _FILE_LAST_READ (1<<3)
#define _FILE_AUTO_LOCK (1<<4)
#define _FILE_MAX_PUSHBACK 8
typedef struct _FILE
{
	/* This is non-standard, but useful. If you allocate your own FILE and
	   register it with fregister, feel free to use modify the following members
	   to customize how it works. Don't call the functions directly, though, as
	   the standard library does various kinds of buffering and conversion. */
	size_t buffersize;
	char* buffer;
	void* user;
	size_t (*read_func)(void* ptr, size_t size, size_t nmemb, void* user);
	size_t (*write_func)(const void* ptr, size_t size, size_t nmemb, void* user);
	int (*seek_func)(void* user, off_t offset, int whence);
	off_t (*tell_func)(void* user);
	void (*seterr_func)(void* user);
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
	size_t numpushedback;
	unsigned char pushedback[_FILE_MAX_PUSHBACK];
} FILE;
#endif
