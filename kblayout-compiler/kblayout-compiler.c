/*
 * Copyright (c) 2014 Jonas 'Sortie' Termansen.
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
 * kblayout-compiler.c
 * Keyboard layout compiler.
 */

#include <assert.h>
#include <endian.h>
#include <errno.h>
#include <error.h>
#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "kblayout.h"

#define DEFAULT_COMPRESSION "none"
#define DEFAULT_FORMAT "sortix-kblayout-1"

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))

bool accept_any(uint32_t combination)
{
	(void) combination;
	return true;
}

bool accept_no_shift(uint32_t combination)
{
	if ( combination & 0b0001 )
		return false;
	return true;
}

bool accept_no_shift_no_alt_gr(uint32_t combination)
{
	if ( combination & 0b0101 )
		return false;
	return true;
}

bool accept_shift_no_alt_gr(uint32_t combination)
{
	if ( combination & 0b0100 )
		return false;
	if ( !(combination & 0b0001) )
		return false;
	return true;
}

bool accept_no_shift_alt_gr(uint32_t combination)
{
	if ( combination & 0b0001 )
		return false;
	if ( !(combination & 0b0100) )
		return false;
	return true;
}

bool accept_shift_alt_gr(uint32_t combination)
{
	if ( !(combination & 0b0001) )
		return false;
	if ( !(combination & 0b0100) )
		return false;
	return true;
}

bool accept_lower_no_altgr(uint32_t combination)
{
	if ( (combination & 0b0100) )
		return false;
	if ( (combination & 0b0001) && !(combination & 0b0010) )
		return false;
	if ( !(combination & 0b0001) && (combination & 0b0010) )
		return false;
	return true;
}

bool accept_upper_no_altgr(uint32_t combination)
{
	if ( (combination & 0b0100) )
		return false;
	if ( (combination & 0b0001) && (combination & 0b0010) )
		return false;
	if ( !(combination & 0b0001) && !(combination & 0b0010) )
		return false;
	return true;
}

bool accept_numlock(uint32_t combination)
{
	if ( !(combination & 0b1000) )
		return false;
	return true;
}

struct key
{
	const char* name;
	uint32_t scancode;
};

