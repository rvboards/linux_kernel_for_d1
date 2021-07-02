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
 * Modified: Martin <wuyan@allwinnertech>
 * Support compile as module
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/clockchips.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqreturn.h>
#include <linux/sched_clock.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/slab.h>

/* Timer IRQ Enable Register */
#define TIMER_IRQ_EN_REG		0x00
#define TIMER_IRQ_EN(val)		BIT(val)  /* val=0:timer0, val=1:timer1 */
/* Timer IRQ Status Register */
#define TIMER_IRQ_ST_REG		0x04
#define TIMER_IRQ_CLEAR(val)		BIT(val)  /* val=0:timer0, val=1:timer1 */
/* Timer IRQ Control Register */
#define TIMER_CTL_REG(val)		(0x10 * val + 0x10)  /* val=0:timer0, val=1:timer1 */
#define TIMER_CTL_ENABLE		BIT(0)
#define TIMER_CTL_RELOAD		BIT(1)
#define TIMER_CTL_CLK_SRC(val)		(((val) & 0x3) << 2)  /* val=0:LOSC, val=1:OSC24M */
#define TIMER_CTL_CLK_SRC_OSC24M	(1)
#define TIMER_CTL_CLK_PRES(val)		(((val) & 0x7) << 4)  /* val=0:/1, val=1:/2, ..., val=7:/128 */
#define TIMER_CTL_ONESHOT		BIT(7)  /* val=0:continuous mode, val=1:single mode */
/* Timer Interval Value Register */
#define TIMER_INTVAL_REG(val)		(0x10 * (val) + 0x14)  /* val=0:timer0, val=1:timer1 */
/* Timer Current Value Register */
#define TIMER_CNTVAL_REG(val)		(0x10 * (val) + 0x18)  /* val=0:timer0, val=1:timer1 */

#define TIMER_SYNC_TICKS		3

static void __iomem *timer_base;
static int timer_size;
static void *store_mem;
static u32 ticks_per_jiffy;  /* how many ticks does the timer need to produce in a jiffy */
static struct clk *clk;
static int irq;

/*
 * When we disable a timer, we need to wait at least for 2 cycles of
 * the timer source clock. We will use for that the clocksource timer
 * that is already setup and runs at the same frequency than the other
 * timers, and we never will be disabled.
 */
static void sunxi_clkevt_sync(void)
{
	u32 old = readl(timer_base + TIMER_CNTVAL_REG(1));

	while ((old - readl(timer_base + TIMER_CNTVAL_REG(1))) < TIMER_SYNC_TICKS)
		cpu_relax();
}

static void sunxi_clkevt_time_stop(u8 timer)
{
	u32 val = readl(timer_base + TIMER_CTL_REG(timer));
	writel(val & ~TIMER_CTL_ENABLE, timer_base + TIMER_CTL_REG(timer));
	sunxi_clkevt_sync();
}

static void sunxi_clkevt_time_setup(u8 timer, unsigned long delay)
{
	writel(delay, timer_base + TIMER_INTVAL_REG(timer));
}

static void sunxi_clkevt_time_start(u8 timer, bool periodic)
{
	u32 val = readl(timer_base + TIMER_CTL_REG(timer));

	if (periodic)
		val &= ~TIMER_CTL_ONESHOT;
	else
		val |= TIMER_CTL_ONESHOT;

	writel(val | TIMER_CTL_ENABLE | TIMER_CTL_RELOAD,
		timer_base + TIMER_CTL_REG(timer));
}

static int sunxi_clkevt_shutdown(struct clock_event_device *evt)
{
	/* we should store the registers for soc timer first */
	memcpy(store_mem, (void *)timer_base, timer_size);
	sunxi_clkevt_time_stop(0);
	return 0;
}

static int sunxi_tick_resume(struct clock_event_device *evt)
{
	/* We should restore the registers for soc time first */
	/* Only the necessary registers shoule be restored */
	writel(*(u32 *)(store_mem + TIMER_IRQ_EN_REG),
		timer_base + TIMER_IRQ_EN_REG);
	writel(*(u32 *)(store_mem + TIMER_IRQ_ST_REG),
		timer_base + TIMER_IRQ_ST_REG);
	writel(*(u32 *)(store_mem + TIMER_CTL_REG(0)),
		timer_base + TIMER_CTL_REG(0));
	writel(*(u32 *)(store_mem + TIMER_INTVAL_REG(0)),
		timer_base + TIMER_INTVAL_REG(0));
	writel(*(u32 *)(store_mem + TIMER_CNTVAL_REG(0)),
		timer_base + TIMER_CNTVAL_REG(0));
	writel(*(u32 *)(store_mem + TIMER_CTL_REG(1)),
		timer_base + TIMER_CTL_REG(1));
	writel(*(u32 *)(store_mem + TIMER_INTVAL_REG(1)),
		timer_base + TIMER_INTVAL_REG(1));
	writel(*(u32 *)(store_mem + TIMER_CNTVAL_REG(1)),
		timer_base + TIMER_CNTVAL_REG(1));

	sunxi_clkevt_time_stop(0);
	return 0;
}

