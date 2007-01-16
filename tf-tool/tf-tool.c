 /*   tf-test - A simple example for libthinkfinger
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

#define MODE_ACQUIRE 1
#define MODE_VERIFY  2

#define BIR_FILE "/tmp/test.bir"

static void
callback (libthinkfinger_state state, void *data)
{
	char *str;
	_Bool *verbose = (_Bool *) data;

	if (*verbose == true) {
		str = "unknown";

		if (state == TF_STATE_SWIPE_0)
			str = "TF_STATE_SWIPE_0";
		else if (state == TF_STATE_SWIPE_1)
			str = "TF_STATE_SWIPE_1";
		else if (state == TF_STATE_SWIPE_2)
			str = "TF_STATE_SWIPE_2";
		else if (state == TF_STATE_SWIPE_SUCCESS)
			str = "TF_STATE_SWIPE_SUCCESS";
		else if (state == TF_STATE_SWIPE_FAILED)
			str = "TF_STATE_SWIPE_FAILED";
		else if (state == TF_STATE_ENROLL_SUCCESS)
			str = "TF_STATE_ENROLL_SUCCESS";
		else if (state == TF_STATE_ACQUIRE_SUCCESS)
			str = "TF_STATE_ACQUIRE_SUCCESS";
		else if (state == TF_STATE_ACQUIRE_FAILED)
			str = "TF_STATE_ACQUIRE_FAILED";
		else if (state == TF_STATE_VERIFY_SUCCESS)
			str = "TF_STATE_VERIFY_SUCCESS";
		else if (state == TF_STATE_VERIFY_FAILED)
			str = "TF_STATE_VERIFY_FAILED";
		else if (state == TF_STATE_COMM_FAILED)
			str = "TF_STATE_COMM_FAILED";
		else if (state == TF_STATE_INITIAL)
			str = "TF_STATE_INITIAL";

		printf ("tf-tool: %s (%i)\n", str, state);
	}

	if (state == TF_STATE_SWIPE_0 || state == TF_STATE_SWIPE_1 ||
	    state == TF_STATE_SWIPE_2)
		printf ("Please swipe your finger...\n");

	if (state == TF_STATE_ENROLL_SUCCESS)
		printf ("Fingerprint enrolled successfully...\n");

	if (state == TF_STATE_ACQUIRE_SUCCESS)
		printf ("Writing bir file...\n");
}

static void
usage (char* name, int error_code)
{
	printf ("Usage: %s [--acquire | --verify] [--verbose] [--no-init]\n", basename (name));
	exit (error_code);
}

static int
acquire (libthinkfinger *tf, _Bool init_scanner, _Bool verbose)
{
 	int tf_state;
	int retval = 0;

	tf = libthinkfinger_init (init_scanner);
	printf ("tf-tool: initialization: %s\n", tf ? "success" : "failed");

	if (tf == NULL)
		return 1;

	libthinkfinger_set_file (tf, BIR_FILE);
	libthinkfinger_set_callback (tf, callback, &verbose);

	tf_state = libthinkfinger_acquire (tf);
	if (tf_state == TF_STATE_ACQUIRE_SUCCESS) {
		printf ("tf-tool: acquire successful\n");
		printf ("tf-tool: fingerprint bir: %s\n", BIR_FILE);
		retval = 0;
	} else if (tf_state == TF_STATE_ACQUIRE_FAILED) {
		printf ("tf-tool: acquire failed\n");
		retval = 1;
	} else if (tf_state == TF_RESULT_COMM_FAILED) {
		printf ("tf-tool: communication error\n");
		retval = 1;
	}

	libthinkfinger_free (tf);
	return retval;
}

int
verify (libthinkfinger *tf, _Bool init_scanner, _Bool verbose)
{
	int tf_state;
	int retval = 0;

	tf = libthinkfinger_init (init_scanner);
	printf ("tf-tool: initialization: %s\n", tf ? "success" : "failed");

	if (tf == NULL)
		return 1;

	libthinkfinger_set_file (tf, BIR_FILE);
	libthinkfinger_set_callback (tf, callback, &verbose);

	tf_state = libthinkfinger_verify (tf);
	if (tf_state == TF_RESULT_VERIFY_SUCCESS) {
		printf ("tf-tool: fingerprint matches\n");
		retval = 0;
	} else if (tf_state == TF_RESULT_VERIFY_FAILED) {
		printf ("tf-tool: fingerprint does not match\n");
		retval = 0;
	} else if (tf_state == TF_RESULT_COMM_FAILED) {
		printf ("tf-tool: communication error\n");
		retval = 1;
	}

	libthinkfinger_free (tf);
	return retval;
}

int
main (int argc, char *argv[])
{
	int i;
	int mode = 0;
	int retval = 0;
	_Bool verbose = false;
	_Bool init_scanner = true;
	libthinkfinger *tf = NULL;

	if (argc == 1){
		usage (argv[0], 0);
		exit (1);
	}

	for (i = 1; i < argc; i++) {
		char *arg = argv[i];
		if (!strcmp (arg, "--acquire")) {
			mode = MODE_ACQUIRE;
		} else if (!strcmp (arg, "--verify")) {
			mode = MODE_VERIFY;
		} else if (!strcmp (arg, "--verbose")) {
			printf ("Running in verbose mode.\n");
			verbose = true;
		} else if (!strcmp (arg, "--no-init")) {
			init_scanner = false;
		} else {
			usage (argv [0], 1);
		}
	}

	if (mode == MODE_ACQUIRE)
		retval = acquire (tf, init_scanner, verbose);
	else if (mode == MODE_VERIFY)
		retval = verify (tf, init_scanner, verbose);
	else {
		usage (argv[0], 1);
		retval = 1;
	}

	exit (retval);
}
