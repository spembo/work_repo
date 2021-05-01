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


/* STBY = 1-> turn on
 * 
 *  IN  1   2
 * ------------
 *      0   0  STOP
 *      0   1  CCW
 *      1   0  CW
 *      1   1  BRAKE
 */




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



static struct mpu9520_framework {
	struct class *class;
	unsigned int major;
	void *drvdata[255];
} mpu9520_frame;


struct mpu9520_drv {
	struct device * dev;
	struct i2c_client * client;
	unsigned int minor;
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
    else
    {
        pr_info("MPU9530 - device recognised \n");
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


static int read_axis_data(struct mpu9520_drv * pDrv, int16_t * data)
{
    uint8_t raw_data_lsb, raw_data_msb;
    struct i2c_client * client = pDrv->client;
    int ret;
    
    /* get x axis */
    raw_data_msb = i2c_smbus_read_byte_data(client, REG_ACCEL_XOUT_H);
	if (raw_data_msb < 0)
		return ret;
		
    raw_data_lsb = i2c_smbus_read_byte_data(client, REG_ACCEL_XOUT_L);
	if (raw_data_lsb < 0)
		return ret;
		
    *(data + 0) = (int16_t)(((int16_t)raw_data_msb << 8) | raw_data_lsb);

    /* get y axis */
    raw_data_msb = i2c_smbus_read_byte_data(client, REG_ACCEL_YOUT_H);
	if (raw_data_msb < 0)
		return ret;
		
    raw_data_lsb = i2c_smbus_read_byte_data(client, REG_ACCEL_YOUT_L);
	if (raw_data_lsb < 0)
		return ret;
		
    *(data + 1) = (int16_t)(((int16_t)raw_data_msb << 8) | raw_data_lsb);

    /* get z axis */
    raw_data_msb = i2c_smbus_read_byte_data(client, REG_ACCEL_ZOUT_H);
	if (raw_data_msb < 0)
		return ret;
		
    raw_data_lsb = i2c_smbus_read_byte_data(client, REG_ACCEL_ZOUT_L);
	if (raw_data_lsb < 0)
		return ret;
		
    *(data + 2) = (int16_t)(((int16_t)raw_data_msb << 8) | raw_data_lsb);
    
    return 0;
}



/*******************************************************************************
 * file operations
 ******************************************************************************/
static int mpu9520_open(struct inode *inode, struct file * file)
{
    /* Identify this device, and associate the device data with the file */
	int minor = iminor(inode);
    pr_info("minor access = %d\n", minor);
	file->private_data = mpu9520_frame.drvdata[minor];

	/* nonseekable_open removes seek, and pread()/pwrite() permissions */
	/* ~~~ always returns 0 ~~~ */
	return nonseekable_open(inode, file);	
}


static int mpu9520_release(struct inode *inode, struct file * file)
{
	pr_info("MPU9520 - release was successful\n");
	return 0;
}


ssize_t mpu9520_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos)
{
    int ret;
    int16_t data[3];
	char str[32];
    struct mpu9520_drv *pDrv = file->private_data;

    pr_info("MPU9520 - attempting read\n");
    ret = read_axis_data(pDrv, data);
	if (ret)
		return ret;
    
    pr_info("MPU9520 - snprint\n");  
	ret = snprintf(str, sizeof(str), "%d\n%d\n%d\n", data[0], 
                                                     data[1], 
                                                     data[2]);
                                                     
    pr_info("MPU9520 - copying to user\n");                                                     
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
		pr_err("MPU9520 -i2c_check_functionality error\n");
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
		pr_err("MPU9250 - failed to initialise device\n");
		return ret;
	}


	/* create the class */
	mpu9520_frame.class = class_create(THIS_MODULE, "MPU9250");
	if (IS_ERR(mpu9520_frame.class))
		return PTR_ERR(mpu9520_frame.class);


	/* register device number (major/minor - dynamic) */
	ret = register_chrdev(0, "MPU9250", &mpu9520_fops);
	if (ret < 0)
		return ret;

	mpu9520_frame.major = ret;
	pr_info("MPU9250 - class created with major number %d.\n", mpu9520_frame.major);


	/* Create a single device file for userspace to interact with. */
	pDrv->minor = 0;
	pDrv->dev = device_create(mpu9520_frame.class, NULL,
				              MKDEV(mpu9520_frame.major, pDrv->minor),
				              pDrv, "MPU9250_%d", pDrv->minor);

    mpu9520_frame.drvdata[pDrv->minor] = pDrv;
    
    pr_info("MPU9250 - probe success\n");
	return 0;
}


static int mpu9520_remove(struct i2c_client *client)
{
	struct mpu9520_drv *pDrv = i2c_get_clientdata(client);

	device_del(pDrv->dev);
	kfree(pDrv);
	unregister_chrdev(mpu9520_frame.major, "MPU9250");
	class_destroy(mpu9520_frame.class);

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
