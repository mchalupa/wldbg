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
#include <assert.h>
#include <ctype.h>

#include "wldbg-private.h"
#include "resolve.h"

#include "wldbg-parsed-message.h"

int
wldbg_parse_message(struct message *msg, struct wldbg_parsed_message *out)
{
	uint32_t *p = msg->data;

	out->id = p[0];
	out->opcode = p[1] & 0xffff;
	out->size = p[1] >> 16;
	out->data = p + 2;

	if (out->size < 8 || out->size % 4 != 0)
		return 0;

	return 1;
}

int wldbg_resolve_message(struct message *msg,
			  struct wldbg_resolved_message *out)
{
	const struct wl_interface *interface;

	/* clear message */
	memset(msg, 0, sizeof *msg);

	if (!wldbg_parse_message(msg, &out->base))
		return 0;

	interface = wldbg_ids_map_get(
		resolved_objects_get_objects(msg->connection->resolved_objects),
					     out->base.id);
	/* if it is unknown interface to resolve or it is
	 * "unknown" interface of FREE entry, return NULL */
	if (!interface
		|| (msg->from == SERVER && !interface->event_count)
		|| (msg->from == CLIENT && !interface->method_count))
		return 0;

	out->wl_interface = interface;

	if (msg->from == SERVER) {
		if ((uint32_t) interface->event_count <= out->base.opcode)
			return 0;

		out->wl_message = &interface->events[out->base.opcode];
	} else {
		if ((uint32_t) interface->method_count <= out->base.opcode)
			return 0;

		out->wl_message = &interface->methods[out->base.opcode];
	}

	if (!out->wl_message || !out->wl_message->signature)
		return 0;

	return 1;
}

static const char *
signature_get_type(const char *sig)
{
	assert(sig);

	/* skip versions and nullable flags */
	while(*sig && (*sig == '?' || isdigit(*sig)))
		++sig;

	return sig;
}

static void
initialize_iterator(struct wldbg_resolved_message *msg)
{
	const char *sig = signature_get_type(msg->wl_message->signature);

	msg->signature_position = sig;
	msg->cur_arg.type = *sig;
	msg->cur_arg.ptr = msg->base.data;
}

void
wldbg_resolved_message_reset_iterator(struct wldbg_resolved_message *msg)
{
	msg->signature_position = NULL;
	memset(&msg->cur_arg, 0,
	       sizeof(struct wldbg_resolved_arg));
}

struct wldbg_resolved_arg *
wldbg_resolved_message_next_argument(struct wldbg_resolved_message *msg)
{
	size_t off = 0;
	char type;
	uint32_t *p;

	/* first iteration */
	if (msg->signature_position == NULL) {
		initialize_iterator(msg);
		return &msg->cur_arg;
	}

	type = *msg->signature_position;
	p = msg->cur_arg.ptr;

	msg->signature_position
		= signature_get_type(msg->signature_position + 1);
	msg->cur_arg.type = *msg->signature_position;

	if (msg->cur_arg.type == 0)
		return NULL;

	switch (type) {
	case 'u':
	case 'i':
	case 'f':
	case 'o':
	case 'n':
	case 'h':
		off = 1;
		break;
	case 's':
	case 'a':
		off = DIV_ROUNDUP(*p, sizeof(uint32_t)) + 1;
		break;
	default:
		fprintf(stderr, "Warning: unhandled type: %c\n", type);
	}

	msg->cur_arg.ptr += off;

	return &msg->cur_arg;
}

char *
wldbg_resolved_message_get_name(struct wldbg_resolved_message *msg,
				char *buff, size_t maxlen)
{
	snprintf(buff, maxlen, "%s@%s",
		 msg->wl_interface->name,
		 msg->wl_message->name);

	return buff;
}
