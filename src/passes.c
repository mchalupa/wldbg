/*
 * Copyright (c) 2014 Marek Chalupa
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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <assert.h>

#include "wldbg.h"
#include "wldbg-pass.h"

static int
load_pass(struct wldbg *wldbg, const char *path)
{
	return -1;
}

static int
count_args(int argc, const char *argv[])
{
	int count;

	for (count = 0; count < argc; ++count) {
		if ((strcmp(argv[count], "--") == 0)
			|| (strcmp(argv[count], ",") == 0))
			break;

		if (strrchr(argv[count], ','))
			return count + 1;
	}

	return count;
}

int
load_passes(struct wldbg *wldbg, int argc, const char *argv[])
{
	int i, rest, count, pass_num = 0;
	char *comma;

	dbg("Loading passes\n");

	rest = argc;
	while (rest) {
		count = count_args(rest, argv + argc - rest);

		if (count == 0) {
			/* -- or , or no arguments left */
			if (rest == 0)
				break;

			if (strcmp(argv[argc - rest], "--") == 0) {
				wldbg->client.path = argv[argc - rest + 1];
				wldbg->client.argv
					= (char * const *) argv + argc - rest + 1;
				wldbg->client.argc = rest - 1;
#ifdef DEBUG
				dbg("Program: %s, argc == %d\n",
					wldbg->client.path,
					wldbg->client.argc);
				for (i = 0; i < wldbg->client.argc; ++i)
					dbg("\targ[%d]: %s\n",
						i, wldbg->client.argv[i]);
#endif /* DEBUG */
				break;
			} else if (strcmp(argv[argc - rest], ",") == 0) {
				/* just skip the comma and process next pass */
				--rest;
				continue;
			}
		} else {
			++pass_num;

			dbg("Pass %d:\n", pass_num);
#ifdef DEBUG
			for (i = 0; i < count; ++i) {
				dbg("\targ[%d]: %s\n", i, argv[argc - rest + i]);
			}
#endif

			rest -= count;
		}
	}
}
