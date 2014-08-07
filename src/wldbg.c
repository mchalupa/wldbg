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

#define _GNU_SOURCE

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

#include "wldbg.h"
#include "wldbg-pass.h"

#include "wayland/wayland-private.h"
#include "wayland/wayland-util.h"
#include "wayland/wayland-os.h"


struct wldbg {
	struct {
		int fd;
		/* TODO get rid of connection??? */
		struct wl_connection *connection;
	} server;

	struct {
		int fd;
		struct wl_connection *connection;

		pid_t pid;
	} client;

	int epoll_fd;

	struct wl_list passes;
};

struct pass {
	struct wldbg_pass pass;
	struct wl_list link;
};

/* copied out from wayland-client.c */
/* renamed from connect_to_socket -> connect_to_wayland_socket */
static int
connect_to_wayland_socket(const char *name)
{
	struct sockaddr_un addr;
	socklen_t size;
	const char *runtime_dir;
	int name_size, fd;

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

static pid_t
spawn_client(struct wldbg *wldbg, const char *path)
{
	struct epoll_event ev;
	int sock[2];
	char sockstr[8];

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sock) != 0) {
		perror("socketpair");
		return -1;
	}

	wldbg->client.fd = sock[0];

	wldbg->client.connection = wl_connection_create(wldbg->client.fd);
	if (!wldbg->client.connection) {
		perror("Failed creating wl_connection (client)");
		goto err;
	}

	ev.events = EPOLLIN;
	ev.data.ptr = wldbg->client.connection;
	if (epoll_ctl(wldbg->epoll_fd, EPOLL_CTL_ADD,
		      wldbg->client.fd, &ev) == -1) {
		perror("Failed adding client's socket to epoll");
		goto err;
	}

	if (snprintf(sockstr, sizeof sockstr, "%d",
		     sock[1]) == sizeof sockstr) {
		perror("Socket number too high, hack the code");
		goto err;
	}

	if (setenv("WAYLAND_SOCKET", sockstr, 1) != 0) {
		perror("Setting WAYLAND_SOCKET failed");
		goto err;
	}

	wldbg->client.pid = fork();

	if (wldbg->client.pid == -1) {
		perror("fork");
		goto err;
	}

	/* exec client in child */
	if (wldbg->client.pid == 0) {
		close(sock[0]);

		/* XXX add arguments */
		execlp(path, path, NULL);

		perror("Exec failed");
		abort();
	}

	close(sock[1]);

	return wldbg->client.pid;

err:
		close(sock[0]);
		close(sock[1]);

		return -1;
}

static int
init_wayland_socket(struct wldbg *wldbg)
{
	struct epoll_event ev;

	wldbg->server.fd = connect_to_wayland_socket(NULL);
	if (wldbg->server.fd == -1) {
		perror("Failed opening wayland socket");
		return -1;
	}

	wldbg->server.connection = wl_connection_create(wldbg->server.fd);
	if (!wldbg->server.connection) {
		perror("Failed creating wl_connection");
		goto err;
	}

	ev.events = EPOLLIN;
	ev.data.ptr = wldbg->server.connection;
	if (epoll_ctl(wldbg->epoll_fd, EPOLL_CTL_ADD,
		      wldbg->server.fd, &ev) == -1) {
		perror("Failed adding wayland socket to epoll");
		goto err;
	}

	return 0;

err:
	close(wldbg->server.fd);
	return -1;
}

static int
process_data(struct wldbg *wldbg, struct wl_connection *connection, int len)
{
	struct pass *pass;
	char buffer[4096];
	struct message message;
	struct wl_connection *write_conn;

	wl_connection_copy(connection, buffer, len);
	wl_connection_consume(connection, len);

	if (connection == wldbg->server.connection) {
		write_conn = wldbg->client.connection;
	} else {
		write_conn = wldbg->server.connection;
	}

	message.data = buffer;
	message.size = len;

	/* process filters */
	wl_list_for_each(pass, &wldbg->passes, link) {
		if (connection == wldbg->server.connection) {
			if (pass->pass.server_pass(pass->pass.user_data, &message)
				== PASS_STOP)
				break;
		} else {
			if (pass->pass.client_pass(pass->pass.user_data, &message)
				== PASS_STOP)
				break;
		}
	}

	/* resend the data */
	wl_connection_copy_fds(connection, write_conn);
	wl_connection_write(write_conn, message.data, message.size);
	wl_connection_flush(write_conn);

	return 0;
}

static int
run(struct wldbg *wldbg)
{
	struct epoll_event ev;
	struct wl_connection *conn;
	int n, len;

	while (1) {
		n = epoll_wait(wldbg->epoll_fd, &ev, 1, -1);

		if (n < 0) {
			perror("epoll_wait");
			return -1;
		}

		if (ev.events & EPOLLERR) {
			fprintf(stderr, "epoll event error");
			return -1;
		} else if (ev.events & EPOLLHUP) {
			fprintf(stderr, "epoll hup\n");
			return 0;
		}

		conn = ev.data.ptr;

		len = wl_connection_read(conn);
		if (len < 0 && errno != EAGAIN) {
			perror("wl_connection_read");
			return -1;
		} else if (len < 0 && errno == EAGAIN)
			continue;

		process_data(wldbg, conn, len);
	}

	return 0;
}

static void
wldbg_destroy(struct wldbg *wldbg)
{
	close(wldbg->epoll_fd);

	if (wldbg->server.connection)
		wl_connection_destroy(wldbg->server.connection);
	if (wldbg->client.connection)
		wl_connection_destroy(wldbg->client.connection);

	close(wldbg->server.fd);
	close(wldbg->client.fd);
}

int main(int argc, char *argv[])
{
	struct wldbg wldbg;
	memset(&wldbg, 0, sizeof wldbg);

	wldbg.epoll_fd = epoll_create1(0);
	if (wldbg.epoll_fd == -1) {
		perror("epoll_create failed");
		return EXIT_FAILURE;
	}

	if (init_wayland_socket(&wldbg) < 0)
		goto err;

	/* XXX check argument and add arguments */
	if (spawn_client(&wldbg, argv[1]) < 0)
		goto err;

	run(&wldbg);

	wldbg_destroy(&wldbg);

	return EXIT_SUCCESS;
err:
	wldbg_destroy(&wldbg);

	return EXIT_FAILURE;
}
