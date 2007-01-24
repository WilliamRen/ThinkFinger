 /*   tf-test - A simple example for libthinkfinger
  *
  *   Fingerprint scanner driver for SGS Thomson Microelectronics fingerprint
  *   reader (found in IBM/Lenovo ThinkPads and IBM/Lenovo USB keyboards with
  *   built-in fingerprint reader).
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

#define MODE_ACQUIRE 1
#define MODE_VERIFY  2
#define MAX_USER     32
#define MAX_PATH     256

#define DEFAULT_BIR_PATH "/tmp/test.bir"
#define BIR_EXTENSION    ".bir"
#define BANNER           "\n"PACKAGE_STRING " ("PACKAGE_BUGREPORT")\n" "Copyright (C) 2006, 2007 Timo Hoenig <thoenig@suse.de>\n"

#if BUILD_PAM
const char* usage_string = "[--acquire | --verify | --add-user <login>] [--verbose] [--force-init]";
#else
const char* usage_string = "[--acquire | --verify] [--verbose] [--force-init]";
#endif

typedef struct {
	int mode;
	char bir[MAX_PATH];
	_Bool verbose;
	_Bool init_scanner;
	int swipe_success;
	int swipe_failed;
} s_tfdata;

static void
print_status (int swipe_success, int swiped_required, int swipe_failed)
{
	printf ("\rPlease swipe your finger (successful swipes %i/%i, failed swipes: %i)...",
		swipe_success, swiped_required, swipe_failed);
	fflush (stdout);
}

static void
callback (libthinkfinger_state state, void *data)
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

		printf ("tf-tool: %s (%i)\n", str, state);
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
			case TF_STATE_SWIPE_SUCCESS:
				print_status (++tfdata->swipe_success, 3, tfdata->swipe_failed);
				break;
			case TF_STATE_SWIPE_FAILED:
				print_status (tfdata->swipe_success, 3, ++tfdata->swipe_failed);
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
			case TF_STATE_SWIPE_SUCCESS:
				print_status (++tfdata->swipe_success, 1, tfdata->swipe_failed);
				break;
			case TF_STATE_SWIPE_FAILED:
				print_status (tfdata->swipe_success, 1, ++tfdata->swipe_failed);
				break;
			case TF_STATE_SWIPE_0:
				print_status (tfdata->swipe_success, 1, tfdata->swipe_failed);
				break;
			default:
				break;
		}
	}
}

static void
usage (char *name)
{
	printf ("Usage: %s %s\n", basename (name), usage_string);
}

static _Bool
user_exists (const char* login)
{
	_Bool retval = false;

	struct passwd *p  = getpwnam (login);
	if (p == NULL)
		retval = false;
	else
		retval = true;

	return retval;
}

static int
acquire (const s_tfdata *tfdata)
{
	libthinkfinger *tf;
	int tf_state;
	int retval = 0;

	printf ("Initializing...");
	fflush (stdout);
	tf = libthinkfinger_init (tfdata->init_scanner);
	if (tf == NULL) {
		retval = 1;
		goto out;
	}
	printf (" done.\n");

	libthinkfinger_set_file (tf, tfdata->bir);
	libthinkfinger_set_callback (tf, callback, (void *)tfdata);

	tf_state = libthinkfinger_acquire (tf);
	switch (tf_state) {
		case TF_STATE_ACQUIRE_SUCCESS:
			retval = 0;
			break;
		case TF_STATE_ACQUIRE_FAILED:
			retval = 1;
			break;
		case TF_RESULT_COMM_FAILED:
			retval = 1;
			break;
		default:
			retval = 1;
			break;
	}

	libthinkfinger_free (tf);
out:
	return retval;
}

static int
verify (const s_tfdata *tfdata)
{
	libthinkfinger *tf;
	int tf_state;
	int retval = 0;

	printf ("Initializing...");
	fflush (stdout);
	tf = libthinkfinger_init (tfdata->init_scanner);
	if (tf == NULL) {
		retval = 1;
		goto out;
	}
	printf (" done.\n");

	libthinkfinger_set_file (tf, tfdata->bir);
	libthinkfinger_set_callback (tf, callback, (void *)tfdata);

	tf_state = libthinkfinger_verify (tf);
	switch (tf_state) {
		case TF_RESULT_VERIFY_SUCCESS:
			retval = 0;
			break;
		case TF_RESULT_VERIFY_FAILED:
			retval = 1;
			break;
		case TF_RESULT_COMM_FAILED:
			retval = 1;
			break;
		default:
			retval = 1;
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
#if BUILD_PAM
	int path_len = 0;
	const char *user;
#endif

	printf ("%s\n", BANNER);

	if (argc == 1) {
		usage (argv[0]);
		retval = 1;
		goto out;
	}

	tfdata.verbose = false;
	tfdata.init_scanner = false;
	tfdata.swipe_success = 0;
	tfdata.swipe_failed = 0;

	for (i = 1; i < argc; i++) {
		char *arg = argv[i];
		if (!strcmp (arg, "--acquire")) {
			snprintf (tfdata.bir, MAX_PATH-1, "%s", DEFAULT_BIR_PATH);
			tfdata.mode = MODE_ACQUIRE;
		} else if (!strcmp (arg, "--verify")) {
			snprintf (tfdata.bir, MAX_PATH-1, "%s", DEFAULT_BIR_PATH);
			tfdata.mode = MODE_VERIFY;
#if BUILD_PAM
		} else if (!strcmp (arg, "--add-user")) {
			user = argv[++i];
			if (strlen (user) > MAX_USER) {
				printf ("User name \"%s\" is too long (maximum %i chars).\n", user, MAX_USER);
				retval = 1;
				goto out;
			}
			if (user_exists (user) == false ) {
				printf ("The user \"%s\" does not exist.\n", user);
				retval = 1;
				goto out;
			}
			if (access (PAM_BIRDIR, R_OK|W_OK|X_OK) != 0) {
				perror ("Could not access " PAM_BIRDIR);
				retval = 1;
				goto out;
			}
			path_len = strlen (PAM_BIRDIR) + strlen ("/") + strlen (user) + strlen (BIR_EXTENSION);
			if (path_len > MAX_PATH-1) {
				printf ("Path \"%s/%s%s\" is too long (maximum %i chars).\n", PAM_BIRDIR, user, BIR_EXTENSION, MAX_PATH-1);
				retval = 1;
				goto out;
			}
			snprintf (tfdata.bir, MAX_PATH-1, "%s/%s%s", PAM_BIRDIR, user, BIR_EXTENSION);
			tfdata.mode = MODE_ACQUIRE;
#endif
		} else if (!strcmp (arg, "--verbose")) {
			printf ("Running in verbose mode.\n");
			tfdata.verbose = true;
		} else if (!strcmp (arg, "--force-init")) {
			tfdata.init_scanner = true;
		} else if (!strcmp (arg, "--help") || !strcmp (arg, "-h")) {
			usage (argv [0]);
			retval = 0;
			goto out;
		} else {
			printf ("Unknown option \"%s\".\n", arg);
			usage (argv [0]);
			retval = 1;
			goto out;
		}
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
		retval = 1;
	}
out:
	exit (retval);
}
