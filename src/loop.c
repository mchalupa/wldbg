#include <stdlib.h>
#include <ctype.h>
#include <sys/epoll.h>

#include "wldbg.h"
#include "wldbg-private.h"

void
wldbg_exit(struct wldbg *wldbg)
{
	wldbg->flags.exit = 1;
}

void
wldbg_error(struct wldbg *wldbg)
{
	wldbg->flags.error = 1;
}

/**
 * Monitor filedescriptor for incoming events and
 * call set-up callbacks
 */
struct wldbg_fd_callback *
wldbg_monitor_fd(struct wldbg *wldbg, int fd,
		 int (*dispatch)(int fd, void *data),
		 void *data)
{
	struct epoll_event ev;
	struct wldbg_fd_callback *cb;

	cb = malloc(sizeof *cb);
	if (!cb)
		return NULL;

	ev.events = EPOLLIN;
	ev.data.ptr = cb;
	if (epoll_ctl(wldbg->epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
		perror("Failed adding fd to epoll");
		free(cb);
		return NULL;
	}

	cb->fd = fd;
	cb->data = data;
	cb->dispatch = dispatch;

	wl_list_insert(&wldbg->monitored_fds, &cb->link);

	return cb;
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
wldbg_separate_messages(struct wldbg *wldbg, int state)
{
	if (state == -1)
		return wldbg->flags.pass_whole_buffer;

	wldbg->flags.pass_whole_buffer = !!state;
	return wldbg->flags.pass_whole_buffer;
}

void
wldbg_connection_set_user_data(struct wldbg_connection *connection,
			       void *user_data,
                               void (*data_destroy_cb)(struct wldbg_connection *c, void *data))
{
	connection->user_data = user_data;
	connection->user_data_destroy_cb = data_destroy_cb;
}

void *
wldbg_connection_get_user_data(struct wldbg_connection *connection)
{
	return connection->user_data;
}

int
wldbg_connection_get_client_pid(struct wldbg_connection *conn)
{
	return conn->client.pid;
}

