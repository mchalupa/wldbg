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

/* private functions used across the wldbg */

#ifndef _WLDBG_PRIVATE_H_
#define _WLDBG_PRIVATE_H_

#include "config.h"

#include <unistd.h>
#include <sys/signalfd.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>

#include "wldbg.h"
#include "wayland/wayland-util.h"
#include "wldbg-pass.h"
#include "wldbg-ids-map.h"

#ifdef DEBUG

extern int debug;
extern int debug_verbose;
extern const char *debug_domain;

#define vdbg(...)                                   \
    do {                                            \
        if (!debug_verbose) break;                  \
        if (debug_domain                            \
            && strcmp(debug_domain, __FILE__) != 0) \
            break;                                  \
        fprintf(stderr, "[%d | %s: %d] ", getpid(), \
                __FILE__, __LINE__);                \
        fprintf(stderr,    __VA_ARGS__);            \
    } while (0)



#define dbg(...)                                    \
    do {                                            \
        if (!debug) break;                          \
        if (debug_domain                            \
            && strcmp(debug_domain, __FILE__) != 0) \
            break;                                  \
        fprintf(stderr, "[%d | %s: %d] ", getpid(), \
                __FILE__, __LINE__);                \
        fprintf(stderr,    __VA_ARGS__);            \
    } while (0)

#define ifdbg(cond, ...)                            \
    do {                                            \
        if (!debug) break;                          \
        if (cond)                                   \
            dbg(__VA_ARGS__);                       \
    } while (0)

#else /* DEBUG */

#define dbg(...)
#define vdbg(...)
#define ifdbg(cond, ...)

#endif /* DEBUG */

struct wldbg_connection;
struct resolved_objects;

struct wldbg {
	int epoll_fd;
	int signals_fd;

	struct wldbg_message message;
	char *buffer;

	sigset_t handled_signals;
	struct wl_list passes;
	struct wl_list monitored_fds;

	unsigned int resolving_objects : 1;

	struct {
        /* pass whole buffer to passes instead of just messages */
		unsigned int pass_whole_buffer : 1;
        /* wldbg is running in main loop */
		unsigned int running           : 1;
        /* some pass raised error */
		unsigned int error             : 1;
        /* some pass asked to exit */
		unsigned int exit              : 1;
        /* running in server mode */
		unsigned int server_mode       : 1;
	} flags;

	struct {
		int fd;
		struct sockaddr_un addr;
		char *old_socket_path;
		char *wldbg_socket_path;
		char *old_socket_name;
		char *wldbg_socket_name;

		char *lock_addr;
		int fd_lock;

		const char *connect_to;
	} server_mode;

	/* this will be list later */
	struct wl_list connections;
	int connections_num;
};

struct pass {
	struct wldbg_pass wldbg_pass;
	struct wl_list link;
	char *name;
};

struct wldbg_connection {
	struct wldbg *wldbg;

	struct {
		int fd;
		/* TODO get rid of connection??? */
		struct wl_connection *connection;
		pid_t pid;
	} server;

	struct {
		int fd;
		struct wl_connection *connection;

		char *program;
		/* path to the binary */
		char *path;
		/* pointer to arguments and number of arguments */
		int argc;
		char **argv;

		pid_t pid;
	} client;

	struct resolved_objects *resolved_objects;
	struct wl_list link;
};

struct wldbg_fd_callback {
	int fd;
	void *data;
	int (*dispatch)(int fd, void *data);
	struct wl_list link;
};

struct resolved_objects_ids {
	/* id's allocated by client */
	struct wldbg_ids_map client_objects;
	/* id's allocated by server */
	struct wldbg_ids_map server_objects;
};

struct resolved_objects {
	struct resolved_objects_ids objects;

	/* these are shared between connections */
	struct wl_list *interfaces;

	/* these are specific for connection */
	struct wl_list additional_interfaces;
};

#endif /* _WLDBG_PRIVATE_H_ */
