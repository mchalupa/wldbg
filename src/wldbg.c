/*
 * Copyright (c) 2014 - 2015 Marek Chalupa
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

#ifdef DEBUG
int debug = 0;
int debug_verbose = 0;
#endif

/* defined in interactive.c */
int
interactive_init(struct wldbg *wldbg, struct cmd_options *opts,
		 int argc, const char *argv[]);

/* defined in passes.c */
int
load_passes(struct wldbg *wldbg, int argc, const char *argv[]);

static int
dispatch_messages(int fd, void *data);

static struct wldbg_connection *
wldbg_connection_create(struct wldbg *wldbg)
{
	const char *sock_name = NULL;
	int fd;

	struct wldbg_connection *conn = malloc(sizeof *conn);
	if (!conn)
		return NULL;

	conn->resolved_objects = create_resolved_objects();
	if (!conn->resolved_objects) {
		free(conn);
		return NULL;
	}

	conn->wldbg = wldbg;

	if (wldbg->flags.server_mode)
		sock_name = WLDBG_SERVER_MODE_SOCKET_NAME;

	fd = connect_to_wayland_server(conn, sock_name);
	if (fd < 0) {
		destroy_resolved_objects(conn->resolved_objects);
		free(conn);
		return NULL;
	}

	if (wldbg_monitor_fd(wldbg, conn->server.fd,
			     dispatch_messages, conn) < 0) {
		destroy_resolved_objects(conn->resolved_objects);
		free(conn);
		close(fd);
		return NULL;
	}

	return conn;
}

static void
wldbg_connection_destroy(struct wldbg_connection *conn)
{
	destroy_resolved_objects(conn->resolved_objects);

	wl_connection_destroy(conn->server.connection);
	wl_connection_destroy(conn->client.connection);

	/* XXX new version of wl_connection_destroy does not close
	 * filedescriptors, so if we will update, uncomment this

	if (close(conn->server.fd) < 0)
		perror("wldbg_connectin_destroy: closing server fd");
	if (close(conn->client.fd) < 0)
		perror("wldbg_connectin_destroy: closing client fd");
	*/

	free(conn);
}

/**
 * Register new connection in wldbg
 */
static int
wldbg_add_connection(struct wldbg_connection *conn)
{
	struct wldbg *wldbg = conn->wldbg;

	assert(wldbg->connections_num >= 0);

	wl_list_insert(&wldbg->connections, &conn->link);
	++wldbg->connections_num;

	vdbg("Adding connection (%d) [%p]\n",
	     wldbg->connections_num, conn);

	return wldbg->connections_num;
}

/**
 * Unregister connection in wldbg
 */
static int
wldbg_remove_connection(struct wldbg_connection *conn)
{
	struct wldbg *wldbg = conn->wldbg;

	dbg("Removing connection (%d) [%p]\n",
	     wldbg->connections_num, conn);

	--wldbg->connections_num;
	assert(wldbg->connections_num >= 0
	       && "BUG: removed more connections than added");

	wl_list_remove(&conn->link);

	return wldbg->connections_num;
}

void
wldbg_foreach_connection(struct wldbg *wldbg,
			 void (*func)(struct wldbg_connection *))
{
	struct wldbg_connection *conn, *tmp;

	wl_list_for_each_safe(conn, tmp, &wldbg->connections, link)
		func(conn);
}

struct wldbg_fd_callback {
	int fd;
	void *data;
	int (*dispatch)(int fd, void *data);
	struct wl_list link;
};

/**
 * Monitor filedescriptor for incoming events and
 * call set-up callbacks
 */
