/*
 * Copyright (c) 2014 Marek Chalupa
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

#ifndef _WLDBG_IDS_MAP_H_
#define _WLDBG_IDS_MAP_H_

#include "wayland/wayland-util.h"

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

#endif /* _WLDBG_IDS_MAP_H_ */
