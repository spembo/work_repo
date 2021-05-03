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


/****************************************************************************** 
 * tb6612 IO:
 * ----------------------------------------------------------------------------
 * STBY 		0 = disable, 1 = enable			DIR_xA		DIR_xB		OUT
 * DIR_1A		stop / direction control		-------------------------------
 * DIR_1B		"									0			0		STOP
 * DIR_2A		"									0			1		CCW
 * DIR_2B		"									1			0		CW
 * PWM_1A		speed control						1			1		BRAKE
 * PWM_1B		"
 *
 *
 * **** DEVICE TREE PIN DETAILS ***********************************************
 * FUNC			PIN		H-PIN	MODE	MODE_FUNC	PIN-NAME
 * ----------------------------------------------------------------------------
 * DIR_1A		U18		P9_12	7		GPIO1_28	AM335X_PIN_GPMC_BEN1
 * DIR_1B		U17		P9_13	7		GPIO0_31	AM335X_PIN_GPMC_WPN
 * DIR_2A		R13		P9_15	7		GPIO1_16	AM335X_PIN_GPMC_A0
 * DIR_2B		U4		P8_34	7		GPIO2_17	AM335X_PIN_LCD_DATA11
 * PWM_1A		U14		P9_14	6		EHRPWM1A	AM335X_PIN_GPMC_A2
 * PWM_1B		T14		P9_16	6		EHRPWM1B	AM335X_PIN_GPMC_A3
 ******************************************************************************/




/*******************************************************************************
 * data
 ******************************************************************************/
struct tb6612_dev {
	dev_t dev_num;
	struct cdev cdev;
	struct device_node * of_node;
	int gpio_dir_1A, gpio_dir_1B, gpio_dir_2A, gpio_dir_2B;
};


struct tb6612_drv {
	dev_t device_num_base;
	struct class *tb_class;
	struct device *tb_device;
};

struct tb6612_dev tb6612_device;


/*******************************************************************************
 * device specific code
 ******************************************************************************/




/*******************************************************************************
 * file operations
 ******************************************************************************/
static int tb6612_open(struct inode *inode, struct file * file)
{
    /* Identify this device, and associate the device data with the file */
	int minor = iminor(inode);
    pr_info("minor access = %d\n", minor);
	file->private_data = tb6612_frame.drvdata[minor];

	/* nonseekable_open removes seek, and pread()/pwrite() permissions */
	/* ~~~ always returns 0 ~~~ */
	return nonseekable_open(inode, file);	
}


static int tb6612_release(struct inode *inode, struct file * file)
{
	pr_info("MPU9520 - release was successful\n");
	return 0;
}


/* file operations of the driver */
struct file_operations tb6612_fops=
{
	.open = tb6612_open,
	.release = tb6612_release,
	.llseek = no_llseek,
	.owner = THIS_MODULE
};



/*******************************************************************************
 * driver / device registration
 ******************************************************************************/
static int tb6612_probe(struct platform_device *pdev)
{
	int ret = 0;

	printk(KERN_EMERG "TB6612 - probe() enter!\n");

	/*1. Get device node*/
	tb6612_device.of_node = of_find_node_by_name(NULL, "tb6612");
	if(tb6612_device.of_node  == NULL) {
		printk(KERN_EMERG "beep node not found!\n");
		return -EINVAL;
	}

	/*2. get gpio property*/
	tb6612_device.gpio_dir_1A = of_get_named_gpio(tb6612_device.gpio_dir_1A, "beep-gpio", 0);
	if(tb6612_device.beep_gpio < 0) {
		printk(KERN_EMERG "can't get beep-gpio!\n");
		return -EINVAL;     
	}

	/*3. set gpio output,output high default,close beep*/
	ret = gpio_direction_output(tb6612_device.beep_gpio, 1);
	if(ret < 0)
		printk(KERN_EMERG "can't set beep-gpio!\n");
	
	ret = misc_register(&beep_miscdev);
	if(ret < 0) {
		printk(KERN_EMERG "misc device register failed!\n");
		return -EINVAL;           
	}

	return 0;
}


static int tb6612_remove(struct i2c_client *client)
{

}


static const struct i2c_device_id tb6612_id[] = {
	{ "tb6612", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, tb6612_id);


static const struct of_device_id tb6612_dt_match[] = {
	{ .compatible = "tos,tb6612", },
	{ }
};
MODULE_DEVICE_TABLE(of, tb6612_dt_match);


static struct platform_driver tb6612_platform_drv = {
	.driver = {
		.name = "tb6612",
		.of_match_table = tb6612_dt_match,
	},
	.probe_new = tb6612_probe,
	.remove = tb6612_remove,
	.id_table = tb6612_id,
};


module_platform_driver(pcd_platform_driver);



MODULE_DESCRIPTION("tb6612 driver");
MODULE_AUTHOR("Sam Pemberton <sam@sam.co.uk>");
MODULE_LICENSE("GPL");
