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
#include <signal.h>

#include "wldbg.h"
#include "wldbg-pass.h"

#include "wayland/wayland-private.h"
#include "wayland/wayland-util.h"
#include "wayland/wayland-os.h"

#ifdef DEBUG
int debug = 0;
int debug_verbose = 0;
#endif

/* defined in interactive.c */
int
run_interactive(struct wldbg *wldbg, int argc, const char *argv[]);

/* defined in passes.c */
int
load_passes(struct wldbg *wldbg, int argc, const char *argv[]);

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
spawn_client(struct wldbg *wldbg)
{
	struct epoll_event ev;
	int sock[2];
	char sockstr[8];

	assert(!wldbg->flags.error);
	assert(!wldbg->flags.exit);

	if (!wldbg->client.path) {
		fprintf(stderr, "No client to run.\n");
		return -1;
	}

	dbg("Spawning client: '%s'\n", wldbg->client.path);

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

		execvp(wldbg->client.path, wldbg->client.argv);

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

	assert(!wldbg->flags.error);
	assert(!wldbg->flags.exit);

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

static void
run_passes(struct wldbg *wldbg, struct message *message)
{
	struct pass *pass;

	wl_list_for_each(pass, &wldbg->passes, link) {
		vdbg("Running pass\n");
		if (message->from == SERVER) {
			if (pass->wldbg_pass.server_pass(pass->wldbg_pass.user_data,
				message) == PASS_STOP)
				break;
		} else {
			if (pass->wldbg_pass.client_pass(pass->wldbg_pass.user_data,
				message) == PASS_STOP)
				break;
		}
	}
}

static int
process_one_by_one(struct wldbg *wldbg, struct wl_connection *write_conn,
			struct message *message)
{
	size_t rest = message->size;

	while (rest > 0) {
		message->size = ((uint32_t *) message->data)[1] >> 16;

		run_passes(wldbg, message);

		vdbg("Writning connection\n");
		if (wl_connection_write(write_conn, message->data,
					message->size) < 0) {
			perror("wl_connection_write");
			return -1;
		}

		vdbg("Flushing connection\n");
		if (wl_connection_flush(write_conn) < 0) {
			perror("wl_connection_flush");
			return -1;
		}

		message->data = message->data + message->size;
		rest -= message->size;
	}

	assert(rest == 0 && "Bug!");

	return 0;
}

static int
process_data(struct wldbg *wldbg, struct wl_connection *connection, int len)
{
	char buffer[4096];
	struct message message;
	struct wl_connection *write_conn;

	wl_connection_copy(connection, buffer, len);
	wl_connection_consume(connection, len);

	if (connection == wldbg->server.connection) {
		write_conn = wldbg->client.connection;
		message.from = SERVER;
	} else {
		write_conn = wldbg->server.connection;
		message.from = CLIENT;
	}

	wl_connection_copy_fds(connection, write_conn);

	message.data = buffer;
	message.size = len;

	if (wldbg->flags.one_by_one) {
		if (process_one_by_one(wldbg, write_conn, &message) < 0)
			return -1;
	} else {
		/* process passes */
		run_passes(wldbg, &message);

		/* resend the data */
		vdbg("Writning connection\n");
		if (wl_connection_write(write_conn, message.data, message.size) < 0) {
			perror("wl_connection_write");
			return -1;
		}
		vdbg("Flushing connection\n");
		if (wl_connection_flush(write_conn) < 0) {
			perror("wl_connection_flush");
			return -1;
		}
	}

	return 0;
}

static int
wldbg_run(struct wldbg *wldbg)
{
	struct epoll_event ev;
	struct wl_connection *conn;
	int n, len;

	assert(!wldbg->flags.exit);
	assert(!wldbg->flags.error);

	wldbg->flags.running = 1;

	while (1) {
		if (wldbg->flags.error)
			return -1;

		if (!wldbg->flags.running || wldbg->flags.exit)
			return 0;

		n = epoll_wait(wldbg->epoll_fd, &ev, 1, -1);

		if (n < 0) {
			/* don't print error when we has been interrupted
			 * by user */
			if (errno == EINTR && !wldbg->flags.running)
				return 0;

			perror("epoll_wait");
			return -1;
		}

		if (ev.events & EPOLLERR) {
			fprintf(stderr, "epoll event error\n");
			return -1;
		} else if (ev.events & EPOLLHUP) {
			return 0;
		}

		conn = ev.data.ptr;

		len = wl_connection_read(conn);
		if (len < 0 && errno != EAGAIN) {
			perror("wl_connection_read");
			return -1;
		} else if (len < 0 && errno == EAGAIN)
			continue;

		if (process_data(wldbg, conn, len) < 0)
			return -1;
	}

	wldbg->flags.running = 0;

	return 0;
}

static void
wldbg_destroy(struct wldbg *wldbg)
{
	struct pass *pass, *tmp;

	close(wldbg->epoll_fd);

	if (wldbg->server.connection)
		wl_connection_destroy(wldbg->server.connection);
	if (wldbg->client.connection)
		wl_connection_destroy(wldbg->client.connection);

	wl_list_for_each_safe(pass, tmp, &wldbg->passes, link) {
		if (pass->wldbg_pass.destroy)
			pass->wldbg_pass.destroy(pass->wldbg_pass.user_data);
		free(pass);
	}

	close(wldbg->server.fd);
	close(wldbg->client.fd);
}

/* This global struct will point to the local one.
 * It's for use in signal handlers */
static struct wldbg *_wldbg = NULL;

static void
sighandler(int signum)
{
	int s;

	assert(_wldbg);

	if (signum == SIGCHLD) {
		/* Just print message and let epoll exit with
		 * the right exit code according to if it was HUP or ERR */
		waitpid(_wldbg->client.pid, &s, WNOHANG);
		fprintf(stderr, "Client '%s' exited...\n",
			WIFEXITED(s) ? "" : "abnormally");
	} else if (signum == SIGINT) {
		kill(_wldbg->client.pid, SIGTERM);
		_wldbg->flags.running = 0;
		_wldbg->flags.exit = 1;

		fprintf(stderr, "Interrupted...\n");
	}
}

static int
wldbg_init(struct wldbg *wldbg)
{
	struct sigaction sa;

	memset(wldbg, 0, sizeof *wldbg);
	wl_list_init(&wldbg->passes);

	_wldbg = wldbg;

	sa.sa_flags = 0;
	sa.sa_handler = sighandler;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("SIGINT sigaction");
		return -1;
	}

	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("SIGINT sigaction");
		return -1;
	}

	wldbg->epoll_fd = epoll_create1(0);
	if (wldbg->epoll_fd == -1) {
		perror("epoll_create failed");
		return -1;
	}
}

