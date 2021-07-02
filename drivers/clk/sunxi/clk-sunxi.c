/*
 * Copyright (C) 2016-2020 Allwinnertech
 * Wim Hwang <huangwei@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/clk/sunxi.h>
#include <linux/clk/clk-conf.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/syscore_ops.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include "clk-sunxi.h"
#include "clk-factors.h"
#include "clk-periph.h"
#include "clk-cpu.h"

#if IS_ENABLED(CONFIG_PM_SLEEP)
/*list head use for standby*/
LIST_HEAD(clk_periph_reg_cache_list);
LIST_HEAD(clk_factor_reg_cache_list);
#endif

struct clock_provider {
	void (*clk_init_cb)(struct device_node *);
	struct device_node *np;
	struct list_head node;
};
/*
 * of_sunxi_clocks_init() - Clocks initialize
 */
static void of_sunxi_clocks_init(struct device_node *node)
{
	/* do some soc special init here */
	sunxi_clocks_init(node);
}

static void of_sunxi_fixed_clk_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;
	u32 rate;

	if (of_property_read_u32(node, "clock-frequency", &rate))
		return;

	if (of_property_read_string(node, "clock-output-names", &clk_name)) {
		pr_err("%s:get clock-output-names failed in %s node\n",
						__func__, node->full_name);
		return;
	}

	clk = clk_register_fixed_rate(NULL, clk_name, NULL, CLK_IS_ROOT, rate);
	if (!IS_ERR(clk)) {
		clk_register_clkdev(clk, clk_name, NULL);
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
	}
}

static void of_sunxi_fixed_factor_clk_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;
	const char *parent_name;
	u32 div, mult;

	if (of_property_read_u32(node, "clock-div", &div)) {
		pr_err("%s Fixed factor clock <%s> must have a clock-div property\n",
			__func__, node->name);
		return;
	}

	if (of_property_read_u32(node, "clock-mult", &mult)) {
		pr_err("%s Fixed factor clock <%s> must have a clokc-mult property\n",
			__func__, node->name);
		return;
	}

	if (of_property_read_string(node, "clock-output-names", &clk_name)) {
		pr_err("%s:get clock-output-names failed in %s node\n",
						__func__, node->full_name);
		return;
	}
	parent_name = of_clk_get_parent_name(node, 0);

	clk = clk_register_fixed_factor(NULL, clk_name, parent_name, 0,
					mult, div);
	if (!IS_ERR(clk)) {
		clk_register_clkdev(clk, clk_name, NULL);
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
	}
}

/*
 * of_sunxi_pll_clk_setup() - Setup function for pll factors clk
 */
static void of_sunxi_pll_clk_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;
	const char *lock_mode = NULL;
	struct factor_init_data *factor;

	if (of_property_read_string(node, "clock-output-names", &clk_name)) {
		pr_err("%s:get clock-output-names failed in %s node\n",
						__func__, node->full_name);
		return;
	}
	factor = sunxi_clk_get_factor_by_name(clk_name);
	if (!factor) {
		pr_err("clk %s not found in %s\n", clk_name, __func__);
		return;
	}

	if (!of_property_read_string(node, "lock-mode", &lock_mode))
		sunxi_clk_set_factor_lock_mode(factor, lock_mode);

	clk = sunxi_clk_register_factors(NULL, sunxi_clk_base,
					 &clk_lock, factor);
	if (!IS_ERR(clk)) {
		clk_register_clkdev(clk, clk_name, NULL);
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
	}
}

/*
 * of_sunxi_cpus_pll_clk_setup() - Setup function for prcm pll factors clk
 */
static void of_sunxi_cpus_pll_clk_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;
	const char *lock_mode = NULL;
	struct factor_init_data *factor;

	if (of_property_read_string(node, "clock-output-names", &clk_name)) {
		pr_err("%s:get clock-output-names failed in %s node\n",
						__func__, node->full_name);
		return;
	}
	factor = sunxi_clk_get_factor_by_name(clk_name);
	if (!factor) {
		pr_err("clk %s not found in %s\n", clk_name, __func__);
		return;
	}

	if (!of_property_read_string(node, "lock-mode", &lock_mode))
		sunxi_clk_set_factor_lock_mode(factor, lock_mode);

	clk = sunxi_clk_register_factors(NULL, sunxi_clk_cpus_base,
					 &clk_lock, factor);
	if (!IS_ERR(clk)) {
		clk_register_clkdev(clk, clk_name, NULL);
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
	}
}

/*
 * of_sunxi_periph_clk_setup() - Setup function for periph clk
 */
static void of_sunxi_periph_clk_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;
	struct periph_init_data *periph;

	if (of_property_read_string(node, "clock-output-names", &clk_name)) {
		pr_err("%s:get clock-output-names failed in %s node\n",
						__func__, node->full_name);
		return;
	}

	periph = sunxi_clk_get_periph_by_name(clk_name);
	if (!periph) {
		pr_err("clk %s not found in %s\n", clk_name, __func__);
		return;
	}

	if (!strcmp(clk_name, "losc_out")) {
		clk = sunxi_clk_register_periph(periph, 0);
	} else
		clk = sunxi_clk_register_periph(periph,
					sunxi_clk_base);
	if (!IS_ERR(clk)) {
		clk_register_clkdev(clk, clk_name, NULL);
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
	}
}

