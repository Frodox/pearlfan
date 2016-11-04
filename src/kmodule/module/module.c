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
#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/usb.h>

#include "convert.h"
#include "devinfo.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Thomas Venries <thomas.venries@gmail.com>");
MODULE_DESCRIPTION("Pearl USB LED Fan module");
MODULE_VERSION("v0.7");

static struct usb_device_id id_table[] = {
	{ USB_DEVICE(PFAN_VID, PFAN_PID) },
	{ },
};
MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_device *pfan_device;

static int send(struct usb_device *udev, void *data)
{
	char buf[8];
	int bytes = 0;
	int l = 0;
	int ret;

	memset(buf, 0, 8);
	buf[7] = 0x02;

	ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x09, 0x21, 0x200,
						0x00, data, 0x008, 10 * HZ);
	if (ret < 0)
		return ret;
	bytes += ret;

	ret = usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1), buf, 8, &l,
							10 * HZ);
	if (ret < 0)
		return ret;
	return bytes;
}

static int pfan_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int pfan_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t pfan_write(struct file *f,
		const char __user *buffer,
		size_t cnt,
		loff_t *off)
{
	struct pfan_data *data = NULL;
	u16 display[PFAN_IMG_W];
	int bytes = 0;
	int i;
	int j;
	int ret;

	if (cnt == 0) {
		pr_err("%s: buffer size is null !\n", __func__);
		return 0;
	}

	if (!buffer) {
		pr_err("%s: buffer is null but its size is not !\n", __func__);
		return -EINVAL;
	}

	if (cnt > sizeof(struct pfan_data)
			|| cnt < sizeof(struct pfan_data)) {
		pr_err("%s: the size's value is unexpected !\n", __func__);
		return -EINVAL;
	}

	data = (struct pfan_data *)buffer;

	for (i = 0; i < data->n; ++i) {
		u64 effect = pfan_convert_effect(i, data->effects[i]);

		ret = send(pfan_device, &effect);
		if (ret < 0)
			return ret;
		else if (ret < 8)
			break;
		bytes += ret;
		for (j = 0; j < 39; j++) {
			memset(&display, 0xFF, sizeof(display));
			pfan_convert_raster(i, data->images[i], display);
			ret = send(pfan_device, (u16 *)&display[j * 4]);
			if (ret < 0)
				return ret;
			else if (ret < 8)
				break;
			bytes += ret;
		}
	}

	return bytes;
}

static struct file_operations pfan_fops = {
	.open    = pfan_open,
	.release = pfan_release,
	.write   = pfan_write
};

static struct usb_class_driver pfan_class = {
	.name = PFAN_CLASSNAME,
	.fops = &pfan_fops,
	.minor_base = 0
};

static int pfan_probe(struct usb_interface *interface,
		const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(interface);
	int ret = usb_register_dev(interface, &pfan_class);

	if (ret < 0) {
		pr_warn("register_dev() error\n");
		return ret;
	}

	pfan_device = usb_get_dev(udev);

	dev_info(&interface->dev, "device now attached\n");

	return 0;
}

static void pfan_disconnect(struct usb_interface *interface)
{
	usb_deregister_dev(interface, &pfan_class);
	usb_put_dev(pfan_device);

	dev_info(&interface->dev, "device now disconnected\n");
}

static struct usb_driver pfan_driver = {
	.name       = "pfan",
	.probe      = pfan_probe,
	.disconnect = pfan_disconnect,
	.id_table   = id_table,
};

static int __init pfan_init(void)
{
	int ret = usb_register(&pfan_driver);

	if (ret < 0) {
		pr_err("unable to register the driver\n");
		return ret;
	}
	pr_info("%s: driver now registered.\n", __func__);
	return 0;
}

static void __exit pfan_exit(void)
{
	pr_info("driver now unregistered.\n");
	usb_deregister(&pfan_driver);
}

module_init(pfan_init);
module_exit(pfan_exit);
