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

static int send(libusb_device_handle *dev_handle, void *data)
{
	uint8_t buf[8];
	int l = 0;

	memset((void *)buf, 0, 8);
	buf[7] = 0x2;

	if (libusb_control_transfer(dev_handle, 0x21, 9, 0x200, 0,
				data, 8, 1000) <= 0)
		return -1;
	return libusb_interrupt_transfer(dev_handle, 0x81, buf, 8, &l, 1000);
}

int pfan_send(libusb_device_handle *dev_handle, int img_n,
		      uint8_t effects[PFAN_DISPLAY_MAX][3],
		      uint16_t displays[PFAN_DISPLAY_MAX][PFAN_IMG_W])
{
	for (uint8_t i = 0; i < img_n; i++) {
		uint64_t effect = pfan_convert_effect(i, effects[i]);

		if (send(dev_handle, &effect) == -1)
			return 1;
		for (uint8_t j = 0; j < 39; ++j)
			if (send(dev_handle, &displays[i][j * 4]) == -1)
				return 1;
	}

	return 0;
}
