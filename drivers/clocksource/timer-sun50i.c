/*
 * Allwinner A1X SoCs timer handling.
 *
 * Copyright (C) 2012 Maxime Ripard
 *
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 *
 * Based on code from
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Benn Huang <benn@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/clockchips.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqreturn.h>
#include <linux/sched_clock.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#include "timer-of.h"

#define TIMER_IRQ_REG		0x00
#define TIMER_IRQ_EN(val)		BIT(val)
#define TIMER_STA_REG		0x04
#define TIMER_IRQ_STA(val)		BIT(val)
#define TIMER_CTL_REG(val)	(0x20 * (val) + 0x20 + 0x00)
#define TIMER_EN			BIT(0)
#define TIMER_RELOAD			BIT(1)
#define TIMER_ONESHOT			BIT(7)
#define TIMER_IVL_REG(val)	(0x20 * (val) + 0x20 + 0x04)
#define TIMER_CVL_REG(val)	(0x20 * (val) + 0x20 + 0x08)
#define TIMER_IVH_REG(val)	(0x20 * (val) + 0x20 + 0x0c)
#define TIMER_CVH_REG(val)	(0x20 * (val) + 0x20 + 0x10)
#define TIMER_VL_MASK		0xffffffff
#define TIMER_VH_MASK		0xffffff
#define TIMER_VH_OFFSET		32

#define TIMER_SYNC_TICKS	3

/* Registers which needs to be saved and restored before and after sleeping */
static u32 regs_offset[] = {
	TIMER_IRQ_REG,
	TIMER_STA_REG,
	TIMER_CTL_REG(0),
	TIMER_IVL_REG(0),
	TIMER_CVL_REG(0),
	TIMER_IVH_REG(0),
	TIMER_CVH_REG(0),
	TIMER_CTL_REG(1),
	TIMER_IVL_REG(1),
	TIMER_CVL_REG(1),
	TIMER_IVH_REG(1),
	TIMER_CVH_REG(1),
};
static u32 regs_backup[ARRAY_SIZE(regs_offset)];

static u64 notrace sun50i_timer_sched_read(void);

static int timer_sun50i_clk_init(struct device_node *np, struct of_timer_clk *of_clk)
{
	int ret = -EINVAL;

	struct clk *bus_clk = NULL;
	struct clk *mclk = NULL;
	struct clk *pclk = NULL;
	struct reset_control *reset = NULL;

	pclk = of_clk_get_by_name(np, "parent");
	if (IS_ERR_OR_NULL(pclk)) {
		ret = PTR_ERR(pclk);
		goto out_pclk_fail;
	}

	mclk = of_clk_get_by_name(np, "mod");
	if (IS_ERR_OR_NULL(mclk)) {
		ret = PTR_ERR(mclk);
		goto out_mclk_fail;
	}

	bus_clk = of_clk_get_by_name(np, "bus");
	if (IS_ERR_OR_NULL(bus_clk)) {
		ret = PTR_ERR(bus_clk);
		goto out_bus_clk_fail;
	}

	if (of_clk) {
		reset = of_reset_control_get(np, NULL);
		if (IS_ERR_OR_NULL(reset)) {
			ret = PTR_ERR(reset);
			goto out_reset_fail;
		}
		ret = reset_control_deassert(reset);
		if (ret)
			goto out_fail;
	}

	ret = clk_prepare_enable(pclk);
	if (ret)
		goto out_fail;

	ret = clk_set_parent(mclk, pclk);
	if (ret)
		goto out_fail;

	ret = clk_prepare_enable(mclk);
	if (ret)
		goto out_fail;

	ret = clk_prepare_enable(bus_clk);
	if (ret)
		goto out_fail;

	if (of_clk) {
		of_clk->clk = mclk;
		of_clk->rate = clk_get_rate(of_clk->clk);
		if (!of_clk->rate) {
			ret = -EINVAL;
			pr_err("Failed to get clock rate for %pOF\n", np);
			goto out_fail;
		}
		of_clk->period = DIV_ROUND_UP(of_clk->rate, HZ);
	}

	return ret;

out_fail:
	if (reset)
		reset_control_put(reset);
out_reset_fail:
	clk_put(bus_clk);
out_bus_clk_fail:
	clk_put(mclk);
out_mclk_fail:
	clk_put(pclk);
out_pclk_fail:
	return ret;
}

