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

#include <config.h>

#include <libthinkfinger.h>
#include <pam_thinkfinger-uinput.h>

#include <stdio.h>
#include <stdarg.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>
#include <syslog.h>
#include <security/pam_modules.h>
#include <pwd.h>
#ifdef HAVE_OLD_PAM
#include "pam_thinkfinger-compat.h"
#else
#include <security/pam_ext.h>
#endif

#define MAX_PATH 256

#define PAM_SM_AUTH

volatile static int pam_tf_debug = 0;

typedef struct {
	libthinkfinger *tf;
	const char *user;
	char bir_file[MAX_PATH];
	pthread_t t_pam_prompt;
	pthread_t t_thinkfinger;
	int swipe_retval;
	int prompt_retval;
	int isatty;
	int uinput_fd;
	pam_handle_t *pamh;
} pam_thinkfinger_s;

static void pam_thinkfinger_log (const pam_thinkfinger_s *pam_thinkfinger, int type, const char *format, ...)
{
	char message[LINE_MAX];
	va_list ap;

	if (pam_tf_debug) {
		va_start (ap, format);
		vsnprintf (message, sizeof(message), format, ap);
		va_end(ap);
		pam_syslog (pam_thinkfinger->pamh, type, message);
	}
}

static void pam_thinkfinger_options (const pam_thinkfinger_s *pam_thinkfinger, int argc, const char **argv)
{
	int i;

	for (i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "debug"))
			pam_tf_debug = 1;
		else if (!strcmp(argv[i], " ") || !strcmp(argv[i], "\t"))
			continue;
		else
			pam_thinkfinger_log (pam_thinkfinger, LOG_INFO,
					     "Option '%s' is not recognised or not yet supported.", *(argv+i));
	}
}

static int pam_thinkfinger_user_sanity_check (const pam_thinkfinger_s *pam_thinkfinger)
{
	const char *user = pam_thinkfinger->user;
	size_t len = strlen(user);

	return strstr(user, "../") || user[0] == '-' || user[len - 1] == '/';
}

static int pam_thinkfinger_user_bir_check (pam_thinkfinger_s *pam_thinkfinger)
{
	int retval = -1;
	int fd;

	struct passwd *pw;

	pw = getpwnam (pam_thinkfinger->user);
	if (pw == NULL) {
		pam_thinkfinger_log (pam_thinkfinger, LOG_ERR,
				     "getpwnam(\"%s\") failed: %s.", pam_thinkfinger->user, strerror (errno));
		goto out;
	}

	snprintf (pam_thinkfinger->bir_file, MAX_PATH, "%s/.thinkfinger.bir", pw->pw_dir);
	fd = open (pam_thinkfinger->bir_file, O_RDONLY | O_NOFOLLOW);

	if (fd == -1) {
		pam_thinkfinger_log (pam_thinkfinger, LOG_ERR,
				     "Could not open '%s': (%s).", pam_thinkfinger->bir_file, strerror (errno));

		snprintf (pam_thinkfinger->bir_file, MAX_PATH, "%s/%s.bir", PAM_BIRDIR, pam_thinkfinger->user);
		fd = open (pam_thinkfinger->bir_file, O_RDONLY | O_NOFOLLOW);
	}

	if (fd == -1) {
		pam_thinkfinger_log (pam_thinkfinger, LOG_ERR,
				     "Could not open '%s': (%s).", pam_thinkfinger->bir_file, strerror (errno));
		goto out;
	}

	retval = 0;
	close (fd);

out:
	return retval;
}

