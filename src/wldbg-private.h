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

/* private functions used across the wldbg */

#ifndef _WLDBG_PRIVATE_H_
#define _WLDBG_PRIVATE_H_

#ifdef DEBUG

extern int debug;
extern int debug_verbose;

#define vdbg(...) 							\
	do {								\
		if (!debug_verbose) break;				\
		fprintf(stderr, "[%d | %s: %d] ", getpid(),		\
				__FILE__, __LINE__);			\
		fprintf(stderr,	__VA_ARGS__);				\
	} while (0)



#define dbg(...) 							\
	do {								\
		if (!debug) break;					\
		fprintf(stderr, "[%d | %s: %d] ", getpid(),		\
				__FILE__, __LINE__);			\
		fprintf(stderr,	__VA_ARGS__);				\
	} while (0)

#define ifdbg(cond, ...)			\
	do {					\
		if (!debug) break;		\
		if (cond)			\
			dbg(__VA_ARGS__);	\
	} while (0)

#else

#define dbg(...)
#define ifdbg(cond, ...)

#endif /* DEBUG */

int
copy_arguments(char ***to, int argc, const char*argv[]);

void
free_arguments(char *argv[]);

char *
skip_ws_to_newline(char *str);


#endif /* _WLDBG_PRIVATE_H_ */
