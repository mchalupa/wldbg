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

#ifndef _WLDBG_PRINT_H_
#define _WLDBG_PRINT_H_

#include <wayland/wayland-util.h> /* for wl_list */
#include <regex.h>

struct wldbg_interactive;

/* force = print no matter what filters we have */
void
wldbgi_print_message(struct wldbg_interactive *wldbgi, struct message *message,
                     int force);

void
print_bare_message(struct message *message, struct wl_list *filters);

size_t
wldbg_get_message_name(struct message *message, char *buff, size_t maxsize);

struct print_filter {
	char *filter;
	regex_t regex;
	struct wl_list link;
	int show_only;
};

#endif /* _WLDBG_PRINT_H_ */
