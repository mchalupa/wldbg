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
#include <assert.h>
#include <string.h>
#include <dirent.h>

#include "../src/wldbg.h"
#include "../src/wldbg-pass.h"

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
				printf("    %s\n", entry->d_name);
		}
	}

	closedir(dir);
}

static void
list_passes(int lng)
{
	const char *env;
	char path[256];

	if (lng)
		printf("hardcoded:\n");
	printf("    dump (hardcoded)\n    list (hardcoded)\n");

	snprintf(path, sizeof path, "passes/%s", LT_OBJDIR);
	if (lng)
		printf("%s:\n", path);
	list_dir(path);

	if ((env = getenv("HOME"))) {
		snprintf(path, sizeof path, "%s/.wldbg", env);
		if (lng)
			printf("%s:\n", path);
		list_dir(path);
	}

	if (lng)
		printf("/usr/local/lib/wldbg:\n");
	list_dir("/usr/local/lib/wldbg");

	if (lng)
		printf("/usr/lib/wldbg:\n");
	list_dir("/usr/lib/wldbg");

	if (lng)
		printf("/lib/wldbg:\n");
	list_dir("/lib/wldbg");
}
 
static int
list_init(struct wldbg *wldbg, void **data, int argc, const char *argv[])
{
	if (argc == 1) {
		list_passes(0);
	} else if (argc == 2 && (strcmp(argv[1], "long") == 0
		|| strcmp(argv[1], "l") == 0)) {
		list_passes(1);
	} else {
		printf("Usage: wldbg list [long]\n\n");
		wldbg->flags.error = 1;
		exit(-1);
	}

	exit(0);
}

static int
list_in(void *user_data, struct message *message)
{
}

static int
list_out(void *user_data, struct message *message)
{
}


static void
list_destroy(void *data)
{
}

struct wldbg_pass wldbg_pass = {
	.init = list_init,
	.destroy = list_destroy,
	.server_pass = list_in,
	.client_pass = list_out,
};
