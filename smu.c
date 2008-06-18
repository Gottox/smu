/* libsmu - simple markup library
 * Copyright (C) <2007, 2008> Enno Boland <g s01 de>
 *
 * See LICENSE for further informations
 */
#include <stdlib.h>
#include <string.h>

#include "smu.h"

int
main(int argc, char *argv[]) {
	int no = 0;
	FILE *in = stdin;

	if(argc > 1 && strcmp("-v", argv[1]) == 0)
		eprint("simple markup %s (C) Enno Boland\n",VERSION);
	else if(argc > 1 && strcmp("-h", argv[1]) == 0)
		eprint("Usage %s [-n] [file]\n -n escape html strictly\n",argv[0]);
	if(argc > 1 && strcmp("-n", argv[1]) == 0)
		no = 1;
	if(argc > 1 + no
	   && strcmp("-", argv[1 + no]) != 0
	   && !(in = fopen(argv[1 + no],"r")))
		eprint("Cannot open file `%s`\n",argv[1 + no]);
	smu_convert(stdout, in, no);
	fclose(in);
	return EXIT_SUCCESS;
}
