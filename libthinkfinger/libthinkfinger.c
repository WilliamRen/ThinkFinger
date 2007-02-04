/*
 *   ThinkFinger - A driver for the UPEK/SGS Thomson Microelectronics
 *   fingerprint reader.
 *
 *   Copyright (C) 2006 Pavel Machek <pavel@suse.cz>
 *                      Timo Hoenig <thoenig@suse.de>
 *
 *   Copyright (C) 2007 Timo Hoenig <thoenig@suse.de>
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
 *   TODO: move this to documentation
 *   Hardware should be 248 x 4 pixels, 8bit per pixel, but seems to do matching
 *   completely in hardware.
 *
 *   Use thinkfinger to create result.bir files (you'll need to swipe
 *   finger 3 times). Use thinkfinger something.bir to recognize if
 *   it is the right fingerprint. Note: closed-source version will eat
 *   these .bir files (but they are two bytes too long), but thinkfinger
 *   will not eat files from closed-source version (see HACK for line
 *   that breaks it).
 *
 *   TODO: this is not true for all distributions
 *   Note that you need to be root to use this.
 */

#include "libthinkfinger.h"
#include "libthinkfinger-crc.h"

#define VENDOR_ID   0x0483
#define PRODUCT_ID  0x2016
#define USB_TIMEOUT 250

static char init_a[17] = {
	0x43, 0x69, 0x61, 0x6f, 0x04, 0x00, 0x08, 0x01,
	0x00, 0xe8, 0x03, 0x00, 0x00, 0xff, 0x07, 0xdb,
	0x24
};

static char init_b[16] = {
	0x43, 0x69, 0x61, 0x6f, 0x00, 0x00, 0x07, 0x28,
	0x04, 0x00, 0x00, 0x00, 0x06, 0x04, 0xc0, 0xd6
};

static char init_c[16] = {
	0x43, 0x69, 0x61, 0x6f, 0x00, 0x10, 0x07, 0x28,
	0x04, 0x00, 0x00, 0x00, 0x07, 0x04, 0x0f, 0xb6
};

static char init_d[40] = {
	0x43, 0x69, 0x61, 0x6f, 0x00, 0x20, 0x1f, 0x28,
	0x1c, 0x00, 0x00, 0x00, 0x08, 0x04, 0x83, 0x00,
	0x2c, 0x22, 0x23, 0x97, 0xc9, 0xa7, 0x15, 0xa0,
	0x8a, 0xab, 0x3c, 0xd0, 0xbf, 0xdb, 0xf3, 0x92,
	0x6f, 0xae, 0x3b, 0x1e, 0x44, 0xc4, 0x9a, 0x45
};

static char init_e[20] = {
	0x43, 0x69, 0x61, 0x6f, 0x00, 0x30, 0x0b, 0x28,
	0x08, 0x00, 0x00, 0x00, 0x0c, 0x04, 0x03, 0x00,
	0x00, 0x00, 0x6d, 0x7e
};

struct init_table {
	char *data;
	size_t len;
};

static struct init_table init[] = {
	{ init_a, sizeof (init_a) },
	{ init_b, sizeof (init_b) },
	{ init_c, sizeof (init_c) },
	{ init_d, sizeof (init_d) },
	{ init_e, sizeof (init_e) },
	{ 0x0,    0x0 }
};

static char ctrlbuf [1024] = {
	0x43, 0x69, 0x61, 0x6f, 0x00, 0x51, 0x0b, 0x28,
	0xb8, 0x00, 0x00, 0x00, 0x03, 0x02, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xc0, 0xd4, 0x01, 0x00, 0x20, 0x00,
	0x00, 0x00, 0x03
};

static char enroll_init [23] = {
	0x43, 0x69, 0x61, 0x6f, 0x00, 0x50, 0x0e, 0x28,
	0x0b, 0x00, 0x00, 0x00, 0x02, 0x02, 0xc0, 0xd4,
	0x01, 0x00, 0x04, 0x00, 0x08, 0x0f, 0x86
};

