/*
 * Copyright (c) 2013, 2014, 2015, 2016 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * trianglix.cpp
 * The Trianglix Desktop Environment.
 */

#include <sys/display.h>
#include <sys/keycodes.h>
#include <sys/termmode.h>
#include <sys/wait.h>

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <dispd.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <ioleast.h>
#include <libgen.h>
#include <limits.h>
#include <math.h>
#include <poll.h>
#include <pwd.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <timespec.h>
#include <unistd.h>

#include "vector.h"

const int FONT_WIDTH = 8UL;
const int FONT_HEIGHT = 16UL;
const int FONT_NUMCHARS = 256UL;
const int FONT_CHARSIZE = FONT_WIDTH * FONT_HEIGHT / 8UL;

bool use_runes = false;
bool configured_use_runes = false;
uint8_t font[FONT_CHARSIZE * FONT_NUMCHARS];

__attribute__((format(printf, 1, 2)))
char* print_string(const char* format, ...)
{
	char* ret;
	va_list ap;
	va_start(ap, format);
	if ( vasprintf(&ret, format, ap) < 0 )
		ret = NULL;
	va_end(ap);
	return ret;
}

char* gibberish()
{
	size_t length = 1 + arc4random_uniform(15);
	char* result = (char*) malloc(length + 1);
	for ( size_t i = 0; i < length; i++ )
		result[i] = 'A' + arc4random_uniform(26);
	result[length] = '\0';
	return result;
}

// TODO: This might be out of sync with execvpe's exact semantics.
bool has_path_executable(const char* name)
{
	const char* path = getenv("PATH");
	struct stat st;
	if ( !path || strchr(name, '/') )
		return stat(name, &st) == 0;
	char* path_copy = strdup(path);
	if ( !path_copy )
		return false;
	char* input = path_copy;
	while ( char* component = strsep(&input, ":") )
	{
		char* fullpath = print_string("%s/%s", component, name);
		if ( !fullpath )
			return free(fullpath), free(path_copy), false;
		if ( stat(fullpath, &st) == 0 )
			return free(fullpath), free(path_copy), true;
		free(fullpath);
	}
	free(path_copy);
	return errno = ENOENT, false;
}

static float timespec_to_float(struct timespec ts)
{
	return (float) ts.tv_sec + (float) ts.tv_nsec / 1E9;
}

DIR* open_home_directory()
{
	if ( struct passwd* user = getpwuid(getuid()) )
		if ( DIR* dir = opendir(user->pw_dir) )
			return dir;
	return opendir("/");
}

static uint32_t MakeColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
	return a << 24U | r << 16U | g << 8U | b << 0U;
}

static uint32_t BlendPixel(uint32_t bg, uint32_t fg)
{
	uint8_t fg_a = fg >> 24;
	if ( fg_a == 255 )
		return fg;
	if ( fg_a == 0 )
		return bg;
	uint8_t fg_r = fg >> 16;
	uint8_t fg_g = fg >> 8;
	uint8_t fg_b = fg >> 0;
	uint8_t bg_r = bg >> 16;
	uint8_t bg_g = bg >> 8;
	uint8_t bg_b = bg >> 0;
	uint8_t ret_a = 255;
	uint8_t ret_r = ((255-fg_a)*bg_r + fg_a*fg_r) / 256;
	uint8_t ret_g = ((255-fg_a)*bg_g + fg_a*fg_g) / 256;
	uint8_t ret_b = ((255-fg_a)*bg_b + fg_a*fg_b) / 256;
	return ret_a << 24U | ret_r << 16U | ret_g << 8U | ret_b << 0U;
}

static bool ForkAndWait()
{
	unsigned int old_termmode;
	gettermmode(0, &old_termmode);
	pid_t child_pid = fork();
	if ( child_pid < 0 )
		return false;
	if ( child_pid )
	{
		int status;
		waitpid(child_pid, &status, 0);
		sigset_t oldset, sigttou;
		sigemptyset(&sigttou);
		sigaddset(&sigttou, SIGTTOU);
		sigprocmask(SIG_BLOCK, &sigttou, &oldset);
		tcsetpgrp(0, getpgid(0));
		sigprocmask(SIG_SETMASK, &oldset, NULL);
		settermmode(0, old_termmode);
		return false;
	}
	close(0);
	close(1);
	close(2);
	open("/dev/tty", O_RDONLY);
	open("/dev/tty", O_WRONLY);
	open("/dev/tty", O_WRONLY);
	setpgid(0, 0);
	sigset_t oldset, sigttou;
	sigemptyset(&sigttou);
	sigaddset(&sigttou, SIGTTOU);
	sigprocmask(SIG_BLOCK, &sigttou, &oldset);
	tcsetpgrp(0, getpgid(0));
	sigprocmask(SIG_SETMASK, &oldset, NULL);
#if 1 /* Magic to somehow fix a weird keyboard-related bug nortti has. */
	settermmode(0, TERMMODE_NORMAL | TERMMODE_NONBLOCK);
	char c;
	while ( 0 <= read(0, &c, sizeof(c)) );
#endif
	settermmode(0, TERMMODE_NORMAL);
	printf("\e[m\e[2J\e[H");
	fflush(stdout);
	fsync(0);
	fsync(1);
	fsync(2);
	return true;
}

__attribute__((unused))
static void ExecuteFile(const char* path, int curdirfd)
{
	if ( ForkAndWait() )
	{
		fchdir(curdirfd);
		execl(path, path, (char*) NULL);
		_exit(127);
	}
}

__attribute__((unused))
static void DisplayFile(const char* path, int curdirfd)
{
	if ( ForkAndWait() )
	{
		fchdir(curdirfd);
		execlp("editor", "editor", path, (char*) NULL);
		_exit(127);
	}
}

__attribute__((unused))
static void ExecutePath(const char* path, int curdirfd)
{
	if ( ForkAndWait() )
	{
		fchdir(curdirfd);
		execlp(path, path, (char*) NULL);
		_exit(127);
	}
}

void ExecuteShellCommand(const char* command, int curdirfd)
{
	if ( !strcmp(command, "exit") )
		exit(0);
	unsigned int old_termmode;
	gettermmode(0, &old_termmode);
	if ( ForkAndWait() )
	{
		fchdir(curdirfd);
		execlp("sh", "sh", "-c", command, (char*) NULL);
		error(127, errno, "`%s'", "sh");
	}
	unsigned int cur_termmode;
	gettermmode(0, &cur_termmode);
	if ( old_termmode == cur_termmode )
	{
		printf("\e[30;47m\e[J");
		printf("-- Press enter to return to Trianglix --\n");
		printf("\e[25l");
		fflush(stdout);
		settermmode(0, TERMMODE_UNICODE | TERMMODE_UTF8 | TERMMODE_LINEBUFFER);
		char line[256];
		read(0, line, sizeof(line));
		printf("\e[25h");
		fflush(stdout);
	}
}

enum object_type
{
	TYPE_DIRECTORY,
	TYPE_FILE,
};

class action;
class object;

class action
{
public:
	action(const char* name, class object* object) : name(strdup(name)), object(object) { }
	~action() { free(name); }
	const char* get_name() { return name; }
	class object* get_object() { return object; }

private:
	char* name;
	class object* object;

};

class object
{
public:
	object() : parent_object(NULL), error_string("") { }
	virtual ~object() { }
	virtual enum object_type type() = 0;
	virtual class object* factory() { return NULL; }
	virtual const char* title() { return ""; }
	virtual const char* prompt() { return ""; }
	virtual bool is_password_prompt() { return false; }
	virtual bool is_core() { return false; }
	virtual bool is_unstable_core() { return false; }
	virtual const char* error_message() { return error_string; }
	virtual void clear_error() { error_string = ""; }
	virtual class action** list_actions(size_t* /*num_actions*/) { return NULL; }
	virtual void invoke() { }
	virtual class object* open(const char* /*path*/) { return NULL; }
	virtual class object* command_line(const char* /*command*/) { return NULL; }

public:
	class object* parent_object;
	const char* error_string;

};

