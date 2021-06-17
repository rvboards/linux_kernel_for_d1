/*
 * Allwinner DDR Clock driver.
 *
 * Copyright (C) 2020 Allwinner Technology, Inc.
 *	fanqinghua <fanqinghua@allwinnertech.com>
 *
 * Implementation of ddr clock source driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/regmap.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>

#define DRAM_CLK_REG			0x800

#define MC_DFS_CONFIG	        0x1000
#define MC_DFS_CMD(x)	        (0x1800 + (x) * 0x10) //x=0~63
#define LAST_CMD				BIT(31)
#define PADHOLD_CLR				BIT(25)
#define PADHOLD_SET				BIT(24)
#define CMD_CCU					BIT(19)
#define ACC_PHY					BIT(18)
#define CMD_WR					BIT(16)

#define MC_DFS_WDATA_RDATA(x)	(0x1804 + (x) * 0x10) //x=0~63
#define MC_DFS_DATA_MASK(x)	    (0x1808 + (x) * 0x10) //x=0~63
#define MC_DFS_DIV				0x18b0
#define MC_DFS_STATE			0x19b0

#define DIV_SHIFT			0
#define DIV_WIDTH			5

#define DRIVER_NAME	"DDR Clock Driver"

struct sunxi_ddrclk {
	struct device *dev;
	void __iomem	*ccmu_base;
	void __iomem	*mctl_base;
	unsigned int	dram_clk;
	struct clk_hw	hw;
	struct mutex  ddrfreq_lock;
	spinlock_t      lock;
};

#define to_sunxi_ddrclk_hw(_hw) container_of(_hw, struct sunxi_ddrclk, hw)

static inline void mdfs_conf(struct sunxi_ddrclk *ddrclk,
								unsigned int reg,
								unsigned int flag,
								unsigned int data,
								unsigned int dmask,
								unsigned int id)
{
	writel_relaxed(reg | flag, ddrclk->mctl_base + MC_DFS_CMD(id));
	writel_relaxed(data, ddrclk->mctl_base + MC_DFS_WDATA_RDATA(id));
	writel_relaxed(dmask, ddrclk->mctl_base + MC_DFS_DATA_MASK(id));
}

void mctl_dfs(struct sunxi_ddrclk *ddrclk, unsigned int dram_freq_div)
{
	unsigned int cmd_id = 0, freq_id, timeout;
	unsigned long flags;

	switch (dram_freq_div) {
	case 3:
		freq_id = 0;
		break;
	case 4:
		freq_id = 1;
		break;
	case 5:
		freq_id = 2;
		break;
	case 7:
		freq_id = 3;
		break;
	default:
		dev_err(ddrclk->dev, "invalid dram_freq_div:%d\n", dram_freq_div);
		return;
	}

	spin_lock_irqsave(&ddrclk->lock, flags);
	mdfs_conf(ddrclk, 0x15 << 2, ACC_PHY | CMD_WR, (freq_id << 2) | (0x1 << 5), 0xFFFFFF03, cmd_id++);
	mdfs_conf(ddrclk, 0x320, CMD_WR, 0x0, 0xFFFFFFFE, cmd_id++);
	mdfs_conf(ddrclk, 0x304, CMD_WR, 0x2, 0xFFFFFFFE, cmd_id++);
	mdfs_conf(ddrclk, 0x308, 0x0, 0x36000000, 0xC9FFFFFF, cmd_id++);
	mdfs_conf(ddrclk, 0x30, CMD_WR, 0x20, 0xFFFFFFDF, cmd_id++);
	mdfs_conf(ddrclk, 0x4, 0x0, 0x3, 0xFFFFFFFC, cmd_id++);
	mdfs_conf(ddrclk, 0x15 << 2, PADHOLD_SET | ACC_PHY, (freq_id << 2) | (0x1 << 5), 0xFFFFFF00, cmd_id++);
	mdfs_conf(ddrclk, 0x1B0, CMD_WR, (((dram_freq_div + 1) / 4 - 1) << 8) | (0x1 << 5), 0xFFFFFEDF, cmd_id++);
	mdfs_conf(ddrclk, 0x320, CMD_WR, 0x1, 0xFFFFFFFE, cmd_id++);
	mdfs_conf(ddrclk, 0x53 << 2, ACC_PHY | CMD_WR, 0x8, 0xFFFFFFF7, cmd_id++);
	mdfs_conf(ddrclk, 0x16 << 2, ACC_PHY | CMD_WR, 0x0, 0xFFFFFFDF, cmd_id++);
	mdfs_conf(ddrclk, dram_freq_div, CMD_CCU, 0x0, 0x0, cmd_id++);
	mdfs_conf(ddrclk, 0x53 << 2, ACC_PHY | CMD_WR, 0x0, 0xFFFFFFF7, cmd_id++);
	mdfs_conf(ddrclk, 0x60 << 2, ACC_PHY, 0x4, 0xFFFFFFFB, cmd_id++);
	mdfs_conf(ddrclk, 0x16 << 2, ACC_PHY | CMD_WR, 0x20, 0xFFFFFFDF, cmd_id++);
	mdfs_conf(ddrclk, 0x320, CMD_WR, 0x0, 0xFFFFFFFE, cmd_id++);
	mdfs_conf(ddrclk, 0x0, CMD_WR, 0x20000000, 0xDFFFFFFF, cmd_id++);
	mdfs_conf(ddrclk, 0x28, CMD_WR, ((dram_freq_div + 1) / 4 - 1) << 0, 0xFFFFFFFC, cmd_id++);
	mdfs_conf(ddrclk, 0x1B0, CMD_WR, 0x0, 0xFFFFFFDF, cmd_id++);
	mdfs_conf(ddrclk, 0x320, CMD_WR, 0x1, 0xFFFFFFFE, cmd_id++);
	mdfs_conf(ddrclk, 0x324, 0x0, 0x1, 0xFFFFFFFE, cmd_id++);
	mdfs_conf(ddrclk, 0x1BC, 0x0, 0x1, 0xFFFFFFFE, cmd_id++);
	mdfs_conf(ddrclk, 0x4, PADHOLD_CLR, 0x3, 0xFFFFFFF0, cmd_id++);
	mdfs_conf(ddrclk, 0x320, CMD_WR, 0x0, 0xFFFFFFFE, cmd_id++);
	mdfs_conf(ddrclk, 0x304, CMD_WR, 0x0, 0xFFFFFFFD, cmd_id++);
	mdfs_conf(ddrclk, 0x30, CMD_WR, 0x0, 0xFFFFFFDF, cmd_id++);
	mdfs_conf(ddrclk, 0x4, 0x0, 0x1, 0xFFFFFFFC, cmd_id++);
	mdfs_conf(ddrclk, 0x320, CMD_WR | LAST_CMD, 0x1, 0xFFFFFFFE, cmd_id++);

	writel_relaxed(readl_relaxed(ddrclk->mctl_base + MC_DFS_CONFIG) | (0x1 << 8) | 0x1,
					ddrclk->mctl_base + MC_DFS_CONFIG);
	spin_unlock_irqrestore(&ddrclk->lock, flags);
	/* Wait max 200us */
	timeout = 20;
	while (readl_relaxed(ddrclk->mctl_base + MC_DFS_STATE) >> 31 != 0x1) {
		if (!timeout) {
			dev_err(ddrclk->dev, "DRAM DFS timeout!\n");
			return;
		}
		timeout--;
		cpu_relax();
		usleep_range(5, 10);
	}
}