static char scan_sequence_one [17] = {
	0x43, 0x69, 0x61, 0x6f, 0x00, 0x60, 0x08, 0x28,
	0x05, 0x00, 0x00, 0x00, 0x00, 0x30, 0x01, 0x49,
	0x6b
};

static char scan_sequence_two [17] = {
	0x43, 0x69, 0x61, 0x6f, 0x00, 0x80, 0x08, 0x28,
	0x05, 0x00, 0x00, 0x00, 0x00, 0x30, 0x01, 0x6a,
	0xc4
};

struct libthinkfinger_s {
	struct usb_dev_handle *usb_handle;
	const char *file;
	int fd;
	_Bool write_fingerprint;

	libthinkfinger_task task;
	_Bool task_running;

	libthinkfinger_state state;
	libthinkfinger_state_cb cb;
	void *cb_data;
};

static void _libthinkfinger_task_start (libthinkfinger *tf, libthinkfinger_task task)
{
	tf->task = task;
	tf->state = TF_STATE_INITIAL;
	tf->task_running = true;
}

static void _libthinkfinger_task_stop (libthinkfinger *tf)
{
	tf->task_running = false;
	tf->task = TF_TASK_IDLE;
}

static _Bool _libthinkfinger_task_running (libthinkfinger *tf)
{
	return tf->task_running;
}

static int _libthinkfinger_usb_hello (struct usb_dev_handle *handle)
{
	int ret_val = -1;
	char dummy[] = "\x10";

	/* SET_CONFIGURATION 1 -- should not be relevant */
	ret_val = usb_control_msg (handle,	 // usb_dev_handle *dev
				   0x00000000,	 // int requesttype
				   0x00000009,	 // int request
				   0x001,	 // int value
				   0x000,	 // int index
				   dummy,	 // char *bytes
				   0x00000000,	 // int size
				   USB_TIMEOUT); // int timeout

	if (ret_val < 0)
		goto out;

	ret_val = usb_control_msg (handle,	 // usb_dev_handle *dev
				   0x00000040,	 // int requesttype
				   0x0000000c,	 // int request
				   0x100,	 // int value
				   0x400,	 // int index
				   dummy,	 // char *bytes
				   0x00000001,	 // int size
				   USB_TIMEOUT); // int timeout

out:
	return ret_val;
}


static struct usb_device *_libthinkfinger_usb_device_find (void)
{
	struct usb_bus *usb_bus;
	struct usb_device *dev;

	usb_init ();
	usb_find_busses ();
	usb_find_devices ();

	/* TODO: Support systems with two fingerprint readers */
	for (usb_bus = usb_busses; usb_bus; usb_bus = usb_bus->next) {
		for (dev = usb_bus->devices; dev; dev = dev->next) {
			if ((dev->descriptor.idVendor == VENDOR_ID) &&
			    (dev->descriptor.idProduct == PRODUCT_ID))
				return dev;
		}
	}
	return NULL;
}


static void
_libthinkfinger_usb_deinit (libthinkfinger *tf)
{
	if (tf->usb_handle) {
		usb_release_interface (tf->usb_handle, 0);
		usb_close (tf->usb_handle);
	}
	tf->usb_handle = NULL;
}

static libthinkfinger_init_status
_libthinkfinger_usb_init (libthinkfinger *tf)
{
	struct usb_device *usb_dev;
	libthinkfinger_init_status ret_val = TF_INIT_UNDEFINED;

	usb_dev = _libthinkfinger_usb_device_find ();
	if (usb_dev == NULL) {
		ret_val = TF_INIT_USB_DEVICE_NOT_FOUND;
		goto out;
	}

	tf->usb_handle = usb_open (usb_dev);
	if (tf->usb_handle == NULL) {
		ret_val = TF_INIT_USB_OPEN_FAILED;
		goto out;
	}

	if (usb_claim_interface (tf->usb_handle, 0) < 0) {
		ret_val = TF_INIT_USB_CLAIM_FAILED;
		goto out;
	}

	if (_libthinkfinger_usb_hello (tf->usb_handle) < 0) {
		ret_val = TF_INIT_USB_HELLO_FAILED;
		goto out;
	}

	ret_val = TF_INIT_USB_INIT_SUCCESS;

out:
	return ret_val;
}

