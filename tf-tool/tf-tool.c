 /*   tf-test - A simple example for libthinkfinger
  *
  *   ThinkFinger - A driver for the UPEK/SGS Thomson Microelectronics
  *   fingerprint reader.
  *
  *   Copyright (C) 2006, 2007 Timo Hoenig <thoenig@suse.de>
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

#include <sys/types.h>
#include <errno.h>
#include <libgen.h>
#include <pwd.h>

#include <config.h>
#include <libthinkfinger.h>

#define MODE_UNDEFINED 0
#define MODE_ACQUIRE   1
#define MODE_VERIFY    2
#define MAX_USER       32
#define MAX_PATH       256

#define BIR_EXTENSION    ".bir"
#define BANNER           "\n"PACKAGE_STRING " ("PACKAGE_BUGREPORT")\n" "Copyright (C) 2006, 2007 Timo Hoenig <thoenig@suse.de>\n"

const char* usage_string = "[--acquire | --verify] [--verbose] [bir_file]\n  where --verbose and bir_file are optional.\n\n  --verbose defaults to unspecified\n    bir_file defaults to ~/.thinkfinger.bir.\n";

typedef struct {
	int mode;
	char bir[MAX_PATH];
	_Bool verbose;
	int swipe_success;
	int swipe_failed;
} s_tfdata;

static void print_status (int swipe_success, int swiped_required, int swipe_failed)
{
	printf ("\rPlease swipe your finger (successful swipes %i/%i, failed swipes: %i)...",
		swipe_success, swiped_required, swipe_failed);
	fflush (stdout);
}

static void callback (libthinkfinger_state state, void *data)
{
	char *str;
	s_tfdata *tfdata = (s_tfdata *) data;

	if (tfdata->verbose == true) {
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

		printf ("tf-tool: %s (0x%02x)\n", str, state);
	}

	if (tfdata->mode == MODE_ACQUIRE) {
		switch (state) {
			case TF_STATE_ACQUIRE_SUCCESS:
				printf (" done.\n");
				break;
			case TF_STATE_ACQUIRE_FAILED:
				printf (" failed.\n");
				break;
			case TF_STATE_ENROLL_SUCCESS:
				print_status (tfdata->swipe_success, 3, tfdata->swipe_failed);
				printf (" done.\nStoring data (%s)...", tfdata->bir);
				fflush (stdout);
				break;
			case TF_STATE_SWIPE_FAILED:
				print_status (tfdata->swipe_success, 3, ++tfdata->swipe_failed);
				break;
			case TF_STATE_SWIPE_SUCCESS:
				print_status (++tfdata->swipe_success, 3, tfdata->swipe_failed);
				break;
			case TF_STATE_SWIPE_0:
				print_status (tfdata->swipe_success, 3, tfdata->swipe_failed);
				break;
			default:
				break;
		}
	} else if (tfdata->mode == MODE_VERIFY) {
		switch (state) {
			case TF_STATE_VERIFY_SUCCESS:
				printf ("Result: Fingerprint does match.\n");
				break;
			case TF_STATE_VERIFY_FAILED:
				printf ("Result: Fingerprint does *not* match.\n");
				break;
			case TF_STATE_ENROLL_SUCCESS:
				print_status (tfdata->swipe_success, 1, tfdata->swipe_failed);
				printf (" done.\n");
				break;
			case TF_STATE_SWIPE_FAILED:
				print_status (tfdata->swipe_success, 1, ++tfdata->swipe_failed);
				break;
			case TF_STATE_SWIPE_SUCCESS:
				print_status (++tfdata->swipe_success, 1, tfdata->swipe_failed);
				break;
			case TF_STATE_SWIPE_0:
				print_status (tfdata->swipe_success, 1, tfdata->swipe_failed);
				break;
			default:
				break;
		}
	}
}

static void usage (char *name)
{
	printf ("Usage: %s %s\n", basename (name), usage_string);
}

static void raise_error (libthinkfinger_init_status init_status) {
	const char *msg;

	switch (init_status) {
	case TF_INIT_NO_MEMORY:
		msg = "Not enough memory.";
		break;
	case TF_INIT_USB_DEVICE_NOT_FOUND:
		msg = "USB device not found.";
		break;
	case TF_INIT_USB_OPEN_FAILED:
		msg = "Could not open USB device.";
		break;
	case TF_INIT_USB_CLAIM_FAILED:
		msg = "Could not claim USB device.";
		break;
	case TF_INIT_USB_HELLO_FAILED:
		msg = "Sending HELLO failed.";
		break;
	case TF_INIT_UNDEFINED:
		msg = "Undefined error.";
		break;
	default:
		msg = "Unknown error.";
	}

	printf ("%s\n", msg);

	return;
}

static int acquire (const s_tfdata *tfdata)
{
	libthinkfinger *tf;
	libthinkfinger_init_status init_status;
	libthinkfinger_result tf_result;
	int retval = -1;

	printf ("Initializing...");
	fflush (stdout);
	tf = libthinkfinger_new (&init_status);
	if (init_status != TF_INIT_SUCCESS) {
		raise_error (init_status);
		goto out;
	}
	printf (" done.\n");

	if (libthinkfinger_set_file (tf, tfdata->bir) < 0)
		goto out;
	if (libthinkfinger_set_callback (tf, callback, (void *)tfdata) < 0)
		goto out;

	tf_result = libthinkfinger_acquire (tf);
	switch (tf_result) {
		case TF_RESULT_ACQUIRE_SUCCESS:
			retval = 0;
			break;
		case TF_RESULT_ACQUIRE_FAILED:
			retval = -1;
			break;
		case TF_RESULT_OPEN_FAILED:
			retval = -1;
			printf ("Could not open '%s'.\n", tfdata->bir);
			break;
		case TF_RESULT_SIGINT:
			retval = -1;
			printf ("Interrupted\n.");
			break;
		case TF_RESULT_USB_ERROR:
			printf ("Could not acquire fingerprint (USB error).\n");
			retval = -1;
			break;
		case TF_RESULT_COMM_FAILED:
			printf ("Could not acquire fingerprint (communication with fingerprint reader failed).\n");
			retval = -1;
			break;
		default:
			printf ("Undefined error occured (0x%02x).\n", tf_result);
			retval = -1;
			break;
	}

	libthinkfinger_free (tf);
out:
	return retval;
}

static int verify (const s_tfdata *tfdata)
{
	libthinkfinger *tf;
	libthinkfinger_init_status init_status;
	libthinkfinger_result tf_result;
	int retval = -1;

	printf ("Initializing...");
	fflush (stdout);

	tf = libthinkfinger_new (&init_status);
	if (init_status != TF_INIT_SUCCESS) {
		raise_error (init_status);
		goto out;
	}
	printf (" done.\n");

	if (libthinkfinger_set_file (tf, tfdata->bir) < 0)
		goto out;
	if (libthinkfinger_set_callback (tf, callback, (void *)tfdata) < 0)
		goto out;

	tf_result = libthinkfinger_verify (tf);
	switch (tf_result) {
		case TF_RESULT_VERIFY_SUCCESS:
			retval = 0;
			break;
		case TF_RESULT_VERIFY_FAILED:
			retval = -1;
			break;
		case TF_RESULT_OPEN_FAILED:
			retval = -1;
			printf ("Could not open '%s'.\n", tfdata->bir);
			break;
		case TF_RESULT_SIGINT:
			retval = -1;
			printf ("Interrupted\n.");
			break;
		case TF_RESULT_USB_ERROR:
			printf ("Could not verify fingerprint (USB error).\n");
			retval = -1;
			break;
		case TF_RESULT_COMM_FAILED:
			printf ("Could not verify fingerprint (communication with fingerprint reader failed).\n");
			retval = -1;
			break;
		default:
			printf ("Undefined error occured (0x%02x).\n", tf_result);

			retval = -1;
			break;
	}

	libthinkfinger_free (tf);
out:
	return retval;
}

int
main (int argc, char *argv[])
{
	int i;
	int retval = 0;
	s_tfdata tfdata;
	int user_bir_file = 0;
	unsigned int path_len;

	printf ("%s\n", BANNER);

	if (argc == 1) {
		usage (argv[0]);
		retval = -1;
		goto out;
	}

	tfdata.mode = MODE_UNDEFINED;
	tfdata.verbose = false;
	tfdata.swipe_success = 0;
	tfdata.swipe_failed = 0;

	for (i = 1; i < argc; i++) {
		char *arg = argv[i];
		if (!strcmp (arg, "--acquire")) {
			char *home = getenv ("HOME");
			if (home == NULL) {
				printf ("Could not determine home directory of current user.\n");
				retval = -1;
				goto out;
			}


			if (tfdata.mode != MODE_UNDEFINED) {
				printf ("Mode already set.\n");
				usage (argv [0]);
				retval = -1;
				goto out;
			}

			path_len = strlen (home) + strlen ("/.thinkfinger") + strlen (BIR_EXTENSION);
			if (path_len > MAX_PATH-1) {
				printf ("Path \"%s/.thinkfinger%s\" is too long (maximum %i chars).\n", home, BIR_EXTENSION, MAX_PATH-1);
				retval = -1;
				goto out;
			}
			snprintf (tfdata.bir, MAX_PATH-1, "%s/.thinkfinger%s", home, BIR_EXTENSION);
			tfdata.mode = MODE_ACQUIRE;
		} else if (!strcmp (arg, "--verify")) {
			char *home = getenv ("HOME");
			if (home == NULL) {
				printf ("Could not determine home directory of current user.\n");
				retval = -1;
				goto out;
			}

			path_len = strlen (home) + strlen ("/.thinkfinger") + strlen (BIR_EXTENSION);
			if (path_len > MAX_PATH-1) {
				printf ("Path \"%s/.thinkfinger%s\" is too long (maximum %i chars).\n", home, BIR_EXTENSION, MAX_PATH-1);
				retval = -1;
				goto out;
			}
			snprintf (tfdata.bir, MAX_PATH-1, "%s/.thinkfinger%s", home, BIR_EXTENSION);
			tfdata.mode = MODE_VERIFY;
		} else if (!strcmp (arg, "--verbose")) {
			printf ("Running in verbose mode.\n");
			tfdata.verbose = true;
		} else if (!strcmp (arg, "--help") || !strcmp (arg, "-h")) {
			usage (argv [0]);
			retval = 0;
			goto out;
		} else {
			path_len = strlen (arg);
			if (path_len > MAX_PATH-1) {
				printf ("Path \"%s\" is too long (maximum %i chars).\n", arg, MAX_PATH-1);
				retval = -1;
				goto out;
			}

			if (user_bir_file) {
				printf ("Two output paths specified, but you may only specify one:\n  %s\n  %s\n", tfdata.bir, arg);
				retval = -1;
				goto out;
			}

			snprintf (tfdata.bir, MAX_PATH-1, "%s", arg);
			user_bir_file = 1;
		}
	}

	if (tfdata.mode == MODE_UNDEFINED) {
		printf ("Mode undefined.\n");
		usage (argv [0]);
		retval = -1;
		goto out;
	}

	if (tfdata.verbose == true) {
		printf ("\n* Mode: %s\n* Biometric identification record file: \'%s\'\n\n",
			 (tfdata.mode == MODE_ACQUIRE) ? "acquire" : "verify",
			 tfdata.bir);

	}
	if (tfdata.mode == MODE_ACQUIRE) {
		retval = acquire (&tfdata);
	} else if (tfdata.mode == MODE_VERIFY) {
		retval = verify (&tfdata);
	} else {
		usage (argv[0]);
		retval = -1;
	}
out:
	exit (retval);
}