static int sunxi_clkevt_set_oneshot(struct clock_event_device *evt)
{
	sunxi_clkevt_time_stop(0);
	sunxi_clkevt_time_start(0, false);
	return 0;
}

static int sunxi_clkevt_set_periodic(struct clock_event_device *evt)
{
	sunxi_clkevt_time_stop(0);
	sunxi_clkevt_time_setup(0, ticks_per_jiffy);
	sunxi_clkevt_time_start(0, true);
	return 0;
}

static int sunxi_clkevt_next_event(unsigned long evt,
	struct clock_event_device *unused)
{
	sunxi_clkevt_time_stop(0);
	sunxi_clkevt_time_setup(0, evt - TIMER_SYNC_TICKS);
	sunxi_clkevt_time_start(0, false);

	return 0;
}

static struct clock_event_device sunxi_clockevent = {
	.name = "sunxi_tick",
	.rating = 350,
	.features = CLOCK_EVT_FEAT_PERIODIC |
		CLOCK_EVT_FEAT_ONESHOT |
		CLOCK_EVT_FEAT_DYNIRQ,
	.set_state_shutdown = sunxi_clkevt_shutdown,
	.set_state_periodic = sunxi_clkevt_set_periodic,
	.set_state_oneshot = sunxi_clkevt_set_oneshot,
	.tick_resume = sunxi_tick_resume,
	.set_next_event = sunxi_clkevt_next_event,
};

static void sunxi_timer_clear_interrupt(void)
{
	writel(TIMER_IRQ_CLEAR(0), timer_base + TIMER_IRQ_ST_REG);
}

static irqreturn_t sunxi_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = (struct clock_event_device *)dev_id;

	sunxi_timer_clear_interrupt();
	evt->event_handler(evt);

	return IRQ_HANDLED;
}

static struct irqaction sunxi_timer_irq = {
	.name = "sunxi_timer0",
	.flags = IRQF_TIMER | IRQF_IRQPOLL,
	.handler = sunxi_timer_interrupt,
	.dev_id = &sunxi_clockevent,
};

/*
 * The code below are copied from drivers/clocksource/mmio.c.
 * Since they are not exported, we cannot use it in a module.
 * To meet GKI requirements, we cannot modify kernel common code either.
 * So we have to copy it here, and we modify it to adapt our use.
 */
/* Start of copied code --> */
struct sunxi_clocksource_mmio {
	void __iomem *reg;
	struct clocksource clksrc;
};

static inline
struct sunxi_clocksource_mmio *sunxi_to_mmio_clksrc(struct clocksource *c)
{
	return container_of(c, struct sunxi_clocksource_mmio, clksrc);
}

static u64 sunxi_clocksource_mmio_readl_down(struct clocksource *c)
{
	return ~(u64)readl_relaxed(sunxi_to_mmio_clksrc(c)->reg) & c->mask;
}

static int sunxi_clocksource_mmio_init(void __iomem *base, const char *name,
	unsigned long hz, int rating, unsigned bits,
	u64 (*read)(struct clocksource *))
{
	struct sunxi_clocksource_mmio *cs;

	if (bits > 64 || bits < 16)
		return -EINVAL;

	cs = kzalloc(sizeof(struct sunxi_clocksource_mmio), GFP_KERNEL);
	if (!cs)
		return -ENOMEM;

	cs->reg = base;
	cs->clksrc.name = name;
	cs->clksrc.rating = rating;
	cs->clksrc.read = read;
	cs->clksrc.mask = CLOCKSOURCE_MASK(bits);
	cs->clksrc.flags = CLOCK_SOURCE_IS_CONTINUOUS;

	return clocksource_register_hz(&cs->clksrc, hz);
}
/* <-- End of copied code */

