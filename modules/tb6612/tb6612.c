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
 * STBY			D14		P9_41	7		GPIO0_20	AM335X_PIN_MCASP0_AXR1
 * DIR_1A		U18		P9_12	7		GPIO1_28	AM335X_PIN_GPMC_BEN1
 * DIR_1B		U17		P9_13	7		GPIO0_31	AM335X_PIN_GPMC_WPN
 * DIR_2A		R13		P9_15	7		GPIO1_16	AM335X_PIN_GPMC_A0
 * DIR_2B		U4		P8_34	7		GPIO2_17	AM335X_PIN_LCD_DATA11
 * PWM_1A		U14		P9_14	6		EHRPWM1A	AM335X_PIN_GPMC_A2
 * PWM_1B		T14		P9_16	6		EHRPWM1B	AM335X_PIN_GPMC_A3
 ******************************************************************************
 *
 *
 * **** DEVICE TREE PIN DETAILS ***********************************************
 * FUNC			PIN		H-PIN	MODE	MODE_FUNC	PIN-NAME
 * ----------------------------------------------------------------------------
 * STBY			D14		P9_41	7		GPIO0_20	AM335X_PIN_XDMA_EVENT_INTR1
 * DIR_3A		R3		P8_44	7		GPIO2_09	AM335X_PIN_LCD_DATA3
 * DIR_3B		R4		P8_43	7		GPIO2_08	AM335X_PIN_LCD_DATA2
 * DIR_4A		R1		P8_45	7		GPIO2_06	AM335X_PIN_LCD_DATA0
 * DIR_4B		R2		P8_46	7		GPIO2_07	AM335X_PIN_LCD_DATA1
 * PWM_2A		U10		P8_19	6		EHRPWM2A	AM335X_PIN_GPMC_AD8
 * PWM_2B		T10		P8_13	6		EHRPWM2B	AM335X_PIN_GPMC_AD9
 ******************************************************************************/



/*******************************************************************************
 * data
 ******************************************************************************/
struct tb6612_dev {
	//struct device_node * of_node;
	struct gpio_desc * gpio_stby;
	struct gpio_desc * gpio_dir_1A;
	struct gpio_desc * gpio_dir_1B;
	struct gpio_desc * gpio_dir_2A;
	struct gpio_desc * gpio_dir_2B;
	int minor;
};


struct tb6612_drv {
	struct class * tb_class;
	struct device * dev;
    struct tb6612_dev * tb6612_dev_data;
    int major;
};



/*******************************************************************************
 * device specific code
 ******************************************************************************/
int tb6612_get_dt_gpio(struct device * dev, struct tb6612_dev * p_dev)
{
	int ret = 0;
	
	/* setup standby pin */
	p_dev->gpio_stby = gpiod_get_index(dev, "tb6612", 0, GPIOD_OUT_HIGH);
	if(p_dev->gpio_stby == NULL) {
		pr_err("TB6612 - can't get gpio: stby\n");
		return -EINVAL;     
	}
	
	ret = gpiod_direction_output(p_dev->gpio_stby, 0); /* set to output 0 */
	if(ret != 0){
		pr_err("TB6612 - can't set gpio direction: stby\n");
		return ret;
	}
	
	
	
	
	p_dev->gpio_dir_1A = gpiod_get_index(dev, "tb6612", 1, GPIOD_OUT_HIGH);
	if(p_dev->gpio_dir_1A == NULL) {
		pr_err("TB6612 - can't get gpio: gpio_dir_1A\n");
		return -EINVAL;     
	}
	
	p_dev->gpio_dir_1B = gpiod_get_index(dev, "tb6612", 2, GPIOD_OUT_HIGH);
	if(p_dev->gpio_dir_1B == NULL) {
		pr_err("TB6612 - can't get gpio: gpio_dir_1B\n");
		return -EINVAL;     
	}
	
	p_dev->gpio_dir_2A = gpiod_get_index(dev, "tb6612", 3, GPIOD_OUT_HIGH);
	if(p_dev->gpio_dir_2A == NULL) {
		pr_err("TB6612 - can't get gpio: gpio_dir_2A\n");
		return -EINVAL;     
	}
	
	p_dev->gpio_dir_2B = gpiod_get_index(dev, "tb6612", 4, GPIOD_OUT_HIGH);
	if(p_dev->gpio_dir_2B == NULL) {
		pr_err("TB6612 - can't get gpio: gpio_dir_2B\n");
		return -EINVAL;     
	}
	
	return ret;
}