int
wldbg_monitor_fd(struct wldbg *wldbg, int fd,
			int (*dispatch)(int fd, void *data),
			void *data)
{
	struct epoll_event ev;
	struct wldbg_fd_callback *cb;

	cb = malloc(sizeof *cb);
	if (!cb)
		return -1;

	ev.events = EPOLLIN;
	ev.data.ptr = cb;
	if (epoll_ctl(wldbg->epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
		perror("Failed adding fd to epoll");
		free(cb);
		return -1;
	}

	cb->fd = fd;
	cb->data = data;
	cb->dispatch = dispatch;

	dbg("Adding fd '%d' to epoll (callback: %p)\n", fd, cb);

	wl_list_insert(&wldbg->monitored_fds, &cb->link);

	return 0;
}

/**
 * Stop monitoring filedescriptor and its callback
 */
int
wldbg_remove_callback(struct wldbg *wldbg, struct wldbg_fd_callback *cb)
{
	int fd = cb->fd;

	wl_list_remove(&cb->link);
	free(cb);

	if (epoll_ctl(wldbg->epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
		perror("Failed removing fd from epoll");
		return -1;
	}

	return 0;
}

int
wldbg_dispatch(struct wldbg *wldbg)
{
	struct epoll_event ev;
	struct wldbg_fd_callback *cb;
	struct wldbg_connection *conn;
	int n, ret;

	assert(!wldbg->flags.exit);
	assert(!wldbg->flags.error);

	n = epoll_wait(wldbg->epoll_fd, &ev, 1, -1);


	if (n < 0) {
		/* don't print error when we has been interrupted
		 * by user */
		if (errno == EINTR && wldbg->flags.exit)
			return 0;

		perror("epoll_wait");
		return -1;
	}

	cb = ev.data.ptr;
	assert(cb && "No callback set in event");
	conn = cb->data;

	if (ev.events & EPOLLHUP) {
		/* if we have no other clients to run,
		 * this func will return 0, thus ending the loop */
		ret = wldbg_remove_connection(conn);

		if (wldbg_remove_callback(wldbg, cb) != 0)
			ret = 0;

		wldbg_connection_destroy(conn);

		return ret;
	}

	if (ev.events & EPOLLERR) {
		fprintf(stderr, "epoll event error\n");
		return -1;
	}

	vdbg("cb [%p]: dispatching %p(%d, %p)\n",
	     cb, cb->dispatch, cb->fd, cb->data);

	return cb->dispatch(cb->fd, cb->data);
}

static void
run_passes(struct message *message)
{
	struct pass *pass;
	struct wldbg *wldbg = message->connection->wldbg;

	assert(wldbg && "BUG: No wldbg set in message->connection");

	wl_list_for_each(pass, &wldbg->passes, link) {
		if (message->from == SERVER) {
			if (pass->wldbg_pass.server_pass(
				pass->wldbg_pass.user_data,
				message) == PASS_STOP)
				break;
		} else {
			if (pass->wldbg_pass.client_pass(
				pass->wldbg_pass.user_data,
				message) == PASS_STOP)
				break;
		}
	}
}

static int
process_one_by_one(struct wl_connection *write_conn,
		   struct message *message)
{
	int n = 0;
	size_t rest = message->size;
	struct wldbg *wldbg = message->connection->wldbg;

	while (rest > 0) {
		message->size = ((uint32_t *) message->data)[1] >> 16;

		run_passes(message);

		/* in interactive mode we can quit here. Do not
		 * write into connection if we quit */
		if (wldbg->flags.exit)
			return 0;
		if (wldbg->flags.error)
			return -1;

		if (wl_connection_write(write_conn, message->data,
					message->size) < 0) {
			perror("wl_connection_write");
			return -1;
		}

		if (wl_connection_flush(write_conn) < 0) {
			perror("wl_connection_flush");
			return -1;
		}

		message->data = message->data + message->size;
		rest -= message->size;
		++n;
	}

	assert(rest == 0 && "Bug!");

	return n;
}

static int
process_data(struct wldbg_connection *conn,
	     struct wl_connection *wl_connection, int len)
{
	int ret = 0;
	struct wl_connection *write_wl_conn;
	struct wldbg *wldbg = conn->wldbg;
	struct message *message = &wldbg->message;
	char *buffer = wldbg->buffer;

	/* reset the message */
	memset(message, 0, sizeof *message);

	wl_connection_copy(wl_connection, buffer, len);
	wl_connection_consume(wl_connection, len);

	if (wl_connection == conn->server.connection) {
		write_wl_conn = conn->client.connection;
		message->from = SERVER;
	} else {
		write_wl_conn = conn->server.connection;
		message->from = CLIENT;
	}

	wl_connection_copy_fds(wl_connection, write_wl_conn);

	message->data = buffer;
	message->size = len;
	message->connection = conn;

	if (wldbg->flags.one_by_one) {
		ret = process_one_by_one(write_wl_conn, message);
	} else {
		/* process passes */
		run_passes(message);

		/* if some pass wants exit or an error occured,
		 * do not write into the connection */
		if (wldbg->flags.exit)
			return 0;
		if (wldbg->flags.error)
			return -1;

		/* resend the data. Use message->data, not buffer,
		 * because some pass could have reallocated the data */
		if (wl_connection_write(write_wl_conn,
					message->data, message->size) < 0) {
			perror("wl_connection_write");
			return -1;
		}

		if (wl_connection_flush(write_wl_conn) < 0) {
			perror("wl_connection_flush");
			return -1;
		}

		ret = 1;
	}

	/* What if some pass reallocated the buffer? */

	return ret;
}

static int
dispatch_messages(int fd, void *data)
{
	int len;
	struct wldbg_connection *conn = data;
	struct wl_connection *wl_conn;

	if (fd == conn->client.fd)
		wl_conn = conn->client.connection;
	else
		wl_conn = conn->server.connection;

	vdbg("Reading connection [%p] from %s\n", conn,
		fd == conn->client.fd ? "client" : "server");

	len = wl_connection_read(wl_conn);
	if (len < 0 && errno != EAGAIN) {
		perror("wl_connection_read");
		return -1;
	} else if (len < 0 && errno == EAGAIN)
		return 1;

	return process_data(conn, wl_conn, len);
}

static int
dispatch_signals(int fd, void *data)
{
	int s;
	size_t len;
	struct signalfd_siginfo si;
	struct wldbg *wldbg = data;
	pid_t pid;

	len = read(fd, &si, sizeof si);
	if (len != sizeof si) {
		fprintf(stderr, "reading signal's fd failed\n");
		return -1;
	}

	dbg("Got signal: %d\n", si.ssi_signo);

	if (si.ssi_signo == SIGCHLD) {
		/* Just print message and let epoll exit with
		 * the right exit code according to if it was HUP or ERR */
		pid = waitpid(-1, &s, WNOHANG);
		fprintf(stderr, "Client '%d' exited...\n",
			pid, WIFEXITED(s) ? "" : "abnormally");
	} else if (si.ssi_signo == SIGINT) {
		fprintf(stderr, "Interrupted...\n");

		kill(0, SIGTERM);
		wldbg->flags.exit = 1;
	} else {
		assert(0 && "Got unhandled signal from epoll");
	}

	return 1;
}

static int
create_client_connection_for_fd(struct wldbg_connection *conn, int fd)
{
	conn->client.connection = wl_connection_create(fd);
	if (!conn->client.connection) {
		perror("Failed creating wl_connection (client)");
		return -1;
	}

	if (wldbg_monitor_fd(conn->wldbg, fd,
			     dispatch_messages, conn) < 0) {
		wl_connection_destroy(conn->client.connection);
		return -1;
	}

	conn->client.fd = fd;
	conn->client.pid = get_pid_for_socket(fd);

	return 0;
}

static int
create_client_connection(struct wldbg_connection *conn)
{
	int sock[2];

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sock) != 0) {
		perror("socketpair");
		return -1;
	}

	if (create_client_connection_for_fd(conn, sock[0]) < 0)
		goto err;


	return sock[1];

err:
	close(sock[0]);
	close(sock[1]);
	return -1;
}

