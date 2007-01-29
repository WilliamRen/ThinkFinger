/*   ThinkFinger Pluggable Authentication Module
 *
 *   PAM module for libthinkfinger which is a driver for the UPEK/SGS Thomson
 *   Microelectronics fingerprint reader.
 *
 *   Copyright (C) 2007 Timo Hoenig <thoenig@suse.de>, <thoenig@nouse.net>
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
 */

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>
#include <libthinkfinger.h>
#include <security/pam_ext.h>
#include <security/pam_modules.h>

#include <config.h>

#define MAX_PATH    256
#define SWIPE_RETRY 3


#define PAM_SM_AUTH

struct pam_thinkfinger_s {
	libthinkfinger *tf;
	const char *user;
	pthread_t t_pam_prompt;
	pthread_t t_thinkfinger;
	pthread_mutex_t retval_mutex;
	int retval;
	pam_handle_t *pamh;
};

static int pam_thinkfinger_check_user (const char *user)
{
	int retval = -1;
	int fd;
	char bir_file[MAX_PATH];

	snprintf (bir_file, MAX_PATH-1, "%s/%s.bir", PAM_BIRDIR, user);
	fd = open (bir_file, O_RDONLY);
	if (fd == -1)
		goto out;

	retval = 0;
	close (fd);

out:
	return retval;
}

static int pam_thinkfinger_verify (const struct pam_thinkfinger_s *pam_thinkfinger)
{
	int tf_state;
	int retval = 0;
	char bir_file[MAX_PATH];

	snprintf (bir_file, MAX_PATH, "%s/%s.bir", PAM_BIRDIR, pam_thinkfinger->user);

	if (pam_thinkfinger->tf == NULL)
		return 1;

	libthinkfinger_set_file (pam_thinkfinger->tf, bir_file);

	tf_state = libthinkfinger_verify (pam_thinkfinger->tf);
	if (tf_state == TF_RESULT_VERIFY_SUCCESS) {
		retval = 0;
	} else if ((tf_state == TF_RESULT_VERIFY_FAILED) ||
		   (tf_state == TF_RESULT_COMM_FAILED)) {
		retval = 1;
	}

	return retval;
}

static void thinkfinger_thread (void *data)
{
	struct pam_thinkfinger_s *pam_thinkfinger = data;
	int i = 0;

	for (i = 0; i < SWIPE_RETRY; i++) {
		if (!pam_thinkfinger_verify (pam_thinkfinger)) {
			pthread_mutex_lock (&pam_thinkfinger->retval_mutex);
			pam_thinkfinger->retval = PAM_SUCCESS;
			pthread_mutex_unlock (&pam_thinkfinger->retval_mutex);
			pthread_cancel (pam_thinkfinger->t_pam_prompt);
			fprintf(stdout, "\n");
			goto out;
		}
	}

out:
	pthread_exit (NULL);
}

static void pam_prompt_thread (void *data)
{
	struct pam_thinkfinger_s *pam_thinkfinger = data;
	char *resp;

	pam_prompt (pam_thinkfinger->pamh, PAM_PROMPT_ECHO_OFF, &resp, "Password or swipe finger: ");
	pam_set_item (pam_thinkfinger->pamh, PAM_AUTHTOK, resp);
	pthread_mutex_lock (&pam_thinkfinger->retval_mutex);
	pam_thinkfinger->retval = PAM_SERVICE_ERR;
	pthread_mutex_unlock (&pam_thinkfinger->retval_mutex);
	pthread_cancel (pam_thinkfinger->t_thinkfinger);

	pthread_exit (NULL);
}

PAM_EXTERN
int pam_sm_authenticate (pam_handle_t *pamh,int flags, int argc, const char **argv)
{
	struct pam_thinkfinger_s pam_thinkfinger;
	struct termios term_attr;
	_Bool tty = false;
	
	tty = isatty (STDIN_FILENO);
	if (tty)
		tcgetattr (STDIN_FILENO, &term_attr);

	pam_thinkfinger.retval = PAM_SERVICE_ERR;
	pam_get_user (pamh, &pam_thinkfinger.user, NULL);
	if (pam_thinkfinger_check_user (pam_thinkfinger.user))
		goto out;

	pam_thinkfinger.tf = libthinkfinger_new ();
	if (!pam_thinkfinger.tf)
		goto out;

	pam_thinkfinger.pamh = pamh;
	pthread_mutex_init(&pam_thinkfinger.retval_mutex, NULL);
	pthread_create (&pam_thinkfinger.t_thinkfinger, NULL, (void *) &thinkfinger_thread, &pam_thinkfinger);
	pthread_create (&pam_thinkfinger.t_pam_prompt, NULL, (void *) &pam_prompt_thread, &pam_thinkfinger);
	pthread_join (pam_thinkfinger.t_pam_prompt, NULL);
	pthread_join (pam_thinkfinger.t_thinkfinger, NULL);

	if (pam_thinkfinger.tf)
		libthinkfinger_free (pam_thinkfinger.tf);
	if (tty) {
		tcsetattr (STDIN_FILENO, TCSADRAIN, &term_attr);
	}

out:
	return pam_thinkfinger.retval;
}

PAM_EXTERN
int pam_sm_setcred (pam_handle_t *pamh,int flags,int argc, const char **argv)
{
	return PAM_SUCCESS;
}


#ifdef PAM_STATIC

struct pam_module _pam_thinkfinger_modstruct = {
	"pam_thinkfinger",
	pam_sm_authenticate,
	pam_sm_setcred,
	NULL,
	NULL,
	NULL,
	pam_sm_chauthtok
};

#endif
