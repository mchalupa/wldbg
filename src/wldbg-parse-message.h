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

#endif /*  _WLDBG_PARSED_MESSAGE_H_ */
