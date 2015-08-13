/*
 * Copyright (c) 2014 - 2015 Marek Chalupa
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

#ifndef _WLDBG_RESOLVE_H_
#define _WLDBG_RESOLVE_H_

#include <stdint.h>

struct wldbg;
struct resolved_objects;
struct wldbg_connection;

struct resolved_objects *
wldbg_connection_get_resolved_objects(struct wldbg_connection *connection);

int
wldbg_add_resolve_pass(struct wldbg *wldbg);

const struct wl_interface *
resolved_objects_get(struct resolved_objects *ro, uint32_t id);

const struct wl_interface *
resolved_objects_get_interface(struct resolved_objects *ro, const char *name);

void
resolved_objects_iterate(struct resolved_objects *ro,
			 void (*func)(uint32_t id,
				      const struct wl_interface *intf,
				      void *data),
			 void *data);

void
resolved_objects_interate(struct resolved_objects *ro,
			  void (*func)(uint32_t id,
				       const struct wl_interface *intf,
				       void *data),
			  void *data);

struct resolved_objects *
create_resolved_objects(void);

void
destroy_resolved_objects(struct resolved_objects *ro);

extern const struct wl_interface free_entry;
extern const struct wl_interface unknown_interface;

#endif /* _WLDBG_RESOLVE_H_ */
