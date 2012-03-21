#ifndef _DIR_DECL
#define _DIR_DECL
struct dirent;

#define _DIR_REGISTERED (1<<0)
#define _DIR_ERROR (1<<1)
#define _DIR_EOF (1<<2)
typedef struct _DIR
{
	void* user;
	int (*read_func)(void* user, struct dirent* dirent, size_t* size);
	int (*rewind_func)(void* user);
	int (*fd_func)(void* user);
	int (*close_func)(void* user);
	void (*free_func)(struct _DIR* dir);
	/* Application writers shouldn't use anything beyond this point. */
	struct _DIR* prev;
	struct _DIR* next;
	struct dirent* entry;
	size_t entrysize;
	int flags;
} DIR;
#endif
