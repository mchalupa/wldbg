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
#include <assert.h>
#include <ctype.h>

#include "resolve.h"
#include "util.h"

#include "wldbg-private.h"
#include "wldbg-parse-message.h"

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

	/* clear out */
	memset(out, 0, sizeof *out);
	if (!wldbg_parse_message(msg, &out->base))
		return 0;

	assert(msg->connection && "Message has no connection set");
	/* if we're not resolving objects, just
	 * cease parsing here */
	if (!msg->connection->resolved_objects)
		return 0;

	interface = wldbg_message_get_object(msg, out->base.id);
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
signature_get_type(const char *sig, unsigned int *nullable)
{
	assert(sig);

	*nullable = 0;
	while(*sig && (*sig == '?' || isdigit(*sig))) {
		if (*sig == '?')
			*nullable = 1;
		++sig;
	}

	return sig;
}

static void
initialize_iterator(struct wldbg_resolved_message *msg)
{
	const char *sig = signature_get_type(msg->wl_message->signature,
					     &msg->cur_arg.nullable);

	msg->signature_position = sig;
	msg->data_position = msg->base.data;

	msg->cur_arg.type = *sig;
	/* set pointer to data. If this is string or array that
	 * is NULL, set it to NULL, otherwise make it pointing
	 * to the data */
	if ((*sig == 'a' || *sig == 's')) {
		if (*msg->data_position == 0)
			msg->cur_arg.data = NULL;
		else
			/* skip size argument and point
			 * to data itself */
			msg->cur_arg.data = msg->base.data + 1;
	} else {
		msg->cur_arg.data = msg->base.data;
	}

	assert(!*sig || *sig == 'a' || *sig == 's' || msg->cur_arg.data);
}

void
wldbg_resolved_message_reset_iterator(struct wldbg_resolved_message *msg)
{
	msg->signature_position = NULL;
	msg->data_position = NULL;
	memset(&msg->cur_arg, 0,
	       sizeof(struct wldbg_resolved_arg));
}

struct wldbg_resolved_arg *
wldbg_resolved_message_next_argument(struct wldbg_resolved_message *msg)
{
	size_t size, off = 0;
	char type;

	/* first iteration */
	if (msg->signature_position == NULL) {
		initialize_iterator(msg);
		/* message has no arguments? */
		if (msg->cur_arg.type == 0)
			return NULL;

		return &msg->cur_arg;
	}

	/* need to store type before getting next argument,
	 * so that we know where to shift in data */
	type = *msg->signature_position;

	/* calling next_argument on iterator that reached
	 * the end */
	if (type == 0)
		return NULL;

	/* set new type and position in signature */
	msg->signature_position
		= signature_get_type(msg->signature_position + 1,
				     &msg->cur_arg.nullable);
	msg->cur_arg.type = *msg->signature_position;

	if (msg->cur_arg.type == 0)
		return NULL;

	/* find data of next argument */
	switch (type) {
	case 'u':
	case 'i':
	case 'f':
	case 'o':
	case 'n':
	case 'h':
		++msg->data_position;
		break;
	case 's':
	case 'a':
		/* msg->data_position now points to
		 * size argument of previous string/array */
		size = *msg->data_position;
		if (size == 0) {
			++msg->data_position;
		} else {
			off = DIV_ROUNDUP(size, sizeof(uint32_t));
			msg->data_position += (off + 1);
		}
		break;
	default:
		fprintf(stderr, "Warning: unhandled type: %d (%c)\n", type, type);
	}

	/* ok, we now point to the current argument.
	 * Check if it is string or array and set data */
	type = msg->cur_arg.type;
	if ((type == 'a' || type == 's')) {
		/* msg->data_position is the size of string/array */
		if (*msg->data_position == 0)
			msg->cur_arg.data = NULL;
		else
			/* skip size argument and point
			 * to data itself */
			msg->cur_arg.data = msg->data_position + 1;
	} else
		msg->cur_arg.data = msg->data_position;

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

size_t
wldbg_get_message_name(struct message *message, char *buf, size_t maxsize)
{
	struct wldbg_parsed_message pm;
	const struct wl_interface *interface;
	const struct wl_message *wl_message = NULL;
	int ret;
	size_t written;

	wldbg_parse_message(message, &pm);
	interface = wldbg_message_get_object(message, pm.id);

	if (interface) {
		if (message->from == SERVER)
			wl_message = &interface->events[pm.opcode];
		else
			wl_message = &interface->methods[pm.opcode];
	}

	/* put the interface name into the buffer */
	ret = snprintf(buf, maxsize, "%s",
		       interface ? interface->name : "unknown");
	written = ret;
	if (ret >= (int) maxsize)
		return written;

	/* create name of the message we got */
	if (wl_message) {
		ret = snprintf(buf + ret, maxsize - written,
			       "@%d.%s", pm.id, wl_message->name);

	} else {
		ret = snprintf(buf + ret, maxsize - written,
			       "@%d.%d", pm.id, pm.opcode);
	}

	written += ret;

	// return how many characters we wrote or how
	// many characters we'd wrote in the case of overflow
	return written;
}

