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

#include <stdio.h>

#include "wayland/wayland-private.h"

#include "interactive.h"
#include "interactive-commands.h"
#include "wldbg-private.h"
#include "util.h"

static void
print_object(uint32_t id, const struct wl_interface *intf, void *data)
{
	(void) data;
	if (id >= WL_SERVER_ID_START)
		printf("\tSRV %u -> %s\n",
		       id - WL_SERVER_ID_START, intf ? intf->name : "NULL");
	else
		printf("\t%u -> %s\n", id, intf ? intf->name : "NULL");
}

static void
print_objects(struct wldbg_message *message)
{
	wldbg_message_objects_iterate(message, print_object, NULL);
}

static void
print_breakpoints(struct wldbg_interactive *wldbgi)
{
	struct breakpoint *b;

	if (wl_list_empty(&wldbgi->breakpoints)) {
		printf("No breakpoints\n");
		return;
	}

	wl_list_for_each(b, &wldbgi->breakpoints, link) {
		printf("%u: break on %s\n", b->id, b->description);
	}
}

static void
print_autocmds(struct wldbg_interactive *wldbgi)
{
	struct autocmd *ac;

	if (wl_list_empty(&wldbgi->autocmds)) {
		printf("No autocommands\n");
		return;
	}

	wl_list_for_each(ac, &wldbgi->autocmds, link) {
		printf("%u: run '%s' on '%s'\n", ac->id, ac->cmd, ac->filter);
	}
}

static void
print_filters(struct wldbg_interactive *wldbgi)
{
	struct filter *pf;

	if (wl_list_empty(&wldbgi->filters)) {
		printf("No filters\n");
		return;
	}

	wl_list_for_each(pf, &wldbgi->filters, link) {
		printf("%u: %s %s\n", pf->id,
		       pf->show_only ? "show" : "hide",
		       pf->filter);
	}
}

static void
info_wldbg(struct wldbg_interactive *wldbgi)
{
	struct wldbg *wldbg = wldbgi->wldbg;

	printf("\n-- Wldbg -- \n");

	printf("Monitored fds num: %d\n", wl_list_length(&wldbg->monitored_fds));
	printf("Resolving objects: %d\n", wldbg->resolving_objects);
	printf("Gathering objinfo: %d\n", wldbg->gathering_info);
	printf("Flags:"
	       "\tpass_whole_buffer : %u\n"
	       "\trunning           : %u\n"
	       "\terror             : %u\n"
	       "\texit              : %u\n"
	       "\tserver_mode       : %u\n",
	       wldbg->flags.pass_whole_buffer,
	       wldbg->flags.running,
	       wldbg->flags.error,
	       wldbg->flags.exit,
	       wldbg->flags.server_mode);

	if (!wldbg->flags.server_mode)
		return;

	printf("Server mode:\n"
		"\told socket name: \'%s\'\n"
		"\told socket path: \'%s\'\n"
		"\twldbg socket path: \'%s\'\n"
		"\twldbg socket path: \'%s\'\n"
		"\tlock address: \'%s\'\n"
		"\tconnect to: \'%s\'\n",
		wldbg->server_mode.old_socket_path,
		wldbg->server_mode.wldbg_socket_path,
		wldbg->server_mode.old_socket_name,
		wldbg->server_mode.wldbg_socket_name,
		wldbg->server_mode.lock_addr,
		wldbg->server_mode.connect_to);

	printf("Connections number: %d\n", wldbg->connections_num);
}

static void
info_connections(struct wldbg_interactive *wldbgi)
{
	struct wldbg *wldbg = wldbgi->wldbg;
	struct wldbg_connection *conn;
	int i;
	int n = 0;

	printf("\n-- Connections -- \n");
	wl_list_for_each(conn, &wldbg->connections, link) {
		++n;

		printf("%d.\n", n);
		printf("\tserver: pid=%d\n", conn->server.pid);
		printf("\tclient: pid=%d\n", conn->client.pid);
		printf("\t      : program=\'%s\'\n", conn->client.program);
		printf("\t      : path=\'%s\'\n", conn->client.path);
		printf("\t      : argc=\'%d\'\n", conn->client.argc);
		for (i = 0; i < conn->client.argc; ++i)
			printf("\t      :   argv[%d]=\'%s\'\n",
			       i, conn->client.argv[i]);

	}
}

void
cmd_info_help(int oneline)
{
	if (oneline) {
		printf("Show info about entities");
		return;
	}

	printf("info WHAT (i WHAT)\n"
	       "\n"
	       "objects (o)\n"
	       "objects (o) ID\n"
	       "message (m)\n"
	       "breakpoints (b)\n"
	       "filters (f)\n"
	       "process (proc, p)\n"
	       "connection (conn, c)\n");
}

void
print_object_info(struct wldbg_message *msg, char *buf);

static void
print_objects_info(struct wldbg_message *message, char *buf)
{
	char *id = skip_ws(buf);
	if (*id)
		print_object_info(message, id);
	else
		print_objects(message);
}

int
cmd_info(struct wldbg_interactive *wldbgi,
	 struct wldbg_message *message, char *buf)
{
#define MATCH(buf, str) (strncmp((buf), (str), (sizeof((str)) + 1)) == 0)

	if (MATCH(buf, "m") || MATCH(buf, "message")) {
		printf("Sender: %s (no. %lu), size: %lu\n",
			message->from == SERVER ? "server" : "client",
			message->from == SERVER ? wldbgi->statistics.server_msg_no
						: wldbgi->statistics.client_msg_no,
			message->size);
	} else if (strncmp(buf, "objects", 7) == 0) {
		print_objects_info(message, buf + 7);
	} else if (strncmp(buf, "object", 6) == 0) {
		print_objects_info(message, buf + 6);
	} else if (strncmp(buf, "o", 1) == 0) {
		print_objects_info(message, buf + 1);
	} else if (MATCH(buf, "b") || MATCH(buf, "breakpoints")) {
		print_breakpoints(wldbgi);
	} else if (MATCH(buf, "f") || MATCH(buf, "filters")) {
		print_filters(wldbgi);
	} else if (MATCH(buf, "ac") || MATCH(buf, "autocmd")
		   || MATCH(buf, "autocommands")) {
		print_autocmds(wldbgi);
	} else if (MATCH(buf, "p") || MATCH(buf, "proc")
		   || MATCH(buf, "process")) {
		info_wldbg(wldbgi);
		info_connections(wldbgi);
	} else if (MATCH(buf, "c") || MATCH(buf, "conn")
		   || MATCH(buf, "connection")) {
		info_connections(wldbgi);
	} else {
		printf("Unknown arguments\n");
		cmd_info_help(0);
	}

	return CMD_CONTINUE_QUERY;

#undef MATCH
}