static libthinkfinger_state pam_thinkfinger_verify (const pam_thinkfinger_s *pam_thinkfinger)
{
	libthinkfinger_state tf_state = TF_STATE_VERIFY_FAILED;
	int retry = 20;

	if (pam_thinkfinger->tf == NULL)
		goto out;

	libthinkfinger_set_file (pam_thinkfinger->tf, pam_thinkfinger->bir_file);
	/* if the USB device is being removed while verification (e.g. suspend) retry */
	while ((tf_state = libthinkfinger_verify (pam_thinkfinger->tf)) == TF_RESULT_USB_ERROR && --retry > 0)
		usleep (250000);

	if (retry == 0 && tf_state == TF_STATE_USB_ERROR)
		pam_thinkfinger_log (pam_thinkfinger, LOG_WARNING, "USB device did not reappear in time");
out:
	return tf_state;
}

static void thinkfinger_thread (void *data)
{
	int ret;
	pam_thinkfinger_s *pam_thinkfinger = data;
	libthinkfinger_state tf_state;

	pam_thinkfinger_log (pam_thinkfinger, LOG_NOTICE, "%s called.", __FUNCTION__);

	tf_state = pam_thinkfinger_verify (pam_thinkfinger);
	if (tf_state == TF_RESULT_VERIFY_SUCCESS) {
		pam_thinkfinger->swipe_retval = PAM_SUCCESS;
		pam_thinkfinger_log (pam_thinkfinger, LOG_NOTICE,
				    "User '%s' authenticated (biometric identification record matched).", pam_thinkfinger->user);
	} else if (tf_state == TF_RESULT_VERIFY_FAILED) {
		pam_thinkfinger->swipe_retval = PAM_AUTH_ERR;
		pam_thinkfinger_log (pam_thinkfinger, LOG_NOTICE,
				     "User '%s' verification failed (biometric identification record not matched).",
				     pam_thinkfinger->user);
	} else {
		pam_thinkfinger->swipe_retval = PAM_AUTH_ERR;
		pam_thinkfinger_log (pam_thinkfinger, LOG_NOTICE,
				     "User '%s' verification failed (0x%x).", pam_thinkfinger->user, tf_state);
	}

	ret = uinput_cr (&pam_thinkfinger->uinput_fd);
	if (ret != 0)
		pam_thinkfinger_log (pam_thinkfinger, LOG_ERR,
				     "Could not send carriage return via uinput: %s.", strerror (ret));

	pam_thinkfinger_log (pam_thinkfinger, LOG_NOTICE,
			     "%s returning '%d': %s.", __FUNCTION__, pam_thinkfinger->swipe_retval,
			     pam_thinkfinger->swipe_retval ? pam_strerror (pam_thinkfinger->pamh, pam_thinkfinger->swipe_retval) : "success");
	pthread_exit (NULL);
}

static void pam_prompt_thread (void *data)
{
	pam_thinkfinger_s *pam_thinkfinger = data;
	char *resp;

	/* always returning from pam_prompt due to the CR sent by the keyboard or by uinput */
	pam_prompt (pam_thinkfinger->pamh, PAM_PROMPT_ECHO_OFF, &resp, "Password or swipe finger: ");
	pam_set_item (pam_thinkfinger->pamh, PAM_AUTHTOK, resp);

	/* ThinkFinger thread will return once we call libthinkfinger_free */
	if (pam_thinkfinger->tf != NULL)
		libthinkfinger_free (pam_thinkfinger->tf);

	pthread_exit (NULL);
}

static const char *handle_error (libthinkfinger_init_status init_status)
{
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

	return msg;
}

