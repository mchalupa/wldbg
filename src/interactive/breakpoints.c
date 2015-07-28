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
#include <assert.h>
#include <ctype.h>
#include <regex.h>

#include "wldbg.h"
#include "wldbg-pass.h"
#include "interactive.h"
#include "print.h"
#include "wldbg-private.h"
#include "resolve.h"
#include "util.h"

static unsigned int breakpoint_next_id = 1;

struct breakpoint_re_data
{
	regex_t re;
	char *pattern;
};

void
free_breakpoint(struct breakpoint *b)
{
	assert(b);

	if (b->data_destr)
		b->data_destr(b->data);

	free(b->description);
	free(b);
}

static int
break_on_side(struct message *msg, struct breakpoint *b)
{
	if (msg->from == b->small_data)
		return 1;

	return 0;
}

static int
break_on_id(struct message *msg, struct breakpoint *b)
{
	uint32_t *p = msg->data;

	if (*p == b->small_data)
		return 1;

	return 0;
}

static int
break_on_name(struct message *msg, struct breakpoint *b)
{
	uint32_t *p = msg->data;
	uint32_t id, opcode, bopcode;
	const struct wl_interface *intf;
	const struct wl_message *bmessage, *wl_message = NULL;
	struct resolved_objects *ro = msg->connection->resolved_objects;

	id = p[0];
	opcode = p[1] & 0xffff;
	bopcode = b->small_data;
	bmessage = b->data;

	/* we know that if the opcodes differ, we can
	 * break without any interface checking */
	if (bopcode != opcode)
		return 0;

	intf = resolved_objects_get(ro, id);
	if (!intf)
		return 0;

	if (msg->from == CLIENT) {
		if ((int) opcode < intf->method_count)
			wl_message = &intf->methods[opcode];
	} else {
		if ((int) opcode < intf->event_count)
			wl_message = &intf->events[opcode];
	}

	if (wl_message && bmessage == wl_message)
		return 1;

	return 0;
}

static int
break_on_regex(struct message *msg, struct breakpoint *b)
{
	char buf[128];
	int ret;
	regex_t *re = b->data;

	ret = wldbg_get_message_name(msg, buf, sizeof buf);
	if (ret >= (int) sizeof buf) {
		fprintf(stderr, "BUG: buffer too small for message name\n");
		return 0;
	}

	/* run regular expression */
	ret = regexec(re, buf, 0, NULL, 0);

	/* got we match? */
	if (ret == 0) {
		return 1;
	} else if (ret != REG_NOMATCH)
		fprintf(stderr, "Executing regexp failed!\n");

	return 0;
}

static void
breakpoint_re_data_free(void *data)
{
	struct breakpoint_re_data *rd = data;
	regfree(&rd->re);
	free(rd->pattern);
	free(rd);
}

