#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/device.h>
#include <video/dovefbreg.h>

static unsigned int *lcd0_regbase;
static unsigned int *lcd1_regbase;

/*
 * Initialize lcd reg base address.
 * If success, return 0.
 * If failed, return -1.
 */
int dovefb_save_regbase(void *reg_base, unsigned int id)
{
	if (!reg_base)
		return -1;

	switch (id) {
	case 0:
		lcd0_regbase = reg_base;
		break;
	case 1:
		lcd1_regbase = reg_base;
		break;
	default:
		printk(KERN_WARNING "Unknown lcd id\n");
		return -2;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(dovefb_save_regbase);

/*
 * Get base addr by lcd id
 * if success, return addr;
 * if failed, return 0;
 */
void *dovefb_get_addr_by_id(unsigned int lcd_id)
{
	switch (lcd_id) {
	case 0:
		return lcd0_regbase;
	case 1:
		return lcd1_regbase;
	default:
		printk(KERN_WARNING "Unknown lcd id\n");
	}

	return 0;
}

/*
 * Get current dumb panel io pad mode.
 * if success, return current mode.
 * if failed, return -1.
 */
int dovefb_get_cur_mode(void *reg_base)
{
	if (!reg_base)
		return -1;

	return readl(reg_base+SPU_IOPAD_CONTROL) & 0x0000000F;
}

/*
 * Get total gpio mask length
 * if success, return gpio mask.
 * if failed, return 0.
 */
unsigned int dovefb_get_gpio_mask(unsigned int mode)
{
	unsigned int gpio_mask = 0;

	switch (mode) {
	case 0x2:
		gpio_mask = 0x1F; /* 0~4 */
		break;
	case 0x3:
		gpio_mask = 0x3;  /* 0~1 */
		break;
	case 0x4:
		gpio_mask = 0x7F; /* 0~6 */
		break;
	case 0x5:
		gpio_mask = 0x3F; /* 0~5 */
		break;
	case 0x8:
		gpio_mask = 0xFF; /* 0~7 */
		break;
	default:
		printk(KERN_WARNING "Current IO PAD mode"
			"doesn't has GPIO output\n");
	}

	return 0;
}

/*
 * Get current mask & output gpio settings
 * if success, return 0.
 * if failed, return -1.
 */
int dovefb_get_current_setting(unsigned int lcd_id,
	unsigned int *mask,
	unsigned int *output)
{
	void *reg_base;
	unsigned int reg_val;

	reg_base = dovefb_get_addr_by_id(lcd_id);

	if (!reg_base || !mask || !output)
		return -1;

	reg_val	= readl(reg_base+LCD_SPU_DUMB_CTRL);
	*mask	= (reg_val & 0x000FF000) >> 12;
	*output	= (reg_val & 0x0FF00000) >> 20;

	return 0;
}
EXPORT_SYMBOL_GPL(dovefb_get_current_setting);

/*
 * set LCD GPIO data.
 * mask, is the bit mask you want to set. bit 0 = lcd gpio mask 0
 * data, is the output data you want to set. bit 0 = lcd gpio 0
 * if success, return 0.
 * if failed, return -1.
 */
int dovefb_set_gpio(unsigned int lcd_id, unsigned int mask, unsigned int output)
{
	unsigned int cur_mode;
	unsigned int gpio_mask;
	unsigned int reg_val;
	void *reg_base;

	reg_base = dovefb_get_addr_by_id(lcd_id);

	if (!reg_base) {
		printk(KERN_WARNING "Must init lcd reg base first!!\n");
		return -1;
	}

	/* check current mode. */
	cur_mode = dovefb_get_cur_mode(reg_base);

	/* get total mask */
	gpio_mask = dovefb_get_gpio_mask(cur_mode);

	/* check mask length. */
	if ((gpio_mask == 0) || (mask > gpio_mask))
		return -2;

	/* get current output data */
	reg_val	= readl(reg_base+LCD_SPU_DUMB_CTRL);

	/* clear all settings. */
	reg_val &= ~0x0ffff000;

	/* apply new setting. */
	reg_val |= (mask & 0x000000FF) << 12;
	reg_val |= (output & 0x000000FF) << 20;

	/* write new setting back. */
	writel(reg_val, reg_base+LCD_SPU_DUMB_CTRL);

	return 0;
}
EXPORT_SYMBOL_GPL(dovefb_set_gpio);

/*
 * set specific gpio pin state.
 * pin, is gpio number. 0~7
 * state, is gpio state. 0 or 1.
 * if success, return 0.
 * if failed, return -1.
 */
int dovefb_set_pin_state(unsigned int lcd_id,
	unsigned int pin, unsigned int state)
{
	unsigned int mask;
	unsigned int output;

	dovefb_get_current_setting(lcd_id, &mask, &output);

	/* clear old setting */
	mask &= ~(0x1 << pin);
	output &= ~(0x1 << pin);

	/* calc new setting */
	mask |= 0x1 << pin;
	output |= state << pin;

	/* apply it */
	dovefb_set_gpio(lcd_id, mask, output);

	return 0;
}
EXPORT_SYMBOL_GPL(dovefb_set_pin_state);
