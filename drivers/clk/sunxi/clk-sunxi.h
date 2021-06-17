/*
 * Copyright (C) 2013 Allwinnertech, kevin.z.m <kevin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Adjustable factor-based clock implementation
 */
#ifndef __MACH_SUNXI_CLK_SUNXI_H
#define __MACH_SUNXI_CLK_SUNXI_H

#define CLK_FLAGS_BIT_MAX		(14)  /* the max bit used in include/linux/clk-provider.h */
#define CLK_IS_ROOT			BIT(CLK_FLAGS_BIT_MAX + 1)
#define CLK_IGNORE_AUTORESET    	BIT(CLK_FLAGS_BIT_MAX + 2)
#define CLK_REVERT_ENABLE		BIT(CLK_FLAGS_BIT_MAX + 3)
#define CLK_IGNORE_SYNCBOOT     	BIT(CLK_FLAGS_BIT_MAX + 4)
#define CLK_READONLY			BIT(CLK_FLAGS_BIT_MAX + 5)
#define CLK_IGNORE_DISABLE		BIT(CLK_FLAGS_BIT_MAX + 6)
#define CLK_RATE_FLAT_FACTORS		BIT(CLK_FLAGS_BIT_MAX + 7)
#define CLK_RATE_FLAT_DELAY		BIT(CLK_FLAGS_BIT_MAX + 8)
#define CLK_NO_DISABLE			BIT(CLK_FLAGS_BIT_MAX + 9)
#define CLK_IGNORE_ENABLE_DISABLE	BIT(CLK_FLAGS_BIT_MAX + 10)

#define to_clk_factor(_hw) container_of(_hw, struct sunxi_clk_factors, hw)

#define SETMASK(width, shift)   ((width?((-1U) >> (32-width)):0)  << (shift))
#define CLRMASK(width, shift)   (~(SETMASK(width, shift)))
#define GET_BITS(shift, width, reg)     \
	(((reg) & SETMASK(width, shift)) >> (shift))
#define SET_BITS(shift, width, reg, val) \
	(((reg) & CLRMASK(width, shift)) | (val << (shift)))

#define __SUNXI_ALL_CLK_IGNORE_UNUSED__  1

struct sunxi_reg_ops {
	u32 (*reg_readl)(void __iomem *reg);
	void (*reg_writel)(u32 val, void __iomem *reg);
};

#if IS_ENABLED(CONFIG_PM_SLEEP)
struct sunxi_factor_clk_reg_cache {
	struct list_head node;
	void __iomem *config_reg;
	u32	config_value;
	void __iomem *sdmpat_reg;
	u32 sdmpat_value;
};

struct sunxi_periph_clk_reg_cache {
	struct list_head node;
	void __iomem *mux_reg;
	u32	mux_value;
	void __iomem *divider_reg;
	u32 divider_value;
	void __iomem *gate_enable_reg;
	u32 gate_enable_value;
	void __iomem *gate_reset_reg;
	u32 gate_reset_value;
	void __iomem *gate_bus_reg;
	u32 gate_bus_value;
	void __iomem *gate_dram_reg;
	u32 gate_dram_value;
};

extern struct list_head clk_periph_reg_cache_list;
extern struct list_head clk_factor_reg_cache_list;

void sunxi_factor_clk_save(struct sunxi_factor_clk_reg_cache *factor_clk_reg);
void sunxi_factor_clk_restore(struct sunxi_factor_clk_reg_cache *factor_clk_reg);
void sunxi_periph_clk_save(struct sunxi_periph_clk_reg_cache *periph_clk_reg);
void sunxi_periph_clk_restore(struct sunxi_periph_clk_reg_cache *periph_clk_reg);
#endif

extern spinlock_t clk_lock;
extern void __iomem *sunxi_clk_base;
extern void __iomem *sunxi_clk_cpus_base;
extern void __iomem *sunxi_clk_rtc_base;

void sunxi_clocks_init(struct device_node *node);
void sunxi_cpu_clocks_init(struct device_node *node);

struct factor_init_data *sunxi_clk_get_factor_by_name(const char *name);
struct periph_init_data *sunxi_clk_get_periph_by_name(const char *name);
struct periph_init_data *sunxi_clk_get_periph_rtc_by_name(const char *name);
struct periph_init_data *sunxi_clk_get_periph_cpus_by_name(const char *name);
#endif