static void
help(void)
{
	fprintf(stderr, "wldbg v %s\n", PACKAGE_VERSION);
	fprintf(stderr, "\nUsage:\n");
	fprintf(stderr, "\twldbg [-i|--interactive] ARGUMENTS [PROGRAM]\n");
	fprintf(stderr, "\twldbg pass ARGUMENTS, pass ARGUMENTS,... -- PROGRAM\n");
	fprintf(stderr, "\twldbg [-s|--one-by-one] pass ARGUMENTS,"
			" pass ARGUMENTS,... -- PROGRAM\n");
	fprintf(stderr, "\nTry 'wldbg help' too.\n"
		"For interactive mode description see documentation.\n");
}

static int
parse_opts(struct wldbg *wldbg, int argc, char *argv[])
{
	/* I know about getopt, but we need different behaviour,
	 * so use our own arguments parsing */
	if (strcmp(argv[1], "--help") == 0 ||
		strcmp(argv[1], "-h") == 0) {
		help();
		exit(1);
	} else if (strcmp(argv[1], "--interactive") == 0 ||
		strcmp(argv[1], "-i") == 0) {
		if (run_interactive(wldbg, argc - 2,
					(const char **) argv + 2) < 0)
			return -1;
	} else if (strcmp(argv[1], "--one-by-one") == 0 ||
		strcmp(argv[1], "-s" /* separate/split */) == 0) {

		wldbg->flags.one_by_one = 1;
		if (load_passes(wldbg, argc - 2, (const char **) argv + 2) <= 0) {
			fprintf(stderr, "No passes loaded, exiting...\n");
			return -1;
		}
	} else {
		if (load_passes(wldbg, argc - 1, (const char **) argv + 1) <= 0) {
			fprintf(stderr, "No passes loaded, exiting...\n");
			return -1;
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	struct wldbg wldbg;
#ifdef DEBUG
	const char *dbg_env;
#endif

	if (argc == 1) {
		help();
		exit(1);
	}

#ifdef DEBUG
	dbg_env = getenv("WLDBG_DEBUG");
	if (dbg_env) {
		debug = 1;

		if (strcmp(dbg_env, "verbose") == 0
			|| strcmp(dbg_env, "v") == 0)
			debug_verbose = 1;
	}
#endif

	wldbg_init(&wldbg);

	if (parse_opts(&wldbg, argc, argv) < 0)
		goto err;

	/* if some pass created
	 * an error while initializing, do not proceed */
	if (wldbg.flags.error)
		goto err;

	if (wldbg.flags.exit) {
		wldbg_destroy(&wldbg);
		return EXIT_SUCCESS;
	}

	if (init_wayland_socket(&wldbg) < 0)
		goto err;

	if (spawn_client(&wldbg) < 0)
		goto err;

	if (wldbg_run(&wldbg) < 0)
		goto err;

	wldbg_destroy(&wldbg);
	return EXIT_SUCCESS;
err:
	wldbg_destroy(&wldbg);
	return EXIT_FAILURE;
}
