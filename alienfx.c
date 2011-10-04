#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/usb.h>

#define DRIVER_AUTHOR "Armando Di Cianno, armando@dicianno.org"
#define DRIVER_DESCRIPTION "Alienware AlienFX Driver"

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESCRIPTION);

enum alienfx_type {
	M11XR3,
};

#define ALIENFX_ALL_ON 0x04
#define ALIENFX_ALL_OFF 0x03

static const struct usb_device_id id_table[] = {
	{ USB_DEVICE(0x187c, 0x0522), .driver_info = M11XR3 },
	{},
};
MODULE_DEVICE_TABLE(usb, id_table);


struct alienfx {
        struct usb_device *     udev;
        unsigned char           val;
        enum alienfx_type       type;
};

enum alienfx_op {
	ALIENFX_WRITE = 0x02,
	ALIENFX_READ = 0x01,
};

struct alienfx_buffer {
	unsigned char op;
	unsigned char b1;
	unsigned char b2;
	unsigned char b3;
	unsigned char b4;
	unsigned char b5;
	unsigned char b6;
	unsigned char b7;
	unsigned char b8;
};

static unsigned int alienfx_read(struct alienfx *alien, unsigned int size)
{
	return size;
}

static int alienfx_write(struct alienfx *alien, struct alienfx_buffer *buffer)
{
	int retval = 0;

	buffer->op = ALIENFX_WRITE;
	retval = usb_control_msg(alien->udev, usb_sndctrlpipe(alien->udev, 0),
				 0x09, 0x21, 0x202, 0,
				 buffer,
				 9, 2000);

	return retval;
}

static struct alienfx_buffer *alienfx_buffer_alloc(struct alienfx *alien)
{
	struct alienfx_buffer *buffer;
        buffer = kmalloc(sizeof(struct alienfx_buffer), GFP_KERNEL);
        if (!buffer) {
                dev_err(&alien->udev->dev, "out of memory\n");
                return 0;
        }
	return buffer;
}

#define alienfx_buffer_free kfree

static void change_color(struct alienfx *alien)
{
	int retval = 0;
	struct alienfx_buffer * buffer = alienfx_buffer_alloc(alien);

	dev_info(&alien->udev->dev, "AlienFX: change_color\n");
////////////////////////////////
// go white
	buffer->b1 = 0x03;
	buffer->b2 = 0x01;
	buffer->b3 = buffer->b4 = 0;
	buffer->b5 = 0x01;
	buffer->b6 = 0xFF;
	buffer->b7 = 0xF0;
	buffer->b8 = 0;
////////////////////////////////

	alienfx_write(alien, buffer);

	alienfx_buffer_free(buffer);
}

#define show_set(area) \
static ssize_t show_##area(struct device *dev, struct device_attribute *attr, char *buf)               \
{                                                                       \
        struct usb_interface *intf = to_usb_interface(dev);             \
        struct alienfx *alien = usb_get_intfdata(intf);                   \
                                                                        \
        return sprintf(buf, "%d\n", 0);                        \
}                                                                       \
static ssize_t set_##area(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)    \
{                                                                       \
        struct usb_interface *intf = to_usb_interface(dev);             \
        struct alienfx *alien = usb_get_intfdata(intf);                   \
        change_color(alien);                                              \
        return count;                                                   \
}                                                                       \
static DEVICE_ATTR(area, S_IRUGO | S_IWUSR, show_##area, set_##area);
show_set(keyboard);

/* static ssize_t show_status(struct device *dev, struct device_attribute *attr, char *buf) */
/* { */
/*         struct usb_interface *intf = to_usb_interface(dev); */
/*         struct alienfx *alien = usb_get_intfdata(intf); */
/*         int retval = 0; */
/*         unsigned char *buffer; */

/*         buffer = kmalloc(9, GFP_KERNEL); */
/*         if (!buffer) { */
/*                 dev_err(&alien->udev->dev, "out of memory\n"); */
/*                 return; */
/*         } */
/* 	buffer[0] = 0x02; */
/* 	buffer[1] = 0x06; */
/* 	buffer[2] = buffer[3] = buffer[4] = buffer[5] = buffer[6] = */
/* 		buffer[7] = buffer[8] = 0; */
	       
/* 	retval = usb_control_msg(alien->udev, */
/* 				 usb_sndctrlpipe(alien->udev, 0), */
/* 				 0x09, 0x22, 0x202, 0, */
/* 				 buffer, */
/* 				 9, 2000); */

/*         return sprintf(buffer, "%d\n", 0); */
/* } */

/* static ssize_t set_status(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) */
/* { */
/*         //struct usb_interface *intf = to_usb_interface(dev); */
/*         //struct alienfx *alien = usb_get_intfdata(intf); */
/*         //change_color(alien); */
/*         return count; */
/* } */

static int alienfx_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(interface);
        struct alienfx *dev = NULL;
        int retval = -ENOMEM;
        unsigned char *buffer;

        dev = kzalloc(sizeof(struct alienfx), GFP_KERNEL);
        if (dev == NULL) {
                dev_err(&interface->dev, "out of memory\n");
                goto error_mem;
        }
	
        dev->udev = usb_get_dev(udev);
        dev->type = id->driver_info;
 
        usb_set_intfdata(interface, dev);

	retval = device_create_file(&interface->dev, &dev_attr_keyboard);
        if (retval)
                goto error;

	dev_info(&interface->dev, "AlienFX device connected.\n");
        return 0;

error:
        device_remove_file(&interface->dev, &dev_attr_keyboard);
        usb_set_intfdata (interface, NULL);
        usb_put_dev(dev->udev);
        kfree(dev);
error_mem:
	return retval;
}

static void alienfx_disconnect(struct usb_interface *interface)
{
        struct alienfx *dev;

        dev = usb_get_intfdata(interface);

        /* first remove the files, then set the pointer to NULL */
        device_remove_file(&interface->dev, &dev_attr_keyboard);
        usb_set_intfdata (interface, NULL);

        usb_put_dev(dev->udev);
        kfree(dev);
        dev_info(&interface->dev, "AlienFX device disconnected.\n");
}

static struct usb_driver alienfx_driver = {
        .name =         "alienfx",
        .probe =        alienfx_probe,
        .disconnect =   alienfx_disconnect,
        .id_table =     id_table,
};


static int __init alienfx_init(void)
{
        int retval = 0;

        retval = usb_register(&alienfx_driver);
        if (retval)
                err("usb_register failed. Error number %d", retval);
        return retval;
}

static void __exit alienfx_exit(void)
{
        usb_deregister(&alienfx_driver);
}

module_init(alienfx_init);
module_exit(alienfx_exit);
