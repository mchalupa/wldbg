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

#ifndef _WLDBG_RESOLVE_H_
#define _WLDBG_RESOLVE_H_

struct resolved_objects;
struct wldbg_connection;

struct resolved_objects *
wldbg_connection_get_resolved_objects(struct wldbg_connection *connection);

int
wldbg_add_resolve_pass(struct wldbg *wldbg);

struct wldbg_ids_map *
resolved_objects_get_objects(struct resolved_objects *ro);

struct resolved_objects *
create_resolved_objects(void);

void
destroy_resolved_objects(struct resolved_objects *ro);

#endif /* _WLDBG_RESOLVE_H_ */
