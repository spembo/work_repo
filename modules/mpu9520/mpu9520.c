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

#define MPU9520_WHO_ID      0x71


struct mpu9520_drv {
	struct class *class;
	struct i2c_client * client;
	struct device * dev;
	unsigned int major;
	unsigned int minor;
};




/*******************************************************************************
 * device specific code
 ******************************************************************************/
static int init_mpu9520(struct i2c_client *client)
{   
    int ret = 0;
    uint8_t whoami;


    whoami = i2c_smbus_read_byte_data(client, REG_WHO_AM_I);
    if (whoami != MPU9520_WHO_ID)
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


static int read_axis(struct i2c_client * client, int16_t * data, uint8_t reg_hi,
                                                                uint8_t reg_lo)
{
    int32_t lsb, msb;
       
    msb = i2c_smbus_read_byte_data(client, reg_hi);
	if (msb < 0){
        pr_err("MPU9520 -i2c read error\n");
		return msb;
    }
		
    lsb = i2c_smbus_read_byte_data(client, reg_lo);
	if (lsb < 0){
        pr_err("MPU9520 -i2c read error\n");
		return lsb;
    }
		
    *data = (int16_t)(((msb & 0x000000FF) << 8) | (lsb & 0x000000FF));   
    return 0;
}



/*******************************************************************************
 * file operations
 ******************************************************************************/

/* file operations of the driver */
struct file_operations mpu9520_fops=
{
	.llseek = no_llseek,
	.owner = THIS_MODULE
};



/*******************************************************************************
 * sysfs interface
 ******************************************************************************/
static ssize_t x_axis_show(struct device * dev, struct device_attribute *attr,
			                                                          char *buf)
{
	int ret;
	int16_t x_axis;
    struct mpu9520_drv * pDrv;


    pDrv = dev_get_drvdata(dev);
    ret = read_axis(pDrv->client, &x_axis, REG_ACCEL_XOUT_H, REG_ACCEL_XOUT_L);
    if (ret)
		return ret;
		
	return sprintf(buf, "%d\n", x_axis);
}
static DEVICE_ATTR_RO(x_axis);


static ssize_t y_axis_show(struct device * dev, struct device_attribute *attr,
			                                                          char *buf)
{
	int ret;
	int16_t y_axis;
    struct mpu9520_drv * pDrv;


    pDrv = dev_get_drvdata(dev);
    ret = read_axis(pDrv->client, &y_axis, REG_ACCEL_YOUT_H, REG_ACCEL_YOUT_L);
    if (ret)
		return ret;
		
	return sprintf(buf, "%d\n", y_axis);
}
static DEVICE_ATTR_RO(y_axis);


static ssize_t z_axis_show(struct device * dev, struct device_attribute *attr,
			                                                          char *buf)
{
	int ret;
	int16_t z_axis;
    struct mpu9520_drv * pDrv;


    pDrv = dev_get_drvdata(dev);
    ret = read_axis(pDrv->client, &z_axis, REG_ACCEL_ZOUT_H, REG_ACCEL_ZOUT_L);
    if (ret)
		return ret;
		
	return sprintf(buf, "%d\n", z_axis);
}
static DEVICE_ATTR_RO(z_axis);


/*******************************************************************************
 * driver / device registration
 ******************************************************************************/
static int mpu9520_probe(struct i2c_client * client)
{
	struct mpu9520_drv * pDrv;
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
	ret = init_mpu9520(client);
	if (ret) {
		pr_err("MPU9520 - failed to initialise device\n");
		return ret;
	}


	/* create the class */
	pDrv->class = class_create(THIS_MODULE, "MPU9520");
	if (IS_ERR(pDrv->class))
		return PTR_ERR(pDrv->class);


	/* register device number (major/minor - dynamic) */
	ret = register_chrdev(0, "MPU9520", &mpu9520_fops);
	if (ret < 0)
		return ret;

	pDrv->major = ret;
	pr_info("MPU9520 - class created with major number %d.\n", pDrv->major);


	/* Create a single device file for userspace to interact with. */
	pDrv->minor = 0;
	pDrv->dev = device_create(pDrv->class, NULL,
				              MKDEV(pDrv->major, pDrv->minor),
				              pDrv, "MPU9520_%d", pDrv->minor);
    
    /* Create a sysfs files on the client device node */
	ret = device_create_file(pDrv->dev, &dev_attr_x_axis);
	if (ret)
		return ret;
		
	ret = device_create_file(pDrv->dev, &dev_attr_y_axis);
	if (ret)
		return ret;
		
	ret = device_create_file(pDrv->dev, &dev_attr_z_axis);
	if (ret)
		return ret;

    pr_info("MPU9520 - probe success\n");
	return 0;
}


static int mpu9520_remove(struct i2c_client *client)
{
	struct mpu9520_drv * pDrv = i2c_get_clientdata(client);
	
	device_del(pDrv->dev);
	unregister_chrdev(pDrv->major, "MPU9520");
	class_destroy(pDrv->class);
	kfree(pDrv);
    pr_info("MPU9520 - remove success\n");
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
