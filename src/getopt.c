/*
 * Copyright (c) 2015 Marek Chalupa
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <assert.h>

#include "wldbg-private.h"
#include "getopt.h"

static int
is_prefix_of(const char *what, const char *src)
{
	assert(what);
	assert(src);

	while (*what) {
		/* src is shorter than what */
		if (!*src)
			return 0;

		if (*what != *src)
			return 0;

		++what;
		++src;
	}

	return 1;
}

static int
set_opt(const char *arg, struct wldbg_options *opts)
{
	int match = 0;

	if (*arg == '\0') {
		fprintf(stderr, "Error: empty option\n");
		return 0;
	}

	if (is_prefix_of(arg, "help")) {
		return 0;
	} else if (is_prefix_of(arg, "interactive")) {
		dbg("Command line option: interactive\n");
		opts->interactive = 1;
		match = 1;
	} else if (is_prefix_of(arg, "server-mode")) {
		dbg("Command line option: server-mode\n");
		opts->server_mode = 1;
		match = 1;
	} else if (is_prefix_of(arg, "pass-whole-buffer")) {
		dbg("Command line option: pass-whole-buffer\n");
		opts->pass_whole_buffer = 1;
		match = 1;
	} else if (is_prefix_of(arg, "objinfo")) {
		dbg("Command line option: objinfo\n");
		opts->objinfo = 1;
		match = 1;
	}

	if (!match) {
		fprintf(stderr, "Ignoring unknown option: %s\n", arg);
	}

	return 1;
}

int get_opts(int argc, char *argv[], struct wldbg_options *opts)
{
	int n = 1;
	for (; n < argc; ++n) {
		/* separator */
		if (strcmp("--", argv[n]) == 0) {
			++n;
			break;
		}

		/* options */
		if (is_prefix_of("--", argv[n])) {
			if (!set_opt(argv[n] + 2, opts))
				return -1;
		} else if (is_prefix_of("-", argv[n])) {
			/* -g is a synonym for objinfo */
			if (argv[n][1] == 'g' && argv[n][2] == 0) {
				/* objinfo */
				set_opt("objinfo", opts);
				continue;
			}

			if (!set_opt(argv[n] + 1, opts))
				return -1;
		} else
			break;
	}

	dbg("Passes or programs are starting at %d\n", n);
	/* return how many options we have */
	return n;
}