struct key keys[] =
{
	{ "KESC", 0x01 },
	{ "K1", 0x02 },
	{ "K2", 0x03 },
	{ "K3", 0x04 },
	{ "K4", 0x05 },
	{ "K5", 0x06 },
	{ "K6", 0x07 },
	{ "K7", 0x08 },
	{ "K8", 0x09 },
	{ "K9", 0x0A },
	{ "K0", 0x0B },
	{ "KSYM1", 0x0C },
	{ "KSYM2", 0x0D },
	{ "KBKSPC", 0x0E },
	{ "KTAB", 0x0F },
	{ "KQ", 0x10 },
	{ "KW", 0x11 },
	{ "KE", 0x12 },
	{ "KR", 0x13 },
	{ "KT", 0x14 },
	{ "KY", 0x15 },
	{ "KU", 0x16 },
	{ "KI", 0x17 },
	{ "KO", 0x18 },
	{ "KP", 0x19 },
	{ "KSYM3", 0x1A },
	{ "KSYM4", 0x1B },
	{ "KENTER", 0x1C },
	{ "KLCTRL", 0x1D },
	{ "KA", 0x1E },
	{ "KS", 0x1F },
	{ "KD", 0x20 },
	{ "KF", 0x21 },
	{ "KG", 0x22 },
	{ "KH", 0x23 },
	{ "KJ", 0x24 },
	{ "KK", 0x25 },
	{ "KL", 0x26 },
	{ "KSYM5", 0x27 },
	{ "KSYM6", 0x28 },
	{ "KSYM7", 0x29 },
	{ "KLSHIFT", 0x2A },
	{ "KSYM8", 0x2B },
	{ "KZ", 0x2C },
	{ "KX", 0x2D },
	{ "KC", 0x2E },
	{ "KV", 0x2F },
	{ "KB", 0x30 },
	{ "KN", 0x31 },
	{ "KM", 0x32 },
	{ "KSYM9", 0x33 },
	{ "KSYM10", 0x34 },
	{ "KSYM11", 0x35 },
	{ "KRSHIFT", 0x36 },
	{ "KSYM12", 0x37 },
	{ "KLALT", 0x38 },
	{ "KSPACE", 0x39 },
	{ "KCAPSLOCK", 0x3A },
	{ "KF1", 0x3B },
	{ "KF2", 0x3C },
	{ "KF3", 0x3D },
	{ "KF4", 0x3E },
	{ "KF5", 0x3F },
	{ "KF6", 0x40 },
	{ "KF7", 0x41 },
	{ "KF8", 0x42 },
	{ "KF9", 0x43 },
	{ "KF10", 0x44 },
	{ "KNUMLOCK", 0x45 },
	{ "KSCROLLLOCK", 0x46 },
	{ "KPAD7", 0x47 },
	{ "KPAD8", 0x48 },
	{ "KPAD9", 0x49 },
	{ "KSYM13", 0x4A },
	{ "KPAD4", 0x4B },
	{ "KPAD5", 0x4C },
	{ "KPAD6", 0x4D },
	{ "KSYM14", 0x4E },
	{ "KPAD1", 0x4F },
	{ "KPAD2", 0x50 },
	{ "KPAD3", 0x51 },
	{ "KPAD0", 0x52 },
	{ "KSYM15", 0x53 },
	{ "KALTSYSRQ", 0x54 },
	{ "KNO_STANDARD_MEANING_1", 0x55 /* Sometimes F11, F12, or even FN */ },
	{ "KNO_STANDARD_MEANING_2", 0x56 /* Possibly Windows key? */ },
	{ "KF11", 0x57 },
	{ "KF12", 0x58 },
/* [0x59,", 0x7F] are not really standard. */
	{ "KPADENTER", (0x80 + 0x1C) },
	{ "KRCTRL", (0x80 + 0x1D) },
	{ "KFAKELSHIFT", (0x80 + 0x2A) },
	{ "KSYM16", (0x80 + 0x35) },
	{ "KFAKERSHIFT", (0x80 + 0x36) },
	{ "KCTRLPRINTSCRN", (0x80 + 0x37) },
	{ "KRALT", (0x80 + 0x38) },
	{ "KCTRLBREAK", (0x80 + 0x46) },
	{ "KHOME", (0x80 + 0x47) },
	{ "KUP", (0x80 + 0x48) },
	{ "KPGUP", (0x80 + 0x49) },
	{ "KLEFT", (0x80 + 0x4B) },
	{ "KRIGHT", (0x80 + 0x4D) },
	{ "KEND", (0x80 + 0x4F) },
	{ "KDOWN", (0x80 + 0x50) },
	{ "KPGDOWN", (0x80 + 0x51) },
	{ "KINSERT", (0x80 + 0x52) },
	{ "KDELETE", (0x80 + 0x53) },
	{ "KLSUPER", (0x80 + 0x5B) },
	{ "KRSUPER", (0x80 + 0x5C) },
	{ "KMENU", (0x80 + 0x5D) },
};

struct modifier
{
	char* name;
};

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

bool get_option_variable(const char* option, char** varptr,
                         const char* arg, int argc, char** argv, int* ip,
                         const char* argv0)
{
	size_t option_len = strlen(option);
	if ( strncmp(option, arg, option_len) != 0 )
		return false;
	if ( arg[option_len] == '=' )
	{
		*varptr = strdup(arg + option_len + 1);
		return true;
	}
	if ( arg[option_len] != '\0' )
		return false;
	if ( *ip + 1 == argc )
	{
		fprintf(stderr, "%s: expected operand after `%s'\n", argv0, option);
		exit(1);
	}
	*varptr = strdup(argv[++*ip]), argv[*ip] = NULL;
	return true;
}

