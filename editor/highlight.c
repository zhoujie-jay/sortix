/*
 * Copyright (c) 2013, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * highlight.c
 * Syntax highlighting.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#include "editor.h"
#include "highlight.h"

enum language language_of_path(const char* path)
{
	size_t path_length = strlen(path);
	if ( 2 <= path_length &&
	     (!strcmp(path+path_length-2, ".c") ||
		  !strcmp(path+path_length-2, ".h")) )
		return LANGUAGE_C_CXX;
	if ( 4 <= path_length &&
	     (!strcmp(path+path_length-4, ".c++") ||
		  !strcmp(path+path_length-4, ".h++") ||
		  !strcmp(path+path_length-4, ".cxx") ||
		  !strcmp(path+path_length-4, ".hxx") ||
		  !strcmp(path+path_length-4, ".cpp") ||
		  !strcmp(path+path_length-4, ".hpp")) )
		return LANGUAGE_C_CXX;
	if ( (5 <= path_length && !strcmp(path+path_length-5, ".diff")) ||
	     (6 <= path_length && !strcmp(path+path_length-6, ".patch")) )
		return LANGUAGE_DIFF;
	return LANGUAGE_NONE;
}

static size_t recognize_constant(const wchar_t* string, size_t string_length)
{
	bool binary = false;
	bool hex = false;
	size_t result = 0;
	if ( result < string_length && string[result] == L'0' )
	{
		result++;
		if ( result < string_length && (string[result] == L'x' ||
		                                string[result] == L'X') )
		{
			result++;
			hex = true;
		}
		if ( result < string_length && (string[result] == L'b' ||
		                                string[result] == L'B') )
		{
			result++;
			binary = true;
		}
	}
	bool floating = false;
	bool exponent = false;
	while ( result < string_length )
	{
		if ( (binary && L'0' <= string[result] && string[result] <= L'1') ||
		     (!binary && L'0' <= string[result] && string[result] <= L'9') ||
		     (hex && L'a' <= string[result] && string[result] <= L'f') ||
		     (hex && L'A' <= string[result] && string[result] <= L'F') )
		{
			result++;
			continue;
		}
		if ( string[result] == L'.' )
		{
			if ( binary || hex || floating )
				return 0;
			floating = true;
			result++;
			continue;
		}
		if ( !(hex || binary) &&
		     (string[result] == L'e' || string[result] == L'E') )
		{
			if ( !result )
				return 0;
			if ( exponent )
				return 0;
			floating = true;
			result++;
			continue;
		}
		break;
	}
	if ( result == ((hex || binary) ? 2 : 0) )
		return 0;
	if ( floating )
	{
		if ( result < string_length && (string[result] == L'l' ||
		                                string[result] == L'L') )
			result++;
		else if ( result < string_length && (string[result] == L'f' ||
			                                 string[result] == L'F') )
			result++;
	}
	else
	{
		if ( result < string_length && (string[result] == L'u' ||
		                                string[result] == L'U') )
			result++;
		if ( result < string_length && (string[result] == L'l' ||
			                            string[result] == L'L') )
			result++;
		if ( result < string_length && (string[result] == L'l' ||
			                            string[result] == L'L') )
			result++;
	}
	return result;
}

static void editor_colorize_c_cxx(struct editor* editor);
static void editor_colorize_diff(struct editor* editor);

void editor_colorize(struct editor* editor)
{
	if ( editor->color_lines_length != editor->lines_used ||
	     editor->highlight_source == LANGUAGE_NONE )
	{
		for ( size_t i = 0; i < editor->color_lines_used; i++ )
			free(editor->color_lines[i].data);
		free(editor->color_lines);
		editor->color_lines_used = 0;
		editor->color_lines_length = 0;
		editor->color_lines = NULL;
	}

	if ( editor->highlight_source == LANGUAGE_NONE )
		return;

	if ( !editor->color_lines )
	{
		editor->color_lines = (struct color_line*)
			malloc(sizeof(struct color_line) * editor->lines_used);
		if ( !editor->color_lines )
			return;
		editor->color_lines_used = editor->lines_used;
		editor->color_lines_length = editor->lines_used;
		for ( size_t i = 0; i < editor->lines_used; i++ )
			editor->color_lines[i].data = NULL,
			editor->color_lines[i].length = 0;
	}

	for ( size_t i = 0; i < editor->lines_used; i++ )
	{
		if ( editor->color_lines[i].length == editor->lines[i].used )
			continue;

		editor->color_lines[i].data = (uint8_t*) malloc(editor->lines[i].used);
		if ( !editor->color_lines[i].data )
		{
			for ( size_t n = 0; n < i; i++ )
				free(editor->color_lines[n].data);
			free(editor->color_lines);
			editor->color_lines_used = 0;
			editor->color_lines_length = 0;
			editor->color_lines = NULL;
			return;
		}

		editor->color_lines[i].length = editor->lines[i].used;
	}

	switch ( editor->highlight_source )
	{
	case LANGUAGE_NONE: break;
	case LANGUAGE_C_CXX: editor_colorize_c_cxx(editor); break;
	case LANGUAGE_DIFF: editor_colorize_diff(editor); break;
	}
}

static void editor_colorize_c_cxx(struct editor* editor)
{
	enum
	{
		STATE_INIT,
		STATE_LINE_COMMENT,
		STATE_MULTI_LINE_COMMENT,
		STATE_PREPROCESSOR,
		STATE_PREPROCESSOR_VALUE,
		STATE_SINGLE_QUOTE,
		STATE_DOUBLE_QUOTE,
		STATE_NUMBER,
		STATE_KEYWORD,
		STATE_TYPE,
	} state = STATE_INIT, prev_state = STATE_INIT;

	bool escaped = false;
	size_t fixed_state = 0;
	size_t multi_expiration = 0;
	for ( size_t y = 0; y < editor->lines_used; y++ )
	{
		struct line* line = &editor->lines[y];
		for ( size_t x = 0; x < line->used; x++ )
		{
			wchar_t pc = x ? line->data[x-1] : '\0';
			wchar_t c = line->data[x];
			wchar_t nc = x+1 < line->used ? line->data[x+1] : L'\0';
			uint8_t color = 7;

			// The character makes you leave this state.

			if ( !fixed_state && (state == STATE_KEYWORD ||
			                      state == STATE_TYPE ||
			                      state == STATE_NUMBER ) )
				state = STATE_INIT;

			// The character makes you enter a new state.
			if ( !fixed_state )
			{
				if ( state == STATE_INIT && c == L'#' )
					state = STATE_PREPROCESSOR;
				if ( state == STATE_PREPROCESSOR && c == '<' )
					state = STATE_PREPROCESSOR_VALUE, fixed_state = 1;
				if ( state == STATE_PREPROCESSOR && c == '"' )
					state = STATE_PREPROCESSOR_VALUE, fixed_state = 1;
			}

			// TODO: Detect NULL as a value.

			if ( !fixed_state && state == STATE_INIT &&
			     !(x && (iswalnum(pc) || pc == L'_')) )
			{
				size_t number_length = recognize_constant(line->data + x,
				                                          line->used - x);
				if ( number_length )
				{
					state = STATE_NUMBER;
					fixed_state = number_length;
				}
			}

			if ( !fixed_state && state == STATE_INIT && c == L'\'' )
				state = STATE_SINGLE_QUOTE, fixed_state = 1, escaped = false;

			if ( !fixed_state && state == STATE_INIT && c == L'"' )
				state = STATE_DOUBLE_QUOTE, fixed_state = 1, escaped = false;

			if ( !fixed_state && (state == STATE_INIT ||
			                      state == STATE_PREPROCESSOR) )
			{
				if ( c == L'/' && nc == L'/' )
					state = STATE_LINE_COMMENT, fixed_state = 2;
				else if ( c == L'/' && nc == L'*' )
				{
					prev_state = state;
					multi_expiration = 0;
					state = STATE_MULTI_LINE_COMMENT;
					fixed_state = 2;
				}
			}

			if ( !fixed_state && state == STATE_INIT )
			{
				const wchar_t* keywords[] =
				{
					L"alignas",
					L"alignof",
					L"and",
					L"and_eq",
					L"asm",
					L"bitand",
					L"bitor",
					L"break",
					L"case",
					L"catch",
					L"class",
					L"compl",
					L"const_cast",
					L"constexpr",
					L"continue",
					L"decltype",
					L"default",
					L"delete",
					L"do",
					L"dynamic_cast",
					L"else",
					L"enum",
					L"false",
					L"final",
					L"for",
					L"friend",
					L"goto",
					L"if",
					L"namespace",
					L"new",
					L"not",
					L"not_eq",
					L"nullptr",
					L"operator",
					L"or",
					L"or_eq",
					L"override",
					L"private",
					L"protected",
					L"public",
					L"reinterpret_cast",
					L"return",
					L"sizeof",
					L"static_assert",
					L"static_cast",
					L"struct",
					L"switch",
					L"template",
					L"this",
					L"thread_local",
					L"throw",
					L"true",
					L"try",
					L"typedef",
					L"typeid",
					L"typename",
					L"union",
					L"using",
					L"virtual",
					L"while",
					L"xor",
					L"xor_eq",
				};

				bool cannot_be_keyword = x && (iswalnum(pc) || pc == L'_');
				for ( size_t i = 0;
				      !cannot_be_keyword && i < sizeof(keywords) / sizeof(keywords[0]);
				      i++ )
				{
					const wchar_t* keyword = keywords[i];
					if ( c != keyword[0] )
						continue;
					size_t keyword_length = wcslen(keyword);
					if ( (x - line->used) < keyword_length )
						continue;
					if ( wcsncmp(line->data + x, keyword, keyword_length) != 0 )
						continue;

					if ( keyword_length < line->used - x )
					{
						wchar_t wc = line->data[x + keyword_length];
						if ( iswalnum(wc) || wc == L'_' )
							continue;
					}

					state = STATE_KEYWORD;
					fixed_state = keyword_length;
				}
			}

			if ( !fixed_state && state == STATE_INIT )
			{
				const wchar_t* types[] =
				{
					L"auto",
					L"blkcnt_t",
					L"blksize_t",
					L"bool",
					L"char",
					L"char16_t",
					L"char32_t",
					L"clockid_t",
					L"clock_t",
					L"const",
					L"dev_t",
					L"double",
					L"explicit",
					L"extern",
					L"FILE",
					L"float",
					L"fpos_t",
					L"fsblkcnt_t",
					L"fsfilcnt_t",
					L"gid_t",
					L"id_t",
					L"inline",
					L"ino_t",
					L"int",
					L"int16_t",
					L"int32_t",
					L"int64_t",
					L"int8_t",
					L"intmax_t",
					L"intptr_t",
					L"locale_t",
					L"long",
					L"mode_t",
					L"mutable",
					L"nlink_t",
					L"noexcept",
					L"off_t",
					L"pid_t",
					L"ptrdiff_t",
					L"register",
					L"restrict",
					L"short",
					L"signed",
					L"size_t",
					L"ssize_t",
					L"static",
					L"suseconds_t",
					L"thread_local",
					L"timer_t",
					L"time_t",
					L"trace_t",
					L"uid_t",
					L"uint16_t",
					L"uint32_t",
					L"uint64_t",
					L"uint8_t",
					L"uintmax_t",
					L"uintptr_t",
					L"unsigned",
					L"useconds_t",
					L"va_list",
					L"void",
					L"volatile",
					L"wchar_t",
				};

				bool cannot_be_type = x && (iswalnum(pc) || pc == L'_');
				for ( size_t i = 0;
				      !cannot_be_type && i < sizeof(types) / sizeof(types[0]);
				      i++ )
				{
					const wchar_t* type = types[i];
					if ( c != type[0] )
						continue;
					size_t type_length = wcslen(type);
					if ( (x - line->used) < type_length )
						continue;
					if ( wcsncmp(line->data + x, type, type_length) != 0 )
						continue;
					if ( (x - line->used) != type_length &&
					     (iswalnum(line->data[x+type_length]) ||
					      line->data[x+type_length] == L'_') )
						continue;
					state = STATE_TYPE;
					fixed_state = type_length;
				}
			}

			// The current state uses a non-default color.

			if ( state == STATE_SINGLE_QUOTE ||
			     state == STATE_DOUBLE_QUOTE ||
			     state == STATE_NUMBER ||
			     state == STATE_PREPROCESSOR_VALUE )
				color = 5;

			if ( state == STATE_PREPROCESSOR )
				color = 3;

			if ( state == STATE_LINE_COMMENT ||
			     state == STATE_MULTI_LINE_COMMENT )
				color = 6;

			if ( state == STATE_KEYWORD )
				color = 1;

			if ( state == STATE_TYPE )
				color = 2;

			// The character is the last character in this state.

			if ( !fixed_state )
			{
				if ( state == STATE_SINGLE_QUOTE && !escaped && c == L'\'' )
					state = STATE_INIT, fixed_state = 1;
				if ( state == STATE_DOUBLE_QUOTE && !escaped && c == L'"' )
					state = STATE_INIT, fixed_state = 1;
				if ( state == STATE_PREPROCESSOR_VALUE && c == '>' )
					state = STATE_PREPROCESSOR;
				if ( state == STATE_PREPROCESSOR_VALUE && c == '"' )
					state = STATE_PREPROCESSOR;
			}

			if ( (state == STATE_SINGLE_QUOTE || state == STATE_DOUBLE_QUOTE) )
			{
				if ( !escaped && c == L'\\' )
					escaped = true;
				else if ( escaped )
					escaped = false;
			}

			if ( !fixed_state && state == STATE_MULTI_LINE_COMMENT )
			{
				if ( multi_expiration ==  1 )
					state = prev_state, multi_expiration = 0;
				else if ( c == L'*' && nc == L'/' )
					multi_expiration = 1;
			}

			if ( state == STATE_PREPROCESSOR )
				escaped = c == L'\\' && !nc;

			editor->color_lines[y].data[x] = color;

			if ( fixed_state )
				fixed_state--;
		}

		if ( state == STATE_LINE_COMMENT ||
		     state == STATE_PREPROCESSOR ||
		     state == STATE_PREPROCESSOR_VALUE ||
		     state == STATE_SINGLE_QUOTE ||
		     state == STATE_DOUBLE_QUOTE )
		{
			if ( state == STATE_PREPROCESSOR && escaped )
				escaped = false;
			else
				state = STATE_INIT;
		}
	}
}

static void editor_colorize_diff(struct editor* editor)
{
	for ( size_t y = 0; y < editor->lines_used; y++ )
	{
		struct line* line = &editor->lines[y];
		uint8_t color = 7;
		if ( line->used && line->data[0] == L'-' )
			color = 1;
		else if ( line->used && line->data[0] == L'+' )
			color = 2;
		else if ( line->used && line->data[0] == L'@' )
			color = 6;
		else if ( line->used && !iswblank(line->data[0]) )
			color = 4 + 8;
		for ( size_t x = 0; x < line->used; x++ )
		{
			editor->color_lines[y].data[x] = color;
		}
	}
}
