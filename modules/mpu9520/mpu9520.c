#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/fs.h>
//#include <linux/interrupt.h>
//#include <linux/wait.h>
#include <linux/gpio/consumer.h>



/*******************************************************************************
 * data
 ******************************************************************************/
#define REG_ACCEL_CONFIG    0x1C
#define REG_ACCEL_CONFIG2   0x1D

#define REG_INT_PIN_CFG     0x37
#define REG_INT_ENABLE      0x38

#define REG_ACCEL_XOUT_H    0x3B
#define REG_ACCEL_XOUT_L    0x3C
#define REG_ACCEL_YOUT_H    0x3D
#define REG_ACCEL_YOUT_L    0x3E
#define REG_ACCEL_ZOUT_H    0x3F
#define REG_ACCEL_ZOUT_L    0x40

#define REG_PWR_MGMT_1      0x6B
#define REG_WHO_AM_I        0x75

#define MPU9250_WHO_ID      0x71


/*Device private data structure - instance of a device */
struct mpu9520_dev {
	struct cdev c_dev;
	unsigned int minor;
    int16_t x_axis, y_axis, z_axis;
};


/*Driver private data structure - instance of an mpu9529 driver */
struct mpu9520_drv {
    struct class * class; 
	struct i2c_client * client;
	unsigned int major;
	struct device * dev;
	struct mpu9520_dev dev_data;
};




/*******************************************************************************
 * device specific code
 ******************************************************************************/
static int init_mpu9250(struct i2c_client *client)
{   
    int ret = 0;
    uint8_t whoami;

    whoami = i2c_smbus_read_byte_data(client, REG_WHO_AM_I);
    if (whoami != MPU9250_WHO_ID)
    {
        pr_err("MPU9530 - whoami not recognised \n");
        ret = -1;
        return ret;
    }


    ret |= i2c_smbus_write_byte_data(client, REG_PWR_MGMT_1, 0x80); /* reset device */
    msleep(100);
    ret |= i2c_smbus_write_byte_data(client, REG_PWR_MGMT_1, 0x00); /* wake up */ 
    msleep(100); // Delay 100 ms for PLL to get established 
    ret |= i2c_smbus_write_byte_data(client, REG_PWR_MGMT_1, 0x01); /* clock source PLL */

    /* Set accelerometer full-scale range */
    ret |= i2c_smbus_write_byte_data(client, REG_ACCEL_CONFIG, 0x00); /* +-2G */

    /* Set accelerometer sample rate configuration */
    ret |= i2c_smbus_write_byte_data(client, REG_ACCEL_CONFIG2, 0x09); /* 218.1 Hz */

    /* turn interrupts off */
    ret |= i2c_smbus_write_byte_data(client, REG_INT_PIN_CFG, 0x00);    
    ret |= i2c_smbus_write_byte_data(client, REG_INT_ENABLE, 0x00);

    return ret;
}


void read_axis_data(struct mpu9520_drv * pDrv)
{
    uint8_t raw_data_lsb, raw_data_msb;
    struct i2c_client * client = pDrv->client;
    

    raw_data_msb = i2c_smbus_read_byte_data(client, REG_ACCEL_XOUT_H);
    raw_data_lsb = i2c_smbus_read_byte_data(client, REG_ACCEL_XOUT_L);
    pDrv->dev_data.x_axis = (int16_t)(((int16_t)raw_data_msb << 8) | raw_data_lsb);

    raw_data_msb = i2c_smbus_read_byte_data(client, REG_ACCEL_YOUT_H);
    raw_data_lsb = i2c_smbus_read_byte_data(client, REG_ACCEL_YOUT_L);
    pDrv->dev_data.y_axis = (int16_t)(((int16_t)raw_data_msb << 8) | raw_data_lsb);

    raw_data_msb = i2c_smbus_read_byte_data(client, REG_ACCEL_ZOUT_H);
    raw_data_lsb = i2c_smbus_read_byte_data(client, REG_ACCEL_ZOUT_L);
    pDrv->dev_data.z_axis = (int16_t)(((int16_t)raw_data_msb << 8) | raw_data_lsb);
}



