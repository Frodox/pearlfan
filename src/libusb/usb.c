/*
 * The GPL License (GPLv3)
 *
 * Copyright © 2016 Thomas "Ventto" Venriès  <thomas.venries@gmail.com>
 *
 * Pearlfan is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Pearlfan is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Pearlfan.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "convert.h"
#include "usb.h"

#define PFAN_DISPLAY_MAX    8

libusb_device_handle *pfan_open(libusb_context *ctx, int vid, int pid)
{
	libusb_device_handle *dev_handle =
		libusb_open_device_with_vid_pid(ctx, vid, pid);

	if (!dev_handle) {
		fprintf(stderr, "Device can not be opened or found.\n");
		fprintf(stderr, "You may need permission.\n\n");
		return NULL;
	}

	if (libusb_kernel_driver_active(dev_handle, 0) == 1) {
		if (libusb_detach_kernel_driver(dev_handle, 0) != 0) {
			libusb_close(dev_handle);
			fprintf(stderr, "Can not dettach the device from the driver.\n\n");
			return NULL;
		}
	}

	if (libusb_claim_interface(dev_handle, 0) < 0) {
		libusb_close(dev_handle);
		fprintf(stderr, "Cannot claim the interface.\n\n");
		return NULL;
	}

	return dev_handle;
}

void pfan_close(libusb_context *ctx, libusb_device_handle *dev_handle)
{
	libusb_close(dev_handle);
	libusb_exit(ctx);
}

static int send(libusb_device_handle *dev, void *data)
{
	uint8_t buf[8];
	int bytes = 0;
	int l = 0;
	int ret;

	memset((void *)buf, 0, 8);
	buf[7] = 0x2;

	bytes = libusb_control_transfer(dev, 0x21, 9, 0x200, 0, data, 8, 1000);
	if (bytes < 0)
		return bytes;

	if ((ret = libusb_interrupt_transfer(dev, 0x81, buf, 8, &l, 1000)) < 0)
		return ret;
	return bytes;
}

int pfan_send(libusb_device_handle *dev_handle, int img_n,
		      uint8_t effects[PFAN_DISPLAY_MAX][3],
		      uint16_t displays[PFAN_DISPLAY_MAX][PFAN_IMG_W])
{
	int bytes = 0;
	int ret = 0;

	for (uint8_t i = 0; i < img_n; i++) {
		uint64_t effect = pfan_convert_effect(i, effects[i]);

		ret = send(dev_handle, &effect);
		if (ret < 0)
			return ret;
		else if (ret < 8)
			break;
		bytes += 8;

		for (uint8_t j = 0; j < 39; ++j) {
			ret = send(dev_handle, &displays[i][j * 4]);
			if (ret < 0)
				return ret;
			else if (ret < 8)
				break;
			bytes += 8;
		}
	}

	return bytes;
}
