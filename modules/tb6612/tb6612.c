#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>

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


// TODO:    

/*******************************************************************************
 * data
 ******************************************************************************/
struct tb6612_dev {
	dev_t dev_num;
	struct device_node * of_node;
	int gpio_dir_1A, gpio_dir_1B, gpio_dir_2A, gpio_dir_2B;
};


struct tb6612_drv {
	dev_t device_num_base;
	struct class * tb_class;
	struct device * tb_device;
    struct tb6612_dev * tb6612_dev;
};



/*******************************************************************************
 * device specific code
 ******************************************************************************/




/*******************************************************************************
 * file operations
 ******************************************************************************/
struct file_operations tb6612_fops=
{
	.llseek = no_llseek,
	.owner = THIS_MODULE
};



/*******************************************************************************
 * driver / device registration
 ******************************************************************************/
struct tb6612_drv pDrv;
struct tb6612_dev pDev;

static int tb6612_probe(struct platform_device *pdev)
{
/*	int ret = 0;
    struct tb6612_drv * pDrv;
    struct tb6612_dev * pDev;
*/

    pr_info("TB6612 - probe() enter\n");

/*	pDrv = kmalloc(sizeof(*pDrv), GFP_KERNEL);
	if (!pDrv)
		return -ENOMEM;

	pDev = kmalloc(sizeof(*pDev), GFP_KERNEL);
	if (!pDev)
		return -ENOMEM;

    pDev->tb6612_dev = pDev;
*/

	/*1. Get device node*/
	pDev.of_node = of_find_node_by_name(NULL, "tb6612");
	if(pDev.of_node  == NULL) {
		pr_err("TB6612 - node not found!\n");
		return -EINVAL;
	}

	/*2. get gpio property*/
	pDev.gpio_dir_1A = of_get_named_gpio(pDev.of_node, "tb6612:dir_1A", 0);
	if(pDev.gpio_dir_1A < 0) {
		pr_err("TB6612 - can't get named gpio: tb6612:dir_1A!\n");
		return -EINVAL;     
	}
	
    pr_info("TB6612 - probe() return\n");
	return 0;
}


static int tb6612_remove(struct platform_device *pdev)
{
/*    kfree(pDev);
    kfree(pDrv);
*/
    return 0;
}


static const struct platform_device_id tb6612_id[] = {
	{ .name = "tb6612", .driver_data = 0 },
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
	.probe = tb6612_probe,
	.remove = tb6612_remove,
	.id_table = tb6612_id,
};


module_platform_driver(tb6612_platform_drv);



MODULE_DESCRIPTION("tb6612 driver");
MODULE_AUTHOR("Sam Pemberton <sam@sam.co.uk>");
MODULE_LICENSE("GPL");
