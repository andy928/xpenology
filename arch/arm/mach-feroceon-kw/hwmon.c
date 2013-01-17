/*
 * hwmon.c - temperature monitoring driver for Kirkwood SoC
 *
 * Inspired from other hwmon drivers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/hwmon.h>
#include <linux/sysfs.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/platform_device.h>
#include <linux/cpu.h>
#include <asm/io.h>
#include "mvOs.h"

#define THERMAL_SENS_STATUS_REG		0x10078	

#define THERMAL_VALID_OFFS		9
#define THERMAL_VALID_MASK		0x1

#define THERMAL_SENS_OFFS		10
#define THERMAL_SENS_MASK		0x1FF

#define THERMAL_SENS_MAX_TIME_READ	16
#define KW_TEMPER_FORMULA(x)		(((322 - x) * 10000)/13625)	/* in Celsius */
static struct device *hwmon_dev;


typedef enum { 
	SHOW_TEMP, 
	SHOW_NAME} SHOW;

	
	
static int kw_temp_read_temp(void)
{
	u32 reg = 0;
	u32 reg1 = 0;
	u32 i, temp_cels;

	/* read the Thermal Sensor Status Register */
	reg = MV_REG_READ(THERMAL_SENS_STATUS_REG);
	
	/* Check if temperature reading is valid */
	if ( ((reg >> THERMAL_VALID_OFFS) & THERMAL_VALID_MASK) != 0x1 ) {
		/* No valid temperature */ 
		printk(KERN_ERR "kw-hwmon: The reading temperature is not valid !\n");
		return -EIO;
	}
	
	/*
	 * Read the thermal sensor looking two successive readings that differ
	 * in LSb only.
	 */

	for (i=0; i<THERMAL_SENS_MAX_TIME_READ; i++) {
	  
		/* Read the raw temperature */
		reg = MV_REG_READ(THERMAL_SENS_STATUS_REG);
		reg >>= THERMAL_SENS_OFFS;	
		reg &= THERMAL_SENS_MASK;


		if (((reg ^ reg1) & 0x1FE) == 0x0) {
			break;
		}	
		/* save the current reading for the next iteration */
		reg1 = reg;
	}

	if (i==THERMAL_SENS_MAX_TIME_READ) {
		printk(KERN_WARNING "kw-hwmon: Thermal sensor is unstable!\n");
	}
	
	/* Convert the temperature to celsium */
	temp_cels = KW_TEMPER_FORMULA(reg);
	
	return temp_cels;
}


/*
 * Sysfs stuff
 */

static ssize_t show_name(struct device *dev, struct device_attribute
			  *devattr, char *buf) {
	return sprintf(buf, "%s\n", "kw-hwmon");
}

static ssize_t show_temp(struct device *dev,
			 struct device_attribute *devattr, char *buf) {

	return sprintf(buf, "%d\n", kw_temp_read_temp());

}

static SENSOR_DEVICE_ATTR(temp_input, S_IRUGO, show_temp, NULL,
			  SHOW_TEMP);
static SENSOR_DEVICE_ATTR(name, S_IRUGO, show_name, NULL, SHOW_NAME);

static struct attribute *kw_temp_attributes[] = {
	&sensor_dev_attr_name.dev_attr.attr,
	&sensor_dev_attr_temp_input.dev_attr.attr,
	NULL
};

static const struct attribute_group kw_temp_group = {
	.attrs = kw_temp_attributes,
};

static int __devinit kw_temp_probe(struct platform_device *pdev)
{
	int err;
	err = sysfs_create_group(&pdev->dev.kobj, &kw_temp_group);
	if (err) {
		goto exit;
	}
	hwmon_dev = hwmon_device_register(&pdev->dev);
	if (IS_ERR(hwmon_dev)) {
		dev_err(&pdev->dev, "Class registration failed (%d)\n",
			err);
		goto exit;
	}

	printk(KERN_INFO "Kirkwood hwmon thermal sensor initialized.\n");

	return 0;

exit:
	sysfs_remove_group(&pdev->dev.kobj, &kw_temp_group);
	return err;
}

static int __devexit kw_temp_remove(struct platform_device *pdev)
{
	struct kw_temp_data *data = platform_get_drvdata(pdev);

	hwmon_device_unregister(hwmon_dev);
	sysfs_remove_group(&pdev->dev.kobj, &kw_temp_group);
	platform_set_drvdata(pdev, NULL);
	kfree(data);
	return 0;
}


static struct platform_driver kw_temp_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "kw-temp",
	},
	.probe = kw_temp_probe,
	.remove = __devexit_p(kw_temp_remove),
};

static int __init kw_temp_init(void) 
{
	return platform_driver_register(&kw_temp_driver);
}

static void __exit kw_temp_exit(void)
{
	platform_driver_unregister(&kw_temp_driver);
}


MODULE_AUTHOR("Marvell Semiconductors");
MODULE_DESCRIPTION("Marvell Kirkwood SoC hwmon driver");
MODULE_LICENSE("GPL");

module_init(kw_temp_init)
module_exit(kw_temp_exit)