/*******************************************************************************
 * file operations
 ******************************************************************************/
static int mpu9520_open(struct inode *inode, struct file * file)
{
    int ret;
	int minor;
    struct mpu9520_dev * pDev;

    /* confirm on which device file open was attempted by user space */
    minor = MINOR(inode->i_rdev);
    pr_info("minor access = %d\n", minor);

    /* get device's private data structure */
	pDev = container_of(inode->i_cdev, struct mpu9520_dev, c_dev);

    /* supply device private data to other methods of the driver */
	file->private_data = pDev;

	/* nonseekable_open removes seek, and pread()/pwrite() permissions */
	/* ~~~ always returns 0 ~~~ */
	ret = nonseekable_open(inode, file);	
	return ret;
}


static int mpu9520_release(struct inode *inode, struct file * file)
{
	pr_info("release was successful\n");
	return 0;
}


ssize_t mpu9520_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos)
{
    int ret;
	char str[32];
    struct mpu9520_drv *pDrv = file->private_data;


    read_axis_data(pDrv);
	ret = snprintf(str, sizeof(str), "%d\n%d\n%d\n", pDrv->dev_data.x_axis, 
                                                     pDrv->dev_data.y_axis, 
                                                     pDrv->dev_data.z_axis);
	if (copy_to_user(buf, str, ret))
		ret = -EFAULT;

	return ret;
}


/* file operations of the driver */
struct file_operations mpu9520_fops=
{
	.open = mpu9520_open,
	.release = mpu9520_release,
	.read = mpu9520_read,
	.llseek = no_llseek,
	.owner = THIS_MODULE
};



/*******************************************************************************
 * driver / device registration
 ******************************************************************************/
static int mpu9520_probe(struct i2c_client * client)
{
	struct mpu9520_drv *pDrv;
	int ret;

    /* create driver */
	if (!i2c_check_functionality(client->adapter, 
                    (I2C_FUNC_SMBUS_READ_I2C_BLOCK | I2C_FUNC_SMBUS_BYTE_DATA))){
		pr_err("i2c_check_functionality error\n");
		return -EIO;
	}

	pDrv = kmalloc(sizeof(*pDrv), GFP_KERNEL);
	if (!pDrv)
		return -ENOMEM;


	/* Associate the i2c client and driver */
	i2c_set_clientdata(client, pDrv);
	pDrv->client = client;


	/* initialise physical device */
	ret = init_mpu9250(client);
	if (ret) {
		pr_err("MPU9250 failed to initialise device\n");
		return ret;
	}


	/* create the class */
	pDrv->class = class_create(THIS_MODULE, "MPU9250");
	if (IS_ERR(pDrv->class))
		return PTR_ERR(pDrv->class);


	/* register device number (major/minor - dynamic) */
	ret = register_chrdev(0, "MPU9250", &mpu9520_fops);
	if (ret < 0)
		return ret;

	pDrv->major = ret;
	pr_info("MPU9250 class created with major number %d.\n", pDrv->major);


	/* Create a single device file for userspace to interact with. */
	pDrv->dev_data.minor = 0;
	pDrv->dev = device_create(pDrv->class, 
                              NULL,
				              MKDEV(pDrv->major, pDrv->dev_data.minor),
				              pDrv,         /* void * drvdata */
                              "MPU9250_%d", pDrv->dev_data.minor);

    pr_info("MPU9250 probe success");
	return 0;
}


static int mpu9520_remove(struct i2c_client *client)
{
	struct mpu9520_drv *pDrv = i2c_get_clientdata(client);

	device_del(pDrv->dev);
	kfree(pDrv);
	unregister_chrdev(pDrv->major, "MPU9250");
	class_destroy(pDrv->class);

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
	.probe_new = mpu9520_probe,
	.remove = mpu9520_remove,
	.id_table = mpu9520_id,
};
module_i2c_driver(mpu9520_driver);


MODULE_DESCRIPTION("mpu9520 driver");
MODULE_AUTHOR("Sam Pemberton <sam@sam.co.uk>");
MODULE_LICENSE("GPL");