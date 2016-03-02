/*
 * Copyright (c) 2013 Jonas 'Sortie' Termansen.
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
 * getopt/getopt_long.c
 * Command-line parsing utility function.
 */

#include <assert.h>
#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>

char* optarg = NULL;
int opterr = 1;
int optind = 1;
int optopt = 0;

static char* const* optcurargv;
static const char* optcurarg;
static int optcurind;
static size_t optcurpos;

static bool optcurvalid = false;

static bool is_supported_option_declaration(const char* shortopts)
{
	// Optional parameters to options only make sense after a option character.
	if ( shortopts[0] == ':' && shortopts[1] == ':' )
		return false;

	for ( size_t i = 0; shortopts[i]; i++ )
	{
		if ( shortopts[i] == '-' || shortopts[i] == '+' || shortopts[i] == ';' )
			return false;

		// We support a: and a::, but a::: doesn't make sense.
		if ( shortopts[i] == ':' &&
		     shortopts[i+1] == ':' &&
		     shortopts[i+2] == ':' )
			return false;

		// We don't support that -W foo is the same as --foo.
		if ( shortopts[i] == 'W' && shortopts[i+1] == ';' )
			return false;
	}
	return true;
}

// TODO: It would appear that it is allowed to abbreviate options if the
//       abbreviation isn't ambiguous. (WTF)
static
const struct option* find_long_option(char* arg,
                                      const struct option* longopts,
                                      int* option_index,
                                      char** option_arg)
{
	char* argname = arg + 2;
	size_t argname_length = strcspn(argname, "=");

	for ( int i = 0; longopts && longopts[i].name; i++ )
	{
		if ( longopts[i].has_arg == no_argument &&
		     argname[argname_length] == '=' )
			continue;
		if ( strncmp(argname, longopts[i].name, argname_length) != 0 )
			continue;
		if ( longopts[i].name[argname_length] != '\0' )
			continue;
		return *option_arg = (argname[argname_length] == '=' ?
		                      argname + argname_length + 1 :
		                      NULL),
		       *option_index = i,
		       &longopts[i];
	}
	return NULL;
}

int getopt_long(int argc, char* const* argv, const char* shortopts,
                const struct option* longopts, int* longindex)
{
	assert(shortopts);
	assert(0 <= optind);

	// Verify that the input string was compatible.
	if ( !is_supported_option_declaration(shortopts) )
		return errno = EINVAL, -1;

	// The caller will handle missing arguments if the short options string
	// start with a colon character.
	bool caller_handles_missing_argument = false;
	if ( shortopts[0] == ':' )
		shortopts++,
		caller_handles_missing_argument = false;

	// Check if we have finished processing the command line.
	if ( argc <= optind )
		return optcurvalid = false, -1;

	char* arg = argv[optind];

	// Stop if we encountered a NULL pointer.
	if ( !arg )
		return optcurvalid = false, -1;

	// Stop if we encounteed an argument that didn't start with -.
	if ( arg[0] != '-' )
		return optcurvalid = false, -1;

	// Stop if we encounted the string "-" as an argument.
	if ( arg[1] == '\0' )
		return optcurvalid = false, -1;

	// Stop and skip the argument if we encountered the string "--".
	if ( arg[1] == '-' && arg[2] == '\0' )
		return optcurvalid = false, optind++, -1;

	// Check if we encountered a long option.
	if ( arg[1] == '-' && arg[2] != '\0' )
	{
		// Locate the long option.
		int option_index;
		char* option_arg;
		const struct option* option =
			find_long_option(arg, longopts, &option_index, &option_arg);

		// Increment so we don't process the next argument on the next call.
		optind++;

		// Verify the long option is allowed.
		if ( !option )
		{
			if ( !caller_handles_missing_argument && opterr )
				error(0, 0, "unrecognized option `%s'", arg);
			return '?';
		}

		if ( longindex )
			*longindex = option_index;

		// Check whether the next argument is the parameter to this option.
		if ( !option_arg && option->has_arg != no_argument )
		{
			if ( optind + 1 < argc && argv[optind] &&
			     (option->has_arg == required_argument || argv[optind][0] != '-') )
				option_arg = argv[optind++];
		}

		// Return an error if a required option parameter was missing.
		if ( !option_arg && option->has_arg == required_argument )
		{
			if ( caller_handles_missing_argument )
				return ':';
			if ( opterr )
				error(0, 0, "option requires an argument `--%s'", option->name);
			return '?';
		}

		return optarg = option_arg,
		       option->flag ? (*option->flag = option->val, 0) : option->val;
	}

	// Unfortunately, there is no standardized optpos global variable, and
	// programs are supposed to be able to restart the parsing by setting optind
	// to 1. We need to keep track of the current position in the current
	// argument, but it would have to be invalidated whenever we restart the
	// parsing. We keep track of what the parsing input is supposed to be so we
	// can easily invalidate the current position in the current argument
	// whenever something fishy is going on.
	if ( !optcurvalid || optcurargv != argv || optcurarg != arg || optcurind != optind )
	{
		optcurargv = argv;
		optcurarg = arg;
		optcurind = optind;
		optcurpos = 1;
		optcurvalid = true;
	}

	int short_option = (unsigned char) arg[optcurpos++];

	// The character : can't be a option character.
	if ( short_option == ':' )
		short_option = '?';

	// Verify the short option is allowed.
	char* opt_decl = strchr(shortopts, short_option);
	if ( !opt_decl )
	{
		if ( !caller_handles_missing_argument && opterr )
				error(0, 0, "invalid option -- '%c'", short_option);
		optopt = short_option;
		short_option = '?';
	}

	// Check whether the short option requires an parameter.
	if ( opt_decl && opt_decl[1] == ':' )
	{
		// This argument cannot contain more short options after this.
		optcurvalid = false;

		// A double :: means optional parameter.
		bool required = opt_decl[2] != ':';

		// The rest of the argument is the parameter if non-empty.
		if ( arg[optcurpos] )
		{
			optarg = arg + optcurpos;
			optind += 1;
		}

		// Otherwise, the next element (if any) is the parameter.
		else if ( optind + 1 < argc && argv[optind+1] &&
		          (required || argv[optind+1][0] != '-') )
		{
			optarg = argv[optind+1];
			optind += 2;
		}

		// It's okay to have a missing parameter if it was optional.
		else if ( !required )
		{
			optarg = NULL;
			optind += 1;
		}

		// Return an error if a required option parameter was missing.
		else
		{
			if ( caller_handles_missing_argument )
				return optopt = short_option, ':';
			if ( opterr )
				error(0, 0, "option requires an argument -- '%c'", short_option);
			return optopt = short_option, '?';
		}
	}

	// If we reached the last option, use the next index next time.
	else if ( !arg[optcurpos] )
	{
		optind++;
		optcurvalid = false;
	}

	return short_option;
}
