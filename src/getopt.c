/*
 * Copyright (c) 2015 Marek Chalupa
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <assert.h>

#include "wldbg-private.h"

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
	} else if (is_prefix_of(arg, "one-by-one")) {
		dbg("Command line option: one-by-one\n");
		opts->one_by_one = 1;
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
			if (!set_opt(argv[n] + 1, opts))
				return -1;
		} else
			break;
	}

	dbg("Passes or programs are starting at %d\n", n);
	/* return how many options we have */
	return n;
}