static void
_libthinkfinger_parse_scan_reply (libthinkfinger *tf, unsigned char *inbuf)
{
	if (tf == NULL) {
		printf ("Error: libthinkfinger not properly initialized.\n");
		goto out;
	}

	switch (inbuf[18]) {
	case 0x0c:
		tf->state = TF_STATE_SWIPE_0;
		break;
	case 0x0d: case 0x0e:
		switch (inbuf[18]-0x0c) {
		case 0x01:
			tf->state = TF_STATE_SWIPE_1;
			break;
		case 0x02:
			tf->state = TF_STATE_SWIPE_2;
			break;
		default:
			break;
		}
		break;
	case 0x20:
		tf->state = TF_STATE_SWIPE_SUCCESS;
		break;
	case 0x00:
		tf->state = TF_STATE_ENROLL_SUCCESS;
		break;
	case 0x1c:
	case 0x1e:
	case 0x24:
	case 0x0b:
		tf->state = TF_STATE_SWIPE_FAILED;
		break;
	default:
#ifdef LIBTHINKFINGER_DEBUG
		printf ("Unknown state 0x%x\n", inbuf[18]);
#endif
		break;
	}

out:
	return;
}

static int
_libthinkfinger_store_fingerprint (libthinkfinger *tf, unsigned char *data)
{
	int ret_val = -1;

	if ((tf == NULL) || (tf->fd < 0)) {
		printf ("Error: libthinkfinger not properly initialized.\n");
		goto out;
	}

	if (write (tf->fd, data+18, 0x40-18) < 0) {
		perror ("Error");
		goto out;
	}

	tf->write_fingerprint = true;
	ret_val = 0;
out:
	return ret_val;
}

/* returns 1 if it understood the packet */
static int
_libthinkfinger_parse (libthinkfinger *tf, unsigned char *inbuf)
{
	int ret_val = -1;

	libthinkfinger_state state = tf->state;
	const char fingerprint_is[] = {
		0x00, 0x00, 0x00, 0x02, 0x12, 0xff, 0xff, 0xff,
		0xff
	};

	if (tf == NULL) {
		printf ("Error: libthinkfinger not properly initialized.\n");
		goto out;
	}

	switch (inbuf[7]) {
	case 0x28:
		if (!memcmp(inbuf+9, fingerprint_is, 9)) {
			ret_val = _libthinkfinger_store_fingerprint (tf, inbuf);
			if (ret_val < 0)
				tf->state = TF_STATE_ACQUIRE_FAILED;
			break;
		}
		switch (inbuf[6]) {
		case 0x07:
			tf->state = TF_STATE_COMM_FAILED;
			break;
		case 0x0b:
			tf->state = TF_STATE_VERIFY_FAILED;
			break;
		case 0x13:
			switch (inbuf[14]) {
			case 0x00:
				tf->state = TF_STATE_VERIFY_FAILED;
				break;
			case 0x01:
				tf->state = TF_STATE_VERIFY_SUCCESS;
				break;
			}
			break;
		case 0x14:
			_libthinkfinger_parse_scan_reply (tf, inbuf);
			break;
		default:
			ret_val = 0;
			break;
		}
		ret_val = 1;
		break;
	default:
		ret_val = 0;
	}

	if (tf->state != state && tf->cb != NULL)
		tf->cb (tf->state, tf->cb_data);
out:
	return ret_val;
}

#define SILENT 1
#define PARSE 2

