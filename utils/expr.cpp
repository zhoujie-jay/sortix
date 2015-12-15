/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/>.

    expr.cpp
    Evaluate expressions.

*******************************************************************************/

#include <errno.h>
#include <error.h>
#include <inttypes.h>
#include <locale.h>
#include <regex.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: Support the `expr + foo' GNU syntax where foo is treated as a string
//       literal, but this requires disambiguating stuff like this:
//         `expr foo : + foo'
//         `expr + 5 + + 2 +'
// TODO: Support the other GNU function extensions documented in help().

char* strdup_or_die(const char* str)
{
	char* result = strdup(str);
	if ( !str )
		error(2, errno, "strdup");
	return result;
}

char* strndup_or_die(const char* str, size_t n)
{
	char* result = strndup(str, n);
	if ( !str )
		error(2, errno, "strndup");
	return result;
}

char* print_intmax_or_die(intmax_t value)
{
	char value_string[sizeof(intmax_t) * 3];
	snprintf(value_string, sizeof(value_string), "%ji", value);
	return strdup_or_die(value_string);
}

__attribute__((noreturn))
void syntax_error()
{
	error(2, 0, "syntax error");
	__builtin_unreachable();
}

__attribute__((noreturn))
void non_integer_argument()
{
	error(2, 0, "non-integer argument");
	__builtin_unreachable();
}

__attribute__((noreturn))
void division_by_zero()
{
	error(2, 0, "division by zero");
	__builtin_unreachable();
}

char* interpret(char** tokens, size_t num_tokens);

char* interpret_left_associative(char** tokens,
                                 size_t num_tokens,
                                 const char* operator_name,
                                 char* (*next)(char**, size_t, const void*),
                                 const void* next_context,
                                 char* (*function)(const char*, const char*))
{
	size_t depth = 0;
	for ( size_t n = num_tokens; n != 0; n-- )
	{
		size_t i = n - 1;
		if ( !strcmp(tokens[i], ")") )
		{
			depth++;
			continue;
		}
		if ( !strcmp(tokens[i], "(") )
		{
			if ( depth == 0 )
				syntax_error();
			depth--;
			continue;
		}
		if ( depth != 0 )
			continue;
		if ( strcmp(tokens[i], operator_name) != 0 )
			continue;
		if ( i == 0 )
			syntax_error();
		if ( i + 1 == num_tokens )
			syntax_error();
		char** left_tokens = tokens;
		size_t num_left_tokens = i;
		char** right_tokens = tokens + i + 1;
		size_t num_right_tokens = num_tokens - (i + 1);
		char* left_value =
			interpret_left_associative(left_tokens, num_left_tokens,
			                           operator_name, next, next_context,
			                           function);
		char* right_value = next(right_tokens, num_right_tokens, next_context);
		char* value = function(left_value, right_value);
		free(left_value);
		free(right_value);
		return value;
	}

	if ( 0 < depth )
		syntax_error();

	return next(tokens, num_tokens, next_context);
}

char* bool_to_boolean_value(bool b)
{
	return strdup_or_die(b ? "1" : "0");
}

char* interpret_literal(char** tokens,
                        size_t num_tokens,
                        const void* = NULL)
{
	if ( num_tokens != 1 )
		syntax_error();
	return strdup_or_die(tokens[0]);
}

char* interpret_parentheses(char** tokens,
                            size_t num_tokens,
                            const void* = NULL)
{
	if ( 2 <= num_tokens &&
	     strcmp(tokens[0], "(") == 0 &&
	     strcmp(tokens[num_tokens-1], ")") == 0 )
		return interpret(tokens + 1, num_tokens - 2);
	return interpret_literal(tokens, num_tokens);
}

char* evaluate_and(const char* a, const char* b)
{
	if ( strcmp(a, "") != 0 && strcmp(a, "0") != 0 &&
	     strcmp(b, "") != 0 && strcmp(b, "0") != 0 )
		return strdup_or_die(a);
	return strdup_or_die("0");
}

char* evaluate_or(const char* a, const char* b)
{
	if ( strcmp(a, "") != 0 && strcmp(a, "0") != 0 )
		return strdup_or_die(a);
	if ( strcmp(b, "") != 0 && strcmp(b, "0") != 0 )
		return strdup_or_die(b);
	return strdup_or_die("0");
}

int compare_values(const char* a, const char* b)
{
	// TODO: Compute using arbitrary length integers.
	char* a_endptr;
	char* b_endptr;
	intmax_t a_int = strtoimax((char*) a, &a_endptr, 10);
	intmax_t b_int = strtoimax((char*) b, &b_endptr, 10);
	if ( a[0] && !*a_endptr && b[0] && !*b_endptr )
	{
		if ( a_int < b_int )
			return -1;
		if ( a_int > b_int )
			return 1;
		return 0;
	}
	return strcoll(a, b);
}

