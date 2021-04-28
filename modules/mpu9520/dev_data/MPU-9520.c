// SPDX-License-Identifier: GPL-2.0
/*
 * Simple adxl
*/

#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/fs.h>

#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/wait.h>

#include <linux/gpio/consumer.h>

#define VL_IOC_SET_RATE _IOW('v','s',int32_t*)
#define VL_IOC_GET_RATE _IOR('v','g',int32_t*)


#define ADXL_REG_POWER_CTL		0x2D
#define ADXL_REG_DATA_FORMAT	0x31
#define ADXL_REG_DATAX0			0x32
#define ADXL_REG_DATAX1			0x33
#define ADXL_REG_DATAY0			0x34
#define ADXL_REG_DATAY1			0x35
#define ADXL_REG_DATAZ0			0x36
#define ADXL_REG_DATAZ1			0x37

#define ADXL_CMD_PCTRL_MEASURE		0x08
#define ADXL_CMD_FORMAT_2G		0
#define ADXL_CMD_FORMAT_RES_NORM	0


/*
 * A global framework for axis devices.
 */
static struct axis_framework {
	struct class *class; /* A class of drivers for devices and sysfs framework */
	unsigned int major;
	void *drvdata[255];
} axis_dev;


/* Simple device registration */
#define AXIS_DEV_DATA(minor) axis_dev.drvdata[(minor)]
#define REGISTER_AXIS_DEV(minor, private) AXIS_DEV_DATA((minor)) = (private)


/* mpu9520 is an instance of an axis_dev driver. */
struct mpu9520_drv {
	struct device *dev;
	struct i2c_client *client;
	unsigned int minor;
	unsigned int rate;
};


static int mpu9520_read_xaxis(struct mpu9520_drv *data, int *val)
{
	struct i2c_client *client = data->client;
	u16 tries = 20;
	u8 ax_hi, ax_lo;
	int ret;

	/*
	 *  Artificial delay to slow down device operations.
	 *  This could instead take the variable, and use it to program a
	 *  feature on the mpu9520 before initiating the range find...
	 */
	//unsigned int delay = 1000000 / (data->rate ? data->rate : 1);
	//usleep_range(delay, delay + 2000);


	do {
		ret = i2c_smbus_read_byte_data(client, ADXL_REG_DATAX1);
		if (ret < 0)
			return ret;
		else {
			ax_hi = ret;
			break;
		}
		usleep_range(1000, 5000);
	} while (--tries);
	if (!tries)
		return -ETIMEDOUT;


	tries = 20;
	usleep_range(1000, 5000);
	do {
		ret = i2c_smbus_read_byte_data(client, ADXL_REG_DATAX0);
		if (ret < 0)
			return ret;
		else {
			ax_lo = ret;
			break;              // bug????????????????
		}
		usleep_range(1000, 5000);
	} while (--tries);
	if (!tries)
		return -ETIMEDOUT;
	

	*val = (int)((s16)((ax_hi << 8) + ax_lo));

	return 0;
}


static int mpu9520_open(struct inode *inode, struct file *file)
{
	int minor = iminor(inode);

	/* Identify this device, and associate the device data with the file */
	file->private_data = AXIS_DEV_DATA(minor);

	/* nonseekable_open removes seek, and pread()/pwrite() permissions */
	return nonseekable_open(inode, file);
}


static ssize_t mpu9520_read(struct file *file, char __user *buf, size_t count, loff_t * ppos)
{
	struct mpu9520_drv *data = file->private_data;
	int ret, val;
	char str[32];

	ret = mpu9520_read_xaxis(data, &val);
	if (ret)
		return ret;

	ret = snprintf(str, sizeof(str), "%d\n", val);

	if (copy_to_user(buf, str, ret))
		ret = -EFAULT;

	return ret;
}


static long mpu9520_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct mpu9520_drv *data = file->private_data;
	int value = 0;

	switch(cmd) {
	case VL_IOC_SET_RATE:
		if (copy_from_user(&value ,(int32_t*) arg, sizeof(value)))
			return -EFAULT;

		dev_info(data->dev, "Setting rate at %d\n", value);
		data->rate = value;

		break;
	case VL_IOC_GET_RATE:
		value = data->rate;

		if (copy_to_user((int32_t*) arg, &value, sizeof(value)))
			return -EFAULT;

		break;
	default:
		 /* ENOTTY: 25 Inappropriate ioctl for device. */
		return -ENOTTY;
        }

        return 0;
}

static int mpu9520_release(struct inode *inode, struct file *file)
{
	return 0;
}

/*
 * Specifying the .owner ensures that a reference is taken during any calls to
 * the function pointers in this table.
 *
 * This prevents unloading this module while an application is reading from
 * the device for example.
 *
 * Removing the .owner would allow userspace to remove the module while it's in
 * active use, leading towards a kernel panic.
 */
