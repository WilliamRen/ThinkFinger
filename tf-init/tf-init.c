 /*   tf-init - Initialize fingerprint scanner
  *
  *   Fingerprint scanner driver for SGS Thomson Microelectronics fingerprint
  *   reader (found in IBM/Lenovo ThinkPads and IBM/Lenovo USB keyboards with
  *   built-in fingerprint reader).
  *
  *   Copyright (C) 2006 Timo Hoenig <thoenig@suse.de>
  *
  *   This program is free software; you can redistribute it and/or modify
  *   it under the terms of the GNU General Public License as published by
  *   the Free Software Foundation; either version 2 of the License, or
  *   (at your option) any later version.
  *
  *   This program is distributed in the hope that it will be useful,
  *   but WITHOUT ANY WARRANTY; without even the implied warranty of
  *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  *   GNU General Public License for more details.
  *
  *   You should have received a copy of the GNU General Public License
  *   along with this program; if not, write to the
  *   Free Software Foundation, Inc.,
  *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  *
  */

#include <libgen.h>
#include <libthinkfinger.h>

static void
usage (char* name, int error_code)
{
	printf ("Usage: %s [ --help | --verbose ]\n", basename (name));
	exit (error_code);
}

int
main (int argc, char *argv[])
{
	int i = 0;
	int verbose = 0;
	int retval = 0;
	libthinkfinger *tf = NULL;

	for (i = 1; i < argc; i++) {
		char *arg = argv[i];
		if (!strcmp (arg, "--help")) {
			usage (argv [0], 1);
		} else if (!strcmp (arg, "--verbose")) {
			printf ("Running in verbose mode.\n");
			verbose = 1;
		}
	}

	tf = libthinkfinger_init (true);
	if (tf == NULL) {
		retval = 1;
		goto out;
	}

	libthinkfinger_free (tf);

out:
	exit (retval);
}