char* evaluate_eq(const char* a, const char* b)
{
	return bool_to_boolean_value(compare_values(a, b) == 0);
}

char* evaluate_gt(const char* a, const char* b)
{
	return bool_to_boolean_value(0 < compare_values(a, b));
}

char* evaluate_ge(const char* a, const char* b)
{
	return bool_to_boolean_value(0 <= compare_values(a, b));
}

char* evaluate_lt(const char* a, const char* b)
{
	return bool_to_boolean_value(compare_values(a, b) < 0);
}

char* evaluate_le(const char* a, const char* b)
{
	return bool_to_boolean_value(compare_values(a, b) <= 0);
}

char* evaluate_neq(const char* a, const char* b)
{
	return bool_to_boolean_value(compare_values(a, b) != 0);
}

char* evaluate_integer_function(const char* a, const char* b,
                                intmax_t (*function)(intmax_t, intmax_t))
{
	// TODO: Compute using arbitrary length integers.
	char* a_endptr;
	char* b_endptr;
	intmax_t a_int = strtoimax((char*) a, &a_endptr, 10);
	intmax_t b_int = strtoimax((char*) b, &b_endptr, 10);
	if ( !a[0] || *a_endptr || !b[0] || *b_endptr )
		non_integer_argument();
	return print_intmax_or_die(function(a_int, b_int));
}

intmax_t integer_add(intmax_t a, intmax_t b)
{
	return a + b;
}

char* evaluate_add(const char* a, const char* b)
{
	return evaluate_integer_function(a, b, integer_add);
}

intmax_t integer_sub(intmax_t a, intmax_t b)
{
	return a - b;
}

char* evaluate_sub(const char* a, const char* b)
{
	return evaluate_integer_function(a, b, integer_sub);
}

intmax_t integer_mul(intmax_t a, intmax_t b)
{
	return a * b;
}

char* evaluate_mul(const char* a, const char* b)
{
	return evaluate_integer_function(a, b, integer_mul);
}

intmax_t integer_div(intmax_t a, intmax_t b)
{
	if ( b == 0 )
		division_by_zero();
	return a / b;
}

char* evaluate_div(const char* a, const char* b)
{
	return evaluate_integer_function(a, b, integer_div);
}

intmax_t integer_mod(intmax_t a, intmax_t b)
{
	if ( b == 0 )
		division_by_zero();
	return a % b;
}

char* evaluate_mod(const char* a, const char* b)
{
	return evaluate_integer_function(a, b, integer_mod);
}

char* evaluate_match(const char* a, const char* b)
{
	regex_t regex;
	int status = regcomp(&regex, b, 0);
	if ( status != 0 )
	{
		char errbuf[256];
		const char* errmsg = errbuf;
		char* erralloc = NULL;
		size_t errbuf_needed;
		if ( sizeof(errbuf) < (errbuf_needed = regerror(status, &regex, errbuf,
		                                                sizeof(errbuf))) )
		{
			if ( (erralloc = (char*) malloc(errbuf_needed)) )
			{
				errmsg = erralloc;
				regerror(status, &regex, erralloc, errbuf_needed);
			}
		}
		error(2, 0, "compiling regular expression: %s", errmsg);
		free(erralloc);
	}

	char* result;

	regmatch_t rm[2];
	if ( regexec(&regex, a, 2, rm, 0) == 0 && rm[0].rm_so == 0 )
	{
		if ( 0 <= rm[1].rm_so )
			result = strndup_or_die(a + rm[1].rm_so, rm[1].rm_eo - rm[1].rm_so);
		else
			result = print_intmax_or_die(rm[0].rm_eo);
	}
	else
	{
		if ( 0 < regex.re_nsub )
			result = strdup_or_die("");
		else
			result = strdup_or_die("0");
	}

	regfree(&regex);

	return result;
}

struct binary_operator
{
	const char* operator_name;
	char* (*function)(const char*, const char*);
};

struct binary_operator binary_operators[] =
{
	{ "|", evaluate_or },
	{ "&", evaluate_and },
	{ "=", evaluate_eq },
	{ ">", evaluate_gt },
	{ ">=", evaluate_ge },
	{ "<", evaluate_lt },
	{ "<=", evaluate_le },
	{ "!=", evaluate_neq },
	{ "+", evaluate_add },
	{ "-", evaluate_sub },
	{ "*", evaluate_mul },
	{ "/", evaluate_div },
	{ "%", evaluate_mod },
	{ ":", evaluate_match },
};

char* interpret_binary_operator(char** tokens,
                                size_t num_tokens,
                                const void* context)
{
	size_t index = *(const size_t*) context;
	size_t next_index = index + 1;

	char* (*next)(char**, size_t, const void*);
	const void* next_context;

	if ( next_index == sizeof(binary_operators) / sizeof(binary_operators[0]) )
	{
		next = interpret_parentheses;
		next_context = NULL;
	}
	else
	{
		next = interpret_binary_operator;
		next_context = &next_index;
	}

	struct binary_operator* binop = &binary_operators[index];
	return interpret_left_associative(tokens, num_tokens, binop->operator_name,
	                                  next, next_context, binop->function);
}

