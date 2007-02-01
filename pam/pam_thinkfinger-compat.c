/*   ThinkFinger Pluggable Authentication Module - Compat Functions
 *
 *   PAM module for libthinkfinger which is a driver for the UPEK/SGS Thomson
 *   Microelectronics fingerprint reader.  These functions add support for
 *   PAM versions older than 0.99.1.0.
 *
 *   Copyright (C) 2007 Stephan Berberig <s.berberig@arcor.de>
 *                      Luca Capello <luca@pca.it>
 *
 *   This file is derived from the Linux-PAM_0.99.1.0 sources, released
 *   under a double license (3-clause BSD or GNU GPL).  In this specific
 *   case, the terms of the GNU General Public License apply.
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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>

#include <security/pam_modules.h>
#include <security/_pam_macros.h>
#include <security/pam_appl.h>

#include "pam_thinkfinger-compat.h"

/*
 * libpam/pam_syslog.c
 */

static const char *_pam_choice2str (int choice)
{
	switch (choice) {
	case PAM_AUTHENTICATE:
		return "auth";
	case PAM_SETCRED:
		return "setcred";
	case PAM_ACCOUNT:
		return "account";
	case PAM_OPEN_SESSION:
	case PAM_CLOSE_SESSION:
		return "session";
	case PAM_CHAUTHTOK:
		return "chauthtok";
	}
	return "";
}

void pam_vsyslog (const pam_handle_t *pamh, int priority, const char *fmt, va_list args)
{
	char *msgbuf1 = NULL, *msgbuf2 = NULL;
	int save_errno = errno;

	if (pamh && pamh->mod_name) {
		if (asprintf (&msgbuf1, "%s(%s:%s):", pamh->mod_name,
			      pamh->service_name?pamh->service_name:"<unknown>",
			      _pam_choice2str (pamh->choice)) < 0) {
			syslog (LOG_AUTHPRIV|LOG_ERR, "asprintf: %m");
			return;
		}
	}

	errno = save_errno;
	if (vasprintf (&msgbuf2, fmt, args) < 0) {
		syslog (LOG_AUTHPRIV|LOG_ERR, "vasprintf: %m");
		_pam_drop (msgbuf1);
		return;
	}

	errno = save_errno;
	syslog (LOG_AUTHPRIV|priority, "%s %s",
		(msgbuf1 ? msgbuf1 : _PAM_SYSTEM_LOG_PREFIX), msgbuf2);

	_pam_drop (msgbuf1);
	_pam_drop (msgbuf2);
}


/*
 * libpam/pam_vprompt.c
 */

void pam_syslog (const pam_handle_t *pamh, int priority, const char *fmt, ...)
{
	va_list args;

	va_start (args, fmt);
	pam_vsyslog (pamh, priority, fmt, args);
	va_end (args);
}

int pam_vprompt (pam_handle_t *pamh, int style, char **response, const char *fmt, va_list args)
{
	struct pam_message msg;
	struct pam_response *pam_resp = NULL;
	const struct pam_message *pmsg;
	const struct pam_conv *conv;
	const void *convp;
	char *msgbuf;
	int ret_val;

	if (response)
		*response = NULL;

	ret_val = pam_get_item (pamh, PAM_CONV, &convp);
	if (ret_val != PAM_SUCCESS)
		return ret_val;
	conv = convp;
	if (conv == NULL || conv->conv == NULL) {
		pam_syslog (pamh, LOG_ERR, "no conversation function");
		return PAM_SYSTEM_ERR;
	}

	if (vasprintf (&msgbuf, fmt, args) < 0) {
		pam_syslog(pamh, LOG_ERR, "vasprintf: %m");
		return PAM_BUF_ERR;
	}

	msg.msg_style = style;
	msg.msg = msgbuf;
	pmsg = &msg;

	ret_val = conv->conv (1, &pmsg, &pam_resp, conv->appdata_ptr);
	if (ret_val != PAM_SUCCESS && pam_resp != NULL)
		pam_syslog (pamh, LOG_WARNING, "unexpected response from failed conversation function");
	if (response)
		*response = pam_resp == NULL ? NULL : pam_resp->resp;
	else if (pam_resp && pam_resp->resp) {
			_pam_overwrite (pam_resp->resp);
			_pam_drop( pam_resp->resp);
		}
	_pam_overwrite (msgbuf);
	_pam_drop (pam_resp);
	free (msgbuf);
	if (ret_val != PAM_SUCCESS)
		pam_syslog (pamh, LOG_ERR, "conversation failed");

	return ret_val;
}

int pam_prompt (pam_handle_t *pamh, int style, char **response, const char *fmt, ...)
{
	va_list args;
	int ret_val;

	va_start (args, fmt);
	ret_val = pam_vprompt (pamh, style, response, fmt, args);
	va_end (args);

	return ret_val;
}