/**
 * Spawn client (program)
 */
struct wldbg_connection *
spawn_client(struct wldbg *wldbg, char *path, char *argv[])
{
	char sockstr[8];
	int sock;
	struct wldbg_connection *conn;

	assert(!wldbg->flags.error);
	assert(!wldbg->flags.exit);

	conn = wldbg_connection_create(wldbg);
	if (!conn)
		return NULL;

	sock = create_client_connection(conn);
	if (sock < 0)
		goto err;

#ifdef DEBUG
	dbg("Spawning client: '%s'\n", path);

	int i;
	for (i = 0; argv[i] != NULL; ++i)
		dbg("\targv[%d]: %s\n", i, argv[i]);
#endif /* DEBUG */


	if (snprintf(sockstr, sizeof sockstr, "%d",
		     sock) >= (int) sizeof sockstr) {
		perror("Socket number too high, hack the code");
		goto err_sock;
	}

	if (setenv("WAYLAND_SOCKET", sockstr, 1) != 0) {
		perror("Setting WAYLAND_SOCKET failed");
		goto err_sock;
	}

	conn->client.pid = fork();

	if (conn->client.pid == -1) {
		perror("fork");
		goto err_sock;
	}

	/* exec client in child */
	if (conn->client.pid == 0) {
		close(conn->client.fd);

		execvp(path, argv);

		perror("Exec failed");
		abort();
	}

	close(sock);

	return conn;

err_sock:
	close(sock);
err:
	wldbg_connection_destroy(conn);
	return NULL;
}