/*
 * of_sunxi_cpu_clk_setup() - Setup function for cpu clk
 */
static void of_sunxi_cpu_clk_setup(struct device_node *node)
{
	sunxi_cpu_clocks_init(node);
}

/**
 * of_periph_cpus_clk_setup() - Setup function for periph cpus clk
 */
void of_sunxi_periph_cpus_clk_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;
	struct periph_init_data *periph;

	if (of_property_read_string(node, "clock-output-names", &clk_name)) {
		pr_err("%s:get clock-output-names failed in %s node\n",
						__func__, node->full_name);
		return;
	}

	periph = sunxi_clk_get_periph_cpus_by_name(clk_name);
	if (!periph) {
		pr_err("clk %s not found in %s\n", clk_name, __func__);
		return;
	}

	/* register clk */
	if (!strcmp(clk_name, "losc_out") ||
				!strcmp(clk_name, "dcxo_out") ||
				!strcmp(clk_name, "r_dma") ||
				!strcmp(clk_name, "hosc32k")) {
		clk = sunxi_clk_register_periph(periph, 0);
	} else
		clk = sunxi_clk_register_periph(periph,
					sunxi_clk_cpus_base);

	if (!IS_ERR(clk)) {
		clk_register_clkdev(clk, clk_name, NULL);
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
		return;
	}

	pr_err("clk %s not found in %s\n", clk_name, __func__);
}

/**
 * of_periph_rtc_clk_setup() - Setup function for periph rtc clk
 */
void of_sunxi_periph_rtc_clk_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;
	struct periph_init_data *periph;

	if (of_property_read_string(node, "clock-output-names", &clk_name)) {
		pr_err("%s:get clock-output-names failed in %s node\n",
						__func__, node->full_name);
		return;
	}

	periph = sunxi_clk_get_periph_rtc_by_name(clk_name);
	if (!periph) {
		pr_err("clk %s not found in %s\n", clk_name, __func__);
		return;
	}

	/* register clk */
	clk = sunxi_clk_register_periph(periph,
					sunxi_clk_rtc_base);

	if (!IS_ERR(clk)) {
		clk_register_clkdev(clk, clk_name, NULL);
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
		return;
	}

	pr_err("clk %s not found in %s\n", clk_name, __func__);
}

#if IS_ENABLED(CONFIG_PM_SLEEP)
void sunxi_factor_clk_save(struct sunxi_factor_clk_reg_cache *factor_clk_reg)
{
	factor_clk_reg->config_value = readl(factor_clk_reg->config_reg);
	if (factor_clk_reg->sdmpat_reg)
		factor_clk_reg->sdmpat_value = readl(factor_clk_reg->sdmpat_reg);
}

void sunxi_factor_clk_restore(struct sunxi_factor_clk_reg_cache *factor_clk_reg)
{
	if (factor_clk_reg->sdmpat_reg)
		writel(factor_clk_reg->sdmpat_value, factor_clk_reg->sdmpat_reg);
	writel(factor_clk_reg->config_value, factor_clk_reg->config_reg);

}

void sunxi_periph_clk_save(struct sunxi_periph_clk_reg_cache *periph_clk_reg)
{
	if (periph_clk_reg->gate_dram_reg)
		periph_clk_reg->gate_dram_value = readl(periph_clk_reg->gate_dram_reg);
	if (periph_clk_reg->gate_reset_reg)
		periph_clk_reg->gate_reset_value = readl(periph_clk_reg->gate_reset_reg);
	if (periph_clk_reg->gate_enable_reg)
		periph_clk_reg->gate_enable_value = readl(periph_clk_reg->gate_enable_reg);
	if (periph_clk_reg->divider_reg)
		periph_clk_reg->divider_value = readl(periph_clk_reg->divider_reg);
	if (periph_clk_reg->mux_reg)
		periph_clk_reg->mux_value = readl(periph_clk_reg->mux_reg);
	if (periph_clk_reg->gate_bus_reg)
		periph_clk_reg->gate_bus_value = readl(periph_clk_reg->gate_bus_reg);
}

void sunxi_periph_clk_restore(struct sunxi_periph_clk_reg_cache *periph_clk_reg)
{
	/* we should take care of the order, fix me */
	if (periph_clk_reg->gate_dram_reg)
		writel(periph_clk_reg->gate_dram_value, periph_clk_reg->gate_dram_reg);
	if (periph_clk_reg->gate_reset_reg)
		writel(periph_clk_reg->gate_reset_value, periph_clk_reg->gate_reset_reg);
	if (periph_clk_reg->gate_enable_reg)
		writel(periph_clk_reg->gate_enable_value, periph_clk_reg->gate_enable_reg);
	if (periph_clk_reg->divider_reg)
		writel(periph_clk_reg->divider_value, periph_clk_reg->divider_reg);
	if (periph_clk_reg->mux_reg)
		writel(periph_clk_reg->mux_value, periph_clk_reg->mux_reg);
	if (periph_clk_reg->gate_bus_reg)
		writel(periph_clk_reg->gate_bus_value, periph_clk_reg->gate_bus_reg);
}
#endif