static unsigned long sunxi_ddr_clk_recalc_rate(struct clk_hw *hw,
					       unsigned long parent_rate)
{
	struct sunxi_ddrclk *ddrclk = to_sunxi_ddrclk_hw(hw);
	unsigned int reg_val, div;
	unsigned long rate = parent_rate;

	reg_val = readl_relaxed(ddrclk->mctl_base + MC_DFS_DIV);
	if (reg_val == 0)
		reg_val = readl_relaxed(ddrclk->ccmu_base + DRAM_CLK_REG);

	div = (reg_val >> DIV_SHIFT) & GENMASK(DIV_WIDTH - 1, 0);
	do_div(rate, div + 1);

	return rate << 1;
}

static long sunxi_ddr_clk_round_rate(struct clk_hw *hw,
				     unsigned long target_rate,
				     unsigned long *prate)
{
	if (target_rate <= (*prate << 1) / 8)
		return (*prate << 1) / 8;
	else if (target_rate <= (*prate << 1) / 6)
		return (*prate << 1) / 6;
	else if (target_rate <= (*prate << 1) / 5)
		return (*prate << 1) / 5;
	else
		return (*prate << 1) / 4;
}

static int sunxi_ddr_clk_set_rate(struct clk_hw *hw, unsigned long drate,
				  unsigned long prate)
{
	struct sunxi_ddrclk *ddrclk = to_sunxi_ddrclk_hw(hw);

	dev_info(ddrclk->dev, "prate:%ld, drate:%ld\n", prate, drate);
	mutex_lock(&ddrclk->ddrfreq_lock);
	mctl_dfs(ddrclk, ((prate << 1) / drate) - 1);
	mutex_unlock(&ddrclk->ddrfreq_lock);

	return 0;
}