#define GET_OPTION_VARIABLE(str, varptr) \
        get_option_variable(str, varptr, arg, argc, argv, &i, argv0)

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... INPUT -o OUTPUT\n", argv0);
	fprintf(fp, "Compile a keyboard layout description to a binary representation.\n");
	fprintf(fp, "\n");
	fprintf(fp, "Mandatory arguments to long options are mandatory for short options too.\n");
	fprintf(fp, "      --compression=FORMAT  compression algorithm [%s]\n", DEFAULT_COMPRESSION);
	fprintf(fp, "      --format=FORMAT       format version [%s]\n", DEFAULT_FORMAT);
	fprintf(fp, "  -o, --output=FILE         write result to FILE\n");
	fprintf(fp, "      --help                display this help and exit\n");
	fprintf(fp, "      --version             output version information and exit\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

static
void compress_actions_none(FILE* fp,
                           struct kblayout_action* actions,
                           size_t actions_length)
{
	fwrite(actions, sizeof(struct kblayout_action), actions_length, fp);
}

static
void compress_actions_runlength_bevlq(FILE* fp,
                                      struct kblayout_action* actions,
                                      size_t actions_length)
{
	for ( size_t i = 0; i < actions_length; )
	{
		uint8_t j;
		j = 0;
		while ( i + 1 + j < actions_length && j < 127 &&
		        actions[i+1+j].type == actions[i].type &&
		        actions[i+1+j].codepoint == actions[i].codepoint )
			j++;
		uint8_t to_write = 0;
		uint8_t extra_written = 0;
		if ( 0 < j )
		{
			to_write = 1;
			fputc(0x00 | j, fp);
			extra_written = j;
		}
		else
		{
			j = 0;
			while ( i + 1 + j < actions_length && j < 127 &&
			        !(actions[i+1+j].type == actions[i+j].type &&
			          actions[i+1+j].codepoint == actions[i+j].codepoint) )
				j++;
			fputc(0x80 | j, fp);
			to_write = j;
			extra_written = 0;
		}
		for ( size_t n = 0; n < to_write; n++ )
		{
			fputc(actions[i+n].type & 0xFF, fp);
			if ( actions[i+n].codepoint < 1 << 7 )
				fputc(0x00 | (actions[i+n].codepoint >> 0*7 & 0x7F), fp);
			else if ( actions[i+n].codepoint < 1 << 2 * 7 )
				fputc(0x80 | (actions[i+n].codepoint >> 1*7 & 0x7F), fp),
				fputc(0x00 | (actions[i+n].codepoint >> 0*7 & 0x7F), fp);
			else if ( actions[i+n].codepoint < 1 << 3 * 7 )
				fputc(0x80 | (actions[i+n].codepoint >> 2*7 & 0x7F), fp),
				fputc(0x80 | (actions[i+n].codepoint >> 1*7 & 0x7F), fp),
				fputc(0x00 | (actions[i+n].codepoint >> 0*7 & 0x7F), fp);
			else if ( actions[i+n].codepoint < 1 << 4 * 7 )
				fputc(0x80 | (actions[i+n].codepoint >> 3*7 & 0x7F), fp),
				fputc(0x80 | (actions[i+n].codepoint >> 2*7 & 0x7F), fp),
				fputc(0x80 | (actions[i+n].codepoint >> 1*7 & 0x7F), fp),
				fputc(0x00 | (actions[i+n].codepoint >> 0*7 & 0x7F), fp);
			else
				fputc(0x80 | (actions[i+n].codepoint >> 4*7 & 0x7F), fp),
				fputc(0x80 | (actions[i+n].codepoint >> 3*7 & 0x7F), fp),
				fputc(0x80 | (actions[i+n].codepoint >> 2*7 & 0x7F), fp),
				fputc(0x80 | (actions[i+n].codepoint >> 1*7 & 0x7F), fp),
				fputc(0x00 | (actions[i+n].codepoint >> 0*7 & 0x7F), fp);
		}
		i += to_write + extra_written;
	}
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	bool verbose = false;

	char* arg_compression = strdup(DEFAULT_COMPRESSION);
	char* arg_format = strdup(DEFAULT_FORMAT);
	char* arg_output = NULL;

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
			case 'o':
				free(arg_output);
				if ( *(arg+1) )
					arg_output = strdup(arg + 1);
				else
				{
					if ( i + 1 == argc )
					{
						error(0, 0, "option requires an argument -- 'o'");
						fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
						exit(125);
					}
					arg_output = strdup(argv[i+1]);
					argv[++i] = NULL;
				}
				arg = "o";
				break;
			case 'v': verbose = true; break;
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
		else if ( !strcmp(arg, "--verbose") )
			verbose = true;
		else if ( GET_OPTION_VARIABLE("--compression", &arg_compression) ) { }
		else if ( GET_OPTION_VARIABLE("--format", &arg_format) ) { }
		else if ( GET_OPTION_VARIABLE("--output", &arg_output) ) { }
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	if ( argc == 1 )
	{
		help(stdout, argv0);
		exit(1);
	}

	compact_arguments(&argc, &argv);

	const char* compression = arg_compression;
	const char* format = arg_format;
	const char* output_path = arg_output;

	if ( argc == 1 )
	{
		fprintf(stderr, "%s: No input file was specified\n", argv0);
		fprintf(stderr, "Try `%s --help' for more information.\n", argv0);
		exit(1);
	}

	const char* input_path = argv[1];

	char* name = NULL;
	size_t num_scancodes = 0;
	struct modifier modifiers[KBLAYOUT_MAX_NUM_MODIFIERS];
	size_t num_modifiers = 0;
	bool (*condition)(uint32_t) = accept_any;

	struct kblayout_action* actions = NULL;

	FILE* input = fopen(input_path, "r");
	char* line = NULL;
	size_t line_size = 0;
	ssize_t line_length;
	while ( 0 <= (line_length = getline(&line, &line_size, input)) )
	{
		if ( line_length && line[line_length-1] == '\n' )
			line[--line_length] = '\0';
		if ( line[0] == '/' && line[1] == '/' )
			continue;
		if ( !line[0] )
			continue;
		if ( strncmp(line, "name \"", strlen("name \"")) == 0 )
		{
			name = strdup(line + strlen("name \""));
			if ( strlen(name) && name[strlen(name)-1] == '\"' )
				name[strlen(name)-1] = '\0';
			continue;
		}
		if ( strncmp(line, "modifier ", strlen("modifier ")) == 0 )
		{
			modifiers[num_modifiers++].name = strdup(line + strlen("modifier "));
			continue;
		}
		if ( line[line_length-1] == ':' )
		{
			line[--line_length] = '\0';
			if ( !strcmp(line, "-shift") )
				condition = accept_no_shift;
			else if ( !strcmp(line, "-shift & -altgr") )
				condition = accept_no_shift_no_alt_gr;
			else if ( !strcmp(line, "+shift & -altgr") )
				condition = accept_shift_no_alt_gr;
			else if ( !strcmp(line, "-shift & +altgr") )
				condition = accept_no_shift_alt_gr;
			else if ( !strcmp(line, "+shift & +altgr") )
				condition = accept_shift_alt_gr;
			else if ( !strcmp(line, "shift = caps & -altgr") )
				condition = accept_lower_no_altgr;
			else if ( !strcmp(line, "shift ^ caps & -altgr") )
				condition = accept_upper_no_altgr;
			else if ( !strcmp(line, "+numlock") )
				condition = accept_numlock;
			else
				printf("%s\n", line);
			continue;
		}

		const char* statement = line;
		if ( statement[0] == '\t' )
			statement++;
		else
			condition = accept_any;

		bool found_key = false;
		for ( size_t i = 0; i < ARRAY_SIZE(keys); i++ )
		{
			if ( strncmp(statement, keys[i].name, strlen(keys[i].name)) != 0 )
				continue;
			if ( statement[strlen(keys[i].name)+0] != ':' )
				continue;
			if ( statement[strlen(keys[i].name)+1] != ' ' )
				continue;
			found_key = true;
			if ( num_scancodes <= keys[i].scancode )
			{
				size_t old_action_length = num_scancodes * (1 << num_modifiers);
				size_t old_action_size = sizeof(struct kblayout_action) * old_action_length;
				num_scancodes = keys[i].scancode + 1;
				size_t new_action_length = num_scancodes * (1 << num_modifiers);
				size_t new_action_size = sizeof(struct kblayout_action) * new_action_length;
				actions = (struct kblayout_action*) realloc(actions, new_action_size);
				memset((uint8_t*) actions + old_action_size, 0, new_action_size - old_action_size);
			}
			for ( size_t j = 0; j < 1U << num_modifiers; j++ )
			{
				if ( !condition(j) )
					continue;
				assert(keys[i].scancode < num_scancodes);
				struct kblayout_action* action = &actions[keys[i].scancode * (1 << num_modifiers) + j];
				const char* expr = statement + strlen(keys[i].name) + 2;
				if ( strncmp(expr, "dead ", strlen("dead ")) == 0 )
				{
					const char* which = expr + strlen("dead ");
					wchar_t wc;
					mbstate_t ps;
					memset(&ps, 0, sizeof(ps));
					size_t count = mbrtowc(&wc, which, strlen(which), &ps);
					assert(0 < count);
					action->type = htole32(KBLAYOUT_ACTION_TYPE_DEAD);
					action->codepoint = htole32((uint32_t) wc);
				}
				else if ( strncmp(expr, "modify ", strlen("modify ")) == 0 )
				{
					const char* which = expr + strlen("modify ");
					for ( size_t n = 0; n < num_modifiers; n++ )
					{
						if ( strcmp(modifiers[n].name, which) != 0 )
							continue;
						action->type = htole32(KBLAYOUT_ACTION_TYPE_MODIFY);
						action->codepoint = htole32((uint32_t) n);
						break;
					}
				}
				else if ( strncmp(expr, "toggle ", strlen("toggle ")) == 0 )
				{
					const char* which = expr + strlen("toggle ");
					for ( size_t n = 0; n < num_modifiers; n++ )
					{
						if ( strcmp(modifiers[n].name, which) != 0 )
							continue;
						action->type = htole32(KBLAYOUT_ACTION_TYPE_TOGGLE);
						action->codepoint = htole32((uint32_t) n);
						break;
					}
				}
				else if ( expr[0] == '"' && 2 <= strlen(expr) && expr[strlen(expr)-1] == '"' )
				{
					wchar_t wc;
					mbstate_t ps;
					memset(&ps, 0, sizeof(ps));
					size_t count = mbrtowc(&wc, expr + 1, strlen(expr) - 2, &ps);
					if ( wc == L'\\' )
					{
						count = mbrtowc(&wc, expr + 1 + count, strlen(expr) - 2 - count, &ps);
						switch ( wc )
						{
						case L'a': wc = L'\a'; break;
						case L'b': wc = L'\b'; break;
						case L'e': wc = L'\e'; break;
						case L'f': wc = L'\f'; break;
						case L'n': wc = L'\n'; break;
						case L'r': wc = L'\r'; break;
						case L't': wc = L'\t'; break;
						case L'v': wc = L'\v'; break;
						default: break;
						}
					}
					assert(0 < count);
					action->type = htole32(KBLAYOUT_ACTION_TYPE_CODEPOINT);
					action->codepoint = htole32((uint32_t) wc);
				}
			}
			break;
		}
		assert(found_key);
	}
	free(line);
	fclose(input);

	if ( verbose )
	{
		for ( size_t i = 0; i < num_scancodes; i++ )
		{
			struct kblayout_action* row = &actions[i * (1 << num_modifiers)];
			bool any_actions = false;
			for ( size_t j = 0; !any_actions && j < 1U << num_modifiers; j++ )
				if ( row[j].type || row[j].codepoint )
					any_actions = true;
			if ( !any_actions )
				continue;
			struct key* key = NULL;
			for ( size_t j = 0; !key && j < ARRAY_SIZE(keys); j++ )
				if ( keys[j].scancode == i )
					key = &keys[j];
			assert(key);
			printf("%-16s", key->name);
			for ( size_t j = 0; j < 1U << num_modifiers; j++ )
			{
				if ( row[j].type == 0 && row[j].codepoint )
					printf("%lc\t", row[j].codepoint);
				else if ( row[j].type == 1 && row[j].codepoint )
					printf("d%lc\t", row[j].codepoint);
				else if ( row[j].type == 2 && row[j].codepoint )
					printf("m%u\t", row[j].codepoint);
				else
					printf("-\t");
			}
			printf("\n");
		}
	}

	if ( !strcmp(compression, "default") )
		compression = DEFAULT_COMPRESSION;

	if ( !strcmp(format, "default") )
		format = DEFAULT_FORMAT;

	if ( strcmp(compression, "none") != 0 &&
	     strcmp(compression, "runlength-bevlq") != 0 )
	{
		fprintf(stderr, "%s: Unsupported compression `%s'\n", argv0, compression);
		fprintf(stderr, "Try `%s --help' for more information.\n", argv0);
		exit(1);
	}

	if ( strcmp(format, "sortix-kblayout-1") != 0 )
	{
		fprintf(stderr, "%s: Unsupported format `%s'\n", argv0, format);
		fprintf(stderr, "Try `%s --help' for more information.\n", argv0);
		exit(1);
	}

	if ( !output_path )
	{
		fprintf(stderr, "%s: No output file was specified\n", argv0);
		fprintf(stderr, "Try `%s --help' for more information.\n", argv0);
		exit(1);
	}

	FILE* output = fopen(output_path, "w");
	if ( !output )
		error(1, errno, "%s", output_path);

	uint32_t compression_algorithm;
	void (*compressor)(FILE*, struct kblayout_action*, size_t);

	if ( !strcmp(compression, "runlength-bevlq") )
		compression_algorithm = KBLAYOUT_COMPRESSION_RUNLENGTH_BEVLQ,
		compressor = compress_actions_runlength_bevlq;
	else
		compression_algorithm = KBLAYOUT_COMPRESSION_NONE,
		compressor = compress_actions_none;

	struct kblayout header;
	memset(&header, 0, sizeof(header));
	strncpy(header.magic, format, sizeof(header.magic));
	strncpy(header.name, name ? name : "", sizeof(header.name));
	header.num_modifiers = htole32((uint32_t) num_modifiers);
	header.num_scancodes = htole32((uint32_t) num_scancodes);
	header.compression_algorithm = htole32((uint32_t) compression_algorithm);
	fwrite(&header, 1, sizeof(header), output);

	size_t actions_length = num_scancodes * (1 << num_modifiers);
	compressor(output, actions, actions_length);

	fclose(output);

	return 0;
}