class directory : public object
{
public:
	directory(DIR* dir)
		: dir(dir), path(canonicalize_file_name_at(dirfd(dir), ".")) { }
	virtual ~directory() { closedir(dir); free(path); }

public:
	virtual enum object_type type() { return TYPE_DIRECTORY; }
	virtual const char* title() { return path; }
	virtual class action** list_actions(size_t* num_actions);
	virtual class object* open(const char* path);
	virtual class object* command_line(const char* command);

private:
	DIR* dir;
	char* path;

};

class file : public object
{
public:
	file(const char* path) : path(strdup(path)) { }
	virtual ~file() { free(path); }

public:
	virtual enum object_type type() { return TYPE_FILE; }
	virtual void invoke();

private:
	char* path;

};

class object* directory::open(const char* path)
{
	if ( !strcmp(path, ".") )
		return this;
	int fd = openat(dirfd(dir), path, O_RDONLY | O_CLOEXEC);
	if ( fd < 0 )
		return NULL;
	if ( !strcmp(path, "..") )
	{
		struct stat old_stat, new_stat;
		if ( fstat(dirfd(dir), &old_stat) == 0 && fstat(fd, &new_stat) == 0 )
		{
			if ( old_stat.st_dev == new_stat.st_dev &&
			     old_stat.st_ino == new_stat.st_ino )
				return this;
		}
	}
	struct stat st;
	fstat(fd, &st);
	if ( S_ISDIR(st.st_mode) )
		return new directory(fdopendir(fd));
	close(fd);
	if ( char* file_path = canonicalize_file_name_at(dirfd(dir), path) )
	{
		class object* result = new file(file_path);
		free(file_path);
		return result;
	}
	else
	{
		error(0, errno, "canonicalize_file_name_at: %s", path);
		sleep(1);
	}
	return NULL;
}

class action** directory::list_actions(size_t* num_actions)
{
	struct dirent** entries;
	int num_entries = dscandir_r(dir, &entries, NULL, NULL, alphasort_r, NULL);
	if ( num_entries < 0 )
		return NULL;
	action** actions = new class action*[num_entries + 1];
	for ( int i = 0; i < num_entries; i++ )
		actions[i] = new action(entries[i]->d_name, open(entries[i]->d_name));
	for ( int i = 0; i < num_entries; i++ )
		free(entries[i]);
	free(entries);
	actions[num_entries] = new action("Back", parent_object);
	return *num_actions = (size_t) num_entries + 1, actions;
}

class object* directory::command_line(const char* command)
{
	if ( !strncmp(command, "cd ", 3) )
	{
		class object* new_object = open(command + 3);
		if ( new_object && new_object->type() == TYPE_DIRECTORY )
			return new_object;
		delete new_object;
	}

	ExecuteShellCommand(command, dirfd(dir));

	return NULL;
}

class object* directory_command_line(const char* command)
{
	if ( !strncmp(command, "cd ", 3) )
	{
		class object* user_directory = new directory(open_home_directory());
		class object* new_object = user_directory->open(command + 3);
		if ( new_object != user_directory )
			delete user_directory;
		if ( new_object && new_object->type() == TYPE_DIRECTORY )
			return new_object;
		delete new_object;
	}

	DIR* dir = open_home_directory();
	ExecuteShellCommand(command, dirfd(dir));
	closedir(dir);

	return NULL;
}

void file::invoke()
{
	struct stat st;
	if ( stat(path, &st) == 0 )
	{
		char* dir_path = strdup(path);
		dirname(dir_path);
		DIR* dir = opendir(dir_path);
		if ( !dir )
			dir = opendir("/");
		if ( S_ISREG(st.st_mode) && (st.st_mode & 0111) )
			ExecuteFile(path, dirfd(dir));
		else if ( S_ISREG(st.st_mode) && !(st.st_mode & 0111) )
			DisplayFile(path, dirfd(dir));
		closedir(dir);
	}
}

class shell_command : public object
{
public:
	shell_command(const char* command) : command(strdup(command)) { }
	virtual ~shell_command() { free(command); }

public:
	virtual enum object_type type() { return TYPE_FILE; }
	virtual void invoke();

private:
	char* command;

};

void shell_command::invoke()
{
	DIR* dir = NULL;
	if ( struct passwd* user = getpwuid(getuid()) )
		dir = opendir(user->pw_dir);
	if ( !dir )
		dir = opendir("/");
	ExecuteShellCommand(command, dirfd(dir));
	closedir(dir);
}

class path_program : public object
{
public:
	path_program(const char* filename) : filename(strdup(filename)) { }
	virtual ~path_program() { free(filename); }

public:
	virtual enum object_type type() { return TYPE_FILE; }
	virtual void invoke();

private:
	char* filename;

};

void path_program::invoke()
{
	DIR* dir = NULL;
	if ( struct passwd* user = getpwuid(getuid()) )
		dir = opendir(user->pw_dir);
	if ( !dir )
		dir = opendir("/");
	ExecutePath(filename, dirfd(dir));
	closedir(dir);
}

class exec_program : public object
{
public:
	exec_program(const char* program) : program(strdup(program)) { }
	virtual ~exec_program() { free(program); }

public:
	virtual enum object_type type() { return TYPE_FILE; }
	virtual void invoke();

private:
	char* program;

};

void exec_program::invoke()
{
	execlp(program, program, (char*) NULL);
}

class development : public object
{
public:
	development() { }
	virtual ~development() { }

public:
	virtual enum object_type type() { return TYPE_DIRECTORY; }
	virtual const char* title() { return "Development"; }
	virtual class action** list_actions(size_t* num_actions);
	virtual class object* command_line(const char* command);

};

class action** development::list_actions(size_t* num_actions)
{
	struct stat st;
	class action** actions = new action*[4 + 1];
	size_t index = 0;
	if ( has_path_executable("editor") )
		actions[index++] = new action("Editor", new path_program("editor"));
	if ( has_path_executable("sh") )
		actions[index++] = new action("Shell", new path_program("sh"));
	if ( has_path_executable("make") &&
	     has_path_executable("colormake") &&
	     stat("/src/system", &st) == 0 )
		actions[index++] = new action("Recompile System",
		                              new shell_command("colormake -C /src/system system"));
	if ( stat("/src", &st) == 0 )
		actions[index++] = new action("Source Code", new directory(opendir("/src")));
	actions[index++] = new action("Reboot GUI", new exec_program("trianglix"));
	actions[index++] = new action("Back", parent_object);
	return *num_actions = index, actions;
}

class object* development::command_line(const char* command)
{
	return directory_command_line(command);
}

class games : public object
{
public:
	games() { }
	virtual ~games() { }

public:
	virtual enum object_type type() { return TYPE_DIRECTORY; }
	virtual const char* title() { return "Games"; }
	virtual class action** list_actions(size_t* num_actions);
	virtual class object* command_line(const char* command);

};

class action** games::list_actions(size_t* num_actions)
{
	class action** actions = new action*[3 + 1];
	size_t index = 0;
	if ( has_path_executable("asteroids") )
		actions[index++] = new action("Asteroids", new path_program("asteroids"));
	if ( has_path_executable("aquatinspitz") )
		actions[index++] = new action("Aquatinspitz", new path_program("aquatinspitz"));
	if ( has_path_executable("quake") )
		actions[index++] = new action("Quake", new path_program("quake"));
	actions[index++] = new action("Back", parent_object);
	return *num_actions = index, actions;
}

class object* games::command_line(const char* command)
{
	return directory_command_line(command);
}