static struct breakpoint *
create_breakpoint(struct resolved_objects *ro, char *buf)
{
	int id, i, opcode;
	struct breakpoint *b;
	struct breakpoint_re_data *rd;
	const struct wl_interface *intf = NULL;
	char *at;

	b = calloc(1, sizeof *b);
	if (!b) {
		fprintf(stderr, "Out of memory\n");
		return NULL;
	}

	b->id = breakpoint_next_id;

	/* parse buf and find out what the breakpoint
	 * should be */
	if (strcmp(buf, "server\n") == 0) {
		b->applies = break_on_side;
		b->description = strdup("message from server");
		if (!b->description)
			goto err_mem;
		b->small_data = SERVER;
	} else if (strcmp(buf, "client\n") == 0) {
		b->applies = break_on_side;
		b->description = strdup("message from client");
		if (!b->description)
			goto err_mem;
		b->small_data = CLIENT;
	} else if (strncmp(buf, "id ", 3) == 0) {
		id = str_to_uint(buf + 3);
		if (id == -1) {
			printf("Wrong id\n");
			goto err;
		}

		b->applies = break_on_id;
		b->description = strdup("break on id XXX");
		if (!b->description)
			goto err_mem;
		snprintf(b->description + 12, 4, "%d", id);
		b->small_data = id;
	} else if (strncmp(buf, "re ", 3) == 0) {
		b->applies = break_on_regex;
		rd = malloc(sizeof *rd);
		if (!rd)
			goto err_mem;

		b->data = rd;
		b->data_destr = breakpoint_re_data_free;
		rd->pattern = strdup(remove_newline(skip_ws(buf + 3)));
		if (!rd->pattern)
			goto err_mem;

		if (regcomp(&rd->re, rd->pattern, REG_EXTENDED) != 0) {
			fprintf(stderr, "Failed compiling regular expression\n");
			goto err;
		}

		printf("regex: %s\n", rd->pattern);
		b->description = strdupf("regex matching '%s'", rd->pattern);
		if (!b->description)
			goto err_mem;
	} else {
		if ((at = strchr(buf, '@'))) {
			/* split the string on '@' */
			*at = 0;
			intf = resolved_objects_get_interface(ro, buf);
			if (!intf) {
				printf("Unknown interface\n");
				goto err;
			}

			/* find the request/event with name pointed
			 * by at + 1 */
			++at;
			at[strlen(at) - 1] = 0; /* delete newline */
			opcode = -1;

			/* XXX what if interface has method and event
			 * with the same name? Handle it... */
			for (i = 0; i < intf->method_count; ++i)
				if (strcmp(intf->methods[i].name, at) == 0) {
					b->data = (struct wl_message *) &intf->methods[i];
					opcode = i;
				}

			if (opcode == -1)
				for (i = 0; i < intf->event_count; ++i)
					if (strcmp(intf->events[i].name, at) == 0) {
						b->data = (struct wl_message *) &intf->events[i];
						opcode = i;
					}

			if (opcode == -1) {
				printf("Couldn't find method/event name\n");
				goto err;
			}

			b->small_data = opcode;
			b->applies = break_on_name;
			b->description = malloc(1024);
			if (!b->description) {
				printf("No memory\n");
				goto err;
			}

			snprintf(b->description, 1024, "break on %s@%s",
				 intf->name, ((struct wl_message *) b->data)->name);
		} else {
			printf("Wrong syntax\n");
			goto err;
		}
	}

	++breakpoint_next_id;
	dbg("Created breakpoint %u\n", b->id);

	return b;

err_mem:
	fprintf(stderr, "Out of memory\n");
err:
	free_breakpoint(b);
	return NULL;
}

static void
delete_breakpoint(char *buf, struct wldbg_interactive *wldbgi)
{
	int id;
	struct breakpoint *b, *tmp;

	id = str_to_uint(buf);
	if (id == -1) {
		printf("Need valid id\n");
		return;
	}

	wl_list_for_each_safe(b, tmp, &wldbgi->breakpoints, link) {
		if (b->id == (unsigned) id) {
			wl_list_remove(&b->link);
			free_breakpoint(b);

			return;
		}
	}

	printf("Haven't found breakpoint with id %d\n", id);
	return;
}

int
cmd_break(struct wldbg_interactive *wldbgi,
	  struct message *message,
	  char *buf)
{
	struct breakpoint *b;
	struct resolved_objects *ro = message->connection->resolved_objects;

	(void) message;

	if (strncmp(buf, "delete ", 7) == 0) {
		delete_breakpoint(buf + 7, wldbgi);
		return CMD_CONTINUE_QUERY;
	} else if (strncmp(buf, "d ", 2) == 0) {
		delete_breakpoint(buf + 2, wldbgi);
		return CMD_CONTINUE_QUERY;
	}

	b = create_breakpoint(ro, buf);
	if (b) {
		wl_list_insert(wldbgi->breakpoints.next, &b->link);
		printf("created breakpoint %u\n", b->id);
	}

	return CMD_CONTINUE_QUERY;
}

void
cmd_break_help(int oneline)
{
	if (oneline) {
		printf("Set or delete breakpoints");
		return;
	}

	printf("Set or delete breakpoints\n"
	       "\n"
	       "\tbreakpoint id ID                - break on id ID\n"
	       "\tbreakpoint side server|client   - break on message from server/client\n"
	       "\tbreakpoint re REGEXP            - break on given REGEXP\n"
	       "\tbreakpoint interface@message    - break on known interface@message\n"
	       "\tbreakpoint delete ID            - delete breakpoint id\n"
	       "\tbreakpoint d ID                 - delete breakpoint id\n"
	       "\n"
	       "Example: b re wl_surface.*\n");
}
