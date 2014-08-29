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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <assert.h>
#include <wayland-server-protocol.h>
#include <wayland-client-protocol.h>

#include "wldbg.h"
#include "wldbg-pass.h"
#include "passes.h"

/* this pass analyze the connection and translates object id
 * to human-readable names */

/* special interfaces that will be set to
 * id's that has been deleted or are unknown */
struct wl_interface free_entry = {
	"FREE",
	1, 0, NULL, 0, NULL
};

struct wl_interface unknown_interface = {
	"unknown",
	1, 0, NULL, 0, NULL
};

struct interface
{
	struct wl_interface *interface;
	struct wl_list link;
};

struct resolve {
	struct wldbg *wldbg;
	/* pointer to wldbg->resolved_objects */
	struct wldbg_ids_map *objects;
	struct wl_list interfaces;
};

static void
add_interface(struct resolve *resolve, struct interface *intf)
{
	struct interface *i;

	wl_list_for_each(i, &resolve->interfaces, link) {
		if (strcmp(i->interface->name, intf->interface->name) == 0) {
			dbg("Already got '%s' interface, discarding\n",
				intf->interface->name);
			free(intf);
			return;
		}
	}

	wl_list_insert(resolve->interfaces.next, &intf->link);
}

static struct wl_interface *
get_interface(struct resolve *resolve, const char *name)
{
	struct interface *i;

	wl_list_for_each(i, &resolve->interfaces, link) {
		if (strcmp(i->interface->name, name) == 0) {
			return i->interface;
		}
	}

	dbg("RESOLVE: Didn't find '%s' interface\n", name);
	return NULL;
}

static struct interface *
libwayland_get_interface(void *handle, const char *intf)
{
	struct wl_interface *interface;
	struct interface *ret;

	dbg("Loading interface '%s' from libwayland\n", intf);

	interface = dlsym(handle, intf);
	if (!interface) {
		dbg("Failed loading interface '%s' from libwayland: %s\n",
			intf, dlerror());
		return NULL;
	}

	ret = malloc(sizeof *ret);
	if (!ret)
		return NULL;

	ret->interface = interface;

	return ret;
}

static void
parse_libwayland(struct resolve *resolve)
{
	struct interface *intf;
	void *handle;

	handle = dlopen("libwayland-client.so", RTLD_NOW);
	if (!handle) {
		fprintf(stderr, "Loading pass: %s\n", dlerror());
		return;
	}

	if ((intf = libwayland_get_interface(handle, "wl_display_interface")))
		add_interface(resolve, intf);
	if ((intf = libwayland_get_interface(handle, "wl_registry_interface")))
		add_interface(resolve, intf);
	if ((intf = libwayland_get_interface(handle, "wl_callback_interface")))
		add_interface(resolve, intf);
	if ((intf = libwayland_get_interface(handle,
						"wl_compositor_interface")))
		add_interface(resolve, intf);
	if ((intf = libwayland_get_interface(handle, "wl_shm_pool_interface")))
		add_interface(resolve, intf);
	if ((intf = libwayland_get_interface(handle, "wl_shm_interface")))
		add_interface(resolve, intf);
	if ((intf = libwayland_get_interface(handle, "wl_buffer_interface")))
		add_interface(resolve, intf);
	if ((intf = libwayland_get_interface(handle,
						"wl_data_offer_interface")))
		add_interface(resolve, intf);
	if ((intf = libwayland_get_interface(handle,
						"wl_data_source_interface")))
		add_interface(resolve, intf);
	if ((intf = libwayland_get_interface(handle,
						"wl_data_device_interface")))
		add_interface(resolve, intf);
	if ((intf = libwayland_get_interface(handle,
					"wl_data_device_manager_interface")))
		add_interface(resolve, intf);
	if ((intf = libwayland_get_interface(handle, "wl_shell_interface")))
		add_interface(resolve, intf);
	if ((intf = libwayland_get_interface(handle,
						"wl_shell_surface_interface")))
		add_interface(resolve, intf);
	if ((intf = libwayland_get_interface(handle, "wl_surface_interface")))
		add_interface(resolve, intf);
	if ((intf = libwayland_get_interface(handle, "wl_seat_interface")))
		add_interface(resolve, intf);
	if ((intf = libwayland_get_interface(handle, "wl_pointer_interface")))
		add_interface(resolve, intf);
	if ((intf = libwayland_get_interface(handle, "wl_keyboard_interface")))
		add_interface(resolve, intf);
	if ((intf = libwayland_get_interface(handle, "wl_touch_interface")))
		add_interface(resolve, intf);
	if ((intf = libwayland_get_interface(handle, "wl_output_interface")))
		add_interface(resolve, intf);
	if ((intf = libwayland_get_interface(handle, "wl_region_interface")))
		add_interface(resolve, intf);
	if ((intf = libwayland_get_interface(handle,
						"wl_subcompositor_interface")))
		add_interface(resolve, intf);
	if ((intf = libwayland_get_interface(handle,
						"wl_subsurface_interface")))
		add_interface(resolve, intf);
}

static int
wl_message_new_id_pos(const char *signature)
{
	int n = 0;

	while (signature[n] != '\0') {
		if (signature[n] == 'n')
			return n;

		++n;
	}

	return -1;
}

