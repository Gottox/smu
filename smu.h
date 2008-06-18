/* libsmu - simple markup library
 * Copyright (C) <2007, 2008> Enno Boland <g s01 de>
 *
 * See LICENSE for further informations
 */
#include <stdio.h>

/**
 * Converts contents of a simple markup stream (in) and prints them to out.
 * If suppresshtml == 1, it will create plain text of the simple markup instead
 * of HTML.
 *
 * Returns 0 on success.
 */
int smu_convert(FILE *out, FILE *in, int suppresshtml);

/** utility */
void eprint(const char *format, ...);

