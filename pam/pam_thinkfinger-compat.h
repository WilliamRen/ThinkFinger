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
