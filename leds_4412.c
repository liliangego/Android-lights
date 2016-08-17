#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/device.h>  // class_create
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/delay.h>
 
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

#include <linux/leds.h> // led_classdev


//参考:(linux-3.0.86\drivers\leds\leds-s3c24xx.c)
// s3c_gpio_cfgpin(led_gpios[i], S3C_GPIO_OUTPUT);
// gpio_set_value(led_gpios[arg], !cmd);

/*
static int led_gpios[] = {
	EXYNOS4212_GPM4(0),
	EXYNOS4212_GPM4(1),
	EXYNOS4212_GPM4(2),
	EXYNOS4212_GPM4(3),
};
*/
struct led_desc {
	int gpio;
	char *name;
};

static struct led_desc led_4412_gpios[] = {
	{EXYNOS4212_GPM4(0),"led1"},
	{EXYNOS4212_GPM4(1),"led2"},
	{EXYNOS4212_GPM4(2),"led3"},
	{EXYNOS4212_GPM4(3),"led4"},
};


struct led_classdev_4412 {
	struct led_classdev	cdev;
	unsigned int gpio;
};


static struct led_classdev_4412 *led_devs;
#define  LED_NUM (sizeof(led_4412_gpios)/sizeof(led_4412_gpios[0]))


static void brightness_set_4412(struct led_classdev *led_cdev,
			    enum led_brightness brightness)
{
	struct led_classdev_4412 *dev = (struct led_classdev_4412 *)led_cdev;
	
	led_cdev->brightness = brightness;

	/* 传进来的亮度不等于LED_OFF,就要点亮 */
	if(brightness != LED_OFF)
		gpio_set_value(dev->gpio, 0);
	else
		gpio_set_value(dev->gpio, 1);
		
}

static int __init leds_init(void)
{
	int i,ret;
	/* 1.alloc led_classdev */
	led_devs = kzalloc(sizeof(struct led_classdev_4412) * LED_NUM, GFP_KERNEL);
	if (led_devs == NULL) {
			printk("No memory for device\n");
			return -ENOMEM;
	}
	/* 4 leds */
	for(i = 0; i < LED_NUM; i++){
		/* led off */
		s3c_gpio_cfgpin(led_4412_gpios[i].gpio, S3C_GPIO_OUTPUT);//output 
		gpio_set_value(led_4412_gpios[i].gpio, 1);  // set 1 - led off
		
		/* 2.set */
		led_devs[i].cdev.max_brightness = LED_FULL; // 255
		led_devs[i].cdev.brightness_set = brightness_set_4412;
		led_devs[i].cdev.flags 			= LED_CORE_SUSPENDRESUME;
		led_devs[i].cdev.brightness 	= LED_OFF; // 0
		led_devs[i].cdev.name 			= led_4412_gpios[i].name;
		led_devs[i].gpio 				= led_4412_gpios[i].gpio;

		/* 3.register led_classdev */
		ret = led_classdev_register(NULL, &led_devs[i].cdev);
		if (ret < 0) {
			/* 如果i=0注册成功,i=1注册失败.需要把之前都取消注册 */
			i--;
			while(i >= 0)
			{
				led_classdev_unregister(&led_devs[i].cdev);
				i--;
			}
			kfree(led_devs);
			return -EIO;
		}
	}
	return 0;
}


static void __exit leds_exit(void){
	int i;
	for(i = 0; i < LED_NUM; i++)
		led_classdev_unregister(&led_devs[i].cdev);
	kfree(led_devs);
}

module_init(leds_init);
module_exit(leds_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Liliang");

