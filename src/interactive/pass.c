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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "wayland/wayland-private.h"

#include "wldbg-pass.h"
#include "interactive.h"
#include "passes.h"
#include "wldbg-private.h"
#include "interactive-commands.h"

void
cmd_pass_help(int oneline)
{
	if (oneline) {
		printf("Add, remove, list passes");
		return;
	}

	printf("Possible arguments:\n");
	printf("\tlist\t\t- list available passes\n");
	printf("\tloaded\t\t- list loaded passes\n");
	printf("\tadd NAME\t- add pass NAME.so\n");
	printf("\tremove NAME\t- remove pass NAME\n");
}

static void
add_pass(struct wldbg *wldbg, const char *name)
{
	struct pass *pass;

	dbg("Adding pass '%s'\n", name);

	pass = create_pass(name);
	if (pass) {
		/* XXX add arguments */
		if (pass_init(wldbg, pass, 0, NULL) != 0) {
			fprintf(stderr, "Failed initializing pass '%s'\n",
				name);
			dealloc_pass(pass);
		} else {
			/* insert always at the head */
			wl_list_insert(wldbg->passes.next, &pass->link);

			dbg("Added pass '%s'\n", name);
		}
	} else {
		fprintf(stderr, "Failed adding pass '%s'\n", name);
	}
}

static void
loaded_passes(struct wldbg *wldbg)
{
	struct pass *pass;

	printf("Loaded passes:\n");
	wl_list_for_each(pass, &wldbg->passes, link) {
		printf("\t - %s\n", pass->name);
	}
}

static void
remove_pass(struct wldbg *wldbg, const char *name)
{
	struct pass *pass, *tmp;

	dbg("Removing pass '%s'\n", name);

	wl_list_for_each_safe(pass, tmp, &wldbg->passes, link) {
		if (strcmp(pass->name, name) == 0) {
			wl_list_remove(&pass->link);

			free(pass->name);
			free(pass);

			dbg("Removed pass '%s'\n", name);
			return;
		}
	}

	fprintf(stderr, "Didn't find pass '%s'\n", name);
}

int
cmd_pass(struct wldbg_interactive *wldbgi,
	 struct wldbg_message *message,
	 char *buf)
{
	(void) message;
	(void) buf;

	if (strcmp(buf, "list") == 0) {
		list_passes(1);
	} else if (strcmp(buf, "loaded") == 0) {
		loaded_passes(wldbgi->wldbg);
	} else if (strncmp(buf, "add ", 4) == 0) {
		add_pass(wldbgi->wldbg, buf + 4);
	} else if (strncmp(buf, "remove ", 7) == 0) {
		remove_pass(wldbgi->wldbg, buf + 7);
	} else
		cmd_pass_help(0);

	return CMD_CONTINUE_QUERY;
}