#ifndef MODULE
CLK_OF_DECLARE(sunxi_clocks_init, "allwinner,clk-init", of_sunxi_clocks_init);
CLK_OF_DECLARE(sunxi_fixed_clk, "allwinner,fixed-clock", of_sunxi_fixed_clk_setup);
CLK_OF_DECLARE(sunxi_pll_clk, "allwinner,pll-clock", of_sunxi_pll_clk_setup);
CLK_OF_DECLARE(sunxi_cpus_pll_clk, "allwinner,cpus-pll-clock", of_sunxi_cpus_pll_clk_setup);
CLK_OF_DECLARE(sunxi_fixed_factor_clk, "allwinner,fixed-factor-clock", of_sunxi_fixed_factor_clk_setup);
CLK_OF_DECLARE(sunxi_cpu_clk, "allwinner,cpu-clock", of_sunxi_cpu_clk_setup);
CLK_OF_DECLARE(sunxi_periph_clk, "allwinner,periph-clock", of_sunxi_periph_clk_setup);
CLK_OF_DECLARE(periph_cpus_clk, "allwinner,periph-cpus-clock", of_sunxi_periph_cpus_clk_setup);
CLK_OF_DECLARE(periph_rtc_clk, "allwinner,periph-rtc-clock", of_sunxi_periph_rtc_clk_setup);
#else
static const struct of_device_id sunxi_clock_init_tab[] = {
	{ .compatible = "allwinner,clk-init",
	  .data = of_sunxi_clocks_init,
	},
	{ .compatible = "allwinner,fixed-clock",
	  .data = of_sunxi_fixed_clk_setup,
	},
	{ .compatible = "allwinner,pll-clock",
	  .data = of_sunxi_pll_clk_setup,
	},
	{ .compatible = "allwinner,cpus-pll-clock",
	  .data = of_sunxi_cpus_pll_clk_setup,
	},
	{ .compatible = "allwinner,fixed-factor-clock",
	  .data = of_sunxi_fixed_factor_clk_setup,
	},
	{ .compatible = "allwinner,cpu-clock",
	  .data = of_sunxi_cpu_clk_setup,
	},
	{ .compatible = "allwinner,periph-clock",
	  .data = of_sunxi_periph_clk_setup,
	},
	{ .compatible = "allwinner,periph-cpus-clock",
	  .data = of_sunxi_periph_cpus_clk_setup,
	},
	{ .compatible = "allwinner,periph-rtc-clock",
	  .data = of_sunxi_periph_rtc_clk_setup,
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sunxi_clock_init_tab);

static int of_clocks_init(struct platform_device *pdev)
{
	const struct of_device_id *match;
	const struct of_device_id *matches = sunxi_clock_init_tab;
	struct device_node *np;
	struct clock_provider *clk_provider, *next;
	bool is_init_done;
	bool force = false;
	LIST_HEAD(clk_provider_list);

	/* First prepare the list of the clocks providers */
	for_each_matching_node_and_match(np, matches, &match) {
		struct clock_provider *parent;

		if (!of_device_is_available(np))
			continue;

		parent = kzalloc(sizeof(*parent), GFP_KERNEL);
		if (!parent) {
			list_for_each_entry_safe(clk_provider, next,
						 &clk_provider_list, node) {
				list_del(&clk_provider->node);
				of_node_put(clk_provider->np);
				kfree(clk_provider);
			}
			of_node_put(np);
			return -EINVAL;
		}
		parent->clk_init_cb = match->data;
		parent->np = of_node_get(np);
		list_add_tail(&parent->node, &clk_provider_list);
	}

	while (!list_empty(&clk_provider_list)) {
		is_init_done = false;
		list_for_each_entry_safe(clk_provider, next,
					&clk_provider_list, node) {
				clk_provider->clk_init_cb(clk_provider->np);
				of_clk_set_defaults(clk_provider->np, true);

				list_del(&clk_provider->node);
				of_node_put(clk_provider->np);
				kfree(clk_provider);
				is_init_done = true;
		}

		/*
		 * We didn't manage to initialize any of the
		 * remaining providers during the last loop, so now we
		 * initialize all the remaining ones unconditionally
		 * in case the clock parent was not mandatory
		 */
		if (!is_init_done)
			force = true;
	}
	return 0;
}

static struct platform_driver sunxi_clk_init_driver = {
	.driver = {
		.name	= "sunxi-clk-init",
		.of_match_table = sunxi_clock_init_tab,
	},
	.probe = of_clocks_init,
};
module_platform_driver(sunxi_clk_init_driver);
MODULE_LICENSE("GPL");
#endif