static void
_libthinkfinger_ask_scanner_raw (libthinkfinger *tf, int flags, char *ctrldata, int len)
{
	int real_len;
	unsigned char inbuf[10240];

	if (tf == NULL) {
		printf ("Error: libthinkfinger not properly initialized.\n");
		goto out;
	}

	if (_libthinkfinger_task_running (tf) == false) {
		goto out;
	}

	real_len = usb_bulk_read (tf->usb_handle, 0x01, (char *) inbuf, 0x40, USB_TIMEOUT);
	if (tf->write_fingerprint == true) {
		if ((inbuf[0] == 0x43) && (inbuf[1] == 0x69)) {
			tf->write_fingerprint = false;
			tf->state = TF_STATE_ACQUIRE_SUCCESS;
			if (tf->cb != NULL)
				tf->cb (tf->state, tf->cb_data);
			goto out_result;
		} else {
			if (write (tf->fd, inbuf, real_len) < 0)
				perror ("Error");
		}
	}
	if (flags & PARSE) {
		if (_libthinkfinger_parse (tf, inbuf))
			flags |= SILENT;
	}

	*((short *) (ctrldata+len-2)) = udf_crc ((u8*)&(ctrldata[4]), len-6, 0);

	usb_bulk_write (tf->usb_handle, 0x02, (char *) ctrldata, len, USB_TIMEOUT);

out_result:
	switch (tf->state) {
	case TF_STATE_ACQUIRE_SUCCESS:
	case TF_STATE_ACQUIRE_FAILED:
	case TF_STATE_VERIFY_SUCCESS:
	case TF_STATE_VERIFY_FAILED:
	case TF_STATE_COMM_FAILED: {
		_libthinkfinger_task_stop (tf);
		break;
	}
	default:
		break;
	}

out:
	return;
}

static void
_libthinkfinger_init (libthinkfinger *tf)
{
	int i = 0;

	_libthinkfinger_usb_deinit (tf);
	_libthinkfinger_usb_init (tf);
	_libthinkfinger_task_start (tf, TF_TASK_INIT);
	do {
		_libthinkfinger_ask_scanner_raw (tf, SILENT, init[i].data, init[i].len);
	} while (init[++i].data);
	_libthinkfinger_task_stop (tf);
	_libthinkfinger_usb_deinit (tf);
	_libthinkfinger_usb_init (tf);
}

int
libthinkfinger_verify (libthinkfinger *tf)
{
	int ret_val = -1;
	int header = 13*3-1;
	int filesize;

	if (tf == NULL) {
		printf ("Error: libthinkfinger not properly initialized.\n");
		goto out;
	}

	tf->fd = open (tf->file, O_RDONLY);
	if (tf->fd < 0) {
		printf ("Error: file descriptor not set.\n");
		goto out;
	}

	filesize = read (tf->fd, ctrlbuf+header, 10240);
	filesize -= 2; // HACK!
	*((short *) (ctrlbuf+8)) = filesize + 28;
	ctrlbuf[5] = (filesize+20511) >> 8;
	ctrlbuf[6] = (filesize+20511) & 0xff;
	ctrlbuf[header+filesize] = 0x4f;
	ctrlbuf[header+filesize+1] = 0x47;

	_libthinkfinger_init (tf);
	_libthinkfinger_task_start (tf, TF_TASK_VERIFY);
	_libthinkfinger_ask_scanner_raw (tf, 0, ctrlbuf, header+filesize+2);
	while (_libthinkfinger_task_running (tf) == true) {
		_libthinkfinger_ask_scanner_raw (tf, PARSE, scan_sequence_one, sizeof(scan_sequence_one));
		_libthinkfinger_ask_scanner_raw (tf, PARSE, scan_sequence_two, sizeof(scan_sequence_two));
	}
	_libthinkfinger_task_stop (tf);

	switch (tf->state) {
	case TF_STATE_VERIFY_SUCCESS:
		ret_val = TF_RESULT_VERIFY_SUCCESS;
		break;
	case TF_STATE_VERIFY_FAILED:
		ret_val = TF_RESULT_VERIFY_FAILED;
		break;
	case TF_STATE_COMM_FAILED:
		ret_val = TF_RESULT_COMM_FAILED;
		break;
	default:
		break;
	}

	if (close (tf->fd) == 0)
		tf->fd = 0;
out:
	return ret_val;
}

