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

#ifndef _WLDBG_UTIL_H_
#define _WLDBG_UTIL_H_

#include "wayland/wayland-util.h"

#define DIV_ROUNDUP(n, a) ( ((n) + ((a) - 1)) / (a) )

/* we need just something like dynamic array. Wl_map is pain in the ass
 * for our purpose - belive me, I tried it ;) */
struct wldbg_ids_map {
	uint32_t count;
	struct wl_array data;
};

void
wldbg_ids_map_init(struct wldbg_ids_map *map);

void
wldbg_ids_map_release(struct wldbg_ids_map *map);

void
wldbg_ids_map_insert(struct wldbg_ids_map *map, uint32_t id,
			const struct wl_interface *intf);

const struct wl_interface *
wldbg_ids_map_get(struct wldbg_ids_map *map, uint32_t id);

#endif /* _WLDBG_UTIL_H_ */
