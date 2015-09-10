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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "wldbg.h"
#include "wldbg-pass.h"
#include "wldbg-ids-map.h"

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
		     void *data)
{
	void **p;
	size_t size;

	if (id >= map->count) {
		size = (id - map->count + 1) * sizeof(void *);
		p = wl_array_add(&map->data, size);

		/* set newly allocated memory to 0s */
		memset(p, 0, size);
	}

	p = ((void **) map->data.data) + id;
	assert(p);

	map->count = map->data.size / sizeof(void *);
	*p = data;
}

void *
wldbg_ids_map_get(struct wldbg_ids_map *map, uint32_t id)
{
	if (id < map->count)
		return ((void **) map->data.data)[id];

	return NULL;
}