int
libthinkfinger_acquire (libthinkfinger *tf)
{
	int ret_val = -1;

	if (tf == NULL) {
		printf ("Error: libthinkfinger not properly initialized.\n");
		goto out;
	}

	tf->fd = open (tf->file, O_RDWR | O_CREAT, 0600);
	if (tf->fd < 0) {
		printf ("Error: file descriptor not set.\n");
		goto out;
	}

	_libthinkfinger_init (tf);
	_libthinkfinger_task_start (tf, TF_TASK_ACQUIRE);
	_libthinkfinger_ask_scanner_raw (tf, SILENT, enroll_init, sizeof(enroll_init));
	while (_libthinkfinger_task_running (tf) == true) {
		_libthinkfinger_ask_scanner_raw (tf, PARSE, scan_sequence_one, sizeof(scan_sequence_one));
		_libthinkfinger_ask_scanner_raw (tf, PARSE, scan_sequence_two, sizeof(scan_sequence_two));
	}
	_libthinkfinger_task_stop (tf);
	switch (tf->state) {
	case TF_STATE_ACQUIRE_SUCCESS:
		ret_val = TF_RESULT_ACQUIRE_SUCCESS;
		break;
	case TF_STATE_ACQUIRE_FAILED:
		ret_val = TF_RESULT_ACQUIRE_FAILED;
		break;
	case TF_STATE_COMM_FAILED:
		ret_val = TF_RESULT_COMM_FAILED;
		break;
	default:
		break;
	}

	close (tf->fd);
out:
	return ret_val;
}

int
libthinkfinger_set_file (libthinkfinger *tf, const char *file)
{
	int ret_val = -1;

	if (tf == NULL) {
		printf ("Error: libthinkfinger not properly initialized.\n");
		goto out;
	}

	tf->file = file;
	ret_val = 0;
out:
	return ret_val;
}

int
libthinkfinger_set_callback (libthinkfinger *tf, libthinkfinger_state_cb cb, void *cb_data)
{
	int ret_val = -1;

	if (tf == NULL) {
		printf ("Error: libthinkfinger not properly initialized.\n");
		goto out;
	}

	tf->cb = cb;
	tf->cb_data = cb_data;
	ret_val = 0;
out:
	return ret_val;
}

libthinkfinger *
libthinkfinger_new (libthinkfinger_init_status *init_status)
{
	libthinkfinger *tf;

	tf = calloc(1, sizeof(libthinkfinger));

	if (tf == NULL) {
		/* failed to allocate memory */
		*init_status = TF_INIT_NO_MEMORY;
		goto out;
	}

	tf->usb_handle = NULL;
	tf->file = NULL;
	tf->fd = -1;
	tf->write_fingerprint = false;
	tf->task = TF_TASK_UNDEFINED;
	tf->task_running = false;
	tf->state = TF_STATE_INITIAL;
	tf->cb = NULL;
	tf->cb_data = NULL;

	*init_status = _libthinkfinger_usb_init (tf);
	if (*init_status != TF_INIT_USB_INIT_SUCCESS)
		goto outfree;
	else {
		*init_status = TF_INIT_SUCCESS;
		goto out;
	}

outfree:
	libthinkfinger_free (tf);
	tf = NULL;

out:
	return tf;
}

void
libthinkfinger_free (libthinkfinger *tf)
{
	if (tf == NULL) {
		printf ("Error: libthinkfinger not properly initialized.\n");
		goto out;
	}

	_libthinkfinger_usb_deinit (tf);

	if (tf->fd)
		close (tf->fd);

	free(tf);
out:
	return;
}

/** @mainpage libthinkfinger
 *
 * \section sec_intro Introduction
 *
 * ThinkFinger is a driver for the SGS Thomson Microelectronics fingerprint reader
 * found in most IBM/Lenovo ThinkPads.
 */

