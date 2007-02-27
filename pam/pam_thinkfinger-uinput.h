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

#ifndef PAM_THINKFINGER_UINPUT_H
#define PAM_THINKFINGER_UINPUT_H

int uinput_cr    (int *fd);
int uinput_close (int *fd);
int uinput_open  (int *fd);

#endif /* PAM_THINKFINGER_UINPUT_H */