static int sunxi_timer_init(struct device_node *node)
{
	unsigned long rate = 0;
	struct resource res;
	int ret;
	u32 val;

	timer_base = of_iomap(node, 0);
	if (!timer_base) {
		pr_crit("Can't map registers");
		ret = -ENXIO;
		goto fail_of_iomap;
	}

	if (of_address_to_resource(node, 0, &res)) {
		ret = -EINVAL;
		goto fail_of_address_to_resource;
	}

	timer_size = resource_size(&res);

	store_mem = (void *)kmalloc(timer_size, GFP_KERNEL);
	if (!store_mem) {
		ret = -ENOMEM;
		goto fail_kmalloc;
	}

	irq = irq_of_parse_and_map(node, 0);
	if (irq <= 0) {
		pr_crit("Can't parse IRQ");
		ret = -EINVAL;
		goto fail_irq_of_parse_and_map;
	}

	clk = of_clk_get(node, 0);
	if (IS_ERR(clk)) {
		pr_crit("Can't get timer clock");
		ret = PTR_ERR(clk);
		goto fail_of_clk_get;
	}

	ret = clk_prepare_enable(clk);
	if (ret) {
		pr_err("Failed to prepare clock");
		goto fail_clk_prepare_enable;
	}

	rate = clk_get_rate(clk);

	writel(~0, timer_base + TIMER_INTVAL_REG(1));
	writel(TIMER_CTL_ENABLE | TIMER_CTL_RELOAD |
		TIMER_CTL_CLK_SRC(TIMER_CTL_CLK_SRC_OSC24M),
		timer_base + TIMER_CTL_REG(1));

	ret = sunxi_clocksource_mmio_init(timer_base + TIMER_CNTVAL_REG(1),
		node->name, rate, 350, 32, sunxi_clocksource_mmio_readl_down);
	if (ret) {
		pr_err("Failed to register clocksource");
		goto fail_clocksource_mmio_init;
	}

	ticks_per_jiffy = DIV_ROUND_UP(rate, HZ);

	writel(TIMER_CTL_CLK_SRC(TIMER_CTL_CLK_SRC_OSC24M),
			timer_base + TIMER_CTL_REG(0));

	/* Make sure timer is stopped before playing with interrupts */
	sunxi_clkevt_time_stop(0);

	/* clear timer0 interrupt */
	sunxi_timer_clear_interrupt();

	sunxi_clockevent.cpumask = cpu_possible_mask;
	sunxi_clockevent.irq = irq;

	clockevents_config_and_register(&sunxi_clockevent, rate,
		TIMER_SYNC_TICKS, 0xffffffff);

	ret = setup_irq(irq, &sunxi_timer_irq);
	if (ret) {
		pr_err("failed to setup irq %d\n", irq);
		goto fail_setup_irq;
	}

	/* Enable timer0 interrupt */
	val = readl(timer_base + TIMER_IRQ_EN_REG);
	writel(val | TIMER_IRQ_EN(0), timer_base + TIMER_IRQ_EN_REG);

#ifdef MODULE
	/*
	 * Hold this module, do not allow user to remove.
	 * Since we don't have any way to unregister clockevent and clocksource.
	 */
	try_module_get(THIS_MODULE);
#endif

	return 0;

fail_setup_irq:
//clockevents_unregister_device(dev);
/*
 * @TODO:
 * For clockevents_config_and_register(), we don't have its deinit function.
 * (Why doesn't kernel/time/clockevents.c provide one?).
 * I think this leads to memory leak if the above setup_irq() fails.
 */
fail_clocksource_mmio_init:
	clk_disable_unprepare(clk);
fail_clk_prepare_enable:
	clk_put(clk);
fail_of_clk_get:
fail_irq_of_parse_and_map:
	kfree(store_mem);
fail_kmalloc:
fail_of_address_to_resource:
	iounmap(timer_base);
fail_of_iomap:
	return ret;
}

//#ifndef MODULE

/*
 * If we use TIMER_OF_DECLARE(), when the interrupt-controller is set to
 * 'wakeupgen', which is probed later than sunxi_timer_init(),
 * sunxi_timer_init() will fail on irq_of_parse_and_map() with an error 'irq:
 * no irq domain found for interrupt-controller@0'.
 * Therefore, we cannot use TIMER_OF_DECLARE() when setting 'wakeupgen' as the
 * interrupt-controller in dts. Let's use module_platform_driver() instead.
 * But if the interrupt-controller is set to 'gic', we can surely use
 * TIMER_OF_DECLARE() here.
 */

//TIMER_OF_DECLARE(sunxi, "allwinner,sunxi-timer", sunxi_timer_init);

//#else  /* if defined MODULE */

static int sunxi_timer_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	return sunxi_timer_init(np);
}

static const struct of_device_id sunxi_timer_match[] = {
	{ .compatible = "allwinner,sunxi-timer"},
	{}
};
MODULE_DEVICE_TABLE(of, sunxi_timer_match);

static struct platform_driver sunxi_timer_driver = {
	.probe  = sunxi_timer_probe,
	.driver = {
		.name = "sunxi-timer",
		.owner = THIS_MODULE,
		.of_match_table = sunxi_timer_match,
	},
};

module_platform_driver(sunxi_timer_driver);

MODULE_AUTHOR("Martin <wuyan@allwinnertech.com");
MODULE_DESCRIPTION("sunxi soc-timer driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.1");

//#endif  /* MODULE */
