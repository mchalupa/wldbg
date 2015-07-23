/*
 * Copyright (c) 2014 Marek Chalupa
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

#ifndef _WLDBG_PASS_H_
#define _WLDBG_PASS_H_

#include <stdint.h>

struct message;
struct wldbg;

/* flags for passes */
enum {
	/* suppress multiple loads of this pass */
	WLDBG_PASS_LOAD_ONCE	= 1,
};

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
	const char *description;

	/* flags for the pass, i. e. WLDBG_PASS_LOAD_ONCE, etc */
	uint64_t flags;
};

enum {
	PASS_NEXT, /* process next pass */
	PASS_STOP  /* stop processing passes */
};

#endif /*  _WLDBG_PASS_H_ */
