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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "wayland/wayland-private.h"

#include "wldbg.h"
#include "wldbg-pass.h"
#include "interactive.h"
#include "util.h"
#include "print.h"
#include "wldbg-private.h"
#include "resolve.h"

static unsigned int breakpoint_next_id = 1;

int str_to_uint(char *str)
{
	char *num, *numtmp;

	/* skip ws to first digit or newline */
	numtmp = num = skip_ws_to_newline(str);

	if (*num == '\n')
		return -1;

	/* check that it is a number */
	while (!isspace(*numtmp)) {
		if (!isdigit(*numtmp))
			return -1;

		++numtmp;
	}

	return atoi(num);
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

static struct breakpoint *
create_breakpoint(struct resolved_objects *ro, char *buf)
{
	int id, i, opcode;
	struct breakpoint *b;
	const struct wl_interface *intf = NULL;
	char *at;

	b= calloc(1, sizeof *b);
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
		b->small_data = SERVER;
	} else if (strcmp(buf, "client\n") == 0) {
		b->applies = break_on_side;
		b->description = strdup("message from client");
		b->small_data = CLIENT;
	} else if (strncmp(buf, "id ", 3) == 0) {
		id = str_to_uint(buf + 3);
		if (id == -1) {
			printf("Wrong id\n");
			goto err;
		}

		b->applies = break_on_id;
		b->description = strdup("break on id XXX");
		snprintf(b->description + 12, 4, "%d", id);
		b->small_data = id;

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
err:
	free(b);
	return NULL;
}

void
free_breakpoint(struct breakpoint *b)
{
	assert(b);

	if (b->data_destr && b->data)
		b->data_destr(b->data);
	if (b->description)
		free(b->description);
	free(b);
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
