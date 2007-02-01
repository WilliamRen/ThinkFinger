/*   ThinkFinger Pluggable Authentication Module - Compat Headers
 *
 *   PAM module for libthinkfinger which is a driver for the UPEK/SGS Thomson
 *   Microelectronics fingerprint reader.  These headers add support for
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

#ifndef PAM_THINKFINGER_COMPAT_H
#define PAM_THINKFINGER_COMPAT_H

/*
 * libpam/pam_private.h
 */

/* Values for select arg to _pam_dispatch() */
#define PAM_NOT_STACKED   0
#define PAM_AUTHENTICATE  1
#define PAM_SETCRED       2
#define PAM_ACCOUNT       3
#define PAM_OPEN_SESSION  4
#define PAM_CLOSE_SESSION 5
#define PAM_CHAUTHTOK     6

#define _PAM_SYSTEM_LOG_PREFIX "PAM"

struct handlers {
	struct handler *authenticate;
	struct handler *setcred;
	struct handler *acct_mgmt;
	struct handler *open_session;
	struct handler *close_session;
	struct handler *chauthtok;
};

struct service {
	struct loaded_module *module;	/* Only used for dynamic loading */
	int modules_allocated;
	int modules_used;
	int handlers_loaded;

	struct handlers conf;		/* the configured handlers */
	struct handlers other;		/* the default handlers */
};

#include <sys/time.h>

typedef enum { PAM_FALSE, PAM_TRUE } _pam_boolean;

struct _pam_fail_delay {
	_pam_boolean set;
	unsigned int delay;
	time_t begin;
	const void *delay_fn_ptr;
};

struct _pam_former_state {
	/* this is known and set by _pam_dispatch() */
	int choice;	/* which flavor of module function did we call? */

	/* state info for the _pam_dispatch_aux() function */
	int depth;	/* how deep in the stack were we? */
	int impression;	/* the impression at that time */
	int status;	/* the status before returning incomplete */

	/* state info used by pam_get_user() function */
	int fail_user;
	int want_user;
	char *prompt;	/* saved prompt information */

	/* state info for the pam_chauthtok() function */
	_pam_boolean update;
};

struct pam_handle {
	char *authtok;
	unsigned caller_is;
	struct pam_conv *pam_conversation;
	char *oldauthtok;
	char *prompt;				/* for use by pam_get_user() */
	char *service_name;
	char *user;
	char *rhost;
	char *ruser;
	char *tty;
	struct pam_data *data;
	struct pam_environ *env;		/* structure to maintain environment list */
	struct _pam_fail_delay fail_delay;	/* helper function for easy delays */
	struct service handlers;
	struct _pam_former_state former;	/* library state - support for event driven applications */
	const char *mod_name;			/* Name of the module currently executed */
	int choice;				/* Which function we call from the module */
};


/*
 * libpam/include/security/_pam_types.h
 */

/* -------------- Special defines used by Linux-PAM -------------- */

#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#define PAM_GNUC_PREREQ(maj, min) ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
#define PAM_GNUC_PREREQ(maj, min) 0
#endif

#if PAM_GNUC_PREREQ(2,5)
#define PAM_FORMAT(params) __attribute__((__format__ params))
#else
#define PAM_FORMAT(params)
#endif

#if PAM_GNUC_PREREQ(3,3)
#define PAM_NONNULL(params) __attribute__((__nonnull__ params))
#else
#define PAM_NONNULL(params)
#endif


/*
 * libpam/include/security/pam_ext.h
 */

#include <stdarg.h>

extern void PAM_FORMAT ((printf, 3, 0)) PAM_NONNULL ((3))
pam_vsyslog (const pam_handle_t *pamh, int priority, const char *fmt, va_list args);

extern void PAM_FORMAT ((printf, 3, 4)) PAM_NONNULL ((3))
pam_syslog (const pam_handle_t *pamh, int priority, const char *fmt, ...);

extern int PAM_FORMAT ((printf, 4, 0)) PAM_NONNULL ((1,4))
pam_vprompt (pam_handle_t *pamh, int style, char **response, const char *fmt, va_list args);

extern int PAM_FORMAT ((printf, 4, 5)) PAM_NONNULL ((1,4))
pam_prompt (pam_handle_t *pamh, int style, char **response, const char *fmt, ...);

#define pam_error (pamh, fmt...) pam_prompt(pamh, PAM_ERROR_MSG, NULL, fmt)
#define pam_verror (pamh, fmt, args) pam_vprompt(pamh, PAM_ERROR_MSG, NULL, fmt, args)

#define pam_info (pamh, fmt...) pam_prompt (pamh, PAM_TEXT_INFO, NULL, fmt)
#define pam_vinfo (pamh, fmt, args) pam_vprompt (pamh, PAM_TEXT_INFO, NULL, fmt, args)

#endif /* PAM_THINKFINGER_COMPAT_H */