static int of_timer_sun50i_clk_init(struct device_node *np, struct of_timer_clk *to)
{
	struct device_node *sub_np;
	int ret = -EINVAL;

	sub_np = of_parse_phandle(np, "sunxi-timers", 0);
	if (IS_ERR_OR_NULL(sub_np)) {
		pr_err("can't parse timer0 property\n");
		return ret;
	}

	ret = timer_sun50i_clk_init(sub_np, to);
	if (ret) {
		return ret;
	}

	sub_np = of_parse_phandle(np, "sunxi-timers", 1);
	if (ret) {
		pr_err("can't parse timer1 property\n");
		return ret;
	}

	ret = timer_sun50i_clk_init(sub_np, NULL);
	return ret;
}

/*
 * When we disable a timer, we need to wait at least for 2 cycles of
 * the timer source clock. We will use for that the clocksource timer
 * that is already setup and runs at the same frequency than the other
 * timers, and we never will be disabled.
 */
static void sun50i_clkevt_sync(void)
{
	u64 old, new;

	old = sun50i_timer_sched_read();

	do {
		cpu_relax();
		new = sun50i_timer_sched_read();
	} while (new - old < TIMER_SYNC_TICKS);
}

static void sun50i_clkevt_time_stop(void __iomem *base, u8 timer)
{
	u32 val = readl(base + TIMER_CTL_REG(timer));
	writel(val & ~TIMER_EN, base + TIMER_CTL_REG(timer));
	sun50i_clkevt_sync();
}

static void sun50i_clkevt_time_setup(void __iomem *base, u8 timer,
				    unsigned long delay)
{
	u32 value;

	value = delay & TIMER_VL_MASK;
	writel(value, base + TIMER_IVL_REG(timer));
	value = ((delay >> TIMER_VH_OFFSET) & (TIMER_VH_MASK));
	writel(value, base + TIMER_IVH_REG(timer));
}

static void sun50i_clkevt_time_start(void __iomem *base, u8 timer,
				    bool periodic)
{
	u32 val = readl(base + TIMER_CTL_REG(timer));

	if (periodic)
		val &= ~TIMER_ONESHOT;
	else
		val |= TIMER_ONESHOT;

	writel(val | TIMER_EN | TIMER_RELOAD,
	       base + TIMER_CTL_REG(timer));
}

static inline void save_regs(void __iomem *base)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(regs_offset); i++)
		regs_backup[i] = readl(base + regs_offset[i]);
}

static inline void restore_regs(void __iomem *base)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(regs_offset); i++)
		writel(regs_backup[i], base + regs_offset[i]);
}

static int sun50i_clkevt_shutdown(struct clock_event_device *evt)
{
	struct timer_of *to = to_timer_of(evt);

	save_regs(timer_of_base(to));
	sun50i_clkevt_time_stop(timer_of_base(to), 0);

	return 0;
}

static int sun50i_tick_resume(struct clock_event_device *evt)
{
	struct timer_of *to = to_timer_of(evt);

	restore_regs(timer_of_base(to));
	sun50i_clkevt_time_stop(timer_of_base(to), 0);

	return 0;
}

static int sun50i_clkevt_set_oneshot(struct clock_event_device *evt)
{
	struct timer_of *to = to_timer_of(evt);

	sun50i_clkevt_time_stop(timer_of_base(to), 0);
	sun50i_clkevt_time_start(timer_of_base(to), 0, false);

	return 0;
}

static int sun50i_clkevt_set_periodic(struct clock_event_device *evt)
{
	struct timer_of *to = to_timer_of(evt);

	sun50i_clkevt_time_stop(timer_of_base(to), 0);
	sun50i_clkevt_time_setup(timer_of_base(to), 0, timer_of_period(to));
	sun50i_clkevt_time_start(timer_of_base(to), 0, true);

	return 0;
}