class core : public object
{
public:
	core(size_t depth = 0) : error(NULL), title_string(gibberish()), depth(depth) { }
	virtual ~core() { free(error); free(title_string); }
	virtual enum object_type type() { return TYPE_DIRECTORY; }
	virtual bool is_core() { return true; }
	virtual bool is_unstable_core() { return 2 <= depth; }
	virtual const char* title() { return title_string; }
	virtual const char* prompt() { return "|ROOT>"; }
	virtual class action** list_actions(size_t* num_actions);
	virtual class object* command_line(const char* command);

private:
	char* error;
	char* title_string;
	size_t depth;

};

class action** core::list_actions(size_t* num_actions_ptr)
{
	if ( 2 <= depth && arc4random_uniform(10000) < 200 )
		write(1, NULL, 1);

	size_t num_actions = 1 + arc4random_uniform(31);
	class action** actions = new action*[num_actions];
	for ( size_t i = 0; i < num_actions; i++ )
	{
		char* name = gibberish();
		actions[i] = new action(name, new core(depth + 1));
		free(name);
	}
	return *num_actions_ptr = num_actions, actions;
}

class object* core::command_line(const char* /*command*/)
{
	if ( arc4random_uniform(2) == 0 )
		return new core();
	else
	{
		free(error);
		error_string = error = gibberish();
		return NULL;
	}
}

class decide_runes : public object
{
public:
	decide_runes(bool answer) : answer(answer) { }
	virtual ~decide_runes() { }
	virtual enum object_type type() { return TYPE_FILE; }
	virtual void invoke();

private:
	bool answer;

};

void decide_runes::invoke()
{
	use_runes = answer;
	configured_use_runes = true;
	if ( answer )
	{
		unlink("/etc/rune-disable");
		close(::open("/etc/rune-enable", O_WRONLY | O_CREAT, 0644));
	}
	else
	{
		unlink("/etc/rune-enable");
		close(::open("/etc/rune-disable", O_WRONLY | O_CREAT, 0644));
	}
}

struct create_user_prompt
{
	const char* prompt;
	bool is_password_prompt;
};

struct create_user_prompt create_user_prompts[] =
{
	{ "Enter username:", false },
	{ "Enter password:", true },
};

const size_t num_create_user_prompts = sizeof(create_user_prompts) / sizeof(create_user_prompts[0]);

class create_user : public object
{
public:
	create_user();
	virtual ~create_user();
	virtual enum object_type type() { return TYPE_DIRECTORY; }
	virtual const char* title() { return "Create user"; }
	virtual const char* prompt();
	virtual bool is_password_prompt();
	virtual class object* command_line(const char* command);

private:
	size_t prompt_index;
	char* answers[num_create_user_prompts];

};

create_user::create_user()
{
	prompt_index = 0;
}

create_user::~create_user()
{
	for ( size_t i = 0; i < prompt_index; i++ )
		free(answers[i]);
}

const char* create_user::prompt()
{
	return create_user_prompts[prompt_index].prompt;
}

bool create_user::is_password_prompt()
{
	return create_user_prompts[prompt_index].is_password_prompt;
}

class object* create_user::command_line(const char* command)
{
	if ( prompt_index == 0 && !command[0] )
		return error_string = "Username cannot be empty", (class object*) NULL;
	if ( prompt_index == 0 && strchr(command, '/') )
		return error_string = "Username cannot contain slash characters", (class object*) NULL;

	answers[prompt_index] = strdup(command);

	if ( prompt_index + 1 < num_create_user_prompts )
	{
		prompt_index++;
		return NULL;
	}

	uid_t highest_uid = 0;
	FILE* fp = openpw();
	while ( struct passwd* user = fgetpwent(fp) )
		if ( highest_uid < user->pw_uid )
			highest_uid = user->pw_uid;
	fclose(fp);

	struct passwd user;
	memset(&user, 0, sizeof(user));
	user.pw_uid = highest_uid + 1;
	user.pw_gid = (gid_t) user.pw_uid;
	user.pw_dir = print_string("/home/%s", answers[0]);
	user.pw_gecos = strdup(answers[0]);
	user.pw_name = strdup(answers[0]);
	user.pw_passwd = strdup(answers[1]);
	user.pw_shell = strdup("sh");

	mkdir(user.pw_dir, 777);

	fp = fopen("/etc/passwd", "a");
	fprintf(fp, "%s:%s:%ju:%ju:%s:%s:%s\n",
		user.pw_name,
		user.pw_passwd,
		(uintmax_t) user.pw_uid,
		(uintmax_t) user.pw_gid,
		user.pw_gecos,
		user.pw_dir,
		user.pw_shell);
	fclose(fp);

	return parent_object;
}

class administration : public object
{
public:
	administration() { }
	virtual ~administration() { }

public:
	virtual enum object_type type() { return TYPE_DIRECTORY; }
	virtual const char* title() { return "Administration"; }
	virtual class action** list_actions(size_t* num_actions);

};

class action** administration::list_actions(size_t* num_actions)
{
	class action** actions = new action*[4 + 1];
	size_t index = 0;
#if 0 // TODO: Until crypt_newhash is used for the password.
	actions[index++] = new action("Create user", new create_user());
#endif
	actions[index++] = new action("Enable Runes", new decide_runes(true));
	actions[index++] = new action("Disable Runes", new decide_runes(false));
	actions[index++] = new action("Trinit Core", new core());
	actions[index++] = new action("Back", parent_object);
	return *num_actions = index, actions;
}

class exiter : public object
{
public:
	exiter() { }
	virtual ~exiter() { }

public:
	virtual enum object_type type() { return TYPE_FILE; }
	virtual void invoke();

};

void exiter::invoke()
{
	exit(0);
}

class desktop : public object
{
public:
	desktop() { }
	virtual ~desktop() { }

public:
	virtual enum object_type type() { return TYPE_DIRECTORY; }
	virtual const char* title() { return "Desktop"; }
	virtual class action** list_actions(size_t* num_actions);
	virtual class object* command_line(const char* command);

};

class action** desktop::list_actions(size_t* num_actions)
{
	class action** actions = new action*[*num_actions = 7];
	actions[0] = new action("Home", new directory(open_home_directory()));
	actions[1] = new action("Filesystem", new directory(opendir("/")));
	actions[2] = new action("Games", new games());
	actions[3] = new action("Shell", new path_program("sh"));
	actions[4] = new action("Development", new development());
	actions[5] = new action("Administration", new administration());
	actions[6] = new action("Logout", new exiter());
	return actions;
}

class object* desktop::command_line(const char* command)
{
	return directory_command_line(command);
}

class object* log_user_in(struct passwd* user)
{
	setuid(user->pw_uid);
	setenv("USERNAME", user->pw_name, 1);
	setenv("HOME", user->pw_dir, 1);
	setenv("SHELL", user->pw_shell, 1);
	setenv("DEFAULT_STUFF", "NO", 1);
	return new desktop();
}

class FrameBufferInfo;
struct Desktop;
struct RenderInfo;

struct Desktop
{
	struct timespec init_time;
	struct timespec since_init;
	struct timespec last_update;
	struct timespec experienced_time;
	class object* object;
	action** actions;
	size_t num_actions;
	size_t selected;
	float rotate_angle;
	float color_angle;
	float text_angle;
	float size_factor;
	bool control;
	bool shift;
	bool lshift;
	bool rshift;
	bool show_help;
	bool angry;
	char buffer_overflow_fix_me;
	char command[2048];
	char warning_buffer[64];
	const char* warning;
	bool critical_warning;
	bool flashing_warning;
	bool rune_warning;
};

struct RenderInfo
{
	int frame;
	struct Desktop desktop;
};

