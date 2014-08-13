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

#ifndef _WLDBG_PASS_H_
#define _WLDBG_PASS_H_

struct message;
struct wldbg;

struct wldbg_pass {
	int (*init)(struct wldbg *wldbg, struct wldbg_pass *pass,
			int argc, const char *argv[]);

	void (*destroy)(void *user_data);

	/* function that will be run for messages from server */
	int (*server_pass)(void *user_data, struct message *message);

	/* function that will be run for messages from client */
	int (*client_pass)(void *user_data, struct message *message);

	/* print help for the pass */
	void (*help)(void *user_data);

	void *user_data;
};

enum {
	PASS_NEXT, /* process next pass */
	PASS_STOP  /* stop processing passes */
};

#endif /*  _WLDBG_PASS_H_ */
