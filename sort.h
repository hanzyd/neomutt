/**
 * @file
 * Assorted sorting methods
 *
 * @authors
 * Copyright (C) 1996-2000 Michael R. Elkins <me@mutt.org>
 * Copyright (C) 2019 Pietro Cerutti <gahr@gahr.ch>
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MUTT_SORT_H
#define MUTT_SORT_H

#include <stdbool.h>
#include <sys/types.h>
#include "config/lib.h"
#include "options.h" // IWYU pragma: keep

struct Address;
struct Mailbox;
struct ThreadsContext;

/**
 * typedef sort_t - Prototype for a function to compare two emails
 * @param a First email
 * @param b Second email
 * @retval -1 a precedes b
 * @retval  0 a and b are identical
 * @retval  1 b precedes a
 */
typedef int (*sort_t)(const void *a, const void *b);

sort_t mutt_get_sort_func(enum SortType method);

void mutt_sort_headers(struct Mailbox *m, struct ThreadsContext *threads, bool init, off_t *vsize);
int perform_auxsort(int retval, const void *a, const void *b);

const char *mutt_get_name(const struct Address *a);

int sort_code(int rc);

#endif /* MUTT_SORT_H */