class FrameBufferInfo
{
public:
	FrameBufferInfo(uint8_t* buffer, size_t pitch, int xres, int yres) :
		buffer(buffer), pitch(pitch), xres(xres), yres(yres) { }
	FrameBufferInfo(const FrameBufferInfo& o) :
		buffer(o.buffer), pitch(o.pitch), xres(o.xres), yres(o.yres) { }
	FrameBufferInfo(struct dispd_framebuffer* fb)
	{
		buffer = dispd_get_framebuffer_data(fb);
		pitch = dispd_get_framebuffer_pitch(fb);
		xres = dispd_get_framebuffer_width(fb);
		yres = dispd_get_framebuffer_height(fb);
	}
	~FrameBufferInfo() { }

public:
	uint8_t* buffer;
	size_t pitch;
	int xres;
	int yres;

public:
	uint32_t* GetLinePixels(int ypos) const
	{
		return (uint32_t*) (buffer + ypos * pitch);
	}

};

unsigned char char_to_rune(unsigned char c)
{
	switch ( tolower(c) )
	{
	case 'a': return 0x89;
	case 'b': return 0x8c;
	case 'c': return 0x85;
	case 'd': return 0x8b;
	case 'e': return 0x88;
	case 'f': return 0x80;
	case 'g': return 0x85;
	case 'h': return 0x86;
	case 'i': return 0x88;
	case 'j': return 0x88;
	case 'k': return 0x85;
	case 'l': return 0x8e;
	case 'm': return 0x8d;
	case 'n': return 0x87;
	case 'o': return 0x83;
	case 'p': return 0x8c;
	case 'q': return 0x85;
	case 'r': return 0x84;
	case 's': return 0x8a;
	case 't': return 0x8b;
	case 'u': return 0x81;
	case 'v': return 0x81;
	case 'w': return 0x81;
	case 'x': return 0x85;
	case 'y': return 0x81;
	case 'z': return 0x8a;
	default: return c;
	}
}

static void RenderChar(FrameBufferInfo fb, char c, uint32_t color,
                       int pos_x, int pos_y, int max_w, int max_h)
{
	unsigned char uc = c;
	if ( use_runes )
		uc = char_to_rune(uc);
	int xres = fb.xres;
	int yres = fb.yres;
	if ( xres <= pos_x || yres <= pos_y )
		return;
	int avai_x = xres - pos_x;
	int avai_y = yres - pos_y;
	if ( avai_x < max_w )
		max_w = avai_x;
	if ( avai_y < max_h )
		max_h = avai_y;
	int width = FONT_WIDTH + 1 < max_w ? FONT_WIDTH + 1 : max_w;
	int height = FONT_HEIGHT < max_h ? FONT_HEIGHT : max_h;
	for ( int y = 0; y < height; y++ )
	{
		int pixely = pos_y + y;
		if ( pixely < 0 )
			continue;
		uint32_t* pixels = fb.GetLinePixels(pixely);
		uint8_t linebitmap = font[uc * FONT_CHARSIZE + y];
		for ( int x = 0; x < width; x++ )
		{
			int pixelx = pos_x + x;
			if ( pixelx < 0 )
				continue;
			if ( x != FONT_WIDTH && linebitmap & 1U << (7-x) )
				pixels[pixelx] = BlendPixel(pixels[pixelx], color);
		}
	}
}

static void RenderCharShadow(FrameBufferInfo fb, char c, uint32_t color,
                             int pos_x, int pos_y, int max_w, int max_h)
{
	RenderChar(fb, c, MakeColor(0, 0, 0, 64), pos_x-1, pos_y-1, max_w, max_h);
	RenderChar(fb, c, MakeColor(0, 0, 0, 64), pos_x+1, pos_y-1, max_w, max_h);
	RenderChar(fb, c, MakeColor(0, 0, 0, 64), pos_x-1, pos_y+1, max_w, max_h);
	RenderChar(fb, c, MakeColor(0, 0, 0, 64), pos_x+1, pos_y+1, max_w, max_h);
	RenderChar(fb, c, color, pos_x+0, pos_y+0, max_w, max_h);
}

static void RenderText(FrameBufferInfo fb, const char* str, uint32_t color,
                       int pos_x, int pos_y, int max_w, int max_h)
{
	int char_width = FONT_WIDTH+1;
	int char_height = FONT_HEIGHT;
	while ( *str && 0 < max_w )
	{
		RenderChar(fb, *str++, color, pos_x, pos_y, max_w, max_h);
		pos_x += char_width;
		max_w -= char_width;
		(void) char_height;
	}
}

static void RenderTextShadow(FrameBufferInfo fb, const char* str, uint32_t color,
                             int pos_x, int pos_y, int max_w, int max_h)
{
	int char_width = FONT_WIDTH+1;
	int char_height = FONT_HEIGHT;
	while ( *str && 0 < max_w )
	{
		RenderCharShadow(fb, *str++, color, pos_x, pos_y, max_w, max_h);
		pos_x += char_width;
		max_w -= char_width;
		(void) char_height;
	}
}

typedef struct
{
	float src;
	float dst;
} fspan_t;

size_t ClampFloatToSize(float val, size_t max)
{
	if ( val < 0.0f )
		return 0;
	if ( (float) max <= val )
		return max;
	size_t ret = (size_t) val;
	if ( max < ret )
		ret = max;
	return ret;
}

int compare_triangle_points_by_height(const void* a, const void* b)
{
	Vector av = *(const Vector*) a;
	Vector bv = *(const Vector*) b;
	if ( av.y < bv.y )
		return -1;
	else if ( bv.y < av.y )
		return 1;
	return 0;
}

static void SortTrianglePointsByHeight(Vector points[3])
{
	qsort(points, 3, sizeof(Vector), compare_triangle_points_by_height);
}

static float SafeDivide(float a, float b)
{
	if ( b == 0.0f )
		return a < 0.0f ? -INFINITY : INFINITY;
	return a / b;
}

static size_t TriangleHorizontalSpans(fspan_t* spans, size_t numspans, float from,
                                      Vector points[3])
{
	assert(points[0].y <= points[1].y && points[1].y <= points[2].y);
	Vector vec01 = points[1] - points[0];
	Vector vec02 = points[2] - points[0];
	Vector vec20 = points[0] - points[2];
	Vector vec21 = points[1] - points[2];
	Vector vec01hat(-vec01.y, vec01.x);
	bool flipped = vec02.Dot(vec01hat) < 0.0f;
	float dirx_01 = SafeDivide(vec01.x, vec01.y);
	float dirx_02 = SafeDivide(vec02.x, vec02.y);
	float dirx_20 = -SafeDivide(vec20.x, vec20.y);
	float dirx_21 = -SafeDivide(vec21.x, vec21.y);
	for ( size_t i = 0; i < numspans; i++ )
	{
		float y = from + (float) i;
		if ( y < points[0].y ) { spans[i].src = spans[i].dst = 0.0f; continue; }
		if ( points[2].y < y ) { return i; /* Nothing beyond here. */ }
		float xpos01 = points[0].x + dirx_01 * (y - points[0].y);
		float xpos02 = points[0].x + dirx_02 * (y - points[0].y);
		float xpos20 = points[2].x + dirx_20 * (points[2].y - y);
		float xpos21 = points[2].x + dirx_21 * (points[2].y - y);
		float xsrc;
		float xdst;
		if ( flipped )
			xsrc = xpos01 > xpos21 ? xpos01 : xpos21,
			xdst = xpos20;
		else
			xsrc = xpos02,
			xdst = xpos01 < xpos21 ? xpos01 : xpos21;
		spans[i].src = xsrc;
		spans[i].dst = xdst;
	}
	return numspans;
}

