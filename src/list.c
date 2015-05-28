/*
 * Copyright (c) 2014 - 2015 Marek Chalupa
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

	snprintf(path, sizeof path, "passes/%s", LT_OBJDIR);
	list_dir(path);

	if ((env = getenv("HOME"))) {
		snprintf(path, sizeof path, "%s/.wldbg", env);
		list_dir(path);
	}

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
list_in(void *user_data, struct message *message)
{
	(void) user_data;
	(void) message;

	return PASS_STOP;
}

static int
list_out(void *user_data, struct message *message)
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
