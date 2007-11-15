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
 *   TODO: this is not true for all distributions
 *   Note that you need to be root to use this.
 */

#include "libthinkfinger.h"
#include "libthinkfinger-crc.h"

#define USB_VENDOR_ID     0x0483
#define USB_PRODUCT_ID    0x2016
#define USB_TIMEOUT       5000
#define USB_WR_EP         0x02
#define USB_RD_EP         0x81
#define DEFAULT_BULK_SIZE 0x40
#define INITIAL_SEQUENCE  0x60

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

/* TODO: dynamic */
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

/* TODO: dynamic */
static char init_end[120] = {
	0x43, 0x69, 0x61, 0x6f, 0x00, 0x40, 0x6f, 0x28,
	0x6c, 0x00, 0x00, 0x00, 0x0b, 0x04, 0x03, 0x00,
	0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x03, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf4, 0x01,
	0x00, 0x00, 0x64, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00,
	0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00,
	0x0a, 0x00, 0x64, 0x00, 0xf4, 0x01, 0x32, 0x00,
	0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0xd6, 0x66
};

static char deinit[10] = {
	0x43, 0x69, 0x61, 0x6f, 0x07, 0x00, 0x01, 0x00,
	0x1c, 0x62
};

static char device_busy[9] = {
	0x43, 0x69, 0x61, 0x6f, 0x09, 0x00, 0x00, 0x91,
	0x9e
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

static char ctrlbuf[1024] = {
	0x43, 0x69, 0x61, 0x6f, 0x00, 0x51, 0x0b, 0x28,
	0xb8, 0x00, 0x00, 0x00, 0x03, 0x02, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xc0, 0xd4, 0x01, 0x00, 0x20, 0x00,
	0x00, 0x00, 0x03
};

static char enroll_init[23] = {
	0x43, 0x69, 0x61, 0x6f, 0x00, 0x50, 0x0e, 0x28,
	0x0b, 0x00, 0x00, 0x00, 0x02, 0x02, 0xc0, 0xd4,
	0x01, 0x00, 0x04, 0x00, 0x08, 0x0f, 0x86
};

static unsigned char scan_sequence[17] = {
	0x43, 0x69, 0x61, 0x6f, 0x00, 0xff, 0x08, 0x28,
	0x05, 0x00, 0x00, 0x00, 0x00, 0x30, 0x01, 0xff,
	0xff
};

static unsigned char termination_request = 0x01;

struct libthinkfinger_s {
	struct sigaction sigint_action;
	struct sigaction sigint_action_old;
	struct usb_dev_handle *usb_dev_handle;
	const char *file;
	int fd;

	pthread_mutex_t usb_deinit_mutex;
	libthinkfinger_task task;
	_Bool task_running;
	_Bool result_pending;
	unsigned char next_sequence;

	libthinkfinger_state state;
	libthinkfinger_state_cb cb;
	void *cb_data;
};

static void sigint_handler (int unused, siginfo_t *sinfo, void *data) {
	termination_request = 0x00;
	return;
}

static int _libthinkfinger_set_sigint (libthinkfinger *tf)
{
	int retval;
	
	tf->sigint_action.sa_sigaction = &sigint_handler;
	retval = sigaction (SIGINT, &tf->sigint_action, &tf->sigint_action_old);

	return retval;
}

static int _libthinkfinger_restore_sigint (libthinkfinger *tf)
{
	int retval = 0;

	retval = sigaction(SIGINT, &tf->sigint_action_old, NULL);

	return retval;
}

static _Bool _libthinkfinger_result_pending (libthinkfinger *tf)
{
	return tf->result_pending;
}

static void _libthinkfinger_set_result_pending (libthinkfinger *tf, _Bool pending)
{
	tf->result_pending = pending;
}

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

static libthinkfinger_result _libthinkfinger_get_result (libthinkfinger_state state)
{
	libthinkfinger_result retval;
	switch (state) {
		case TF_STATE_ACQUIRE_SUCCESS:
			retval = TF_RESULT_ACQUIRE_SUCCESS;
			break;
		case TF_STATE_ACQUIRE_FAILED:
			retval = TF_RESULT_ACQUIRE_FAILED;
			break;
		case TF_STATE_VERIFY_SUCCESS:
			retval = TF_RESULT_VERIFY_SUCCESS;
			break;
		case TF_STATE_VERIFY_FAILED:
			retval = TF_RESULT_VERIFY_FAILED;
			break;
		case TF_STATE_OPEN_FAILED:
			retval = TF_RESULT_OPEN_FAILED;
			break;
		case TF_STATE_SIGINT:
			retval = TF_RESULT_SIGINT;
			break;
		case TF_STATE_USB_ERROR:
			retval = TF_RESULT_USB_ERROR;
			break;
		case TF_STATE_COMM_FAILED:
			retval = TF_RESULT_COMM_FAILED;
			break;
		default:
			retval = TF_RESULT_UNDEFINED;
			break;
	}

	return retval;
}

#ifdef USB_DEBUG
static void usb_dump (const char *func, unsigned char *bytes, int req_size, int size)
{
	if (size >= 0) {
		fprintf (stderr, "\n%s\t(0x%x/0x%x): ", func, req_size, size);
		while (size-- > 0) {
			fprintf(stderr, "%2.2x", *bytes);
			bytes++;
		}
		fprintf (stderr, "\n");
	} else
		fprintf (stderr, "Error: %s (%i, %s)\n", func, size, usb_strerror ());

	return;
}
#endif

static int _libthinkfinger_usb_hello (struct usb_dev_handle *handle)
{
	int retval = -1;
	char dummy[] = "\x10";

	/* SET_CONFIGURATION 1 -- should not be relevant */
	retval = usb_control_msg (handle,	 // usb_dev_handle *dev
				   0x00000000,	 // int requesttype
				   0x00000009,	 // int request
				   0x001,	 // int value
				   0x000,	 // int index
				   dummy,	 // char *bytes
				   0x00000000,	 // int size
				   USB_TIMEOUT); // int timeout
	if (retval < 0)
		goto out;
	retval = usb_control_msg (handle,	 // usb_dev_handle *dev
				   0x00000040,	 // int requesttype
				   0x0000000c,	 // int request
				   0x100,	 // int value
				   0x400,	 // int index
				   dummy,	 // char *bytes
				   0x00000001,	 // int size
				   USB_TIMEOUT); // int timeout

out:
	return retval;
}

static int _libthinkfinger_usb_write (libthinkfinger *tf, char *bytes, int size) {
	int usb_retval = -1;

	if (tf->usb_dev_handle == NULL) {
#ifdef USB_DEBUG
		fprintf (stderr, "_libthinkfinger_usb_write error: USB handle is NULL.\n");
#endif
		goto out;
	}

	usb_retval = usb_bulk_write (tf->usb_dev_handle, USB_WR_EP, bytes, size, USB_TIMEOUT);
	if (usb_retval >= 0 && usb_retval != size)
		fprintf (stderr, "Warning: usb_bulk_write expected to write 0x%x (wrote 0x%x bytes).\n",
			 size, usb_retval);

#ifdef USB_DEBUG
	usb_dump ("usb_bulk_write", (unsigned char*) bytes, size, usb_retval);
#endif
out:
	return usb_retval;
}

static int _libthinkfinger_usb_read (libthinkfinger *tf, char *bytes, int size) {
	int usb_retval = -1;

	if (tf->usb_dev_handle == NULL) {
#ifdef USB_DEBUG
		fprintf (stderr, "_libthinkfinger_usb_read error: USB handle is NULL.\n");
#endif
		goto out;
	}

	usb_retval = usb_bulk_read (tf->usb_dev_handle, USB_RD_EP, bytes, size, USB_TIMEOUT);
	if (usb_retval >= 0 && usb_retval != size)
		fprintf (stderr, "Warning: usb_bulk_read expected to read 0x%x (read 0x%x bytes).\n",
			 size, usb_retval);
#ifdef USB_DEBUG
	usb_dump ("usb_bulk_read", (unsigned char*) bytes, size, usb_retval);
#endif
out:
	return usb_retval;
}

static void _libthinkfinger_usb_flush (libthinkfinger *tf)
{
	char buf[64];

	_libthinkfinger_usb_read (tf, buf, DEFAULT_BULK_SIZE);

	return;
}

static struct usb_device *_libthinkfinger_usb_device_find (void)
{
	struct usb_bus *usb_bus;
	struct usb_device *dev = NULL;

	usb_init ();
	usb_find_busses ();
	usb_find_devices ();

	/* TODO: Support systems with two fingerprint readers */
	for (usb_bus = usb_busses; usb_bus; usb_bus = usb_bus->next) {
		for (dev = usb_bus->devices; dev; dev = dev->next) {
			if ((dev->descriptor.idVendor == USB_VENDOR_ID) &&
			    (dev->descriptor.idProduct == USB_PRODUCT_ID)) {
				goto out;
			}
		}
	}
out:
	return dev;
}

static void _libthinkfinger_usb_deinit_lock (libthinkfinger *tf)
{
	if (pthread_mutex_lock (&tf->usb_deinit_mutex) < 0)
		fprintf (stderr, "pthread_mutex_lock failed: (%s).\n", strerror (errno));
	return;
}

static void _libthinkfinger_usb_deinit_unlock (libthinkfinger *tf)
{
	if (pthread_mutex_unlock (&tf->usb_deinit_mutex) < 0)
		fprintf (stderr, "pthread_mutex_unlock failed: (%s).\n", strerror (errno));
	return;
}

static void _libthinkfinger_usb_deinit (libthinkfinger *tf)
{
	int usb_retval;

#ifdef USB_DEBUG
fprintf (stderr, "USB deinitialization...\n");
#endif

	_libthinkfinger_usb_deinit_lock (tf);
	if (tf->usb_dev_handle == NULL) {
#ifdef USB_DEBUG
		fprintf (stderr, "No USB handle.\n");
#endif

		goto out;
	}

	while (_libthinkfinger_task_running (tf) == true) {
#ifdef USB_DEBUG
		fprintf (stderr, "USB task running...waiting.\n");
#endif

		termination_request = 0x00;
		usleep (50000);
	}

#ifdef USB_DEBUG
	fprintf (stderr, "sending deinitialization sequence.\n");
#endif

	usb_retval = _libthinkfinger_usb_write (tf, deinit, sizeof(deinit));
	if (usb_retval < 0 && usb_retval != -ETIMEDOUT)
		goto usb_close;
	 _libthinkfinger_usb_flush (tf);

usb_close:
	usb_release_interface (tf->usb_dev_handle, 0);
	usb_close (tf->usb_dev_handle);
	tf->usb_dev_handle = NULL;
out:
	_libthinkfinger_usb_deinit_unlock (tf);

#ifdef USB_DEBUG
fprintf (stderr, "USB deinitialization finished.\n");
#endif
	return;
}

static libthinkfinger_init_status _libthinkfinger_usb_init (libthinkfinger *tf)
{
	libthinkfinger_init_status retval = TF_INIT_UNDEFINED;
	struct usb_device *usb_dev;

#ifdef USB_DEBUG
	fprintf (stderr, "USB initialization...\n");
#endif

	usb_dev = _libthinkfinger_usb_device_find ();
	if (usb_dev == NULL) {
#ifdef USB_DEBUG
		fprintf (stderr, "USB error (device not found).\n");
#endif
		retval = TF_INIT_USB_DEVICE_NOT_FOUND;
		goto out;
	}

	tf->usb_dev_handle = usb_open (usb_dev);
	if (tf->usb_dev_handle == NULL) {
#ifdef USB_DEBUG
		fprintf (stderr, "USB error (did not get handle).\n");
#endif
		retval = TF_INIT_USB_OPEN_FAILED;
		goto out;
	}

	if (usb_claim_interface (tf->usb_dev_handle, 0) < 0) {
#ifdef USB_DEBUG
		fprintf (stderr, "USB error (%s).\n", usb_strerror ());
#endif
		retval = TF_INIT_USB_CLAIM_FAILED;
		goto out;
	}

	if (_libthinkfinger_usb_hello (tf->usb_dev_handle) < 0) {
#ifdef USB_DEBUG
		fprintf (stderr, "USB error (sending hello failed).\n");
#endif
		retval = TF_INIT_USB_HELLO_FAILED;
		goto out;
	}

#ifdef USB_DEBUG
	fprintf (stderr, "USB initialization successful.\n");
#endif

	retval = TF_INIT_USB_INIT_SUCCESS;
out:
	return retval;
}

static void _libthinkfinger_parse_scan_reply (libthinkfinger *tf, unsigned char *inbuf)
{
	if (tf == NULL) {
		fprintf (stderr, "Error: libthinkfinger not properly initialized.\n");
		goto out;
	}

	switch (inbuf[18]) {
		case 0x0c:
			tf->state = TF_STATE_SWIPE_0;
			break;
		case 0x0d:
		case 0x0e:
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
			fprintf (stderr, "Unknown state 0x%x\n", inbuf[18]);
#endif
			break;
	}

out:
	return;
}

static int _libthinkfinger_store_fingerprint (libthinkfinger *tf, unsigned char *data)
{
	char inbuf[1024];
	int retval = -1;
	int usb_retval;
	int len;

	if ((tf == NULL) || (tf->fd < 0)) {
		fprintf (stderr, "Error: libthinkfinger not properly initialized.\n");
		goto out;
	}

	if (write (tf->fd, data+18, 0x40-18) < 0) {
		fprintf (stderr, "Error: %s.\n", strerror (errno));
		goto out;
	}

	len = ((data[5] & 0x0f) << 8) + data[6] - 0x37;
	usb_retval = _libthinkfinger_usb_read (tf, inbuf, len);
	if (usb_retval != len)
		fprintf (stderr, "Warning: Expected 0x%x bytes but read 0x%x).\n", len, usb_retval);
	if (write (tf->fd, inbuf, usb_retval) < 0)
		fprintf (stderr, "Error: %s.\n", strerror (errno));
	else
		retval = 0;

	/* reset termination_request */
	termination_request = 0x01;
out:
	return retval;
}

/* returns 1 if it understood the packet */
static int _libthinkfinger_parse (libthinkfinger *tf, unsigned char *inbuf)
{
	int retval = -1;

	libthinkfinger_state state = tf->state;
	const char fingerprint_is[] = {
		0x00, 0x00, 0x00, 0x02, 0x12, 0xff, 0xff, 0xff,
		0xff
	};

	if (tf == NULL) {
		fprintf (stderr, "Error: libthinkfinger not properly initialized.\n");
		goto out;
	}

	_libthinkfinger_set_result_pending (tf, false);

	switch (inbuf[7]) {
		case 0x28:
			tf->next_sequence = (inbuf[5] + 0x20) & 0x00ff;
			if (tf->state == TF_STATE_ENROLL_SUCCESS && !memcmp(inbuf+9, fingerprint_is, 9)) {
				retval = _libthinkfinger_store_fingerprint (tf, inbuf);
				if (retval < 0)
					tf->state = TF_STATE_ACQUIRE_FAILED;
				else
					tf->state = TF_STATE_ACQUIRE_SUCCESS;
				_libthinkfinger_task_stop (tf);
				break;
			}
			switch (inbuf[6]) {
				case 0x07:
					tf->state = TF_STATE_COMM_FAILED;
					_libthinkfinger_task_stop (tf);
					break;
				case 0x0b:
					tf->state = TF_STATE_VERIFY_FAILED;
					_libthinkfinger_task_stop (tf);
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
					_libthinkfinger_task_stop (tf);
					break;
				case 0x14:
					_libthinkfinger_parse_scan_reply (tf, inbuf);
					break;
				default:
					retval = 0;
					break;
			}
			retval = 1;
			break;
		case 0xa1:
			/* device is busy, result pending */
			_libthinkfinger_set_result_pending (tf, true);
			retval = 1;
		default:
			retval = 0;
	}

	if (tf->state != state && tf->cb != NULL)
		tf->cb (tf->state, tf->cb_data);
out:
	return retval;
}

#define SILENT 1
#define PARSE 2

static void _libthinkfinger_ask_scanner_raw (libthinkfinger *tf, int flags, char *ctrldata, int read_size, int write_size)
{
	int usb_retval;
	unsigned char inbuf[10240];

	if (tf == NULL) {
		fprintf (stderr, "Error: libthinkfinger not properly initialized.\n");
		goto out;
	}

	if (_libthinkfinger_task_running (tf) == false)
		goto out;

	_libthinkfinger_set_result_pending (tf, true);
	while (_libthinkfinger_result_pending (tf) == true) {
		usb_retval = _libthinkfinger_usb_read (tf, (char *)&inbuf, read_size);
		if (usb_retval < 0 && usb_retval != -ETIMEDOUT)
			goto out_usb_error;

		if (flags & PARSE) {
			if (_libthinkfinger_parse (tf, inbuf))
				flags |= SILENT;
			if (_libthinkfinger_task_running (tf) == false)
				goto out_result;
			if (_libthinkfinger_result_pending (tf) == true) {
				_libthinkfinger_usb_write (tf, (char *)device_busy, sizeof(device_busy));
				if (usb_retval < 0 && usb_retval != -ETIMEDOUT)
					goto out_usb_error;
			}
		} else {
			_libthinkfinger_set_result_pending (tf, false);
		}
	}

	if (termination_request == 0x00) {
		ctrldata[14] = termination_request;
		tf->state = TF_STATE_SIGINT;
	}

	*((short *) (ctrldata+write_size-2)) = udf_crc ((u8*)&(ctrldata[4]), write_size-6, 0);
	usb_retval = _libthinkfinger_usb_write (tf, (char *)ctrldata, write_size);
	if (usb_retval < 0 && usb_retval != -ETIMEDOUT)
		goto out_usb_error;
	else {
		goto out_result;
	}

out_usb_error:
	tf->state = TF_STATE_USB_ERROR;

out_result:
	switch (tf->state) {
		case TF_STATE_ACQUIRE_SUCCESS:
		case TF_STATE_ACQUIRE_FAILED:
		case TF_STATE_VERIFY_SUCCESS:
		case TF_STATE_VERIFY_FAILED:
		case TF_STATE_SIGINT:
		case TF_STATE_USB_ERROR:
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

static libthinkfinger_init_status _libthinkfinger_init (libthinkfinger *tf)
{
	libthinkfinger_init_status retval = TF_INIT_UNDEFINED;
	int i = 0;

	retval = _libthinkfinger_usb_init (tf);
	if (retval != TF_INIT_USB_INIT_SUCCESS)
		goto out;

	_libthinkfinger_task_start (tf, TF_TASK_INIT);
	do {
		_libthinkfinger_ask_scanner_raw (tf, SILENT, init[i].data, DEFAULT_BULK_SIZE, init[i].len);
	} while (init[++i].data);
	_libthinkfinger_usb_flush (tf);
	_libthinkfinger_ask_scanner_raw (tf, SILENT, (char *)&init_end, 0x34, sizeof(init_end));
	_libthinkfinger_task_stop (tf);

	retval = TF_INIT_SUCCESS;
out:
	return retval;
}

static void _libthinkfinger_scan (libthinkfinger *tf) {
	tf->next_sequence = INITIAL_SEQUENCE;
	_libthinkfinger_set_sigint (tf);
	while (_libthinkfinger_task_running (tf)) {
		scan_sequence[5] = tf->next_sequence;
		_libthinkfinger_ask_scanner_raw (tf, PARSE, (char *)scan_sequence, DEFAULT_BULK_SIZE, sizeof (scan_sequence));
	}

	if (termination_request == 0x00) {
		_libthinkfinger_usb_flush (tf);
		goto out;
	}

out:
	_libthinkfinger_restore_sigint (tf);
	return;
}

static void _libthinkfinger_verify_run (libthinkfinger *tf)
{
	int header = 13*3-1;
	int filesize;

	tf->fd = open (tf->file, O_RDONLY | O_NOFOLLOW);
	if (tf->fd < 0) {
		fprintf (stderr, "Error while opening \"%s\": %s.\n", tf->file, strerror (errno));
		_libthinkfinger_usb_flush (tf);
		tf->state = TF_STATE_OPEN_FAILED;
		goto out;
	}

	filesize = read (tf->fd, ctrlbuf+header, sizeof(ctrlbuf)-header);
	*((short *) (ctrlbuf+8)) = filesize + 28;
	ctrlbuf[5] = (filesize+20511) >> 8;
	ctrlbuf[6] = (filesize+20511) & 0xff;
	ctrlbuf[header+filesize] = 0x4f;
	ctrlbuf[header+filesize+1] = 0x47;

	_libthinkfinger_task_start (tf, TF_TASK_VERIFY);
	_libthinkfinger_ask_scanner_raw (tf, SILENT, ctrlbuf, DEFAULT_BULK_SIZE, header+filesize+2);
	_libthinkfinger_scan (tf);

	if (close (tf->fd) == 0)
		tf->fd = 0;
out:
	return;
}

libthinkfinger_result libthinkfinger_verify (libthinkfinger *tf)
{
	libthinkfinger_result retval = TF_RESULT_UNDEFINED;

	if (tf == NULL) {
		fprintf (stderr, "Error: libthinkfinger not properly initialized.\n");
		goto out;
	}
	
	_libthinkfinger_init (tf);
	_libthinkfinger_verify_run (tf);
	retval = _libthinkfinger_get_result (tf->state);
out:
	return retval;
}

static void _libthinkfinger_acquire_run (libthinkfinger *tf)
{
	tf->fd = open (tf->file, O_RDWR | O_CREAT | O_NOFOLLOW, 0600);
	if (tf->fd < 0) {
		fprintf (stderr, "Error while opening \"%s\": %s.\n", tf->file, strerror (errno));
		_libthinkfinger_usb_flush (tf);
		tf->state = TF_STATE_OPEN_FAILED;
		goto out;
	}

	_libthinkfinger_task_start (tf, TF_TASK_ACQUIRE);
	_libthinkfinger_ask_scanner_raw (tf, SILENT, enroll_init, DEFAULT_BULK_SIZE, sizeof(enroll_init));
	_libthinkfinger_scan (tf);

	if (close (tf->fd) == 0)
		tf->fd = 0;
out:
	return;
}

libthinkfinger_result libthinkfinger_acquire (libthinkfinger *tf)
{
	libthinkfinger_result retval = TF_RESULT_UNDEFINED;

	if (tf == NULL) {
		fprintf (stderr, "Error: libthinkfinger not properly initialized.\n");
		goto out;
	}

	_libthinkfinger_init (tf);
	_libthinkfinger_acquire_run (tf);
	retval = _libthinkfinger_get_result (tf->state);
out:
	return retval;
}

int libthinkfinger_set_file (libthinkfinger *tf, const char *file)
{
	int retval = -1;

	if (tf == NULL) {
		fprintf (stderr, "Error: libthinkfinger not properly initialized.\n");
		goto out;
	}

	tf->file = file;
	retval = 0;
out:
	return retval;
}

int libthinkfinger_set_callback (libthinkfinger *tf, libthinkfinger_state_cb cb, void *cb_data)
{
	int retval = -1;

	if (tf == NULL) {
		fprintf (stderr, "Error: libthinkfinger not properly initialized.\n");
		goto out;
	}

	tf->cb = cb;
	tf->cb_data = cb_data;
	retval = 0;
out:
	return retval;
}

libthinkfinger *libthinkfinger_new (libthinkfinger_init_status *init_status)
{
	libthinkfinger *tf = NULL;

	tf = calloc(1, sizeof(libthinkfinger));
	if (tf == NULL) {
		/* failed to allocate memory */
		*init_status = TF_INIT_NO_MEMORY;
		goto out;
	}


	tf->usb_dev_handle = NULL;
	tf->file = NULL;
	tf->fd = -1;
	tf->task = TF_TASK_UNDEFINED;
	tf->task_running = false;
	tf->state = TF_STATE_INITIAL;
	tf->cb = NULL;
	tf->cb_data = NULL;
	if (pthread_mutex_init (&tf->usb_deinit_mutex, NULL) < 0)
		fprintf (stderr, "pthread_mutex_init failed: (%s).\n", strerror (errno));

	if ((*init_status = _libthinkfinger_init (tf)) != TF_INIT_SUCCESS)
		goto out;

	_libthinkfinger_usb_flush (tf);
	_libthinkfinger_usb_deinit (tf);

	*init_status = TF_INIT_SUCCESS;
out:
	return tf;
}

void libthinkfinger_free (libthinkfinger *tf)
{
	if (tf == NULL) {
		fprintf (stderr, "Error: libthinkfinger not properly initialized.\n");
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

