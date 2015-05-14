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
#include <sys/stat.h>
#include <sys/file.h>

#include "wldbg.h"
#include "wldbg-private.h"
#include "sockets.h"

#include "wayland/wayland-private.h"
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

char *
get_program_for_pid(pid_t pid)
{
	char path[255];
	char *comm;
	ssize_t len;
	int fd;

	if (snprintf(path, sizeof path, "/proc/%d/comm", pid)
		>= (ssize_t) sizeof path) {
		fprintf(stderr, "BUG: buffer too short for path\n");
		return NULL;
	}

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("opening pid's comm file");
		return NULL;
	}

	comm = malloc(255);
	if (!comm) {
		close(fd);
		return NULL;
	}

	len = read(fd, comm, 255);
	if (len < 0) {
		perror("reading pid's comm file");
		close(fd);
		free(comm);
		return NULL;
	}

	/* add terminating 0 instead of \n*/
	if (len > 0)
		comm[len - 1] = 0;

	vdbg("Got program with pid %d: %s\n", pid, comm);

	close(fd);
	return comm;
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

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX sizeof (((struct sockaddr_un *) 0)->sun_path)
#endif

#define LOCK_SUFFIX ".lock"

static int
socket_lock(struct wldbg *wldbg, const char *path)
{
	struct stat socket_stat;
	ssize_t size = UNIX_PATH_MAX + sizeof(LOCK_SUFFIX);
	int fd;

	char *lock_addr = malloc(size);
	if (!lock_addr)
		return -1;

	if (snprintf(lock_addr, size, "%s%s", path, LOCK_SUFFIX) >= size) {
		fprintf(stderr, "Lock path is too long: %s.lock\n", path);
		free(lock_addr);
		return -1;
	}

	fd = open(lock_addr, O_CREAT | O_CLOEXEC,
	          (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP));

	if (fd < 0) {
		fprintf(stderr, "unable to open lockfile %s check permissions\n",
			lock_addr);
		goto err;
	}

	if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
		fprintf(stderr, "unable to lock lockfile %s, maybe another compositor is running\n",
			lock_addr);
		goto err;
	}

	if (stat(path, &socket_stat) < 0 ) {
		if (errno != ENOENT) {
			fprintf(stderr, "did not manage to stat file %s\n", path);
			goto err;
		}
	}

	wldbg->server_mode.fd_lock = fd;
	wldbg->server_mode.lock_addr = lock_addr;

	return 0;
err:
	close(fd);
	free(lock_addr);
	return -1;
}

#define WLDBG_SERVER_MODE_SOCKET_NAME "wldbg-wayland-0"

int
server_mode_change_sockets(struct wldbg *wldbg)
{
	int fd;
	char *orig_socket;
	char *new_socket;
	const char *wayland_display = getenv("WAYLAND_DISPLAY");

	if (!wayland_display)
		wayland_display = "wayland-0";

	orig_socket = get_socket_path(wayland_display);
	new_socket = get_socket_path(WLDBG_SERVER_MODE_SOCKET_NAME);

	if (!orig_socket || ! new_socket)
		goto err;

	dbg("Renaming wayland socket: %s -> %s\n",
	    orig_socket, new_socket);

	/* rename the socket */
	if (rename(orig_socket, new_socket) < 0) {
		perror("renaming wayland socket");
		fprintf(stderr, "Is any wayland compositor running? "
		        "If so, try setting WAYLAND_DISPLAY\n");
		goto err;
	}

	/* this will make new clients connect to new_socket
	 * instead of orig_socket */
	wldbg->server_mode.old_socket_path = orig_socket;
	wldbg->server_mode.wldbg_socket_path = new_socket;
	wldbg->server_mode.old_socket_name = strdup(wayland_display);
	wldbg->server_mode.wldbg_socket_name = strdup(WLDBG_SERVER_MODE_SOCKET_NAME);

	dbg("Creating fake socket: %s\n", orig_socket);

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
	if (unlink(wldbg->server_mode.old_socket_path) < 0) {
		perror("deleting named socket");
		/* try continue, we'll probably fail right
		 * the next step */
	}

	dbg("Renaming wayland socket back: %s -> %s\n",
	    wldbg->server_mode.wldbg_socket_path,
	    wldbg->server_mode.old_socket_path);

	/* rename the socket */
	if (rename(wldbg->server_mode.wldbg_socket_path,
		   wldbg->server_mode.old_socket_path) < 0) {
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

	dbg("Binding socket: %s\n", name);

	size = SUN_LEN(addr);
	if (bind(sock, (struct sockaddr *) addr, size) < 0) {
		perror("bind() failed");
		goto err;
	}

	if (listen(sock, 128) < 0) {
		perror("listen() failed:");
		goto err;
	}

	dbg("Server mode: listening on fd %d [%s]\n", sock, name);

	return sock;
err:
	close(sock);
	return -1;
}

int server_mode_add_socket2(struct wldbg *wldbg, const char *name)
{
	char *path = get_socket_path(name);
	if (!path)
		return -1;

	wldbg->server_mode.wldbg_socket_path = path;

	return server_mode_add_socket(wldbg, path);
}
