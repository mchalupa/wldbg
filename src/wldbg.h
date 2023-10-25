/*
 * Copyright (c) 2014 - 2015 Marek Chalupa
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

#ifndef _WLDBG_H_
#define _WLDBG_H_

#include "wldbg-pass.h"
#include "wldbg-objects-info.h"

struct wldbg;
struct wldbg_connection;

struct wldbg_message {
	/* raw data in message */
	void *data;

	/* size of the message in bytes */
	size_t size;

	/* whether it is a message from server or client */
	enum {
		SERVER,
		CLIENT
	} from;

	/* pointer to connection structure */
	struct wldbg_connection *connection;
};

const struct wl_interface *
wldbg_message_get_object(struct wldbg_message *msg, uint32_t id);

const struct wl_interface *
wldbg_message_get_interface(struct wldbg_message *msg, const char *name);

void
wldbg_message_objects_iterate(struct wldbg_message *message,
			      void (*func)(uint32_t id,
			                   const struct wl_interface *intf,
			                   void *data),
			      void *data);

/* mercifully exit wldbg from the pass
 * and let it clean after itself */
void
wldbg_exit(struct wldbg *wldbg);

/* set error flag, wldbg will clean up
 * and exit with error on next iteration */
void
wldbg_error(struct wldbg *wldbg);

/*
 * Set pass_whole_buffer flag in wldbg. If this flag is
 * set, wldbg won't call passes on single messages but
 * on whole buffer that it gots from server or client
 * if state is 0 or 1, then the function sets the flag
 * accordingly. If the state is -1, then the function
 * returns current setting. Other state values are invalid */
int
wldbg_separate_messages(struct wldbg *wldbg, int state);

struct wldbg_fd_callback;

struct wldbg_fd_callback *
wldbg_monitor_fd(struct wldbg *wldbg, int fd,
                int (*dispatch)(int fd, void *data),
                void *data);
int
wldbg_remove_callback(struct wldbg *wldbg, struct wldbg_fd_callback *cb);

#endif /* _WLDBG_H_ */
