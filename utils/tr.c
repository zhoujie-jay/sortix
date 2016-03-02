/*
 * Copyright (c) 2014, 2015 Jonas 'Sortie' Termansen.
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
 * tr.c
 * Translate, squeeze and/or delete characters.
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <limits.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum construct_type
{
	CONSTRUCT_TYPE_NONE,
	CONSTRUCT_TYPE_CHARACTER,
	CONSTRUCT_TYPE_CHARACTER_CLASS,
	CONSTRUCT_TYPE_CHARACTER_RANGE,
	CONSTRUCT_TYPE_CHARACTER_REPEAT,
	CONSTRUCT_TYPE_EQUIVALENCE_CLASS,
};

struct construct_character
{
	unsigned char c;
};

struct construct_character_class
{
	int (*ctype)(int);
};

struct construct_character_range
{
	unsigned char from;
	unsigned char to;
};

struct construct_character_repeat
{
	unsigned char c;
	size_t repetitions;
};

struct construct_equivalence_class
{
	unsigned char c;
};

struct construct
{
	enum construct_type type;
	union
	{
		struct construct_character character;
		struct construct_character_class character_class;
		struct construct_character_range character_range;
		struct construct_character_repeat character_repeat;
		struct construct_equivalence_class equivalence_class;
	};
};

const char* parse_construct_character(const char* string,
                                      struct construct* construct)
{
	if ( !string || string[0] == '\0' )
	{
		construct->type = CONSTRUCT_TYPE_NONE;
		return NULL;
	}

	if ( string[0] == '\\' )
	{
		if ( string[0] == '\0' )
			error(1, 0, "unescaped backslash at end of string");

		if ( '0' <= string[1] && string[1] <= '3' )
		{
			unsigned char value = 0;
			size_t i = 1;
			value = value * 8 + string[i++] - '0';
			if ( '0' <= string[i] && string[i] <= '7' )
				value = value * 8 + string[i++] - '0';
			if ( '0' <= string[i] && string[i] <= '7' )
				value = value * 8 + string[i++] - '0';
			construct->type = CONSTRUCT_TYPE_CHARACTER;
			construct->character.c = value;
			return string + i;
		}
		else if ( '0' <= string[1] && string[1] <= '7' )
		{
			unsigned char value = 0;
			size_t i = 1;
			value = value * 8 + string[i++] - '0';
			if ( '0' <= string[i] && string[i] <= '7' )
				value = value * 8 + string[i++] - '0';
			construct->type = CONSTRUCT_TYPE_CHARACTER;
			construct->character.c = value;
			return string + i;
		}

		unsigned char value;
		switch ( string[1] )
		{
		case '\\': value = '\\'; break;
		case  'a': value = '\a'; break;
		case  'b': value = '\b'; break;
		case  'e': value = '\e'; break;
		case  'f': value = '\f'; break;
		case  'n': value = '\n'; break;
		case  'r': value = '\r'; break;
		case  't': value = '\t'; break;
		case  'v': value = '\v'; break;
		default: value = string[1]; break;
		};

		construct->type = CONSTRUCT_TYPE_CHARACTER;
		construct->character.c = value;
		return string + 2;
	}

	construct->type = CONSTRUCT_TYPE_CHARACTER;
	construct->character.c = (unsigned char) string[0];
	return string + 1;
}

const char* parse_construct(const char* string, struct construct* construct)
{
	if ( !string || string[0] == '\0' )
	{
		construct->type = CONSTRUCT_TYPE_NONE;
		return NULL;
	}

	if ( string[0] == '[' && string[1] == ':' )
	{
		size_t start = 2;
		size_t end = start;
		while ( !(string[end] == '\0' ||
		          (string[end + 0] == ':' && string[end + 1] == ']')) )
			end++;
		if ( string[end] == ':' && string[end + 1] == ']' )
		{
			construct->type = CONSTRUCT_TYPE_CHARACTER_CLASS;
			if ( !strncmp(string, "[:alnum:]", strlen("[:alnum:]")) )
				construct->character_class.ctype = isalnum;
			else if ( !strncmp(string, "[:alpha:]", strlen("[:alpha:]")) )
				construct->character_class.ctype = isalpha;
			else if ( !strncmp(string, "[:blank:]", strlen("[:blank:]")) )
				construct->character_class.ctype = isblank;
			else if ( !strncmp(string, "[:cntrl:]", strlen("[:cntrl:]")) )
				construct->character_class.ctype = iscntrl;
			else if ( !strncmp(string, "[:digit:]", strlen("[:digit:]")) )
				construct->character_class.ctype = isdigit;
			else if ( !strncmp(string, "[:graph:]", strlen("[:graph:]")) )
				construct->character_class.ctype = isgraph;
			else if ( !strncmp(string, "[:lower:]", strlen("[:lower:]")) )
				construct->character_class.ctype = islower;
			else if ( !strncmp(string, "[:print:]", strlen("[:print:]")) )
				construct->character_class.ctype = isprint;
			else if ( !strncmp(string, "[:punct:]", strlen("[:punct:]")) )
				construct->character_class.ctype = ispunct;
			else if ( !strncmp(string, "[:space:]", strlen("[:space:]")) )
				construct->character_class.ctype = isspace;
			else if ( !strncmp(string, "[:upper:]", strlen("[:upper:]")) )
				construct->character_class.ctype = isupper;
			else if ( !strncmp(string, "[:xdigit:]", strlen("[:xdigit:]")) )
				construct->character_class.ctype = isxdigit;
			else
			{
				char* class_name = strndup(string + start, end - start);
				error(1, 0, "invalid character class `%s'", class_name);
				__builtin_unreachable();
			}
			return string + end + 2;
		}
	}

	if ( string[0] == '[' && string[1] == '=' )
	{
		size_t start = 2;
		size_t end = start;
		while ( !(string[end] == '\0' ||
		          (string[end + 0] == '=' && string[end + 1] == ']')) )
			end++;
		if ( string[end] == '=' && string[end + 1] == ']' )
		{
			struct construct eq_construct;
			const char* eq_end =
				parse_construct_character(string + start, &eq_construct);
			if ( !eq_end )
				error(1, 0, "malformed equivalence class");
			if ( eq_end > string + end )
				error(1, 0, "malformed equivalence class");
			if ( eq_end[0] != '=' || eq_end[1] != ']' )
				error(1, 0, "equivalence class operand must be a single character");
			construct->type = CONSTRUCT_TYPE_EQUIVALENCE_CLASS;
			construct->equivalence_class.c = eq_construct.character.c;
			return eq_end + 2;
		}
	}

	if ( string[0] == '[' )
	{
		struct construct c_construct;
		const char* c_end = parse_construct_character(string + 1, &c_construct);
		if ( c_end && c_end[0] == '*' &&
		     c_end[1 + strspn(c_end + 1, "0123456789")] == ']' )
		{
			const char* value = c_end + 1;
			const char* value_end;
			int value_base = value[0] == '0' ? 8 : 10;
			unsigned long repetitions = strtoul((char*) value, (char**) &value_end, value_base);
			assert(value_end[0] == ']');
			construct->type = CONSTRUCT_TYPE_CHARACTER_REPEAT;
			construct->character_repeat.c = c_construct.character.c;
			construct->character_repeat.repetitions = repetitions;
			return value_end + 1;
		}
	}

	struct construct result_construct;
	const char* result = parse_construct_character(string, &result_construct);
	if ( result && result[0] == '-' )
	{
		struct construct second_construct;
		const char* second = parse_construct_character(result + 1, &second_construct);
		if ( second )
		{
			construct->type = CONSTRUCT_TYPE_CHARACTER_RANGE;
			construct->character_range.from = result_construct.character.c;
			construct->character_range.to = second_construct.character.c;
			return second;
		}
	}
	return *construct = result_construct, result;
}

struct construct_iterator
{
	struct construct construct;
	struct construct* counterpart;
	const char* string;
	unsigned char character_counter;
	size_t counter;
	bool loop_done;
};

bool iterate_constructs(struct construct_iterator* iter, unsigned char* out)
{
iterate_retry:
	switch ( iter->construct.type )
	{
	case CONSTRUCT_TYPE_CHARACTER_CLASS:
		while ( !iter->loop_done )
		{
			unsigned char c = iter->character_counter;
			if ( iter->character_counter == UCHAR_MAX )
				iter->loop_done = true;
			else
				iter->character_counter++;
			if ( iter->construct.character_class.ctype(c) )
				return *out = c, true;
		}
		break;
	case CONSTRUCT_TYPE_CHARACTER_RANGE:
		if ( iter->loop_done )
			break;
		*out = iter->character_counter;
		if ( iter->character_counter == iter->construct.character_range.to )
				iter->loop_done = true;
		else if ( iter->character_counter < iter->construct.character_range.to )
			iter->character_counter++;
		else
			iter->character_counter--;
		return true;
	case CONSTRUCT_TYPE_CHARACTER_REPEAT:
		while ( iter->counter < iter->construct.character_repeat.repetitions )
		{
			iter->counter++;
			return *out = iter->construct.character_repeat.c, true;
		}
		break;
	case CONSTRUCT_TYPE_NONE:
	case CONSTRUCT_TYPE_CHARACTER:
	case CONSTRUCT_TYPE_EQUIVALENCE_CLASS:
		break;
	}

	if ( !(iter->string = parse_construct(iter->string, &iter->construct)) )
		return false;

	switch ( iter->construct.type )
	{
	case CONSTRUCT_TYPE_NONE:
		__builtin_unreachable();
	case CONSTRUCT_TYPE_CHARACTER:
		return *out = iter->construct.character.c, true;
	case CONSTRUCT_TYPE_CHARACTER_CLASS:
		iter->character_counter = 0;
		iter->loop_done = false;
		goto iterate_retry;
	case CONSTRUCT_TYPE_CHARACTER_RANGE:
		iter->character_counter = iter->construct.character_range.from;
		iter->loop_done = false;
		goto iterate_retry;
	case CONSTRUCT_TYPE_CHARACTER_REPEAT:
		if ( iter->counterpart && !iter->construct.character_repeat.repetitions )
		{
			struct construct* counterpart = iter->counterpart;
			size_t repetitions = 0;
			switch ( iter->counterpart->type )
			{
			case CONSTRUCT_TYPE_NONE:
				repetitions = 0;
				break;
			case CONSTRUCT_TYPE_CHARACTER:
				repetitions = 1;
				break;
			case CONSTRUCT_TYPE_CHARACTER_CLASS:
				repetitions = 0;
				for ( unsigned char c = 0; true; c++ )
				{
					if ( counterpart->character_class.ctype(c) )
						repetitions++;
					if ( c == UCHAR_MAX )
						break;
				}
				break;
			case CONSTRUCT_TYPE_CHARACTER_REPEAT:
				repetitions = counterpart->character_repeat.repetitions;
				break;
			case CONSTRUCT_TYPE_CHARACTER_RANGE:
				repetitions = 0;
				for ( unsigned char c = counterpart->character_range.from; true; )
				{
					repetitions++;
					if ( c < counterpart->character_range.to )
						c++;
					if ( counterpart->character_range.to < c )
						c--;
					if ( c == counterpart->character_range.to )
						break;
				}
				break;
			case CONSTRUCT_TYPE_EQUIVALENCE_CLASS:
				repetitions = 1;
				break;
			}
			iter->construct.character_repeat.repetitions = repetitions;
		}
		goto iterate_retry;
	case CONSTRUCT_TYPE_EQUIVALENCE_CLASS:
		return *out = iter->construct.equivalence_class.c, true;
	}
	__builtin_unreachable();
}

struct construct_iterator_repeat
{
	struct construct_iterator iterator;
	unsigned char last_c;
	bool has_last_c;
};

unsigned char iterate_constructs_repeat(struct construct_iterator_repeat* iter)
{
	unsigned char c;
	if ( !iterate_constructs(&iter->iterator, &c) )
	{
		if ( !iter->has_last_c  )
			error(1, 0, "when not truncating set1, string2 must be non-empty");
		c = iter->last_c;
	}
	return iter->has_last_c = true, iter->last_c = c;
}

void calculate_translator(unsigned char translator[UCHAR_MAX + 1],
                          const char* string_1,
                          const char* string_2)
{
	for ( unsigned char i = 0; true; i++ )
	{
		translator[i] = i;
		if ( i == UCHAR_MAX )
			break;
	}

	struct construct_iterator s1i;
	memset(&s1i, 0, sizeof(s1i));
	s1i.construct.type = CONSTRUCT_TYPE_NONE;
	s1i.string = string_1;

	struct construct_iterator_repeat s2i;
	memset(&s2i, 0, sizeof(s2i));
	s2i.iterator.construct.type = CONSTRUCT_TYPE_NONE;
	s2i.iterator.string = string_2;
	s2i.iterator.counterpart = &s1i.construct;

	unsigned char c1;
	while ( iterate_constructs(&s1i, &c1) )
	{
		unsigned char c2 = iterate_constructs_repeat(&s2i);
		translator[c1] = c2;
	}
}

void calculate_translator_complement(unsigned char translator[UCHAR_MAX + 1],
                                     const char* string_1,
                                     const char* string_2)
{
	for ( unsigned char i = 0; true; i++ )
	{
		translator[i] = i;
		if ( i == UCHAR_MAX )
			break;
	}

	bool s1_members[UCHAR_MAX + 1];
	memset(&s1_members, 0, sizeof(s1_members));

	struct construct_iterator s1i;
	memset(&s1i, 0, sizeof(s1i));
	s1i.construct.type = CONSTRUCT_TYPE_NONE;
	s1i.string = string_1;

	unsigned char c1;
	while ( iterate_constructs(&s1i, &c1) )
		s1_members[c1] = true;

	struct construct_iterator_repeat s2i;
	memset(&s2i, 0, sizeof(s2i));
	s2i.iterator.construct.type = CONSTRUCT_TYPE_NONE;
	s2i.iterator.string = string_2;

	for ( unsigned char i = 0; true; i++ )
	{
		if ( !s1_members[i] )
			translator[i] = iterate_constructs_repeat(&s2i);
		if ( i == UCHAR_MAX )
			break;
	}
}

void calculate_character_set(bool deletes[UCHAR_MAX + 1],
                             const char* string_1)
{
	for ( unsigned char i = 0; true; i++ )
	{
		deletes[i] = false;
		if ( i == UCHAR_MAX )
			break;
	}

	struct construct_iterator s1i;
	memset(&s1i, 0, sizeof(s1i));
	s1i.construct.type = CONSTRUCT_TYPE_NONE;
	s1i.string = string_1;

	unsigned char c1;
	while ( iterate_constructs(&s1i, &c1) )
		deletes[c1] = true;
}

void calculate_character_set_complement(bool deletes[UCHAR_MAX + 1],
                                        const char* string_1)
{
	calculate_character_set(deletes, string_1);

	for ( unsigned char i = 0; true; i++ )
	{
		deletes[i] = !deletes[i];
		if ( i == UCHAR_MAX )
			break;
	}
}

static void compact_arguments(int* argc, char*** argv)
{
	for ( int i = 0; i < *argc; i++ )
	{
		while ( i < *argc && !(*argv)[i] )
		{
			for ( int n = i; n < *argc; n++ )
				(*argv)[n] = (*argv)[n+1];
			(*argc)--;
		}
	}
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s tr [OPTION]... SET1 [SET2]\n", argv0);
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	bool flag_complement = false;
	bool flag_delete = false;
	bool flag_squeeze = false;

	const char* argv0 = argv[0];
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' || !arg[1] )
			continue;
		argv[i] = NULL;
		if ( !strcmp(arg, "--") )
			break;
		if ( arg[1] != '-' )
		{
			char c;
			while ( (c = *++arg) ) switch ( c )
			{
			case 'c': flag_complement = true; break;
			case 'C': flag_complement = true; break;
			case 'd': flag_delete = true; break;
			case 's': flag_squeeze = true; break;
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				help(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--help") )
			help(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--version") )
			version(stdout, argv0), exit(0);
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	(void) flag_delete;
	(void) flag_squeeze;

	if ( argc <= 1 )
		error(1, 0, "missing operand");
	const char* string_1 = argv[1];

	bool deletes[UCHAR_MAX + 1];
	bool squeezes[UCHAR_MAX + 1];
	unsigned char translator[UCHAR_MAX + 1];
	for ( unsigned char i = 0; true; i++ )
	{
		deletes[i] = false;
		squeezes[i] = false;
		translator[i] = i;
		if ( i == UCHAR_MAX )
			break;
	}

	if ( flag_delete && flag_squeeze )
	{
		if ( argc <= 2 )
			error(1, 0, "missing operand after `%s'", string_1);
		const char* string_2 = argv[2];

		if ( 4 <= argc )
			error(1, 0, "extra operand `%s'", argv[3]);

		if ( flag_complement )
			calculate_character_set_complement(deletes, string_1);
		else
			calculate_character_set(deletes, string_1);

		calculate_character_set(squeezes, string_2);
	}
	else if ( flag_delete && !flag_squeeze )
	{
		if ( 3 <= argc )
			error(1, 0, "extra operand `%s'", argv[3]);

		if ( flag_complement )
			calculate_character_set_complement(deletes, string_1);
		else
			calculate_character_set(deletes, string_1);
	}
	else if ( !flag_delete && flag_squeeze )
	{
		if ( argc == 2 )
		{
			if ( flag_complement )
				calculate_character_set_complement(squeezes, string_1);
			else
				calculate_character_set(squeezes, string_1);
		}
		else if ( argc == 3 )
		{
			const char* string_2 = argv[2];

			if ( flag_complement )
				calculate_translator_complement(translator, string_1, string_2);
			else
				calculate_translator(translator, string_1, string_2);

			calculate_character_set(squeezes, string_2);
		}
		else if ( 4 <= argc )
		{
			error(1, 0, "extra operand `%s'", argv[3]);
		}
	}
	else if ( !flag_delete && !flag_squeeze )
	{
		if ( argc <= 2 )
			error(1, 0, "missing operand after `%s'", string_1);
		const char* string_2 = argv[2];

		if ( 4 <= argc )
			error(1, 0, "extra operand `%s'", argv[3]);

		if ( flag_complement )
			calculate_translator_complement(translator, string_1, string_2);
		else
			calculate_translator(translator, string_1, string_2);
	}

	int last_ic = EOF;
	int ic;
	while ( (ic = getchar()) != EOF )
	{
		ic = (int) translator[ic];
		if ( squeezes[(unsigned char) ic] && ic == last_ic )
			continue;
		if ( !deletes[(unsigned char) ic] )
		{
			putchar(ic);
			last_ic = ic;
		}
	}

	if ( ferror(stdin) )
		error(1, 0, "stdin");
	if ( ferror(stdout) || fflush(stdout) == EOF )
		error(1, 0, "stdout");

	return 0;
}