static int
wldbg_run(struct wldbg *wldbg)
{
	int ret = 0;

	assert(!wldbg->flags.exit);
	assert(!wldbg->flags.error);

	wldbg->flags.running = 1;

	while((ret = wldbg_dispatch(wldbg)) > 0) {
		if (wldbg->flags.error) {
			dbg("Exiting for error flag");
			ret = -1;
			break;
		}

		if (wldbg->flags.exit) {
			dbg("Exiting for exit flag");
			ret = 0;
			break;
		}
	}

	wldbg->flags.running = 0;

	return ret;
}

static void
wldbg_destroy(struct wldbg *wldbg)
{
	struct pass *pass, *pass_tmp;
	struct wldbg_fd_callback *cb, *cb_tmp;

	/* free buffer */
	free(wldbg->buffer);

	if (wldbg->epoll_fd >= 0)
		close(wldbg->epoll_fd);
	if (wldbg->signals_fd >= 0)
		close(wldbg->signals_fd);

	wl_list_for_each_safe(pass, pass_tmp, &wldbg->passes, link) {
		if (pass->wldbg_pass.destroy)
			pass->wldbg_pass.destroy(pass->wldbg_pass.user_data);
		free(pass->name);
		free(pass);
	}

	wl_list_for_each_safe(cb, cb_tmp, &wldbg->monitored_fds, link) {
		free(cb);
	}

	if (wldbg->flags.server_mode) {
		/* if we hit an error before creating the socket,
		 * we could destroy some other socket without
		 * this check */
		if (wldbg->server_mode.wldbg_socket_name)
			server_mode_change_sockets_back(wldbg);

		free(wldbg->server_mode.old_socket_name);
		free(wldbg->server_mode.wldbg_socket_name);
	}

	/* if there are any connections left that haven't got
	 * HUP, free them */
	wldbg_foreach_connection(wldbg, wldbg_connection_destroy);
}

static int
wldbg_init(struct wldbg *wldbg)
{
	sigset_t signals;

	memset(wldbg, 0, sizeof *wldbg);
	wldbg->signals_fd = wldbg->epoll_fd = -1;

	/* wl_buffer has size 4096 and probably will
	 * have some time */
	wldbg->buffer = malloc(4096);
	if (!wldbg->buffer)
		return -1;

	wl_list_init(&wldbg->passes);
	wl_list_init(&wldbg->monitored_fds);
	wl_list_init(&wldbg->connections);

	wldbg->epoll_fd = epoll_create1(0);
	if (wldbg->epoll_fd == -1) {
		perror("epoll_create failed");
		goto err_data;
	}

	sigemptyset(&signals);
	sigaddset(&signals, SIGINT);
	sigaddset(&signals, SIGCHLD);

	/* block signals, let them come to signalfd */
	if (sigprocmask(SIG_BLOCK, &signals, NULL) < 0) {
		perror("blocking signals");
		goto err_epoll;
	}

	if ((wldbg->signals_fd = signalfd(-1, &signals, SFD_CLOEXEC)) < 0) {
		perror("signalfd");
		goto err_epoll;
	}

	if (wldbg_monitor_fd(wldbg, wldbg->signals_fd,
			     dispatch_signals, wldbg) < 0)
		goto err_signals;

	wldbg->handled_signals = signals;

	/* init resolving wayland objects */
	if (wldbg_add_resolve_pass(wldbg) < 0)
		goto err_signals;

	return 0;

err_signals:
	close(wldbg->signals_fd);
err_epoll:
	close(wldbg->epoll_fd);
err_data:
	free(wldbg->buffer);
	return -1;
}

