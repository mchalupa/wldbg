/*
 * Copyright (c) 2014 - 2015 Marek Chalupa
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
#include <assert.h>
#include <string.h>
#include <dirent.h>

#include "wldbg-private.h"

static void
list_dir(const char *path)
{
	struct dirent *entry = NULL;
	DIR *dir;
	char *dot;

	dir = opendir(path);
	if (dir) {
		while((entry = readdir(dir))) {
			if (entry->d_type == DT_DIR)
				continue;

			if (!(dot = strrchr(entry->d_name, '.')))
				continue;

			if (strcmp("so", dot + 1) == 0)
				printf("    %s (%s)\n", entry->d_name, path);
		}
	}

	closedir(dir);
}

void
list_passes(void)
{
	const char *env;
	char path[256];

	printf("    list (hardcoded)\n    resolve (hardcoded)\n");

	list_dir(".");
	snprintf(path, sizeof path, "passes/%s", LT_OBJDIR);
	list_dir(path);

	if ((env = getenv("HOME"))) {
		snprintf(path, sizeof path, "%s/.wldbg", env);
		list_dir(path);
	}

	snprintf(path, sizeof path, "%s/wldbg", LIBDIR);
	list_dir(path);

	list_dir("/usr/local/lib/wldbg");
	list_dir("/usr/lib/wldbg");
	list_dir("/lib/wldbg");
}

static int
list_init(struct wldbg *wldbg,
	  struct wldbg_pass *pass,
	  int argc, const char *argv[])
{
	(void) pass;
	(void) argv;

	if (argc == 1) {
		list_passes();
	} else {
		printf("Usage: wldbg list\n\n");
		printf("List all available passes\n");
		wldbg->flags.error = 1;
		exit(-1);
	}

	exit(0);
}

static int
list_in(void *user_data, struct wldbg_message *message)
{
	(void) user_data;
	(void) message;

	return PASS_STOP;
}

static int
list_out(void *user_data, struct wldbg_message *message)
{
	(void) user_data;
	(void) message;

	return PASS_STOP;
}

static void
list_destroy(void *data)
{
	(void) data;
	return;
}

struct wldbg_pass wldbg_pass_list = {
	.init = list_init,
	.destroy = list_destroy,
	.server_pass = list_in,
	.client_pass = list_out,
};