char* interpret(char** tokens, size_t num_tokens)
{
	if ( !num_tokens )
		syntax_error();
	size_t operator_index = 0;
	return interpret_binary_operator(tokens, num_tokens, &operator_index);
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s EXPRESSION\n", argv0);
	fprintf(fp, "  or:  %s OPTION\n", argv0);
	fprintf(fp, "\n");
	fprintf(fp, "      --help     display this help and exit\n");
	fprintf(fp, "      --version  output version information and exit\n");
	fprintf(fp, "\n");
	fprintf(fp, "Print the value of EXPRESSION to standard output.  A blank line below\n");
	fprintf(fp, "separates increasing precedence groups.  EXPRESSION may be:\n");
	fprintf(fp, "\n");
	fprintf(fp, "  ARG1 | ARG2       ARG1 if it is neither null nor 0, otherwise ARG2\n");
	fprintf(fp, "\n");
	fprintf(fp, "  ARG1 & ARG2       ARG1 if neither argument is null or 0, otherwise 0\n");
	fprintf(fp, "\n");
	fprintf(fp, "  ARG1 < ARG2       ARG1 is less than ARG2\n");
	fprintf(fp, "  ARG1 <= ARG2      ARG1 is less than or equal to ARG2\n");
	fprintf(fp, "  ARG1 = ARG2       ARG1 is equal to ARG2\n");
	fprintf(fp, "  ARG1 != ARG2      ARG1 is unequal to ARG2\n");
	fprintf(fp, "  ARG1 >= ARG2      ARG1 is greater than or equal to ARG2\n");
	fprintf(fp, "  ARG1 > ARG2       ARG1 is greater than ARG2\n");
	fprintf(fp, "\n");
	fprintf(fp, "  ARG1 + ARG2       arithmetic sum of ARG1 and ARG2\n");
	fprintf(fp, "  ARG1 - ARG2       arithmetic difference of ARG1 and ARG2\n");
	fprintf(fp, "\n");
	fprintf(fp, "  ARG1 * ARG2       arithmetic product of ARG1 and ARG2\n");
	fprintf(fp, "  ARG1 / ARG2       arithmetic quotient of ARG1 divided by ARG2\n");
	fprintf(fp, "  ARG1 %% ARG2       arithmetic remainder of ARG1 divided by ARG2\n");
	fprintf(fp, "\n");
	fprintf(fp, "  STRING : REGEXP   anchored pattern match of REGEXP in STRING\n");
	fprintf(fp, "\n");
#if 0
	fprintf(fp, "  match STRING REGEXP        same as STRING : REGEXP\n");
	fprintf(fp, "  substr STRING POS LENGTH   substring of STRING, POS counted from 1\n");
	fprintf(fp, "  index STRING CHARS         index in STRING where any CHARS is found, or 0\n");
	fprintf(fp, "  length STRING              length of STRING\n");
	fprintf(fp, "  + TOKEN                    interpret TOKEN as a string, even if it is a\n");
	fprintf(fp, "                               keyword like `match' or an operator like `/'\n");
#endif
	fprintf(fp, "\n");
	fprintf(fp, "  ( EXPRESSION )             value of EXPRESSION\n");
	fprintf(fp, "\n");
	fprintf(fp, "Beware that many operators need to be escaped or quoted for shells.\n");
	fprintf(fp, "Comparisons are arithmetic if both ARGs are numbers, else lexicographical.\n");
	fprintf(fp, "Pattern matches return the string matched between \\<( and \\) or null; if\n");
	fprintf(fp, "\\( and \\) are not used, they return the number of characters matched or 0.\n");
	fprintf(fp, "\n");
	fprintf(fp, "Exit status is 0 if EXPRESSION is neither null nor 0, 1 if EXPRESSION is null\n");
	fprintf(fp, "or 0, 2 if EXPRESSION is syntactically invalid, and 3 if an error occurred.\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
	fprintf(fp, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
	fprintf(fp, "This is free software: you are free to change and redistribute it.\n");
	fprintf(fp, "There is NO WARRANTY, to the extent permitted by law.\n");
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	if ( argc == 2 && !strcmp(argv[1], "--help") )
		help(stdout, argv[0]), exit(0);
	if ( argc == 2 && !strcmp(argv[1], "--version") )
		version(stdout, argv[0]), exit(0);

	char* value = interpret(argv + 1, argc - 1);
	printf("%s\n", value);
	bool success = strcmp(value, "") != 0 && strcmp(value, "0") != 0;
	free(value);

	return success ? 0 : 1;
}