static void RenderTriangle(uint8_t* fb, size_t pitch, size_t xres,
                            size_t yres, float x1, float y1, float x2,
                            float y2, float x3, float y3, uint32_t color)
{
	Vector points[3] = { {x1, y1}, {x2, y2}, {x3, y3}, };
	SortTrianglePointsByHeight(points);
	const size_t NUM_SPANS = 256UL;
	fspan_t spans[NUM_SPANS];
	float y0 = points[0].y;
	while ( true )
	{
		size_t numspans = TriangleHorizontalSpans(spans, NUM_SPANS, y0, points);
		if ( !numspans )
			break;
		for ( size_t i = 0; i < numspans; i++ )
		{
			float ypixelf = y0 + i * 1.0f;
			if ( ypixelf < 0.f || (float) yres <= ypixelf )
				continue;
			size_t ypixel = ypixelf;
			if ( yres <= ypixel )
				continue;
			size_t xsrc = ClampFloatToSize(spans[i].src, xres);
			size_t xdst = ClampFloatToSize(spans[i].dst, xres);
			uint32_t* line = (uint32_t*) (fb + ypixel * pitch);
			for ( size_t xpixel = xsrc; xpixel < xdst; xpixel++ )
				line[xpixel] = color;
		}
		y0 += numspans * 1.0f;
	}
}

const int SENARY_FONT_WIDTH = FONT_HEIGHT-1;
const int SENARY_FONT_HEIGHT = FONT_HEIGHT;

static void RenderSenaryDigit(FrameBufferInfo fb, unsigned int digit,
                              uint32_t pri_color, uint32_t sec_color, int top,
                              int left)
{
	int width = SENARY_FONT_WIDTH-2;
	int height = SENARY_FONT_HEIGHT-2;
	int bottom = top + height;
	int right = left + width;
	int x1, y1, x2, y2, x3, y3;
	if ( digit % 3 == 0 )
	{
		x3 = (left + right) / 2;
		y3 = top;
		x2 = left;
		y2 = bottom-1;
		x1 = right;
		y1 = bottom;
	}
	else if ( digit % 3 == 1 )
	{
		x1 = left+1;
		y1 = top;
		x2 = right-1;
		y2 = (top + bottom) / 2;
		x3 = left+1;
		y3 = bottom;
	}
	else if ( digit % 3 == 2 )
	{
		x1 = right-1;
		y1 = top;
		x2 = left+1;
		y2 = (top + bottom) / 2;
		x3 = right-1;
		y3 = bottom;
	}
	uint32_t color = digit < 3 ? pri_color : sec_color;
	RenderTriangle(fb.buffer, fb.pitch, fb.xres, fb.yres, x1, y1, x2, y2, x3, y3, color);
}