/*******************************************************************************
 * file operations
 ******************************************************************************/
struct file_operations tb6612_fops=
{
	.llseek = no_llseek,
	.owner = THIS_MODULE
};


/*******************************************************************************
 * sysfs interface
 ******************************************************************************/
static ssize_t drive_enable_store(struct device * dev,
        	     struct device_attribute * attr, const char * buf, size_t count)
{
    struct tb6612_drv * p_drv = dev_get_drvdata(dev);
	int enbl = 0;
    
    sscanf(buf, "%d", &enbl);
    
    if(enbl == 0)
	    gpiod_set_value(p_drv->tb6612_dev_data->gpio_stby, 0);
	else if(enbl == 1)
		gpiod_set_value(p_drv->tb6612_dev_data->gpio_stby, 1);
	else
		pr_err("TB6612 - enable: invalid parameter \n");
		
	return count;
}
static DEVICE_ATTR_WO(drive_enable);

 


/*******************************************************************************
 * driver / device registration
 ******************************************************************************/
static int tb6612_probe(struct platform_device *pdev)
{
	int ret = 0;
    struct tb6612_drv * p_drv;
    struct tb6612_dev * p_dev;


	/* allocate memory */
	p_drv = kzalloc(sizeof(*p_drv), GFP_KERNEL);
	if (!p_drv)
		return -ENOMEM;

	p_dev = kzalloc(sizeof(*p_dev), GFP_KERNEL);
	if (!p_dev)
		return -ENOMEM;

    p_drv->tb6612_dev_data = p_dev;
    p_drv->dev = &pdev->dev;
	dev_set_drvdata(&pdev->dev, p_drv);
	

	/* create the class */
	p_drv->tb_class = class_create(THIS_MODULE, "TB6612");
	if (IS_ERR(p_drv->tb_class))
	{
		pr_err("TB6612 - could not create class \n");
		return PTR_ERR(p_drv->tb_class);
	}

	/* register device number (major/minor - dynamic) */
	ret = register_chrdev(0, "TB6612", &tb6612_fops);
	if (ret < 0)
	{
		pr_err("TB6612 - could not register device number \n");
		return ret;
	}
	
	p_drv->major = ret;
	pr_info("TB6612 - class created with major number %d.\n", p_drv->major);


	/* Create a sysfs device file for userspace to interact with. */
	p_drv->tb6612_dev_data->minor = 0;
	p_drv->dev = device_create(p_drv->tb_class, NULL,
				              MKDEV(p_drv->major, p_drv->tb6612_dev_data->minor),
				              p_drv, "TB6612%d", p_drv->tb6612_dev_data->minor);
				           
	/* Create sysfs files on the device node */
	ret = device_create_file(p_drv->dev, &dev_attr_drive_enable);
	if (ret)
		return ret;


	/* get io from device tree */
	ret = tb6612_get_dt_gpio(p_drv->dev, p_dev);
	if(ret < 0)
		return ret;

	
    pr_info("TB6612 - probe() complete \n");
	return 0;
}


static int tb6612_remove(struct platform_device *pdev)
{
	struct tb6612_drv * p_drv = pdev->dev.driver_data;

	device_del(p_drv->dev);
	unregister_chrdev(p_drv->major, "TB6612");
	class_destroy(p_drv->tb_class);
    kfree(p_drv);
    kfree(p_drv->tb6612_dev_data);

	pr_info("TB6612 - remove() complete\n");
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
