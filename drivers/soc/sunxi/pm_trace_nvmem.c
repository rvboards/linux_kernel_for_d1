#define pr_fmt(fmt) "pm_trace: " fmt

#include <linux/pm-trace.h>
#include <linux/pm.h>
#include <asm/pm-trace.h>
#include <linux/export.h>
#include <linux/suspend.h>
#include <linux/rtc.h>
#include <linux/of.h>
#include <linux/nvmem-consumer.h>
#include <linux/platform_device.h>
#include "../drivers/base/power/power.h"

#define USERHASH (16)
#define FILEHASH (997)
#define DEVHASH (1009)

#define DEVSEED (7919)

static uint32_t hash_value_early_read;
static uint32_t dev_hash_value;
static struct nvmem_cell *nc;

static unsigned int hash_string(unsigned int seed, const char *data,
				unsigned int mod)
{
	unsigned char c;

	while ((c = *data++) != 0) {
		seed = (seed << 16) + (seed << 6) - seed + c;
	}

	return seed % mod;
}

void generate_pm_trace(const void *tracedata, unsigned int user)
{
	if (!IS_ERR_OR_NULL(nc))
		nvmem_cell_write(nc, &dev_hash_value, sizeof(uint32_t));
}
EXPORT_SYMBOL(generate_pm_trace);

int show_trace_dev_match(char *buf, size_t size)
{
	unsigned int value = hash_value_early_read;
	int ret = 0;
	struct list_head *entry;

	/*
	 * It's possible that multiple devices will match the hash and we can't
	 * tell which is the culprit, so it's best to output them all.
	 */
	device_pm_lock();
	entry = dpm_list.prev;
	while (size && entry != &dpm_list) {
		struct device *dev = to_device(entry);
		unsigned int hash = hash_string(DEVSEED, dev_name(dev),
						DEVHASH);
		if (hash == value) {
			int len = snprintf(buf, size, "%s\n",
					    dev_driver_string(dev));
			if (len > size)
				len = size;
			buf += len;
			ret += len;
			size -= len;
		}
		entry = entry->prev;
	}
	device_pm_unlock();
	return ret;
}
EXPORT_SYMBOL(show_trace_dev_match);

void set_trace_device(struct device *dev)
{
	dev_hash_value = hash_string(DEVSEED, dev_name(dev), DEVHASH);
}
EXPORT_SYMBOL(set_trace_device);

static int sunxi_pm_trace_probe(struct platform_device *pdev)
{
	struct device_node *nd;
	size_t size;
	int rst = 0;
	unsigned int *v;

	nd = of_find_node_with_property(NULL, "pm-trace");
	if (!nd) {
		pr_err("can not find pm-trace node\n");
		rst = -ENODEV;
		goto nodev;
	}

	nc = of_nvmem_cell_get(nd, NULL);
	if (IS_ERR_OR_NULL(nc)) {
		pr_err("can not get nvmem cell\n");
		rst = PTR_ERR(nc);
		goto nocell;
	}

	v = nvmem_cell_read(nc, &size);
	if (IS_ERR(v)) {
		pr_err("can not read nvmem cell\n");
		rst = -EIO;
		goto err_cell_read;
	}
	if (sizeof(uint32_t) >= size)
		memcpy(&hash_value_early_read, v, size);

	kfree(v);

	return 0;

err_cell_read:
	nvmem_cell_put(nc);
nocell:
	of_node_put(nd);
nodev:
	return rst;
}

static int sunxi_pm_trace_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver pm_trace_drv = {
	.probe = sunxi_pm_trace_probe,
	.remove = sunxi_pm_trace_remove,
	.driver = {
		.name = "pm-trace",
		.owner = THIS_MODULE,
	},
};

static int pm_trace_init(void)
{
	platform_device_register_simple("pm-trace", PLATFORM_DEVID_NONE, 0, 0);
	platform_driver_register(&pm_trace_drv);

	return 0;
}

late_initcall(pm_trace_init);