static int sun50i_clkevt_next_event(unsigned long evt,
				   struct clock_event_device *clkevt)
{
	struct timer_of *to = to_timer_of(clkevt);

	sun50i_clkevt_time_stop(timer_of_base(to), 0);
	sun50i_clkevt_time_setup(timer_of_base(to), 0, evt - TIMER_SYNC_TICKS);
	sun50i_clkevt_time_start(timer_of_base(to), 0, false);

	return 0;
}

static void sun50i_timer_clear_interrupt(void __iomem *base)
{
	writel(TIMER_IRQ_STA(0), base + TIMER_STA_REG);
}

static irqreturn_t sun50i_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = (struct clock_event_device *)dev_id;
	struct timer_of *to = to_timer_of(evt);

	sun50i_timer_clear_interrupt(timer_of_base(to));
	evt->event_handler(evt);

	return IRQ_HANDLED;
}

static struct timer_of to = {
	.flags = TIMER_OF_IRQ | TIMER_OF_BASE,

	.clkevt = {
		.name = "sun50i_tick",
		.rating = 350,
		.features = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
		.set_state_shutdown = sun50i_clkevt_shutdown,
		.set_state_periodic = sun50i_clkevt_set_periodic,
		.set_state_oneshot = sun50i_clkevt_set_oneshot,
		.tick_resume = sun50i_tick_resume,
		.set_next_event = sun50i_clkevt_next_event,
		.cpumask = cpu_possible_mask,
	},

	.of_irq = {
		.handler = sun50i_timer_interrupt,
		.flags = IRQF_TIMER | IRQF_IRQPOLL,
	},
};

static u64 notrace sun50i_timer_sched_read(void)
{
	u64 val_low, val_high;

	val_high = (~readl(timer_of_base(&to) + TIMER_CVH_REG(1))) & TIMER_VH_MASK;

	val_low = (~readl(timer_of_base(&to) + TIMER_CVL_REG(1))) & TIMER_VL_MASK;

	return ((val_high << TIMER_VH_OFFSET) | val_low);
}

static u64 sun50i_timer_readl_down(struct clocksource *c)
{
	return sun50i_timer_sched_read();
}

static int __init sun50i_timer_init(struct device_node *node)
{
	int ret;
	u32 val;

	ret = timer_of_init(node, &to);
	if (ret) {
		pr_err("sun50i timer of init failed\n");
		return ret;
	}

	ret = of_timer_sun50i_clk_init(node, &(to.of_clk));
	if (ret) {
		pr_err("sun50i timer of clk init failed\n");
		return ret;
	}

	sun50i_clkevt_time_setup(timer_of_base(&to), 1, ~0);
	sun50i_clkevt_time_start(timer_of_base(&to), 1, true);

	pr_info("sun50i timer init:0x%llx\n", sun50i_timer_sched_read());

	sched_clock_register(sun50i_timer_sched_read, 56, timer_of_rate(&to));

	ret = clocksource_mmio_init(timer_of_base(&to) + TIMER_CVL_REG(1),
				    node->name, timer_of_rate(&to), 350, 56,
				    sun50i_timer_readl_down);
	if (ret) {
		pr_err("Failed to register clocksource\n");
		return ret;
	}

	/* Make sure timer is stopped before playing with interrupts */
	sun50i_clkevt_time_stop(timer_of_base(&to), 0);

	/* clear timer0 interrupt */
	sun50i_timer_clear_interrupt(timer_of_base(&to));

	clockevents_config_and_register(&to.clkevt, timer_of_rate(&to),
					TIMER_SYNC_TICKS, 0xffffffffffffff);

	/* Enable timer0 interrupt */
	val = readl(timer_of_base(&to) + TIMER_IRQ_REG);
	writel(val | TIMER_IRQ_EN(0), timer_of_base(&to) + TIMER_IRQ_REG);

	return ret;
}
TIMER_OF_DECLARE(sun50i_timer, "allwinner,sun50i-timer", sun50i_timer_init);
