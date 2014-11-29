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
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include <wldbg.h>

/* duplicate argv array */
int
copy_arguments(char ***to, int argc, const char*argv[])
{
	int i, num = 0;
	char **tmp = calloc(argc + 1, sizeof(char *));
	if (!tmp)
		return -1;

	while(*argv != NULL) {
		tmp[num] = strdup(*argv);
		if (!tmp[num]) {
			for (i = 0; i < num; ++i)
				free(tmp[num]);
			free(tmp);
			return -1;
		}

		++num;
		++argv;
	}

	/* calloc did this for us */
	assert(tmp[num] == NULL);
	assert(argc == num);

	*to = tmp;

	return num;
}

void
free_arguments(char **argv)
{
	char **arg_tmp = argv;

	while(*argv != NULL) {
		free(*argv);
		++argv;
	}

	free(arg_tmp);
}

char *
skip_ws_to_newline(char *str)
{
	char *p = str;
	while (*p && *p != '\n' && isspace(*p))
		++p;

	return p;
}
