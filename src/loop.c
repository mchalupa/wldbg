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

