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

#ifndef _WLDBG_INTERACTIVE_H_
#define _WLDBG_INTERACTIVE_H_

#include <stdint.h>
#include <regex.h>

#include "wldbg.h"
#include "wayland/wayland-util.h"

struct wldbg_interactive {
	struct wldbg *wldbg;

	struct {
		uint64_t client_msg_no;
		uint64_t server_msg_no;
	} statistics;

	/* when we run client from interactive mode,
	 * we need to store it's credential here, so that
	 * we can free the allocated memory */
	struct {
		char *path;
		/* XXX
		 * add arguments */
	} client;

	int sigint_fd;

	/* query user on the next message */
	int stop;
	int skip_first_query;

	/* commands history */
	char *last_command;

	/* breakpoints */
	struct wl_list breakpoints;

	/* filters for printing messages */
	struct wl_list filters;
};

struct command {
	/* long form of a command i. e. 'continue' */
	const char *name;
	/* short form of a command i. e. 'c' (continue) */
	const char *shortcut;
	/* function to be called for the command. The last argument
	 * is the rest of user's input */
	int (*func)(struct wldbg_interactive *, struct wldbg_message *, char *);
	/* this function prints help for the command.
	 * If argument is not zero, then print short, oneline description */
	void (*help)(int oneline);
};

/* exit state of command */
enum {
	CMD_DONT_MATCH,
	CMD_END_QUERY,
	CMD_CONTINUE_QUERY,
};

/* array of all commands */
extern const struct command commands[];

int
run_command(char *buf,
		struct wldbg_interactive *wldbgi, struct wldbg_message *message);


struct breakpoint {
	unsigned int id;
	struct wl_list link;
	char *description;

	/* this function returns true if wldbg should stop
	 * on given message */
	int (*applies)(struct wldbg_message *, struct breakpoint *);
	void *data;
	uint64_t small_data;
	/* function to destroy data */
	void (*data_destr)(void *);
};


/* XXX we could use breakpoints to
 * implement this */
struct filter {
	char *filter;
	regex_t regex;
	struct wl_list link;
	int show_only;
    unsigned int id;
};

#endif /* _WLDBG_INTERACTIVE_H_ */
