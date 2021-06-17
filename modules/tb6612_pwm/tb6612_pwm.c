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
#include <linux/pwm.h>


/* **** DEVICE TREE PIN DETAILS ***********************************************
 * FUNC			PIN		H-PIN	MODE	MODE_FUNC	PIN-NAME
 * ----------------------------------------------------------------------------
 * STBY			D14		P9_41	7		GPIO0_20	AM335X_PIN_XDMA_EVENT_INTR1
 * DIR_1A		U18		P9_12	7		GPIO1_28	AM335X_PIN_GPMC_BEN1
 * DIR_1B		U17		P9_13	7		GPIO0_31	AM335X_PIN_GPMC_WPN
 * DIR_2A		R13		P9_15	7		GPIO1_16	AM335X_PIN_GPMC_A0
 * DIR_2B		U4		P8_34	7		GPIO2_17	AM335X_PIN_LCD_DATA11
 * PWM_1A		U14		P9_14	6		EHRPWM1A	AM335X_PIN_GPMC_A2
 * PWM_1B		T14		P9_16	6		EHRPWM1B	AM335X_PIN_GPMC_A3
 * ----------------------------------------------------------------------------
 *
 * EHRPWM1A_MUX1 = GPIO1[18], MODE 6
 * EHRPWM1B_MUX1 = GPIO1[19], MODE 6
 *
 * am33xx-l4.dtsi [2058] --> has entry for ehrpwm0
 * driver based on /drivers/video/backlight/lp855x_bl.c
 *
 * need to create/edit/add to node on bbb_robot.dtsi
 * qcom-msm8974-sony-xperia-castor.dts:532:			compatible = "ti,lp8556";
 ******************************************************************************/

/* NOTE:
 * Ensure that the EHRPWM & EPWMSS are enabled in the kernel config 
 */


/*******************************************************************************
 * data
 ******************************************************************************/
struct tb6612_drv {
	struct device * dev;
	int major;
};


/*******************************************************************************
 * device specific code
 ******************************************************************************/
/*static void lp855x_pwm_ctrl(struct lp855x *lp, int br, int max_br)*/
/*{*/
/*	unsigned int period = lp->pdata->period_ns;*/
/*	unsigned int duty = br * period / max_br;*/
/*	struct pwm_device *pwm;*/

	/* request pwm device with the consumer name */
/*	if (!lp->pwm) {*/
/*		pwm = devm_pwm_get(lp->dev, lp->chipname);*/
/*		if (IS_ERR(pwm))*/
/*			return;*/

/*		lp->pwm = pwm;*/

		/*
		 * FIXME: pwm_apply_args() should be removed when switching to
		 * the atomic PWM API.
		 */
/*		pwm_apply_args(pwm);*/
/*	}*/

/*	pwm_config(lp->pwm, duty, period);*/
/*	if (duty)*/
/*		pwm_enable(lp->pwm);*/
/*	else*/
/*		pwm_disable(lp->pwm);*/
/*}*/

/*******************************************************************************
 * file operations
 ******************************************************************************/


/*******************************************************************************
 * sysfs interface
 ******************************************************************************/

/* https://github.com/SaadAhmad/beaglebone-black-cpp-PWM */


/*******************************************************************************
 * driver / device registration
 ******************************************************************************/
static int tb6612_probe(struct platform_device * pdev)
{
	struct device_node * node;
	struct pwm_device * pwm;

	
	pr_info("pdev->name is %s\n", pdev->name);
	pr_info("pdev->dev.of_node->name is %s\n", pdev->dev.of_node->name);

	node = of_find_node_by_name(NULL, "tb6612");
	pr_info("node name is %s\n", node->name);
	pr_info("node full_name is %s\n", node->full_name);

	pwm = devm_pwm_get(&pdev->dev, NULL);    
// set to NULL: ENOENT 2 No such file or directory:  "can't parse "pwms" property": 
// set to tb6612: EINVAL 22 Invalid argument
// set to pwm_pinA: EINVAL 22 Invalid argument
// set to /tb6612/pwm_pinA: EINVAL 22 Invalid argument
// set to node->full_name: EINVAL 22 Invalid argument

/*[ 2412.633925] pdev->name is tb6612*/
/*[ 2412.633937] pdev->dev.of_node->name is tb6612*/
/*[ 2412.634196] node name is pwm_pinA*/
/*[ 2412.634200] node full_name is pwm_pinA*/
/*[ 2412.634211] TB6612 - unable to request PWM */
/*[ 2412.641840] tb6612: probe of tb6612 failed with error -22*/

// changed device tree to only PWM -------------->
// tb6612: probe of tb6612 failed with error -61



	if (IS_ERR(pwm)) {
		pr_err("TB6612 - unable to request PWM \n");
		return PTR_ERR(pwm);
	}


    pr_info("TB6612 - probe() complete\n");
	return 0;
}


static int tb6612_remove(struct platform_device *pdev)
{
	pr_info("TB6612 - remove() complete\n");
    return 0;
}


static const struct platform_device_id tb6612_id[] = {
	{ .name = "tb6612", .driver_data = 0 },
	{ },
};
MODULE_DEVICE_TABLE(platform, tb6612_id);


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
