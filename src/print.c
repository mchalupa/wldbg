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
#include <ctype.h>

#include "wldbg.h"
#include "wayland/wayland-util.h"
#include "util.h"
#include "print.h"

/* roughly based on wl_closure_print from connection.c */
void
print_bare_message(struct wldbg *wldbg, struct message *message)
{
	int i;
	uint32_t j, id, opcode, pos, len, size, *p;
	struct wl_interface *interface, *obj;
	const char *signature;
	const struct wl_message *wl_message = NULL;

	assert(wldbg->flags.one_by_one
		&& "This function is meant to be used in one-by-one mode");

	p = message->data;
	id = p[0];
	opcode = p[1] & 0xffff;
	size = p[1] >> 16;

	interface = wldbg_ids_map_get(&wldbg->resolved_objects, id);

	/* if we do not know interface or the interface is
	 * unknown_interface or free_entry */
	if (!interface
		|| (message->from == SERVER && !interface->event_count)
		|| (message->from == CLIENT && !interface->method_count)) {
		/* print at least fall-back description */
		printf("unknown@%u.[opcode %u][size %uB]", id, opcode, size);
		return;
	}

	if (message->from == SERVER)
		wl_message = &interface->events[opcode];
	else
		wl_message = &interface->methods[opcode];

	printf("%s@%u.%s(",
		interface->name, id,
		wl_message->name);

	/* 2 is position of the first argument */
	pos = 2;
	signature = wl_message->signature;
	for (i = 0; signature[i] != '\0'; ++i) {
		if (signature[i] == '?' || isdigit(signature[i]))
			continue;

		if (pos > 2)
			printf(", ");

		switch (signature[i]) {
		case 'u':
			printf("%u", p[pos]);
			break;
		case 'i':
			printf("%d", p[pos]);
			break;
		case 'f':
			printf("%f", wl_fixed_to_double(p[pos]));
			break;
		case 's':
			printf("%u:\"%s\"", p[pos], (const char *) (p + pos + 1));
			pos += DIV_ROUNDUP(p[pos], sizeof(uint32_t));
			break;
		case 'o':
			obj = wldbg_ids_map_get(&wldbg->resolved_objects,
						p[pos]);
			if (obj)
				printf("%s@%u",
					obj->name,
					p[pos]);
			else
				printf("nil");
			break;
		case 'n':
			printf("new id %s@",
				(wl_message->types[i]) ?
				 wl_message->types[i]->name :
				  "[unknown]");
			if (p[pos] != 0)
				printf("%u", p[pos]);
			else
				printf("nil");
			break;
		case 'a':
			len = DIV_ROUNDUP(p[pos], sizeof(uint32_t));

			printf("array:");
			for (j = 0; j < 8 && j < len; ++j) {
				printf(" %04x", p[pos + j]);
			}

			if (len > j)
				printf(" ...");

			pos += len;
			break;
		case 'h':
			printf("fd");
			break;
		}

		++pos;
	}

	putchar(')');
}

void
print_message(struct wldbg *wldbg, struct message *message)
{
	printf("%c: ", message->from == SERVER ? 'S' : 'C');

	print_bare_message(wldbg, message);
	putchar('\n');
}
