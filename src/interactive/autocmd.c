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
#include <ctype.h>
#include <assert.h>

#include "wayland/wayland-private.h"

#include "wldbg-pass.h"
#include "interactive.h"
#include "passes.h"
#include "wldbg-private.h"
#include "interactive-commands.h"
#include "util.h"

void
cmd_autocmd_help(int oneline)
{
	if (oneline) {
		printf("Add, remove commands that are run automatically");
		return;
	}

	printf("Possible arguments:\n");
	printf("\tadd\t\t- add new autocmd\n");
	printf("\t  syntax: add FILTER COMMAND\n");
	printf("\t    where REGEXP is an regular expression that is matched against a message\n");
	printf("\tremove ID\t- remove autocmd identified by ID\n");
}

static struct autocmd *
create_autocmd(const char *pattern, const char *cmd)
{
	static unsigned int autocmd_id;
	struct autocmd *ac;

	ac = calloc(1, sizeof *ac);
	if (!ac) {
		fprintf(stderr, "No memory\n");
		return NULL;
	}

	ac->filter = strdup(pattern);
	if (!ac->filter) {
		fprintf(stderr, "No memory\n");
		goto err;
	}

	ac->cmd = strdup(cmd);
	if (!ac->cmd) {
		fprintf(stderr, "No memory\n");
		goto err;
	}

	if (regcomp(&ac->regex, pattern, REG_EXTENDED) != 0) {
		fprintf(stderr, "Failed compiling regular expression\n");
		goto err;
	}

	ac->id = autocmd_id++;

	return ac;

err:
	free(ac->filter);
	free(ac->cmd);
	free(ac);
	return NULL;
}

char *
split_buffer(char *buf, char terminator)
{
	assert(buf);
	if (*buf == 0)
		return NULL;

	while (*buf != 0) {
		/* if the command has given start and end,
		 * respect that */
		if (terminator) {
			/* it is safe to access buf - 1 because in the buffer
			 * there is at least "" in this case */
			if (*buf == terminator && *(buf - 1) != '\\')
				break;
		} else { /* otherwise split on space */
			if (isspace(*buf))
				break;
		}

		++buf;
	}

	*buf = 0;
	return skip_ws(buf+1);
}

void
add_autocmd(struct wldbg_interactive *wldbgi, char *buf)
{
	struct autocmd *ac;
	char *cmd;

	char terminator = 0;
	/* XXX some other terminators? */
	/* this is the case when the regexp is covered
	 * in "" or '' due to whitespaces */
	if (*buf == '\"' || *buf == '\'') {
		terminator = *buf;
		++buf; /* skip the terminator */
	}

	cmd = split_buffer(buf, terminator);
	if (!cmd) {
		printf("Failed parsing command in: '%s'\n", buf);
		return;
	}

	ac = create_autocmd(buf, cmd);
	if (!ac)
		return;

	wl_list_insert(wldbgi->autocmds.next, &ac->link);
	printf("Added autocmd '%s' on '%s'\n", cmd, buf);
}

/* copied from breakpoints.c */
int
message_match_autocmd(struct wldbg_message *msg, struct autocmd *ac)
{
	char buf[128];
	int ret;

	ret = wldbg_get_message_name(msg, buf, sizeof buf);
	if (ret >= (int) sizeof buf) {
		fprintf(stderr, "BUG: buffer too small for message name\n");
		return 0;
	}

	/* run regular expression */
	ret = regexec(&ac->regex, buf, 0, NULL, 0);

	/* got we match? */
	if (ret == 0) {
		return 1;
	} else if (ret != REG_NOMATCH)
		fprintf(stderr, "Executing regexp failed!\n");

	return 0;
}

/* FIXME - don't duplicate code with filters */
static void
remove_autocmd(struct wldbg_interactive *wldbgi, char *buf)
{
	unsigned int id;
	int found = 0;
	struct autocmd *ac, *tmp;

	if (!*buf) {
		cmd_filter_help(0);
		return;
	}

	if (sscanf(buf, "%u", &id) != 1) {
		printf("Failed parsing autocmds's id '%s'\n", buf);
		return;
	}

	wl_list_for_each_safe(ac, tmp, &wldbgi->autocmds, link) {
		if (ac->id == id) {
			found = 1;
			wl_list_remove(&ac->link);

			regfree(&ac->regex);
			free(ac->cmd);
			free(ac->filter);
			free(ac);
			break;
		}
	}

	if (!found)
		printf("Didn't find autocmd with id '%u'\n", id);
}

/* FIXME: implement it using breakpoints - add new private (?) API
 * wldbg_run_on_message() and implement breakpoints, filters and autocommands
 * on this (breakpoints will run function that will set wldbgi->stop and similar)
 */
int
cmd_autocmd(struct wldbg_interactive *wldbgi,
	    struct wldbg_message *message,
	    char *buf)
{
	(void) message;

	if (strncmp(buf, "add ", 4) == 0) {
		add_autocmd(wldbgi, skip_ws(buf + 4));
	} else if (strncmp(buf, "remove ", 7) == 0) {
		remove_autocmd(wldbgi, skip_ws(buf + 7));
	} else
		cmd_pass_help(0);

	return CMD_CONTINUE_QUERY;
}

