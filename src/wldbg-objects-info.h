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

#ifndef _WLDBG_OBJECTS_INFO_H_
#define _WLDBG_OBJECTS_INFO_H_

#include <stdint.h>

struct wl_interface;

struct wldbg_object_info *
wldbg_message_get_object_info(struct wldbg_message *msg, uint32_t id);

struct wldbg_object_info {
	const struct wl_interface *wl_interface;
    /* this is a key to map, but we need it as well
     * when we have just pointer to this struct */
    uint32_t id;
    /* every object has a version, so we can
     * keep it here, instead of in the real info.
     * NOTE: the version may not be set,
     * it depends solely on the object handler */
    uint32_t version;
    /* the real info (according to interface) */
	void *info;
    /* destroy data that are stored in info pointer */
    void (*destroy)(void *);
};

struct wldbg_wl_buffer_info
{
    int32_t width, height, offset, stride;
    uint32_t format;
    unsigned int released : 1;
};

struct wldbg_wl_surface_info {
    struct wldbg_wl_surface_info *commited;

    uint32_t wl_buffer_id;
    uint32_t attached_x, attached_y;
    uint32_t buffer_scale;
    /* XXX do it a callback info */
    uint32_t last_frame_id;
};

struct wldbg_xdg_surface_info {
    /* store last 10 configures */
    struct xdg_configure {
        uint32_t width, height;
        uint32_t serial;
        unsigned int acked : 1;
    } configures[10];
    uint64_t configures_num;

    char *title, *app_id;
    /* store only ids. It is more safe and simpler.
     * Instead of dangling pointers on some improper
     * object destruction, we'll get NULL from map */
    uint32_t wl_surface_id;
    uint32_t parent_id;
};

struct wldbg_wl_seat_info {
    uint32_t capabilities;
    char *name;

    /* FIXME: client can ask for more these objects
    uint32_t pointer_id;
    uint32_t keyboard_id;
    uint32_t touch_id;
    */
};

struct wldbg_wl_keyboard_info {
    /* store last 20 keys */
    uint32_t last_keys[20];

    /* modifiers state */
    struct modifiers {
        uint32_t depressed;
        uint32_t latched;
        uint32_t locked;
    } modifiers;
};

#endif /* _WLDBG_OBJECTS_INFO_H_ */
