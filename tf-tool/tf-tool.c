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

#include <libthinkfinger.h>

static void callback (libthinkfinger_state state, void *data)
{
	char *str = "unknown";

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

int main ()
{
	int i = 0;
	int t;
	libthinkfinger *tf = NULL;

	tf = libthinkfinger_init ();
 
	printf ("tf-tool: intialization... ");
	if (tf) {
		printf ("ok\n");
	} else {
		printf ("not ok\n");
		return 1;
	}

	printf ("tf-tool: setting file... %sok\n",
		 libthinkfinger_set_file (tf, "/tmp/testbir") ? "not " : "");
	printf ("tf-tool: setting callback... %sok\n",
		 libthinkfinger_set_callback (tf, callback, &i) ? "not " : "");

	printf ("tf-tool: acquire fingerprint...\n");
	t = libthinkfinger_acquire (tf);
	if (t == TF_STATE_ACQUIRE_SUCCESS)
		printf ("acquire successful\n");
	else if (t == TF_STATE_ACQUIRE_FAILED)
		printf ("acquire failed\n");
	else if (t == TF_RESULT_COMM_FAILED)
		printf ("tf-tool: communication error\n");

	printf ("tf-tool: verifying fingerprint...\n");
	t = libthinkfinger_verify (tf);
	if (t == TF_RESULT_VERIFY_SUCCESS)
		printf ("tf-tool: fingerprint matches\n");
	else if (t == TF_RESULT_VERIFY_FAILED)
		printf ("tf-tool: fingerprint does not match\n");
	else if (t == TF_RESULT_COMM_FAILED)
		printf ("tf-tool: communication error\n");

exit:
	libthinkfinger_free (tf);
	return 0;
}
