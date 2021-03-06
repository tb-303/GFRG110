#include <boardEnv/mvBoardEnvLib.h>
#include <gpp/mvGpp.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/leds.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>

#define BOARD_NAME		"gflt200"
#define BOARD_HW_VER_EVT1	0
#define BOARD_HW_VER_EVT2	1
#define GPIO_BOARD_VER_0	13
#define GPIO_BOARD_VER_1	15
#define GPIO_BOARD_VER_2	18
#define GPIO_PON_PWR_EN		37
#define GPIO_PON_TX_DIS		21

struct board_gpio {
	unsigned	gpio;
	const char	*label;
};

static struct board_gpio board_gpios[] = {
	{
		.gpio = GPIO_PON_PWR_EN,
		.label = "power-enable",
	},
	{
		.gpio = GPIO_PON_TX_DIS,
		.label = "tx-disable",
	},
};

static int board_gpio_export(struct board_gpio *gpio, struct device *dev)
{
	int rc;

	rc = gpio_request(gpio->gpio, gpio->label);
	if (rc) {
		pr_err(BOARD_NAME ": error %d requesting gpio %u (%s)\n", rc,
			gpio->gpio, gpio->label);
		goto exit;
	}

	/* this is needed to set gpiolib's out flag for the gpio */
	rc = gpio_direction_output(gpio->gpio, gpio_get_value(gpio->gpio));
	if (rc) {
		pr_err(BOARD_NAME ": error %d setting gpio %u (%s) direction\n",
			rc, gpio->gpio, gpio->label);
		goto exit;
	}

	rc = gpio_export(gpio->gpio, false);
	if (rc) {
		pr_err(BOARD_NAME ": error %d exporting gpio %u (%s)\n", rc,
			gpio->gpio, gpio->label);
		goto exit;
	}

	rc = gpio_export_link(dev, gpio->label, gpio->gpio);
	if (rc) {
		pr_err(BOARD_NAME ": error %d linking gpio %u (%s)\n", rc,
			gpio->gpio, gpio->label);
		goto exit;
	}

	rc = 0;
exit:
	return rc;
}

static int board_hw_ver(void)
{
	return gpio_get_value(GPIO_BOARD_VER_0)
		| (gpio_get_value(GPIO_BOARD_VER_1) << 1)
		| (gpio_get_value(GPIO_BOARD_VER_2) << 2);
}

static ssize_t board_hw_ver_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", board_hw_ver());
}

static DEVICE_ATTR(hw_ver, S_IRUGO, board_hw_ver_show, NULL);

static struct gpio_led board_evt1_gpio_leds[] = {
	{
		.name = "sys-blue",
		.gpio = 11,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
	},
	{
		.name = "sys-red",
		.gpio = 12,
		.default_state = LEDS_GPIO_DEFSTATE_ON,
	},
};

static struct gpio_led board_evt2_gpio_leds[] = {
	{
		.name = "sys-blue",
		.gpio = 9,
		.active_low = 1,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
	},
	{
		.name = "sys-red",
		.gpio = 10,
		.active_low = 1,
		.default_state = LEDS_GPIO_DEFSTATE_ON,
	},
};

static int board_gpio_blink_set(unsigned gpio, unsigned long *delay_on,
				unsigned long *delay_off);

static struct gpio_led_platform_data board_gpio_leds_data = {
	.gpio_blink_set = board_gpio_blink_set,
};

int board_gpio_blink_set(unsigned gpio, unsigned long *delay_on,
				unsigned long *delay_off)
{
	int i;
	int active_low;
	MV_U32 mask = 1 << (gpio %32);
	MV_U32 group = gpio / 32;
	MV_U32 cycles_per_ms = mvBoardTclkGet() / 1000;
	unsigned long max_delay = ~0 / cycles_per_ms;

	for (i = 0; i < board_gpio_leds_data.num_leds; i++) {
		if (gpio == board_gpio_leds_data.leds[i].gpio) {
			active_low = board_gpio_leds_data.leds[i].active_low;
			break;
		}
	}

	if (i == board_gpio_leds_data.num_leds)
		return -EINVAL;

	*delay_on = min(*delay_on, max_delay);
	*delay_off = min(*delay_off, max_delay);

	if (*delay_on && *delay_off) {
		if (active_low)
			mvGppBlinkCntrSet(MV_GPP_BLINK_CNTR_A,
						*delay_off * cycles_per_ms,
						*delay_on * cycles_per_ms);
		else
			mvGppBlinkCntrSet(MV_GPP_BLINK_CNTR_A,
						*delay_on * cycles_per_ms,
						*delay_off * cycles_per_ms);

		mvGppBlinkEn(group, mask, mask);
	}
	else
		mvGppBlinkEn(group, mask, 0);

	return 0;
}

static struct platform_device board_gpio_leds_device = {
        .name = "leds-gpio",
        .id = -1,
        .dev.platform_data = &board_gpio_leds_data,
};

static struct i2c_board_info board_i2c_devices[] = {
	{
		I2C_BOARD_INFO("pcf8523", 0x68),
	},
};

int __init board_init(void)
{
	int i;
	int rc;
	int hw_ver;
	struct platform_device *pdev;

	switch ((hw_ver = board_hw_ver())) {
	case BOARD_HW_VER_EVT1:
		board_gpio_leds_data.num_leds
			= ARRAY_SIZE(board_evt1_gpio_leds);
		board_gpio_leds_data.leds = board_evt1_gpio_leds;
		break;
	default:
		pr_err(BOARD_NAME ": unknown hardware version '%d'\n", hw_ver);
		/* fallthrough */
	case BOARD_HW_VER_EVT2:
		board_gpio_leds_data.num_leds
			= ARRAY_SIZE(board_evt2_gpio_leds);
		board_gpio_leds_data.leds = board_evt2_gpio_leds;
		break;
	}

	/* /sys/devices/platform/<board_name> */
	pdev = platform_device_register_simple(BOARD_NAME, -1, NULL, 0);
	if (IS_ERR(pdev)) {
		rc = PTR_ERR(pdev);
		pr_err(BOARD_NAME ": error %d registering device\n", rc);
		return rc;
	}

	/* /sys/devices/platform/board -> /sys/devices/platform/<board_name> */
	rc = sysfs_create_link(&pdev->dev.parent->kobj, &pdev->dev.kobj,
				"board");
	if (rc)
		pr_err(BOARD_NAME ": error %d creating link 'board'\n", rc);

	/* /sys/devices/platform/board/<gpio_name> */
	for (i = 0; i < ARRAY_SIZE(board_gpios); i++)
		board_gpio_export(&board_gpios[i], &pdev->dev);

	/* /sys/devices/platform/board/hw_ver */
	rc = device_create_file(&pdev->dev, &dev_attr_hw_ver);
	if (rc)
		pr_err(BOARD_NAME ": error %d creating attribute 'hw_ver'\n",
			rc);

	rc = platform_device_register(&board_gpio_leds_device);
	if (rc)
		pr_err(BOARD_NAME ": error %d registering GPIO LEDs device\n",
			rc);

	rc = i2c_register_board_info(0, board_i2c_devices,
					ARRAY_SIZE(board_i2c_devices));
	if (rc)
		pr_err(BOARD_NAME ": error %d registering I2C devices\n",
			rc);

	return 0;
}

device_initcall(board_init);