static void RenderBackground(FrameBufferInfo fb, struct Desktop* desktop)
{
	bool was_runes = use_runes;
	use_runes = use_runes || desktop->object->is_core();
	if ( desktop->object->is_unstable_core() )
		use_runes = arc4random_uniform(50) < 40;

	uint32_t background_color = desktop->object->is_core() ? MakeColor(0, 0, 0) :
	                            MakeColor(0x89 * 2/3, 0xc7 * 2/3, 0xff * 2/3);
	if ( desktop->since_init.tv_sec < 3 )
	{
		float factor = timespec_to_float(desktop->since_init) / 3;
		background_color = MakeColor(0x89 * 2/3 * factor, 0xc7 * 2/3 * factor, 0xff * 2/3 * factor);
	}

	for ( int y = 0; y < fb.yres; y++ )
	{
		uint32_t* pixels = fb.GetLinePixels(y);
		for ( int x = 0; x < fb.xres; x++ )
			pixels[x] = background_color;
	}

	if ( desktop->since_init.tv_sec < 3 )
	{

		const char* logo = "trianglix";
		size_t logo_length = strlen(logo);
		int logo_width = (int) logo_length * (FONT_WIDTH+1);
		int logo_height = FONT_HEIGHT;
		int logo_pos_x = (fb.xres - logo_width) / 2;
		int logo_pos_y = (fb.yres - logo_height) / 2;
		uint32_t logo_color = MakeColor(255, 255, 255);
		RenderTextShadow(fb, logo, logo_color, logo_pos_x, logo_pos_y, INT_MAX, INT_MAX);
		use_runes = was_runes;
		return;
	}

	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);

	RenderSenaryDigit(fb, now.tv_sec/(6*6*6*6*6) % 6, MakeColor(255, 255, 255), MakeColor(255, 100, 100), 15, fb.xres-SENARY_FONT_WIDTH*(6+1));
	RenderSenaryDigit(fb, now.tv_sec/(6*6*6*6) % 6, MakeColor(255, 255, 255), MakeColor(255, 100, 100), 15, fb.xres-SENARY_FONT_WIDTH*(5+1));
	RenderSenaryDigit(fb, now.tv_sec/(6*6*6) % 6, MakeColor(255, 255, 255), MakeColor(255, 100, 100), 15, fb.xres-SENARY_FONT_WIDTH*(4+1));
	RenderSenaryDigit(fb, now.tv_sec/(6*6) % 6, MakeColor(255, 255, 255), MakeColor(255, 100, 100), 15, fb.xres-SENARY_FONT_WIDTH*(3+1));
	RenderSenaryDigit(fb, now.tv_sec/6 % 6, MakeColor(255, 255, 255), MakeColor(255, 100, 100), 15, fb.xres-SENARY_FONT_WIDTH*(2+1));
	RenderSenaryDigit(fb, now.tv_sec/1 % 6, MakeColor(255, 255, 255), MakeColor(255, 100, 100), 15, fb.xres-SENARY_FONT_WIDTH*(1+1));

	struct timespec xtime = desktop->experienced_time;

	size_t index = (now.tv_sec / 10) % 5;
	const char* slogans[] =
	{
		"trianglix",
		"you can do anything with trianglix",
		"welcome to trianglix",
		"the only limit is yourself",
		"the unattainable is unknown with trianglix",
	};

	const char* warnings[] =
	{
		"the triangle has been angered",
		"the triangle does not forgive",
		"the triangle never forgets",
		"the triangle pays no attention to the empty mind",
		"the triangle knows your limits",
		"the triangle hates being anthropomorphized",
	};

	bool is_angry = desktop->angry || desktop->object->is_unstable_core();
	const char* slogan = (is_angry ? warnings : slogans)[index];
	size_t slogan_length = strlen(slogan);
	int slogan_width = (int) slogan_length * (FONT_WIDTH+1);
	int slogan_pos_x = (fb.xres - slogan_width) / 2;
	int slogan_pos_y = 15;
	uint32_t slogan_color = desktop->angry ?
	                        MakeColor(255, 0, 0) :
	                        MakeColor(255, 255, 255);
	RenderText(fb, slogan, slogan_color, slogan_pos_x, slogan_pos_y, INT_MAX, INT_MAX);

	bool show_warning = !desktop->flashing_warning || xtime.tv_nsec < 500000000;
	if ( desktop->warning && show_warning && !desktop->show_help )
	{
		const char* warning = desktop->warning;
		size_t warning_length = strlen(warning);
		int warning_width = (int) warning_length * (FONT_WIDTH+1);
		int warning_pos_x = (fb.xres - warning_width) / 2;
		int warning_pos_y = 15 + FONT_HEIGHT * 2;
		uint32_t warning_color = desktop->critical_warning ?
			                    MakeColor(255, 0, 0) :
			                    MakeColor(255, 255, 255);
		bool used_runes = use_runes;
		use_runes = use_runes || desktop->rune_warning;
		RenderText(fb, warning, warning_color, warning_pos_x, warning_pos_y, INT_MAX, INT_MAX);
		use_runes = used_runes;
	}

	const char* title = desktop->object->title();
	if ( desktop->show_help )
		title = "Help";
	size_t title_length = strlen(title);
	int title_width = (int) title_length * (FONT_WIDTH+1);
	int title_pos_x = (fb.xres - title_width) / 2;
	int title_pos_y = fb.yres - 15 - FONT_HEIGHT;
	uint32_t title_color = MakeColor(255, 255, 255);
	RenderText(fb, title, title_color, title_pos_x, title_pos_y, INT_MAX, INT_MAX);

	int alpha = (uint8_t) ((sin(desktop->color_angle) + 1.0f) * 128.f);
	if ( alpha < 0 )
		alpha = 0;
	if ( 255 < alpha )
		alpha = 255;

	uint32_t pri_color = desktop->object->is_core() && !desktop->object->is_unstable_core() ?
	                     MakeColor(255, 255, 255, alpha) :
	                     is_angry ?
	                     MakeColor(114, 21, 36, alpha) :
	                     MakeColor(255, 222, 0, alpha);
	uint32_t sec_color = desktop->object->is_core() && !desktop->object->is_unstable_core() ?
	                     MakeColor(255, 255, 255, 255) :
	                     is_angry ?
	                     MakeColor(255, 0, 0, 255) :
	                     MakeColor(114, 255, 36, 255);
	uint32_t triangle_color = BlendPixel(sec_color, pri_color);

	float size = fb.xres / 10.0 * desktop->size_factor;

	float center_x = fb.xres / 2.0;
	float center_y = fb.yres / 2.0;

	float x1 = center_x + size * cos(desktop->rotate_angle + 2.0*M_PI * 0.0 / 3.0);
	float y1 = center_y + size * sin(desktop->rotate_angle + 2.0*M_PI * 0.0 / 3.0);
	float x2 = center_x + size * cos(desktop->rotate_angle + 2.0*M_PI * 1.0 / 3.0);
	float y2 = center_y + size * sin(desktop->rotate_angle + 2.0*M_PI * 1.0 / 3.0);
	float x3 = center_x + size * cos(desktop->rotate_angle + 2.0*M_PI * 2.0 / 3.0);
	float y3 = center_y + size * sin(desktop->rotate_angle + 2.0*M_PI * 2.0 / 3.0);

	RenderTriangle(fb.buffer, fb.pitch, fb.xres, fb.yres, x1, y1, x2, y2, x3, y3, triangle_color);

	if ( desktop->show_help )
	{
		const char* help_messages[] =
		{
			"Trianglix Basic Usage",
			"",
			"F1 - Basic Usage",
			"Escape - Return to parent object",
			"Arrow Keys - Select action in direction",
			"Enter (empty prompt) - Invoke action",
			"Enter (non-empty prompt) - Execute command",
			"Tab - Next object",
			"Shift-Tab - Previous object",
			"Control-D - Exit shell",
			"Control-Q - Exit editor",
			"<character> - Append character to command prompt",
			NULL,
		};

		for ( size_t i = 0; help_messages[i]; i++ )
		{
			const char* help = help_messages[i];
			int help_length = strlen(help);
			int help_width = (int) help_length * (FONT_WIDTH+1);
			int help_height = FONT_HEIGHT;
			int help_pos_x = (fb.xres - help_width) / 2;
			int help_pos_y = 15 + (3 + i) * (help_height);
			RenderTextShadow(fb, help, MakeColor(255, 255, 255), help_pos_x, help_pos_y, INT_MAX, INT_MAX);
		}

		use_runes = was_runes;
		return;
	}

	float normal_dist = size * 2.0;

	uint32_t unselected_color = MakeColor(255, 255, 255);
	uint32_t selected_color = MakeColor(255, 222, 0);
	for ( size_t i = 0; i < desktop->num_actions; i++ )
	{
		float dist = normal_dist;
		if ( desktop->object->is_unstable_core() )
			dist *= 0.5 + (sin(desktop->text_angle / 50.0 * i) + 1.0) / 2.0;
		const char* file_name = desktop->actions[i]->get_name();
		char* text = new char[1+strlen(file_name)+1+1];
		stpcpy(stpcpy(stpcpy(text, "|"), file_name), ">");
		float text_x = center_x + dist * cos(desktop->text_angle + 2.0*M_PI * (float) i / (float) desktop->num_actions);
		float text_y = center_y + dist * sin(desktop->text_angle + 2.0*M_PI * (float) i / (float) desktop->num_actions);
		int text_width = (int) strlen(text) * (FONT_WIDTH+1);
		int text_height = FONT_HEIGHT;
		float x = text_x - text_width / 2.0f;
		float y = text_y - text_height / 2.0f;
		RenderTextShadow(fb, text, i == desktop->selected ? selected_color : unselected_color, x, y, text_width, text_height);
		delete[] text;
	}

	bool is_password_prompt = desktop->object->is_password_prompt();
	char* password_command = NULL;
	const char* command = desktop->command;
	if ( is_password_prompt )
	{
		password_command = strdup(command);
		for ( size_t i = 0; command[i]; i++ )
			password_command[i] = '*';
		command = password_command;
	}
	int command_length = strlen(command);
	int command_width = (int) command_length * (FONT_WIDTH+1);
	int command_height = FONT_HEIGHT;
	int command_pos_x = (fb.xres - command_width) / 2;
	int command_pos_y = (fb.yres - command_height) / 2;
	RenderTextShadow(fb, command, MakeColor(255, 255, 255), command_pos_x, command_pos_y, INT_MAX, INT_MAX);
	if ( is_password_prompt )
		free(password_command);

	const char* prompt = desktop->object->prompt();
	int prompt_length = strlen(prompt);
	int prompt_width = (int) prompt_length * (FONT_WIDTH+1);
	int prompt_height = FONT_HEIGHT;
	int prompt_pos_x = (fb.xres - prompt_width) / 2;
	int prompt_pos_y = (fb.yres - prompt_height) / 2 - FONT_HEIGHT - 2;
	RenderTextShadow(fb, prompt, MakeColor(255, 255, 255), prompt_pos_x, prompt_pos_y, INT_MAX, INT_MAX);

	const char* error = desktop->object->error_message();
	int error_length = strlen(error);
	int error_width = (int) error_length * (FONT_WIDTH+1);
	int error_height = FONT_HEIGHT;
	int error_pos_x = (fb.xres - error_width) / 2;
	int error_pos_y = (fb.yres - error_height) / 2 + FONT_HEIGHT - 2;
	RenderText(fb, error, MakeColor(255, 0, 0), error_pos_x, error_pos_y, INT_MAX, INT_MAX);

	use_runes = was_runes;
}

static void Render(FrameBufferInfo fb, struct Desktop* desktop)
{
	RenderBackground(fb, desktop);
}

static bool Render(struct dispd_window* window, struct RenderInfo* info)
{
	struct dispd_framebuffer* fb = dispd_begin_render(window);
	if ( !fb )
		return false;
	FrameBufferInfo fbinfo(fb);
	Render(fbinfo, &info->desktop);
	dispd_finish_render(fb);
	return true;
}

void ClearActions(struct Desktop* desktop, class object* no_delete = NULL)
{
	for ( size_t i = 0; i < desktop->num_actions; i++ )
	{
		class object* object = desktop->actions[i]->get_object();
		if ( object != no_delete &&
		     object == desktop->object &&
		     object == desktop->object->parent_object )
			delete object;
		delete desktop->actions[i];
	}
	delete[] desktop->actions;
	desktop->actions = NULL;
	desktop->num_actions = 0;
	desktop->selected = 0;
}

static void UpdateActionList(struct Desktop* desktop)
{
	size_t old_selected = desktop->selected;
	ClearActions(desktop);
	desktop->actions = desktop->object->list_actions(&desktop->num_actions);
	if ( desktop->num_actions )
		desktop->selected = old_selected % desktop->num_actions;
	else
		desktop->selected = 0;
}

void PopObject(struct Desktop* desktop)
{
	if ( desktop->object->parent_object )
	{
		ClearActions(desktop);
		class object* poped_object = desktop->object;
		desktop->object = desktop->object->parent_object;
		delete poped_object;
	}
	UpdateActionList(desktop);
}

void PushObject(struct Desktop* desktop, class object* object)
{
	desktop->object->clear_error();

	while ( class object* product = object->factory() )
	{
		if ( object != desktop->object &&
		     object != desktop->object->parent_object &&
		     object != product )
			delete object;
		object = product;
	}

	if ( object == desktop->object )
		UpdateActionList(desktop);
	else if ( object == desktop->object->parent_object )
		PopObject(desktop);
	else
	{
		ClearActions(desktop, object);
		object->parent_object = desktop->object;
		desktop->object = object;
		UpdateActionList(desktop);
	}
}

