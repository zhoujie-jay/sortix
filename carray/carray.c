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
 * carray.c
 * Convert a binary file to a C array.
 */

#include <errno.h>
#include <error.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void get_short_option_variable(char c, char** varptr,
                               const char* arg, int argc, char** argv, int* ip,
                               const char* argv0)
{
	free(*varptr);
	if ( *(arg+1) )
	{
		*varptr = strdup(arg + 1);
	}
	else
	{
		if ( *ip + 1 == argc )
		{
			error(0, 0, "option requires an argument -- '%c'", c);
			fprintf(stderr, "Try `%s --help' for more information.\n", argv0);
			exit(1);
		}
		*varptr = strdup(argv[*ip + 1]);
		argv[++(*ip)] = NULL;
	}
}

#define GET_SHORT_OPTION_VARIABLE(c, varptr) \
        get_short_option_variable(c, varptr, arg, argc, argv, &i, argv0)

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... [INPUT...] -o OUTPUT\n", argv0);
	fprintf(fp, "Convert a binary file to a C array.\n");
	fprintf(fp, "\n");
	fprintf(fp, "Mandatory arguments to long options are mandatory for short options too.\n");
	fprintf(fp, "  -c, --const               add const keyword\n");
	fprintf(fp, "  -e, --extern              add extern keyword\n");
	fprintf(fp, "      --extern-c            use C linkage\n");
	fprintf(fp, "  -f, --forward             forward declare rather than define\n");
	fprintf(fp, "      --guard=MACRO         use include guard macro MACRO\n");
	fprintf(fp, "  -g, --use-guard           add include guard\n");
	fprintf(fp, "  -i, --include             include prerequisite headers\n");
	fprintf(fp, "      --identifier=NAME     use identifier NAME\n");
	fprintf(fp, "      --includes=INCLUDES   emit raw preprocessor INCLUDES\n");
	fprintf(fp, "  -o, --output=FILE         write result to FILE\n");
	fprintf(fp, "  -r, --raw                 emit only raw \n");
	fprintf(fp, "  -s, --static              add static keyword\n");
	fprintf(fp, "      --type=TYPE           use type TYPE [unsigned char]\n");
	fprintf(fp, "  -v, --volatile            add volatile keyword\n");
	fprintf(fp, "      --char                use type char\n");
	fprintf(fp, "      --signed-char         use type signed char\n");
	fprintf(fp, "      --unsigned-char       use type unsigned char\n");
	fprintf(fp, "      --int8_t              use type int8_t\n");
	fprintf(fp, "      --uint8_t             use type uint8_t\n");
	fprintf(fp, "      --help                display this help and exit\n");
	fprintf(fp, "      --version             output version information and exit\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	bool flag_const = false;
	bool flag_extern_c = false;
	bool flag_extern = false;
	bool flag_forward = false;
	bool flag_guard = false;
	bool flag_include = false;
	bool flag_raw = false;
	bool flag_static = false;
	bool flag_volatile = false;

	char* arg_guard = NULL;
	char* arg_identifier = NULL;
	char* arg_includes = NULL;
	char* arg_output = NULL;
	char* arg_type = NULL;

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
			case 'c': flag_const = true; break;
			case 'e': flag_extern = true; break;
			case 'f': flag_forward = true; break;
			case 'g': flag_guard = true; break;
			case 'i': flag_include = true; break;
			case 'o': GET_SHORT_OPTION_VARIABLE('o', &arg_output); arg = "o"; break;
			case 'r': flag_raw = true; break;
			case 's': flag_static = true; break;
			case 'v': flag_volatile = true; break;
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
		else if ( !strcmp(arg, "--const") )
			flag_const = true;
		else if ( !strcmp(arg, "--extern") )
			flag_extern = true;
		else if ( !strcmp(arg, "--forward") )
			flag_forward = true;
		else if ( !strcmp(arg, "--use-guard") )
			flag_guard = true;
		else if ( !strcmp(arg, "--include") )
			flag_include = true;
		else if ( !strcmp(arg, "--raw") )
			flag_raw = true;
		else if ( !strcmp(arg, "--static") )
			flag_static = true;
		else if ( !strcmp(arg, "--volatile") )
			flag_volatile = true;
		else if ( !strcmp(arg, "--char") )
			free(arg_type), arg_type = strdup("char");
		else if ( !strcmp(arg, "--signed-char") )
			free(arg_type), arg_type = strdup("signed char");
		else if ( !strcmp(arg, "--unsigned-char") )
			free(arg_type), arg_type = strdup("unsigned char");
		else if ( !strcmp(arg, "--int8_t") )
			free(arg_type), arg_type = strdup("int8_t");
		else if ( !strcmp(arg, "--uint8_t") )
			free(arg_type), arg_type = strdup("uint8_t");
		else if ( GET_OPTION_VARIABLE("--guard", &arg_guard) )
			flag_guard = true;
		else if ( GET_OPTION_VARIABLE("--identifier", &arg_identifier) ) { }
		else if ( GET_OPTION_VARIABLE("--includes", &arg_includes) )
			flag_include = true;
		else if ( GET_OPTION_VARIABLE("--output", &arg_output) ) { }
		else if ( GET_OPTION_VARIABLE("--type", &arg_type) ) { }
		else if ( !strcmp(arg, "--extern-c") )
			flag_extern_c = true;
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	const char* output_path = arg_output;

	if ( flag_extern && flag_static )
		error(1, 0, "the --extern and --static are mutually incompatible");
	if ( flag_forward && flag_raw )
		error(1, 0, "the --forward and --raw are mutually incompatible");

	if ( !arg_type )
		arg_type = strdup("unsigned char");

	char* guard = arg_guard;
	if ( !guard )
	{
		if ( output_path )
			guard = strdup(output_path);
		else if ( 2 <= argc && strcmp(argv[1], "-") != 0 )
			asprintf(&guard, "%s_H", argv[1]);
		else
			guard = strdup("CARRAY_H");

		for ( size_t i = 0; guard[i]; i++ )
		{
			if ( 'A' <= guard[i] && guard[i] <= 'Z' )
				continue;
			else if ( 'a' <= guard[i] && guard[i] <= 'z' )
				guard[i] = 'A' + guard[i] - 'a';
			else if ( i != 0 && '0' <= guard[i] && guard[i] <= '9' )
				continue;
			else if ( guard[i] == '+' )
				guard[i] = 'X';
			else if ( i == 0 )
				guard[i] = 'X';
			else
				guard[i] = '_';
		}
	}

	if ( flag_include && !arg_includes )
	{
		if ( !strcmp(arg_type, "int8_t") ||
		     !strcmp(arg_type, "uint8_t") )
			arg_includes = strdup("#include <stdint.h>");
	}

	char* identifier = arg_identifier;
	if ( !identifier )
	{
		if ( output_path )
			identifier = strdup(output_path);
		else if ( 2 <= argc && strcmp(argv[1], "-") != 0 )
			identifier = strdup(argv[1]);
		else
			identifier = strdup("carray");

		for ( size_t i = 0; identifier[i]; i++ )
		{
			if ( i && identifier[i] == '.' && !strchr(identifier + i, '/') )
				identifier[i] = '\0';
			else if ( 'a' <= identifier[i] && identifier[i] <= 'z' )
				continue;
			else if ( 'A' <= identifier[i] && identifier[i] <= 'Z' )
				identifier[i] = 'a' + identifier[i] - 'A';
			else if ( i != 0 && '0' <= identifier[i] && identifier[i] <= '9' )
				continue;
			else if ( guard[i] == '+' )
				identifier[i] = 'x';
			else if ( i == 0 )
				identifier[i] = 'x';
			else
				identifier[i] = '_';
		}
	}

	if ( output_path && !freopen(output_path, "w", stdout) )
		error(1, errno, "%s", output_path);

	if ( flag_guard && guard )
	{
		printf("#ifndef %s\n", guard);
		printf("#define %s\n", guard);
		printf("\n");
	}

	if ( flag_include && arg_includes )
	{
		printf("%s\n", arg_includes);
		printf("\n");
	}

	if ( !flag_raw )
	{
		if ( flag_extern_c )
		{
			printf("#if defined(__cplusplus)\n");
			printf("extern \"C\" {\n");
			printf("#endif\n");
			printf("\n");
		}
		if ( flag_extern )
			printf("extern ");
		if ( flag_static )
			printf("static ");
		if ( flag_const )
			printf("const ");
		if ( flag_volatile )
			printf("volatile ");
		printf("%s %s[]", arg_type, identifier);
		if ( flag_forward )
			printf(";\n");
		else
			printf(" = {\n");
	}

	if ( !flag_forward )
	{
		bool begun_row = false;
		unsigned int position = 0;

		for ( int i = 0; i < argc; i++ )
		{
			if ( i == 0 && 2 <= argc )
				continue;
			FILE* fp;
			const char* arg;
			if ( argc == 1 || !strcmp(argv[i], "-") )
			{
				arg = "<stdin>";
				fp = stdin;
			}
			else
			{
				arg = argv[i];
				fp = fopen(arg, "r");
			}
			if ( !fp )
				error(1, errno, "%s", arg);
			int ic;
			while ( (ic = fgetc(fp)) != EOF )
			{
				printf("%c0x%02X,", position++ ? ' ' : '\t', ic);
				begun_row = true;
				if ( position == (80 - 8) / 6 )
				{
					printf("\n");
					position = 0;
					begun_row = false;
				}
			}
			if ( ferror(fp) )
				error(1, errno, "fgetc: %s", arg);
			if ( fp != stdin )
				fclose(fp);
		}

		if ( begun_row )
			printf("\n");
	}

	if ( !flag_raw )
	{
		if ( !flag_forward )
			printf("};\n");
		if ( flag_extern_c )
		{
			printf("\n");
			printf("#if defined(__cplusplus)\n");
			printf("} /* extern \"C\" */\n");
			printf("#endif\n");
		}
	}

	if ( flag_guard && guard )
	{
		printf("\n");
		printf("#endif\n");
	}

	if ( ferror(stdout) || fflush(stdout) == EOF )
		error(1, errno, "%s", output_path ? output_path : "stdout");

	return 0;
}
