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
 * PWM_1B		T14		P9_16	6		EHRPWM1B	AM335X_PIN_GPMC_A3  <-- gpio1_19 (51)
 ******************************************************************************/


/*******************************************************************************
 * data
 ******************************************************************************/



/*******************************************************************************
 * device specific code
 ******************************************************************************/




/*******************************************************************************
 * file operations
 ******************************************************************************/



/*******************************************************************************
 * driver / device registration
 ******************************************************************************/

static int tb6612_probe(struct platform_device *pdev)
{
    pr_info("TB6612 - probe() enter\n");
    pr_info("TB6612 - probe() return\n");
	return 0;
}


static int tb6612_remove(struct platform_device *pdev)
{
	pr_info("TB6612 - remove() return\n");
    return 0;
}


/*static const struct platform_device_id tb6612_id[] = {
	{ .name = "tb6612", .driver_data = 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, tb6612_id);*/


static const struct of_device_id tb6612_dt_match[] = {
	{ .compatible = "tos,tb6612", },
	{ }
};
MODULE_DEVICE_TABLE(of, tb6612_dt_match);


static struct platform_driver tb6612_platform_drv = {
	.driver = {
		.name = "tb6612",
		.of_match_table = tb6612_dt_match,
		.owner = THIS_MODULE,
	},
	.probe = tb6612_probe,
	.remove = tb6612_remove,
//	.id_table = tb6612_id,
};


module_platform_driver(tb6612_platform_drv);



MODULE_DESCRIPTION("tb6612 driver");
MODULE_AUTHOR("Sam Pemberton <sam@sam.co.uk>");
MODULE_LICENSE("GPL");
