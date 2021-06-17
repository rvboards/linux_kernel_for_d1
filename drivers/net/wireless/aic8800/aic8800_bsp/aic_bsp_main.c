#include <linux/module.h>
#include <linux/inetdevice.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/platform_device.h>

#define DRV_DESCRIPTION       "AIC BSP"
#define DRV_COPYRIGHT         "Copyright(c) 2015-2020 AICSemi"
#define DRV_AUTHOR            "AICSemi"
#define DRV_VERS_MOD          "1.0"

static struct platform_device *aicbsp_pdev;

static struct platform_driver aicbsp_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "aic_bsp",
	},
	//.probe = aicbsp_probe,
	//.remove = aicbsp_remove,
};

static int __init aicbsp_init(void)
{
	int ret;
	printk("%s\n", __func__);
	ret = platform_driver_register(&aicbsp_driver);
	if (ret) {
		pr_err("register platform driver failed: %d\n", ret);
		return ret;
	}

	aicbsp_pdev = platform_device_alloc("aic-bsp", -1);
	ret = platform_device_add(aicbsp_pdev);
	if (ret) {
		pr_err("register platform device failed: %d\n", ret);
		return ret;
	}

	return 0;
}

static void __exit aicbsp_exit(void)
{
	platform_device_del(aicbsp_pdev);
	platform_driver_unregister(&aicbsp_driver);
	printk("%s\n", __func__);
}

module_init(aicbsp_init);
module_exit(aicbsp_exit);

MODULE_DESCRIPTION(DRV_DESCRIPTION);
MODULE_VERSION(DRV_VERS_MOD);
MODULE_AUTHOR(DRV_COPYRIGHT " " DRV_AUTHOR);
MODULE_LICENSE("GPL");
