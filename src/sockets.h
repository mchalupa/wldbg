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

#ifndef _WLDBG_SOCKETS_H_
#define _WLDBG_SOCKETS_H_

int
connect_to_wayland_server(struct wldbg_connection *conn, const char *display);

int
get_pid_for_socket(int fd);

char *
get_program_for_pid(pid_t pid);

int
server_mode_change_sockets(struct wldbg *wldbg);

int
server_mode_change_sockets_back(struct wldbg *wldbg);

int
server_mode_add_socket(struct wldbg *wldbg, const char *name);

/* this version will use $XDG_RUNTIME_DIR as a root dir when
 * creating socket and will lock the socket the same way
 * as compositors do */
int
server_mode_add_socket_with_lock(struct wldbg *wldbg, const char *name);

#endif /* _WLDBG_SOCKETS_H_ */
