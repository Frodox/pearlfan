/*
 * Linux driver for Led Fan by Dream Cheeky (version 0)
 */
#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/usb.h>

#include "includes/cfan_data.h"

MODULE_AUTHOR("Thomas Venries, Franklin Mathieu");
MODULE_DESCRIPTION("CheekyFan module, v0.1");
MODULE_LICENSE("GPL");

#define VENDOR_ID	0x0c45
#define PRODUCT_ID	0x7701

struct usb_cfan {
	struct usb_device	*udev;
	u64     cfg[8];			/* displays configuration */
	u16	displays[8][156];	/* displays's buffer */
	u8	displays_nb;		/* number of displays */
};

static struct usb_driver cfan_driver;
static struct usb_cfan *cfan;

/* Add the device to this driver's supported device list */
static struct usb_device_id id_table[] = {
	{ USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
	{ },
};
MODULE_DEVICE_TABLE(usb, id_table);

/*
 * Global array which helps to convert a given PBM raster to
 * a ventilator's display (see pbm_to_display function below).
 */
static u16 pbm_mask[11];

/* Initializes masks which represent eleven LEDs's activation */
static void pbm_masks_init(void)
{
	pbm_mask[10] = 0xFFF7;	/* led 11 */
	pbm_mask[9] = 0xFFFB;	/* led 10 */
	pbm_mask[8] = 0xFFFD;	/* led 9 */
	pbm_mask[7] = 0xFFFE;	/* led 8 */

	pbm_mask[6] = 0x7FFF;	/* led 7 */
	pbm_mask[5] = 0xBFFF;	/* led 6 */
	pbm_mask[4] = 0xCFFF;	/* led 5 */

	pbm_mask[3] = 0xFEFF;	/* led 4 */
	pbm_mask[2] = 0xF7FF;	/* led 3 */
	pbm_mask[1] = 0xFBFF;	/* led 2 */
	pbm_mask[0] = 0xFCFF;	/* led 1 */
}

/* Converts a given PBM raster to a ventilator's display */
static void pbm_to_display(unsigned char id,
			   char *raster,
			   uint16_t *display)
{
	unsigned char i;
	unsigned char j;
	unsigned char col_end;

	for (i = 0; i < 156; ++i) {
		col_end = 155 - i;
		for (j = 0; j < 11; ++j)
			if (raster[j * 156 + i] == 1)
				display[col_end] &= pbm_mask[10 - j];
	}
	for (i = 0; i < 156; ++i)
		pr_info("%x\n", display[i]);
}

/* Set effect config for a fan view */
static u64 set_config(unsigned char id,
		      unsigned char open_option,
		      unsigned char close_option,
		      unsigned char turn_option)
{
	/* Configuration:
	 * A0 10 <A><B> <C><D> 55 00 00 00
	 * [A] : OpenOption	[0 ; 5]
	 * [B] : CloseOption	[0 ; 5]
	 * [C] : TurnOption	(6 | c)
	 * [D] : Display ID	[0 ; 7]
	 * Warning: Reverse bytes sequence (stack behind ?) :
	 * 00 00 00 55 [CD] [AB] 10 A0
	 * [CDAB] : u16
	 */
	u16 cdab = 0;

	pr_info("cfan:%s: ID: %d\n", __func__, id);
	pr_info("cfan:%s: open option: %d\n", __func__, open_option);
	pr_info("cfan:%s: close option: %d\n", __func__, close_option);
	pr_info("cfan:%s: turn option: %d\n", __func__, turn_option);

	cdab |= close_option;
	cdab |= (open_option << 4);
	cdab |= (id << 8);
	cdab |= (turn_option << 12);

	return 0x00000055000010A0 | (cdab << 16);
}

static u8 write_letter(const unsigned char letter,
		       const unsigned char id,
		       const unsigned char column)
{
	unsigned char col = column;

	if (col)
		cfan->displays[id][col++] = 0xFFFF;

	pr_info("cfan:%s: detected letter: %x\n", __func__, letter);

	switch (letter) {
	case 'A':
		cfan->displays[id][col++] = 0x00FF;
		cfan->displays[id][col++] = 0x6EFF;
		cfan->displays[id][col++] = 0x6EFF;
		cfan->displays[id][col++] = 0x00FF;
		cfan->displays[id][col++] = 0xFEFF;
		break;
	case 'B':
		cfan->displays[id][col++] = 0x82FF;
		cfan->displays[id][col++] = 0x6CFF;
		cfan->displays[id][col++] = 0x6CFF;
		cfan->displays[id][col++] = 0x00FF;
		break;
	case 'C':
		cfan->displays[id][col++] = 0x7CFF;
		cfan->displays[id][col++] = 0x7CFF;
		cfan->displays[id][col++] = 0x7CFF;
		cfan->displays[id][col++] = 0x00FF;
		break;
	case 'D':
		cfan->displays[id][col++] = 0xC6FF;
		cfan->displays[id][col++] = 0xBAFF;
		cfan->displays[id][col++] = 0x7CFF;
		cfan->displays[id][col++] = 0x00FF;
		break;
	case 'P':
		cfan->displays[id][col++] = 0x9EFF;
		cfan->displays[id][col++] = 0x6EFF;
		cfan->displays[id][col++] = 0x6EFF;
		cfan->displays[id][col++] = 0x00FF;
		break;
	}

	return col;
}

/* Send a '8 bytes' packet followed by an INTERRUPT_IN msg */
static int send_data(struct usb_device *udev, void *data)
{
	char buf[8];
	int l = 0;
	int ret;

	memset(buf, 0, 8);
	buf[7] = 0x02;

	ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
			      0x09, 0x21, 0x200, 0x00, data, 0x008,
			      10 * HZ);

