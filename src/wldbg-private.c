/*
 * Copyright (c) 2014 Marek Chalupa
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