PAM_EXTERN
int pam_sm_authenticate (pam_handle_t *pamh, int flags, int argc, const char **argv)
{
	int ret;
	int retval = PAM_AUTH_ERR;
	const char *rhost = NULL;
	pam_thinkfinger_s pam_thinkfinger;
	struct termios term_attr;
	libthinkfinger_init_status init_status;

	pam_thinkfinger.swipe_retval = PAM_SERVICE_ERR;
	pam_thinkfinger.pamh = pamh;

	pam_thinkfinger_options (&pam_thinkfinger, argc, argv);
	pam_thinkfinger_log (&pam_thinkfinger, LOG_INFO, "%s called.", __FUNCTION__);

	pam_thinkfinger.isatty = isatty (STDIN_FILENO);
	if (pam_thinkfinger.isatty == 1)
		tcgetattr (STDIN_FILENO, &term_attr);

	pam_get_item (pamh, PAM_RHOST, (const void **)( const void*) &rhost);
	if (rhost != NULL && strlen (rhost) > 0) {
		pam_thinkfinger_log (&pam_thinkfinger, LOG_ERR, "Error: Remote login from host \"%s\" detected.", rhost);
		goto out;
	}

	if ((retval = pam_get_user(pamh, &pam_thinkfinger.user, NULL)) != PAM_SUCCESS)
		goto out;
	if (pam_thinkfinger_user_sanity_check (&pam_thinkfinger) || pam_thinkfinger_user_bir_check (&pam_thinkfinger) < 0) {
		pam_thinkfinger_log (&pam_thinkfinger, LOG_ERR, "User '%s' is unknown.", pam_thinkfinger.user);
		retval = PAM_USER_UNKNOWN;
		goto out;
	}

	ret = uinput_open (&pam_thinkfinger.uinput_fd);
	if (ret != 0) {
		pam_thinkfinger_log (&pam_thinkfinger, LOG_ERR, "Initializing uinput failed: %s.", strerror (ret));
		retval = PAM_AUTHINFO_UNAVAIL;
		goto out;
	}

	pam_thinkfinger.tf = libthinkfinger_new (&init_status);
	if (init_status != TF_INIT_SUCCESS) {
		pam_thinkfinger_log (&pam_thinkfinger, LOG_ERR, "Error: %s", handle_error (init_status));
		retval = PAM_AUTHINFO_UNAVAIL;
		goto out;
	}

	ret = pthread_create (&pam_thinkfinger.t_pam_prompt, NULL, (void *) &pam_prompt_thread, &pam_thinkfinger);
	if (ret != 0) {
		pam_thinkfinger_log (&pam_thinkfinger, LOG_ERR, "Error calling pthread_create (%s).", strerror (ret));
		goto out;
	}
	ret = pthread_create (&pam_thinkfinger.t_thinkfinger, NULL, (void *) &thinkfinger_thread, &pam_thinkfinger);
	if (ret != 0) {
		pam_thinkfinger_log (&pam_thinkfinger, LOG_ERR, "Error calling pthread_create (%s).", strerror (ret));
		goto out;
	}
	ret = pthread_join (pam_thinkfinger.t_thinkfinger, NULL);
	if (ret != 0) {
		pam_thinkfinger_log (&pam_thinkfinger, LOG_ERR, "Error calling pthread_join (%s).", strerror (ret));
		goto out;
	}
	ret = pthread_join (pam_thinkfinger.t_pam_prompt, NULL);
	if (ret != 0) {
		pam_thinkfinger_log (&pam_thinkfinger, LOG_ERR, "Error calling pthread_join (%s).", strerror (ret));
		goto out;
	}

	if (pam_thinkfinger.uinput_fd > 0)
		uinput_close (&pam_thinkfinger.uinput_fd);
	if (pam_thinkfinger.isatty == 1) {
		tcsetattr (STDIN_FILENO, TCSADRAIN, &term_attr);
	}

	if (pam_thinkfinger.swipe_retval == PAM_SUCCESS)
		retval = PAM_SUCCESS;
	else
		retval = PAM_AUTHINFO_UNAVAIL;
out:
	pam_thinkfinger_log (&pam_thinkfinger, LOG_INFO,
			     "%s returning '%d': %s.", __FUNCTION__, retval, retval ? pam_strerror (pamh, retval) : "success");
	return retval;
}

PAM_EXTERN
int pam_sm_setcred (pam_handle_t *pamh, int flags, int argc, const char **argv)
{
	return PAM_SUCCESS;
}

PAM_EXTERN
int pam_sm_chauthtok (pam_handle_t *pamh, int flags, int argc, const char **argv)
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
