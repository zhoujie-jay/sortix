/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    debugger.cpp
    Internal kernel debugger.

*******************************************************************************/

#include <sys/types.h>

#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sortix/keycodes.h>

#include <sortix/kernel/cpu.h>
#include <sortix/kernel/debugger.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/ioport.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/keyboard.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/symbol.h>
#include <sortix/kernel/thread.h>

#include "kb/layout/us.h"

namespace Sortix {
namespace Debugger {

uint16_t* const VIDEO_MEMORY = (uint16_t*) 0xB8000;

bool ignore_next_f10;
static int column;
static int row;
static Thread* current_thread;
#define current_process (current_thread->process)

// Changes the position of the hardware cursor.
void SetCursor(int x, int y)
{
	unsigned value = x + y * 80;

	// This sends a command to indicies 14 and 15 in the
	// CRT Control Register of the VGA controller. These
	// are the high and low bytes of the index that show
	// where the hardware cursor is to be 'blinking'.
	outport8(0x3D4, 14);
	outport8(0x3D5, (value >> 8) & 0xFF);
	outport8(0x3D4, 15);
	outport8(0x3D5, (value >> 0) & 0xFF);
}

void GetCursor(int* x, int* y)
{
	outport8(0x3D4, 14);
	uint8_t high = inport8(0x3D5);
	outport8(0x3D4, 15);
	uint8_t low = inport8(0x3D5);
	unsigned value = high << 8 | low;
	*x = value % 80;
	*y = value / 80;
}

uint16_t* Character(int x, int y)
{
	return &VIDEO_MEMORY[y * 80 + x];
}

void Scroll()
{
	for ( int y = 0; y < 25-1; y++ )
		for ( int x = 0; x < 80; x++ )
			*Character(x, y) = *Character(x, y+1);
	for ( int x = 0; x < 80; x++ )
		*Character(x, 25-1) = 0x700 | ' ';
}

void Newline()
{
	if ( row + 1 == 25 )
		Scroll();
	else
		row++;
	column = 0;
}

void PrintChar(char c)
{
	if ( c == '\n' )
		Newline();
	else if ( c == '\b' )
	{
		if ( column )
			column--;
		else if ( row )
			column = 80,
			row--;
		*Character(column, row) = 0x700 | ' ';
	}
	else if ( c == '\r' )
		column = 0;
	else if ( c == '\t' )
	{
		do PrintChar(' ');
		while ( column % 8 != 0 );
	}
	else
	{
		if ( column == 80 )
			Newline();
		*Character(column++, row) = 0x700 | (uint16_t) c;
	}

	SetCursor(column, row);
}

void PrintString(const char* str)
{
	while ( *str )
		PrintChar(*str++);
}

size_t PrintCallback(void* /*user*/, const char* str, size_t len)
{
	for ( size_t i = 0; i < len; i++ )
		PrintChar(str[i]);
	return len;
}

void Print(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	vcbprintf(NULL, PrintCallback, format, ap);
	va_end(ap);
}

void PrintSymbol(const char* symbol)
{
	if ( !symbol )
	{
		Print("<unknown>");
		return;
	}
	if ( !(symbol[0] == '_' && symbol[1] == 'Z') )
	{
		Print("%s(...)", symbol);
		return;
	}
	symbol += 2;
	while ( *symbol )
	{
		if ( *symbol == 'N' )
			symbol++;
		else if ( '0' <= *symbol && *symbol <= '9' )
		{
			size_t len = strtoul(symbol, (char**) &symbol, 10);
			for ( size_t i = 0; i < len; i++ )
				PrintChar(*symbol++);
			if ( *symbol == 'L' )
				symbol++; // TODO: What is this?
			if ( '0' <= *symbol && *symbol <= '9' )
				Print("::");
		}
		else if ( *symbol == 'v' )
		{
			symbol++;
			Print("()");
			break;
		}
		else if ( *symbol == 'i' )
		{
			symbol++;
			Print("(...)");
			break;
		}
		else if ( *symbol == 'E' || *symbol == 'P' )
		{
			symbol++;
			if ( *symbol == 'v' )
				Print("()");
			else
				Print("(...)");
			break;
		}
		else
			PrintChar(*symbol++);
	}
}

static bool MatchesSymbol(const Symbol* symbol, uintptr_t address)
{
	return symbol->address <= address &&
	       address <= symbol->address + symbol->size;
}

const char* GetSymbolName(uintptr_t address)
{
	if ( const char* symbol_name = GetKernelSymbolName(address) )
		return symbol_name;
	for ( size_t i = 0; i < current_process->symbol_table_length; i++ )
	{
		const Symbol* symbol = current_process->symbol_table + i;
		if ( MatchesSymbol(symbol, address) )
			return symbol->name;
	}
	return NULL;
}

void ReadCommand(char* buffer, size_t buffer_length)
{
	KBLayoutUS kblayout;
	bool scancode_escaped = false;

	size_t written = 0;
	while ( true )
	{
		// Get a scancode from the keyboard.
		uint16_t iobase = 0x60;
		const uint16_t DATA = 0x0;
		//const uint16_t COMMAND = 0x0;
		const uint16_t STATUS = 0x4;
		while ( (inport8(iobase + STATUS) & (1<<0)) == 0 );
		uint8_t scancode = inport8(iobase + DATA);

		// Handle escaped scancodes.
		const uint8_t SCANCODE_ESCAPE = 0xE0;
		if ( (scancode_escaped = scancode == SCANCODE_ESCAPE) )
			continue;

		// Produce a <sortix/keycodes.h> format integer.
		int offset = (scancode_escaped) ? 0x80 : 0;
		int kbkey = scancode & 0x7F;
		if ( scancode & 0x80 ) { kbkey = -kbkey - offset; }
		else { kbkey = kbkey + offset; }

		if ( !written && kbkey == -KBKEY_F10 )
		{
			if ( !ignore_next_f10 )
			{
				if ( !written )
					PrintString("exit");
				PrintString("\n");
				strncpy(buffer, "exit", buffer_length);
				break;
			}
			ignore_next_f10 = false;
		}

		// Translate the keystroke into unicode.
		uint32_t unicode = kblayout.Translate(kbkey);

		if ( !unicode )
			continue;

		// Ignore depressed keys.
		if ( kbkey < 0 )
			continue;

		// Discard anything but ascii.
		if ( 128 <= unicode )
			continue;

		char c = (char) unicode;

		// Discard tabs.
		if ( c == '\t' )
			continue;

		// Handle backspace.
		if ( c == '\b' )
		{
			if ( !written )
				continue;
			PrintChar(c);
			written--;
			continue;
		}

		// Truncate the user input if it is too long.
		if ( written == buffer_length && c != '\n' )
			continue;

		PrintChar(c);

		// Finish reading the line.
		if ( c == '\n' )
		{
			buffer[written] = '\0';
			break;
		}

		buffer[written++] = c;
	}
}

Process* FindProcess(pid_t least_pid, pid_t max_pid)
{
	Process* best = NULL;
	if ( least_pid <= current_process->pid && current_process->pid <= max_pid )
		best = current_process;
	Thread* first_thread = current_thread;
	for ( Thread* iter = first_thread->scheduler_list_next;
	      iter != first_thread; iter = iter->scheduler_list_next )
		if ( least_pid <= iter->process->pid && iter->process->pid <= max_pid &&
		     (!best || iter->process->pid < best->pid) )
			best = iter->process;
	return best;
}

int ThreadId(Thread* thread)
{
	int ret = 0;
	while ( thread->prevsibling )
		ret++, thread = thread->prevsibling;
	return ret;
}

int main_bt(int /*argc*/, char* /*argv*/[])
{
#if defined(__x86_64__)
	unsigned long ip = current_thread->registers.rip;
	unsigned long bp = current_thread->registers.rbp;
#elif defined(__i386__)
	unsigned long ip = current_thread->registers.eip;
	unsigned long bp = current_thread->registers.ebp;
#endif

	bool userspace = false;
	unsigned long depth = 0;
	do
	{
		if ( 4*1024*1024 <= ip && !userspace )
			Print("     -- userspace --\n"), userspace = true;
		const char* symbol = GetSymbolName(ip);
		Print("%-4lu 0x%zx ", depth, ip);
		PrintSymbol(symbol);
		Print("\n");
		if ( !bp )
			break;
		ip = ((unsigned long*) bp)[1];
		bp = ((unsigned long*) bp)[0];
		depth++;
	} while ( ip );
	return 0;
}

int main_dump(int argc, char* argv[])
{
	unsigned long word_size = 0;
	if ( !strcmp(argv[0], "dump8") ) word_size = 1;
	if ( !strcmp(argv[0], "dump16") ) word_size = 2;
	if ( !strcmp(argv[0], "dump32") ) word_size = 4;
	if ( !strcmp(argv[0], "dump64") ) word_size = 8;
	if ( argc < 2 )
		return 0;

	unsigned long start = strtoul(argv[1], NULL, 0);
	unsigned long length = 1;
	if ( 3 <= argc )
		length = strtoul(argv[2], NULL, 0);

	uint8_t* data = (uint8_t*) start;
	for ( size_t i = 0; i < length; i++ )
	{
		size_t elemlen = (word_size ? word_size : 1) ;
		size_t outlen = elemlen * 2;
		if ( 80 - column < (int) outlen )
			Print("\n");
		if ( !column )
#if __WORDSIZE == 64
			Print("%016lX: ", start + i * elemlen);
#else
			Print("%08lX: ", start + i * elemlen);
#endif
		else if ( word_size )
			Print(" ");
		// TODO: Endianness!
		for ( size_t n = 0; n < (word_size ? word_size : 1); n++ )
			Print("%02X", data[i * elemlen + n]);
	}

	if ( column )
		Print("\n");

	return 0;
}

int main_echo(int argc, char* argv[])
{
	const char* prefix = "";
	for ( int i = 1; i < argc; i++ )
		Print("%s%s", prefix, argv[i]),
		prefix = " ";
	Print("\n");
	return 0;
}

int main_exit(int /*argc*/, char* /*argv*/[])
{
	return -1;
}

int main_pid(int argc, char* argv[])
{
	if ( 2 <= argc )
	{
		int pid = atoi(argv[1]);
		Process* process = FindProcess(pid, pid);
		if ( !process )
		{
			Print("pid: %i: No such process\n", pid);
			return 1;
		}
		current_thread = process->firstthread;
		Memory::SwitchAddressSpace(current_thread->registers.cr3);
	}
	Print("%c %i\t`%s'\n", '*', (int) current_process->pid,
	                            current_process->program_image_path);
	return 0;
}

int main_ps(int /*argc*/, char* /*argv*/[])
{
	pid_t least = 0;
	while ( Process* process = FindProcess(least, INT_MAX) )
	{
		Print("%c %i\t`%s'\n", process == current_process ? '*' : ' ',
		                       (int) process->pid, process->program_image_path);
		least = process->pid + 1;
	}
	return 0;
}

int main_rs(int /*argc*/, char* /*argv*/[])
{
#if defined(__x86_64__)
	Print("rax=0x%lX, ", current_thread->registers.rax);
	Print("rbx=0x%lX, ", current_thread->registers.rbx);
	Print("rcx=0x%lX, ", current_thread->registers.rcx);
	Print("rdx=0x%lX, ", current_thread->registers.rdx);
	Print("rdi=0x%lX, ", current_thread->registers.rdi);
	Print("rsi=0x%lX, ", current_thread->registers.rsi);
	Print("rsp=0x%lX, ", current_thread->registers.rsp);
	Print("rbp=0x%lX, ", current_thread->registers.rbp);
	Print("r8=0x%lX, ", current_thread->registers.r8);
	Print("r9=0x%lX, ", current_thread->registers.r9);
	Print("r10=0x%lX, ", current_thread->registers.r10);
	Print("r11=0x%lX, ", current_thread->registers.r11);
	Print("r12=0x%lX, ", current_thread->registers.r12);
	Print("r13=0x%lX, ", current_thread->registers.r13);
	Print("r14=0x%lX, ", current_thread->registers.r14);
	Print("r15=0x%lX, ", current_thread->registers.r15);
	Print("rip=0x%lX, ", current_thread->registers.rip);
	Print("rflags=0x%lX, ", current_thread->registers.rflags);
	Print("fsbase=0x%lX, ", current_thread->registers.fsbase);
	Print("gsbase=0x%lX, ", current_thread->registers.gsbase);
	Print("cr3=0x%lX, ", current_thread->registers.cr3);
	Print("kernel_stack=0x%lX, ", current_thread->registers.kernel_stack);
	Print("kerrno=%lu, ", current_thread->registers.kerrno);
	Print("signal_pending=%lu.", current_thread->registers.signal_pending);
#elif defined(__i386__)
	Print("eax=0x%lX, ", current_thread->registers.eax);
	Print("ebx=0x%lX, ", current_thread->registers.ebx);
	Print("ecx=0x%lX, ", current_thread->registers.ecx);
	Print("edx=0x%lX, ", current_thread->registers.edx);
	Print("edi=0x%lX, ", current_thread->registers.edi);
	Print("esi=0x%lX, ", current_thread->registers.esi);
	Print("esp=0x%lX, ", current_thread->registers.esp);
	Print("ebp=0x%lX, ", current_thread->registers.ebp);
	Print("eip=0x%lX, ", current_thread->registers.eip);
	Print("eflags=0x%lX, ", current_thread->registers.eflags);
	Print("fsbase=0x%lX, ", current_thread->registers.fsbase);
	Print("gsbase=0x%lX, ", current_thread->registers.gsbase);
	Print("cr3=0x%lX, ", current_thread->registers.cr3);
	Print("kernel_stack=0x%lX, ", current_thread->registers.kernel_stack);
	Print("kerrno=%lX, ", current_thread->registers.kerrno);
	Print("signal_pending=%lu.", current_thread->registers.signal_pending);
#endif
	Print("\n");
	return 0;
}

static void DescribeThread(int tid, Thread* thread)
{
#if defined(__x86_64__)
	unsigned long ip = thread->registers.rip;
#elif defined(__i386__)
	unsigned long ip = thread->registers.eip;
#endif

	Print("%c ", thread == current_thread ? '*' : ' ');
	Print("%i", tid);
	Print("\tip=0x%lx", ip);
	Print("\n");
}

int main_tid(int argc, char* argv[])
{
	if ( 2 <= argc )
	{
		int tid = atoi(argv[1]);
		Thread* thread = current_process->firstthread;
		for ( int i = 0; i < tid && thread; i++ )
			thread = thread->nextsibling;
		if ( !thread )
		{
			Print("tid: %i: No such thread\n", tid);
			return 1;
		}
		current_thread = thread;
		Memory::SwitchAddressSpace(current_thread->registers.cr3);
	}
	DescribeThread(ThreadId(current_thread), current_thread);
	return 0;
}

int main_ts(int /*argc*/, char* /*argv*/[])
{
	int tid = 0;
	for ( Thread* thread = current_process->firstthread; thread; thread = thread->nextsibling )
		DescribeThread(tid++, thread);
	return 0;
}

struct command_registration
{
	const char* command;
	int (*function)(int, char*[]);
	const char* help;
};


static const struct command_registration commands[] =
{
	{ "bt", main_bt,       "bt                  Stack trace" },
	{ "dump", main_dump,   "dump START [LEN]    Dump continuous memory" },
	{ "dump8", main_dump,  "dump8 START [LEN]   Dump 8-bit memory words" },
	{ "dump16", main_dump, "dump16 START [LEN]  Dump 16-bit memory words" },
	{ "dump32", main_dump, "dump32 START [LEN]  Dump 32-bit memory words" },
	{ "dump64", main_dump, "dump16 START [LEN]  Dump 64-bit memory words" },
	{ "echo", main_echo,   "echo [ARG...]       Echo string" },
	{ "exit", main_exit,   "exit                Quit debugger" },
	{ "pid", main_pid,     "pid [NEWPID]        Change current process" },
	{ "ps", main_ps,       "ps                  List processes" },
	{ "rs", main_rs,       "rs                  Print registers" },
	{ "tid", main_tid,     "tid [NEWTID]        Change current thread" },
	{ "ts", main_ts,       "ts                  List threads in current process" },
	{ NULL, NULL, NULL },
};

bool RunCommand()
{
	const size_t BUFFER_LENGTH = 256;
	static char buffer[BUFFER_LENGTH];
	Print("> ");
	ReadCommand(buffer, BUFFER_LENGTH-1);

	static char* argv[256];

	int argc = 0;
	char* input = buffer;
	while ( (argv[argc] = strsep(&input, " \t\n")) )
		if ( argv[argc][0] )
			argc++;

	if ( !argc )
		return true;

	if ( !strcmp(argv[0], "help") )
	{
		Print("You can use the following kernel debugger commands:\n");
		for ( size_t i = 0; commands[i].command; i++ )
			Print("%s\n", commands[i].help);
		Print("\n");
		return true;
	}

	for ( size_t i = 0; commands[i].command; i++ )
		if ( !strcmp(argv[0], commands[i].command) )
			return commands[i].function(argc, argv) != -1;

	Print("%s: No such kernel debugger command\n", argv[0]);

	return true;
}

void Run(bool entered_through_keyboard)
{
	static uint16_t saved_video_memory[80*25];

	current_thread = CurrentThread();

	bool was_enabled = Interrupt::SetEnabled(false);

	ignore_next_f10 = entered_through_keyboard;

	addr_t saved_addrspace = current_thread->registers.cr3;

	memcpy(saved_video_memory, VIDEO_MEMORY, sizeof(saved_video_memory));
	int saved_x, saved_y;
	GetCursor(&saved_x, &saved_y);

	column = saved_x, row = saved_y;
	Print("\nSortix kernel debugger - type `help' for help.\n");

	while ( RunCommand() );
	SetCursor(saved_x, saved_y);
	memcpy(VIDEO_MEMORY, saved_video_memory, sizeof(saved_video_memory));

	Memory::SwitchAddressSpace(saved_addrspace);

	Interrupt::SetEnabled(was_enabled);
}

} // namespace Debugger
} // namespace Sortix
