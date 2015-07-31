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

#ifndef _WLDBG_PARSED_MESSAGE_H_
#define _WLDBG_PARSED_MESSAGE_H_

#include <stdint.h>

struct message;
struct wl_message;
struct wl_interface;
struct wldbg_connection;

struct wldbg_parsed_message {
	uint32_t id;
	uint32_t opcode;
	uint32_t size;
	/* data without header - that is raw_data + 2*sizeof(uint32_t) */
	uint32_t *data;
};

struct wldbg_resolved_arg {
	/* type of argument */
	char type;
	/* pointer to the data in message */
	uint32_t *ptr;
};

struct wldbg_resolved_message {
	struct wldbg_parsed_message base;
	const struct wl_interface *wl_interface;
	const struct wl_message *wl_message;

	/* position of arguments iterator */
	struct wldbg_resolved_arg cur_arg;
	const char *signature_position;
};

int wldbg_parse_message(struct message *msg, struct wldbg_parsed_message *out);
int wldbg_resolve_message(struct message *msg,
			  struct wldbg_resolved_message *out);
char *
wldbg_resolved_message_get_name(struct wldbg_resolved_message *msg,
				char *buff, size_t maxlen);

struct wldbg_resolved_arg *
wldbg_resolved_message_next_argument(struct wldbg_resolved_message *msg);

void
wldbg_resolved_message_reset_iterator(struct wldbg_resolved_message *msg);

size_t
wldbg_get_message_name(struct message *message, char *buff, size_t maxsize);

#endif /*  _WLDBG_PARSED_MESSAGE_H_ */
