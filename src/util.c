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

#include <stdlib.h>
#include <assert.h>

#include "wldbg.h"
#include "wldbg-pass.h"
#include "util.h"

void
wldbg_ids_map_init(struct wldbg_ids_map *map)
{
	map->count = 0;
	wl_array_init(&map->data);
}

void
wldbg_ids_map_release(struct wldbg_ids_map *map)
{
	map->count = 0;
	wl_array_release(&map->data);
}

void
wldbg_ids_map_insert(struct wldbg_ids_map *map, uint32_t id,
			const struct wl_interface *intf)
{
	const struct wl_interface **p;
	size_t size;

	if (id >= map->count) {
		size = (id - map->count + 1) * sizeof(struct wl_interface *);
		wl_array_add(&map->data, size);

		/* XXX shouldn't we zero out the new memory? */
	}

	p = ((const struct wl_interface **) map->data.data) + id;
	assert(p);

	map->count = map->data.size / sizeof(struct wl_interface *);
	*p = intf;
}

const struct wl_interface *
wldbg_ids_map_get(struct wldbg_ids_map *map, uint32_t id)
{
	if (id < map->count)
		return ((struct wl_interface **) map->data.data)[id];

	return NULL;
}