static void HandleKeystroke(int kbkey, struct Desktop* desktop)
{
	int abskbkey = kbkey < 0 ? -kbkey : kbkey;

	switch ( abskbkey )
	{
	case KBKEY_LCTRL: desktop->control = kbkey > 0; break;
	case KBKEY_LSHIFT: desktop->lshift = kbkey > 0; break;
	case KBKEY_RSHIFT: desktop->rshift = kbkey > 0; break;
	}
	desktop->shift = desktop->lshift || desktop->rshift;

	if ( desktop->show_help )
	{
		if ( kbkey == KBKEY_F1 || kbkey == KBKEY_ESC )
			desktop->show_help = false;
		return;
	}

	if ( kbkey == KBKEY_F1 )
	{
		desktop->show_help = true;
		return;
	}

	if ( kbkey == KBKEY_ESC )
		PopObject(desktop);

	size_t& num_actions = desktop->num_actions;
	if ( !num_actions )
		return;

	float selection_x = cos(desktop->text_angle + 2.0*M_PI * (float) desktop->selected / (float) num_actions);
	float selection_y = sin(desktop->text_angle + 2.0*M_PI * (float) desktop->selected / (float) num_actions);

	if ( kbkey == KBKEY_RIGHT )
	{
		float best_diff = 2.0f;
		size_t best = num_actions;
		for ( size_t i = 0; i < num_actions; i++ )
		{
			float x = cos(desktop->text_angle + 2.0*M_PI * (float) i / (float) num_actions);
			float y = sin(desktop->text_angle + 2.0*M_PI * (float) i / (float) num_actions);
			if ( x <= selection_x )
				continue;
			float diff = fabsf(y - selection_y);
			if ( best_diff < diff )
				continue;
			best_diff = diff;
			best = i;
		}
		if ( best != num_actions )
			desktop->selected = best;
	}

	if ( kbkey == KBKEY_LEFT )
	{
		float best_diff = 2.0f;
		size_t best = num_actions;
		for ( size_t i = 0; i < num_actions; i++ )
		{
			float x = cos(desktop->text_angle + 2.0*M_PI * (float) i / (float) num_actions);
			float y = sin(desktop->text_angle + 2.0*M_PI * (float) i / (float) num_actions);
			if ( selection_x <= x )
				continue;
			float diff = fabsf(y - selection_y);
			if ( best_diff < diff )
				continue;
			best_diff = diff;
			best = i;
		}
		if ( best != num_actions )
			desktop->selected = best;
	}

	if ( kbkey == KBKEY_DOWN )
	{
		float best_diff = 2.0f;
		size_t best = num_actions;
		for ( size_t i = 0; i < num_actions; i++ )
		{
			float x = cos(desktop->text_angle + 2.0*M_PI * (float) i / (float) num_actions);
			float y = sin(desktop->text_angle + 2.0*M_PI * (float) i / (float) num_actions);
			if ( y <= selection_y )
				continue;
			float diff = fabsf(x - selection_x);
			if ( best_diff < diff )
				continue;
			best_diff = diff;
			best = i;
		}
		if ( best != num_actions )
			desktop->selected = best;
	}

	if ( kbkey == KBKEY_UP )
	{
		float best_diff = 2.0f;
		size_t best = num_actions;
		for ( size_t i = 0; i < num_actions; i++ )
		{
			float x = cos(desktop->text_angle + 2.0*M_PI * (float) i / (float) num_actions);
			float y = sin(desktop->text_angle + 2.0*M_PI * (float) i / (float) num_actions);
			if ( selection_y <= y )
				continue;
			float diff = fabsf(x - selection_x);
			if ( best_diff < diff )
				continue;
			best_diff = diff;
			best = i;
		}
		if ( best != num_actions )
			desktop->selected = best;
	}

	if ( num_actions && !desktop->shift && kbkey == KBKEY_TAB )
		desktop->selected = (desktop->selected + num_actions - 1) % num_actions;

	if ( num_actions && desktop->shift && kbkey == KBKEY_TAB )
		desktop->selected = (desktop->selected + num_actions + 1) % num_actions;

	if ( kbkey == KBKEY_ENTER && !desktop->command[0] )
	{
		class action* action = desktop->actions[desktop->selected];
		class object* object = action->get_object();
		if ( object->type() == TYPE_DIRECTORY )
			PushObject(desktop, object);
		else if ( object->type() == TYPE_FILE )
		{
			object->clear_error();
			object->invoke();
			UpdateActionList(desktop);
		}
	}
}

void HandleCodepoint(uint32_t codepoint, struct Desktop* desktop)
{
	if ( 128 <= codepoint )
		return;
	char c = (char) codepoint;
	if ( c == '\t' )
		return;
	if ( desktop->show_help )
	{
		if ( c == '\n' )
			desktop->show_help = false;
		return;
	}
	size_t column = 0;
	while ( desktop->command[column] )
		column++;
	if ( c == '\b' || c == 127 )
	{
		if ( column )
			desktop->command[column-1] = '\0';
	}
	else if ( c == '\n' )
	{
		if ( !desktop->command[0] && desktop->num_actions != 0 )
			return;
		if ( class object* new_object = desktop->object->command_line(desktop->command) )
			PushObject(desktop, new_object);
		memset(&desktop->command, 0, sizeof(desktop->command));
		return;
	}
	else
		desktop->command[column] = c;
}

static void InitializeDesktop(struct Desktop* desktop)
{
	memset(desktop, 0, sizeof(*desktop));
	clock_gettime(CLOCK_MONOTONIC, &desktop->init_time);
	desktop->last_update = desktop->init_time;
	desktop->angry = false;
	desktop->rotate_angle = M_PI_2;
	desktop->selected = 0;
	desktop->color_angle = 0.f;
	desktop->text_angle = 0.f;
	desktop->size_factor = 1.f;
	desktop->control = false;
	desktop->lshift = false;
	desktop->rshift = false;
	desktop->actions = NULL;
	desktop->num_actions = 0;
	desktop->object = new class desktop();
	UpdateActionList(desktop);
}

static void HandleKeyboardEvents(int kbfd, struct Desktop* desktop)
{
	if ( desktop->since_init.tv_sec < 3 )
		return;

	const size_t BUFFER_LENGTH = 64;
	uint32_t input[BUFFER_LENGTH];
	while ( true )
	{
		settermmode(kbfd, TERMMODE_KBKEY | TERMMODE_UNICODE | TERMMODE_NONBLOCK);
		ssize_t num_bytes = read(kbfd, input, sizeof(input));
		if ( num_bytes < 0 )
			return;
		size_t num_kbkeys = num_bytes / sizeof(uint32_t);
		for ( size_t i = 0; i < num_kbkeys; i++ )
		{
			int kbkey = KBKEY_DECODE(input[i]);
			if ( kbkey )
				HandleKeystroke(kbkey, desktop);
			else
				HandleCodepoint(input[i], desktop);
		}
	}
}