static uint32_t
get_new_id(const char *signature, uint32_t *data)
{
	int n = 0, pos = 2;
	uint32_t size;

	while (signature[n] != '\0') {
		switch(signature[n]) {
		case 'n':
			vdbg("Found new_id: %u\n", data[pos]);
			return data[pos];
		case 's':
		case 'a':
			size = DIV_ROUNDUP(data[pos], sizeof(uint32_t));
			pos += size + 1;
			break;
		default:
			++pos;
		}

		++n;
	}

	assert(0 && "No new id in message");
}

static void
get_new_ids(struct resolve *resolve, uint32_t *data,
		const struct wl_message *wl_message, const char *guess_type)
{
	uint32_t new_id;
	struct wl_interface *new_intf;
	int id_pos, pos = 0;

	/* search for new_id's in loop in the case there
	 * are more of them in a event/request */
	while ((id_pos = wl_message_new_id_pos(
			wl_message->signature + pos)) >= 0) {

		new_id = get_new_id(wl_message->signature + pos, data);
		new_intf = (void *) wl_message->types[id_pos];

		/* if the type is unknown, we guessed it is
		 * this type (usualy from bind request) */
		if (!new_intf && guess_type){
			dbg("RESOLVE: Guessing unknown type is '%s'\n",
				guess_type);
			new_intf = get_interface(resolve, guess_type);
		}

		if (!new_intf)
			new_intf = &unknown_interface;

		wldbg_ids_map_insert(resolve->objects, new_id,
				  (void *) new_intf);

		dbg("RESOLVE: Got new id %u (%s)\n", new_id, new_intf->name);

		pos = id_pos + 1;
	}
}

static int
resolve_in(void *user_data, struct message *message)
{
	uint32_t id, opcode;
	uint32_t *data = message->data;
	struct resolve *resolve = user_data;
	struct wl_interface *intf;
	const struct wl_message *wl_message;

	id = data[0];
	opcode = data[1] & 0xffff;

	intf = wldbg_ids_map_get(resolve->objects, id);

	if (intf) {
		if (intf == &unknown_interface || intf == &free_entry)
			return PASS_NEXT;

		wl_message = &intf->events[opcode];

		/* handle delete_id event */
		if (id == 1 /* wl_display */
			&& opcode == WL_DISPLAY_DELETE_ID) {
			wldbg_ids_map_insert(resolve->objects, data[2],
						&free_entry);
			dbg("RESOLVE: Freed id %u\n", data[2]);
		} else
			get_new_ids(resolve, data, wl_message, NULL);
	}

	return PASS_NEXT;
}

static int
resolve_out(void *user_data, struct message *message)
{
	uint32_t id, opcode;
	uint32_t *data = message->data;
	struct resolve *resolve = user_data;
	struct wl_interface *intf;
	const struct wl_message *wl_message;
	const char *guess_type = NULL;

	id = data[0];
	opcode = data[1] & 0xffff;

	intf = wldbg_ids_map_get(resolve->objects, id);

	if (intf) {
		/* unknown interface */
		if (intf == &unknown_interface || intf == &free_entry)
			return PASS_NEXT;

		wl_message = &intf->methods[opcode];

		/* data + 4 is the string with the name
		 * of interface in bind request. Use it
		 * to guess the interface of new id */
		if (strcmp(intf->name, "wl_registry") == 0
			&& opcode == WL_REGISTRY_BIND)
				guess_type = (const char *) (data + 4);

		get_new_ids(resolve, data, wl_message, guess_type);
	}

	return PASS_NEXT;
}

static int
resolve_init(struct wldbg *wldbg, struct wldbg_pass *pass,
		int argc, const char *argv[])
{
	struct resolve *resolve;

	resolve = malloc(sizeof *resolve);
	if (!resolve)
		return -1;

	resolve->wldbg = wldbg;
	resolve->objects = &wldbg->resolved_objects;
	wl_list_init(&resolve->interfaces);

	pass->user_data = resolve;

	/* get interfaces from libwayland.so */
	parse_libwayland(resolve);

	/* XXX
	if (path_to_binary)
		parse_binary(... bfd

	*/

	/* id 0 is always empty and 1 is always display */
	wldbg_ids_map_insert(resolve->objects, 0, NULL);
	wldbg_ids_map_insert(resolve->objects, 1,
				get_interface(resolve, "wl_display"));

	return 0;
}

static void
resolve_destroy(void *data)
{
	struct interface *intf, *tmp;
	struct resolve *resolve = data;

	wl_list_for_each_safe(intf, tmp, &resolve->interfaces, link) {
		free(intf);
	}

	free(resolve);
}

struct pass *
create_resolve_pass(void)
{
	struct pass *pass;

	pass = alloc_pass("resolve");
	if (!pass)
		return NULL;

	pass->wldbg_pass.init = resolve_init;
	pass->wldbg_pass.destroy = resolve_destroy;
	pass->wldbg_pass.server_pass = resolve_in;
	pass->wldbg_pass.client_pass = resolve_out;

	return pass;
}

int
wldbg_add_resolve_pass(struct wldbg *wldbg)
{
	struct pass *pass = create_resolve_pass();
	if (!pass)
		return -1;

	if (resolve_init(wldbg, &pass->wldbg_pass, 0, NULL) < 0) {
		dealloc_pass(pass);
		return -1;
	}

	wl_list_insert(wldbg->passes.next, &pass->link);

	return 0;
}
