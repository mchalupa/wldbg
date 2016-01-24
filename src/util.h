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

#ifndef _WLDBG_UTIL_H_
#define _WLDBG_UTIL_H_

#include <stdlib.h>

#ifndef DIV_ROUNDUP
#define DIV_ROUNDUP(n, a) ( ((n) + ((a) - 1)) / (a) )
#endif

struct wldbg;
struct wldbg_connection;
struct wldbg_message;

/* defined in wldbg.c */
void
wldbg_foreach_connection(struct wldbg *wldbg,
			 void (*func)(struct wldbg_connection *));

/* defined in print.c */
size_t
wldbg_get_message_name(struct wldbg_message *message, char *buf, size_t maxsize);

/* defined in util.c */
int
copy_arguments(char ***to, int argc, const char*argv[]);

void
free_arguments(char **argv);

char *
skip_ws(char *str);

int
str_to_uint(char *str);

char *
strdupf(const char *fmt, ...);

char *
remove_newline(char *str);

#endif /* _WLDBG_UTIL_H_ */