const struct clk_ops sunxi_ddrclk_ops = {
	.recalc_rate = sunxi_ddr_clk_recalc_rate,
	.round_rate = sunxi_ddr_clk_round_rate,
	.set_rate = sunxi_ddr_clk_set_rate,
};

static const struct of_device_id clk_ddr_of_match[] = {
	{
		.compatible = "allwinner,clock_ddr",
	},
	{ },
};
MODULE_DEVICE_TABLE(of, clk_ddr_of_match);

static int ddr_clock_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device_node *dram_np;
	struct sunxi_ddrclk *ddrclk;
	struct clk_init_data init;
	struct clk *clk, *parent_clk;
	const char *parent_name;
	int ret = 0;

	if (!np) {
		dev_err(&pdev->dev, "failed to match ddr clock\n");
		return -ENODEV;
	}

	ddrclk = devm_kzalloc(&pdev->dev, sizeof(*ddrclk), GFP_KERNEL);
	if (!ddrclk)
		return -ENOMEM;

	ddrclk->dev = &pdev->dev;
	platform_set_drvdata(pdev, ddrclk);

	mutex_init(&ddrclk->ddrfreq_lock);
	dram_np = of_find_node_by_path("/dram");
	if (!dram_np) {
		dev_err(&pdev->dev, "failed to find dram node\n");
		return -ENODEV;
	}

	ret = of_property_read_u32(dram_np, "dram_clk", &ddrclk->dram_clk);
	if (ret) {
		dev_err(&pdev->dev, "failed to find dram_clk\n");
		return -ENODEV;
	}

	ddrclk->ccmu_base = devm_of_iomap(&pdev->dev, np, 0, NULL);
	if (!ddrclk->ccmu_base) {
		dev_err(&pdev->dev, "map ccmu failed\n");
		return -ENODEV;
	}

	ddrclk->mctl_base = devm_of_iomap(&pdev->dev, np, 1, NULL);
	if (!ddrclk->mctl_base) {
		dev_err(&pdev->dev, "map mctl failed\n");
		return -ENODEV;
	}

	parent_clk = devm_clk_get(&pdev->dev, "pll_ddr");
	if (IS_ERR(parent_clk)) {
		dev_err(&pdev->dev, "clk_get pll_ddr failed\n");
		return -ENODEV;
	}

	parent_name = __clk_get_name(parent_clk);
	if (!parent_name) {
		dev_err(&pdev->dev, "get clk name failed\n");
		return -ENODEV;
	}

	ddrclk->hw.init = &init;
	init.name = "sdram";
	init.ops = &sunxi_ddrclk_ops;
	init.parent_names = &parent_name;
	init.num_parents = 1;
	init.flags |= CLK_SET_RATE_NO_REPARENT | CLK_GET_RATE_NOCACHE;

	clk = devm_clk_register(&pdev->dev, &ddrclk->hw);
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "clk_register failed\n");
		return -ENODEV;
	}

	of_clk_add_provider(np, of_clk_src_simple_get, clk);
	return 0;
}

static int ddr_clock_remove(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;

	of_clk_del_provider(np);
	return 0;
}

static struct platform_driver ddr_clock_driver = {
	.probe   = ddr_clock_probe,
	.remove  = ddr_clock_remove,
	.driver  = {
		.name  = DRIVER_NAME,
		.of_match_table = clk_ddr_of_match,
	},
};

module_platform_driver(ddr_clock_driver);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Allwinner DDR Clock driver");
MODULE_ALIAS("platform:" DRIVER_NAME);
MODULE_AUTHOR("fanqinghua <fanqinghua@allwinnertech.com>");
MODULE_VERSION("1.0.0");