static void
help(void)
{
	fprintf(stderr, "wldbg v %s\n", PACKAGE_VERSION);
	fprintf(stderr, "\nUsage:\n");
	fprintf(stderr, "\twldbg [-i|--interactive] ARGUMENTS [PROGRAM]\n");
	fprintf(stderr, "\twldbg pass ARGUMENTS, pass ARGUMENTS,... -- PROGRAM\n");
	fprintf(stderr, "\twldbg [-s|--server-mode]\n");
	fprintf(stderr, "\nTry 'wldbg help' too.\n"
			"For interactive mode and server-mode description "
			"see documentation.\n");
}

static int
server_mode_accept(int fd, void *data)
{
	struct wldbg *wldbg = data;
	struct sockaddr_un name;
	socklen_t length;
	int client_fd;
	struct wldbg_connection *conn;

	length = sizeof name;
	client_fd = wl_os_accept_cloexec(fd, (struct sockaddr *) &name,
					 &length);
	if (client_fd < 0) {
		perror("accept");
		return -1;
	}

	conn = wldbg_connection_create(wldbg);
	if (!conn)
		goto err;

	if (create_client_connection_for_fd(conn, client_fd) < 0) {
		wldbg_connection_destroy(conn);
		goto err;
	}

	wldbg_add_connection(conn);
	dbg("Created new connection to client: %s\n", name.sun_path);

	return 1;
err:
	close(client_fd);
	return -1;
}

static int
server_mode_init(struct wldbg *wldbg)
{
	int fd;

	fd = server_mode_change_sockets(wldbg);
	if (fd < 0)
		return -1;

	if (wldbg_monitor_fd(wldbg, fd, server_mode_accept, wldbg) < 0) {
		close(fd);
		return -1;
	}

	return 0;
}

static int
parse_opts(struct wldbg *wldbg, struct cmd_options *opts, int argc, char *argv[])
{
	/* XXX rework parsing arguments and passing options */
	if (strcmp(argv[1], "--help") == 0 ||
		strcmp(argv[1], "-h") == 0) {
		help();
		exit(1);
	} else if (strcmp(argv[1], "--interactive") == 0 ||
		strcmp(argv[1], "-i") == 0) {
		if (interactive_init(wldbg, opts, argc - 2,
				     (const char **) argv + 2) < 0)
			return -1;
	} else if (strcmp(argv[1], "--server-mode") == 0 ||
		   strcmp(argv[1], "-s") == 0) {
		wldbg->flags.server_mode = 1;

		if (server_mode_init(wldbg) < 0)
			return -1;

		/* server mode is interactive too -- at least
		 * ATM */
		if (interactive_init(wldbg, NULL, 0, NULL) < 0)
			return -1;
	} else {
		fprintf(stderr, "Not implemented yet\n");
		abort();
	}
#if 0
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
#endif

	return 0;
}

int main(int argc, char *argv[])
{
	struct wldbg wldbg;
	/* XXX program info would be better name? */
	struct cmd_options cmd_opts;
	struct wldbg_connection *conn;

	if (argc == 1) {
		help();
		exit(1);
	}

#ifdef DEBUG
	const char *dbg_env = getenv("WLDBG_DEBUG");
	if (dbg_env) {
		debug = 1;

		if (strcmp(dbg_env, "verbose") == 0
			|| strcmp(dbg_env, "v") == 0)
			debug_verbose = 1;
	}
#endif

	memset(&cmd_opts, 0 , sizeof cmd_opts);
	wldbg_init(&wldbg);

	if (parse_opts(&wldbg, &cmd_opts, argc, argv) < 0)
		goto err;

	/* if some pass created
	 * an error while initializing, do not proceed */
	if (wldbg.flags.error)
		goto err;

	if (wldbg.flags.exit) {
		wldbg_destroy(&wldbg);
		return EXIT_SUCCESS;
	}

	if (wldbg.flags.server_mode) {
		printf("Listening for incoming connections...\n");
	} else {
		conn = spawn_client(&wldbg, cmd_opts.path, cmd_opts.argv);
		if (conn == NULL)
			goto err;

		wldbg_add_connection(conn);
	}

	if (wldbg_run(&wldbg) < 0)
		goto err;

	free(cmd_opts.path);
	if (cmd_opts.argv)
		free_arguments(cmd_opts.argv);

	wldbg_destroy(&wldbg);
	return EXIT_SUCCESS;
err:
	free(cmd_opts.path);
	if (cmd_opts.argv)
		free_arguments(cmd_opts.argv);

	wldbg_destroy(&wldbg);
	return EXIT_FAILURE;
}
