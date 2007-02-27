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

#include <linux/input.h>
#include <linux/uinput.h>
#include <unistd.h>
#include <fcntl.h>

#define UINPUT_DEVICE "/dev/input/uinput"

int uinput_cr (int *fd)
{
	int retval = -1;
	struct input_event ev = {
		.type = EV_KEY,
		.code = KEY_ENTER,
		.time = {0, }
	};

	/* key press */
	ev.value = 1;
	if (write (*fd, &ev, sizeof ev) != sizeof ev)
		goto out;
	/* key release */
	ev.value = 0;
	if (write (*fd, &ev, sizeof ev) != sizeof ev)
		goto out;

	retval = 0;
out:
	return retval;
}

int uinput_close (int *fd)
{
	int retval = -1;

	/* destroy virtual input device */
	if (ioctl (*fd, UI_DEV_DESTROY, 0) < 0)
		goto out;

	close (*fd);

	retval = 0;
out:
	return retval;
}

int uinput_open (int *fd)
{
	int retval = -1;
	int i;
	struct uinput_user_dev device = {
		.name = "Virtual ThinkFinger Keyboard"
	};

	*fd = open (UINPUT_DEVICE, O_WRONLY | O_NDELAY);
	if (*fd < 0)
		goto out;
	/* our single key keyboard */
	i  = ioctl (*fd, UI_SET_EVBIT, EV_KEY) < 0;
	i |= ioctl (*fd, UI_SET_KEYBIT, KEY_ENTER) < 0;

	if (write (*fd, &device, sizeof device) != sizeof device)
		goto out;
	/* create virtual input device */
	if (ioctl (*fd, UI_DEV_CREATE, 0) < 0)
		goto out;

	retval = 0;
out:
	return retval;
}