	if (ret <= 0)
		return ret;

	usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
			  buf, 8, &l,
			  10 * HZ);

	return 0;
}

static int cfan_open(struct inode *i, struct file *f)
{
	return 0;
}

static int cfan_release(struct inode *i, struct file *f)
{
	return 0;
}

static ssize_t cfan_write(struct file *f,
			  const char __user *buffer,
			  size_t cnt,
			  loff_t *off)
{
	struct cfan_data *cfan_data = NULL;
	unsigned char i;

	if (cnt == 0) {
		pr_err("cfan:%s: buffer size is null !\n", __func__);
		return 0;
	}

	if (!buffer) {
		pr_err("cfan:%s: buffer is null but its size is not !\n",
		       __func__);
		return -EINVAL;
	}

	if (cnt > sizeof(struct cfan_data) || cnt < sizeof(struct cfan_data)) {
		pr_err("cfan:%s: the size's value is unexpected !\n", __func__);
		return -EINVAL;
	}

	cfan_data = (struct cfan_data *)buffer;
	cfan->displays_nb = cfan_data->n;
	cfan->cfg[0] = set_config(0, cfan_data->cfg[0][0],
				  cfan_data->cfg[0][1],
				  cfan_data->cfg[0][2]);

	pr_info("cfan:%s: Number of displays: %d\n", __func__,
		cfan->displays_nb);

	/* Clear the ventilator */
	/*
	* for (i = 0; i < 156; i += 4)
	*	send_data(cfan->udev, (u16 *)cfan->displays + i);
	* return 2;
	*/

	pbm_masks_init();
	pbm_to_display(0, cfan_data->bitmaps[0], cfan->displays[0]);

	/* Send config */
	send_data(cfan->udev, &cfan->cfg[0]);

	/* Send display */
	for (i = 0; i < 156; i += 4)
		send_data(cfan->udev, (u16 *)cfan->displays + i);

	return 1;
}

const struct file_operations cfan_fops = {
	.open    = cfan_open,
	.release = cfan_release,
	.write = cfan_write,
};

static struct usb_class_driver cfan_class_driver = {
	.name = "usb/cfan0",
	.fops = &cfan_fops,
	.minor_base = 0
};

static int cfan_probe(struct usb_interface *interface,
		      const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(interface);
	int i;
	int j;
	int ret;

	cfan = kmalloc(sizeof(*cfan), GFP_KERNEL);

	if (!cfan)
		return -ENOMEM;

	memset(cfan, 0x00, sizeof(*cfan));

	for (j = 0; j < 8; ++j)
		for (i = 0; i < 156; i++)
			cfan->displays[j][i] = 0xFFFF;

	/* Increment the reference count of the usb device structure */
	cfan->udev = usb_get_dev(udev);

	/* Increment the ref counter */
	usb_set_intfdata(interface, cfan);

	pr_info("cfan:%s: USB Cheeky Fan has been connected\n", __func__);
	pr_info("cfan:%s: [devnum=%d;bus_id=%d]\n",
		__func__,
		cfan->udev->devnum,
		cfan->udev->bus->busnum);

	ret = usb_register_dev(interface, &cfan_class_driver);

	if (ret < 0) {
		pr_info("cfan: %s(): unable to register the device.\n",
			__func__);
		return ret;
	}

	return 0;
}

static void cfan_disconnect(struct usb_interface *interface)
{
	struct usb_cfan *dev;

	dev = usb_get_intfdata(interface);
	usb_set_intfdata(interface, NULL);
	usb_deregister_dev(interface, &cfan_class_driver);
	kfree(dev);

	pr_info("cfan: %s(): USB Cheeky Fan has been disconnected.\n",
		__func__);
}

static struct usb_driver cfan_driver = {
	.name       =	"cfan",
	.probe      =	cfan_probe,
	.disconnect =	cfan_disconnect,
	.id_table   =	id_table,
};

static int __init usb_cfan_init(void)
{
	int retval = 0;
	/* Register the USB driver. */
	retval = usb_register(&cfan_driver);
	if (retval < 0) {
		pr_info("cfan: %s(): unable to register USB driver.\n",
			__func__);
		return retval;
	}
	return 0;
}

static void __exit usb_cfan_exit(void)
{
	/* Un-register this USB driver. */
	usb_deregister(&cfan_driver);
}

module_init(usb_cfan_init);
module_exit(usb_cfan_exit);
