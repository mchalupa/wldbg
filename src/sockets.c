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

#define _GNU_SOURCE

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <sys/wait.h>

#include "wldbg.h"
#include "wldbg-pass.h"
#include "wldbg-private.h"
#include "resolve.h"
#include "sockets.h"

#include "wayland/wayland-private.h"
#include "wayland/wayland-util.h"
#include "wayland/wayland-os.h"

/* copied out from wayland-client.c */
/* renamed from connect_to_socket -> connect_to_wayland_socket */
static int
connect_to_wayland_socket(const char *name)
{
	struct sockaddr_un addr;
	socklen_t size;
	const char *runtime_dir;
	int name_size, fd;

	dbg("CONNECTING TO %s\n", name);

	runtime_dir = getenv("XDG_RUNTIME_DIR");
	if (!runtime_dir) {
		wl_log("error: XDG_RUNTIME_DIR not set in the environment.\n");
		/* to prevent programs reporting
		 * "failed to create display: Success" */
		errno = ENOENT;
		return -1;
	}

	if (name == NULL)
		name = getenv("WAYLAND_DISPLAY");
	if (name == NULL)
		name = "wayland-0";

	fd = wl_os_socket_cloexec(PF_LOCAL, SOCK_STREAM, 0);
	if (fd < 0)
		return -1;

	memset(&addr, 0, sizeof addr);
	addr.sun_family = AF_LOCAL;
	name_size =
		snprintf(addr.sun_path, sizeof addr.sun_path,
			 "%s/%s", runtime_dir, name) + 1;

	assert(name_size > 0);
	if (name_size > (int)sizeof addr.sun_path) {
		wl_log("error: socket path \"%s/%s\" plus null terminator"
		       " exceeds 108 bytes\n", runtime_dir, name);
		close(fd);
		/* to prevent programs reporting
		 * "failed to add socket: Success" */
		errno = ENAMETOOLONG;
		return -1;
	};

	size = offsetof(struct sockaddr_un, sun_path) + name_size;

	if (connect(fd, (struct sockaddr *) &addr, size) < 0) {
		close(fd);
		return -1;
	}

	return fd;
}

int
get_pid_for_socket(int fd)
{
	struct ucred cr;
	socklen_t len = sizeof cr;

	getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cr, &len);
	vdbg("Got fd's %d pid: %d\n", fd, cr.pid);

	return cr.pid;
}

int
connect_to_wayland_server(struct wldbg_connection *conn, const char *display)
{
	conn->server.fd = connect_to_wayland_socket(display);
	if (conn->server.fd == -1) {
		perror("Failed opening wayland socket");
		return -1;
	}

	conn->server.connection = wl_connection_create(conn->server.fd);
	if (!conn->server.connection) {
		perror("Failed creating wl_connection");
		goto err;
	}

	/* XXX this is shared between clients and can be in
	 * wldbg struct */
	conn->server.pid = get_pid_for_socket(conn->server.fd);

	return conn->server.fd;

err:
	close(conn->server.fd);
	return -1;
}

static char *
get_socket_path(const char *display)
{
	size_t size = sizeof(struct sockaddr_un);
	const char *xdg_env;
	int ret;

	xdg_env = getenv("XDG_RUNTIME_DIR");
	if (!xdg_env)
		xdg_env = "/tmp";

	char *path = malloc(size);
	if (!path) {
		fprintf(stderr, "Out of memory\n");
		return NULL;
	}

	ret = snprintf(path, size, "%s/%s", xdg_env, display);
	if((size_t) ret >= size) {
		fprintf(stderr, "Cannot construct path to new socket\n");
		free(path);
		return NULL;
	}

	return path;
}

int
server_mode_change_sockets(struct wldbg *wldbg)
{
	int fd;
	char *orig_socket = get_socket_path("wayland-0");
	char *new_socket = get_socket_path(WLDBG_SERVER_MODE_SOCKET_NAME);

	if (!orig_socket || ! new_socket)
		goto err;

	/* rename the socket */
	if (rename(orig_socket, new_socket) < 0) {
		perror("renaming wayland socket");
		goto err;
	}

	/* this will make new clients connect to new_socket
	 * instead of orig_socket */
	wldbg->server_mode.old_socket_name = orig_socket;
	wldbg->server_mode.wldbg_socket_name = new_socket;

	/* create new fake socket */
	fd = server_mode_add_socket(wldbg, orig_socket);
	if (fd == -1)
		goto err;

	wldbg->server_mode.fd = fd;

	return fd;

err:
	free(orig_socket);
	free(new_socket);
	return -1;
}

int
server_mode_change_sockets_back(struct wldbg *wldbg)
{
	/* remove wldbg socket - it is now named as the
	 * old socket.. XXX The naming is confusing.. */
	if (unlink(wldbg->server_mode.old_socket_name) < 0) {
		perror("deleting named socket");
		/* try continue, we'll probably fail right
		 * the next step */
	}

	/* rename the socket */
	if (rename(wldbg->server_mode.wldbg_socket_name,
		   wldbg->server_mode.old_socket_name) < 0) {
		perror("renaming wayland socket");
		return -1;
	}

	return 0;
}

int
server_mode_add_socket(struct wldbg *wldbg, const char *name)
{
	socklen_t size;
	struct sockaddr_un *addr = &wldbg->server_mode.addr;
	int sock;

	assert(wldbg->flags.server_mode);
	memset(addr, 0, sizeof *addr);

	sock = socket(PF_LOCAL, SOCK_STREAM, 0);
	if (sock < 0) {
		return -1;
	}

	addr->sun_family = AF_UNIX;
	strcpy(addr->sun_path, name);

	size = SUN_LEN(addr);
	if (bind(sock, (struct sockaddr *) addr, size) < 0) {
		perror("bind() failed");
		goto err;
	}

	if (listen(sock, 128) < 0) {
		perror("listen() failed:");
		goto err;
	}

	dbg("Server mode: listening on fd %d\n", sock);

	return sock;
err:
	close(sock);
	return -1;
}