static const struct file_operations mpu9520_fops = {
	.owner		= THIS_MODULE,
	.open		= mpu9520_open,
	.read  		= mpu9520_read,
	.unlocked_ioctl	= mpu9520_ioctl,
	.release	= mpu9520_release,
	.llseek		= no_llseek,
};

static irqreturn_t mpu9520_irq_handler(int irq, void *dev_id)
{
	struct mpu9520_drv *data = dev_id;

	data->irq_flag = 1;
	wake_up(&data->waitq);

	return IRQ_HANDLED;
}

static int mpu9520_probe(struct i2c_client *client)
{
	struct mpu9520_drv *data;
	int write_val;
	int ret;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_READ_I2C_BLOCK |
				     			I2C_FUNC_SMBUS_BYTE_DATA)) 
	{
		dev_err(&client->dev, "i2c_check_functionality error\n");
		return -EIO;
	}

	data = kmalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	/* Associate the client with our private data */
	i2c_set_clientdata(client, data);
	data->client = client;

	/* Initialise variables */
	data->rate = 3;
	data->irq_flag = 0;


	/* initialise device */
	ret = i2c_smbus_write_byte_data(client, ADXL_REG_POWER_CTL, ADXL_CMD_PCTRL_MEASURE);
	if (ret) {

		printk("Failed to initialise device\n");
		return ret;
	}

	write_val = ADXL_CMD_FORMAT_2G | (ADXL_CMD_FORMAT_RES_NORM << 3);
	ret = i2c_smbus_write_byte_data(client, ADXL_REG_DATA_FORMAT, write_val);
	if (ret) {
		printk("Failed to set measurement params\n");
		return ret;
	}

	/* Register an interrupt handler if an IRQ was provided */
	if (client->irq) {
		int err;

		dev_info(&client->dev, "Registering IRQ on %d\n", client->irq);

		err = devm_request_threaded_irq(&client->dev,
						client->irq,
						NULL, 
						mpu9520_irq_handler,
						IRQF_TRIGGER_RISING | IRQF_ONESHOT,
						"mpu9520", data);
		if (err)
		{
			dev_err(&client->dev, "Failed to register IRQ on %d\n", client->irq);
			printk(KERN_INFO "err number = %d.\n", err);
		}
	}

	/*
	 * A 'axis_dev' framework should be responsible for creating the class.
	 * Linux does not provide this framework, so we create the class ourselves.
	 */
	axis_dev.class = class_create(THIS_MODULE, "axis_dev");
	if (IS_ERR(axis_dev.class))
		return PTR_ERR(axis_dev.class);

	/* register device number (major/minor - dynamic) */
	ret = register_chrdev(0, "axis_dev", &mpu9520_fops);
	if (ret < 0)
		return ret;

	axis_dev.major = ret;
	printk(KERN_INFO "axis_dev class created with major number %d.\n", axis_dev.major);

	/* Create only a single axis_dev device with minor 0. */
	data->minor = 0;

	/* Create the device file for userspace to interact with. */
	data->dev = device_create(axis_dev.class, NULL,
				     MKDEV(axis_dev.major, data->minor),
				     data, "axis_dev%d", data->minor);

	/* Register the device in the axis_dev device list. */
	REGISTER_AXIS_DEV(data->minor, data);

	/* init wait queue */
	init_waitqueue_head(&data->waitq);


	return 0;
}

static int mpu9520_remove(struct i2c_client *client)
{
	struct mpu9520_drv *mpu9520 = i2c_get_clientdata(client);

	device_del(mpu9520->dev);
	kfree(mpu9520);

	/* Framework cleanup. */
	unregister_chrdev(axis_dev.major, "axis_dev");
	class_destroy(axis_dev.class);

	return 0;
}

static const struct i2c_device_id mpu9520_id[] = {
	{ "mpu9520", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, mpu9520_id);

static const struct of_device_id mpu9520_dt_match[] = {
	{ .compatible = "st,mpu9520", },
	{ }
};
MODULE_DEVICE_TABLE(of, mpu9520_dt_match);

static struct i2c_driver mpu9520_driver = {
	.driver = {
		.name = "mpu9520",
		.of_match_table = mpu9520_dt_match,
	},
	.probe = mpu9520_probe,
	.remove = mpu9520_remove,
	.id_table = mpu9520_id,
};
module_i2c_driver(mpu9520_driver);


MODULE_DESCRIPTION("sams mpu9520 driver");
MODULE_AUTHOR("Sam Pemberton <sam@sam.co.uk>");
MODULE_LICENSE("GPL");