static void HandleEvents(int kbfd, struct Desktop* desktop)
{
	const nfds_t NFDS = 1;
	struct pollfd fds[NFDS];
	fds[0].fd = kbfd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;
	if ( 0 < poll(fds, NFDS, 0) )
	{
		if ( fds[0].revents )
			HandleKeyboardEvents(kbfd, desktop);
	}

	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	struct timespec delta_ts = timespec_sub(now, desktop->last_update);
	desktop->last_update = now;
	float delta = delta_ts.tv_sec * 1E0f + delta_ts.tv_nsec * 1E-9f;

	time_t xtime_sec = desktop->experienced_time.tv_sec;
	if ( !desktop->show_help && delta_ts.tv_sec == 0 )
		desktop->experienced_time = timespec_add(desktop->experienced_time, delta_ts);

	desktop->warning = NULL;
	desktop->critical_warning = false;
	desktop->flashing_warning = false;
	desktop->rune_warning = false;
	desktop->angry = false;
	if ( desktop->object->is_unstable_core()  )
	{
		char* stuff = gibberish();
		snprintf(desktop->warning_buffer, sizeof(desktop->warning_buffer),
			     "!! TRINIT CORE UNSTABLE: %s !!", stuff);
		desktop->warning = desktop->warning_buffer;
		desktop->critical_warning = true;
		desktop->flashing_warning = true;
		free(stuff);
	}
	else if ( desktop->object->is_core()  )
	{
		desktop->warning = "TRINIT CORE";
		desktop->critical_warning = true;
		desktop->flashing_warning = true;
	}
	else if ( !configured_use_runes )
	{
		if ( 0 <= xtime_sec && xtime_sec < 15 )
			desktop->warning = "Press F1 for basic usage help";
		if ( 20 <= xtime_sec && xtime_sec < 30 )
			desktop->warning = "Runes has not been configured";
		if ( 45 <= xtime_sec && xtime_sec < 60 )
			desktop->warning = "Note: Runes has not been configured";
		if ( 80 <= xtime_sec && xtime_sec < 100 )
			desktop->warning = "Warning: Runes has not been configured",
			desktop->critical_warning = true;
		if ( 110 <= xtime_sec && xtime_sec < 130 )
		{
			snprintf(desktop->warning_buffer, sizeof(desktop->warning_buffer),
				     "WARNING: UNDEPLOYED RUNES WILL BE DEPLOYED IN: %u",
				     130 - (unsigned int) xtime_sec);
			desktop->warning = desktop->warning_buffer;
			desktop->critical_warning = true;
			desktop->flashing_warning = true;
			desktop->rune_warning = true;
			desktop->angry = true;
		}
		if ( 130 <= xtime_sec )
		{
			use_runes = true;
			configured_use_runes = true;
			unlink("/etc/rune-disable");
			close(::open("/etc/rune-enable", O_WRONLY | O_CREAT, 0666));
		}
	}

	desktop->since_init = timespec_sub(now, desktop->init_time);
	bool init_boosted = timespec_lt(desktop->since_init, timespec_make(5, 0));
	float since_init = desktop->since_init.tv_sec * 1E0f + desktop->since_init.tv_nsec * 1E-9f;

	float rotate_speed = 2*M_PI / (desktop->angry ? 2 : 60);
	float color_speed = 2*M_PI / (desktop->angry ? 1 : 20);
	float text_speed = 2*M_PI / (desktop->angry ? 10 : 20);

	if ( desktop->object->is_core() )
	{
		rotate_speed *= 10;
		text_speed *= 5;
	}

	if ( desktop->object->is_unstable_core() )
	{
		rotate_speed *= 1.5;
		text_speed *= -1.0;
		color_speed *= 20;
	}

	if ( init_boosted )
	{
		rotate_speed *= 1.f + 100.0 * cosf(M_PI_2 * (since_init - 3.0) / 2.0);
		desktop->size_factor = sinf(M_PI_2 * (since_init - 3.0) / 2.0);
	}
	else
		desktop->size_factor = 1.0f;

	desktop->rotate_angle += rotate_speed * delta;
	desktop->color_angle += color_speed * delta;
	desktop->text_angle += text_speed * delta;
}

static int MainLoop(int argc, char* argv[], int kbfd, struct dispd_window* window)
{
	(void) argc;
	(void) argv;
	struct RenderInfo info;
	InitializeDesktop(&info.desktop);
	for ( info.frame = 0; true; info.frame++ )
	{
		HandleEvents(kbfd, &info.desktop);
		if ( !Render(window, &info) )
			return 1;
	}
	return 0;
}

static int CreateKeyboardConnection()
{
	int fd = open("/dev/tty", O_RDONLY | O_CLOEXEC);
	if ( fd < 0 )
		return -1;
	if ( settermmode(fd, TERMMODE_KBKEY | TERMMODE_UNICODE | TERMMODE_NONBLOCK) )
		return close(fd), -1;
	return fd;
}

int main(int argc, char* argv[])
{
	struct stat st;
	if ( stat("/etc/rune-enable", &st) == 0 )
		use_runes = true, configured_use_runes = true;
	else if ( stat("/etc/rune-disable", &st) == 0 )
		use_runes = false, configured_use_runes = true;

	int fontfd = open("/dev/vgafont", O_RDONLY);
	readall(fontfd, font, sizeof(font));
	close(fontfd);
	uint8_t rune_font[26][16] =
	{
		{ 0x00, 0x4a, 0x4a, 0x52, 0x64, 0x48, 0x70, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x00 },
		{ 0x00, 0x40, 0x40, 0x60, 0x60, 0x50, 0x50, 0x50, 0x48, 0x48, 0x48, 0x44, 0x44, 0x42, 0x42, 0x00 },
		{ 0x00, 0x40, 0x40, 0x40, 0x60, 0x50, 0x48, 0x44, 0x44, 0x48, 0x50, 0x60, 0x40, 0x40, 0x40, 0x00 },
		{ 0x00, 0x40, 0x60, 0x50, 0x48, 0x44, 0x60, 0x50, 0x48, 0x44, 0x40, 0x40, 0x40, 0x40, 0x40, 0x00 },
		{ 0x00, 0x60, 0x50, 0x48, 0x44, 0x48, 0x50, 0x60, 0x60, 0x50, 0x48, 0x44, 0x40, 0x40, 0x40, 0x00 },
		{ 0x00, 0x42, 0x44, 0x48, 0x50, 0x60, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x00 },
		{ 0x00, 0x10, 0x10, 0x10, 0x92, 0x54, 0x38, 0x10, 0x38, 0x54, 0x92, 0x10, 0x10, 0x10, 0x10, 0x00 },
		{ 0x00, 0x10, 0x10, 0x10, 0x90, 0x50, 0x30, 0x10, 0x18, 0x14, 0x12, 0x10, 0x10, 0x10, 0x10, 0x00 },
		{ 0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00 },
		{ 0x00, 0x10, 0x10, 0x10, 0x10, 0x12, 0x14, 0x18, 0x10, 0x30, 0x50, 0x90, 0x10, 0x10, 0x10, 0x00 },
		{ 0x00, 0x40, 0x40, 0x40, 0x40, 0x40, 0x44, 0x4c, 0x54, 0x64, 0x44, 0x04, 0x04, 0x04, 0x04, 0x00 },
		{ 0x00, 0x10, 0x38, 0x54, 0x92, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00 },
		{ 0x00, 0x60, 0x50, 0x48, 0x44, 0x48, 0x50, 0x60, 0x60, 0x50, 0x48, 0x44, 0x48, 0x50, 0x60, 0x00 },
		{ 0x00, 0x92, 0x92, 0x54, 0x38, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00 },
		{ 0x00, 0x40, 0x60, 0x50, 0x48, 0x44, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x00 },
		{ 0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x38, 0x54, 0x92, 0x92, 0x00 },
	};

	memcpy(font + 128 * 16, rune_font, sizeof(rune_font));

	if ( !dispd_initialize(&argc, &argv) )
		error(1, 0, "couldn't initialize dispd library");
	struct dispd_session* session = dispd_attach_default_session();
	if ( !session )
		error(1, 0, "couldn't attach to dispd default session");
	if ( !dispd_session_setup_game_rgba(session) )
		error(1, 0, "couldn't setup dispd rgba session");
	struct dispd_window* window = dispd_create_window_game_rgba(session);
	if ( !window )
		error(1, 0, "couldn't create dispd rgba window");

	int kbfd = CreateKeyboardConnection();
	if ( kbfd < 0 )
		error(1, 0, "couldn't create keyboard connection");

	int ret = MainLoop(argc, argv, kbfd, window);

	close(kbfd);

	dispd_destroy_window(window);
	dispd_detach_session(session);

	return ret;
}
