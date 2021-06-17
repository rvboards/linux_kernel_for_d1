// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020, allwinnertech.
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
 */

#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/bitops.h>
#include <linux/rpmsg.h>
#include <linux/errno.h>
#include "rpmsg_internal.h"
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/spinlock.h>

#define SX_MSG_MAX_CORE         5
#define SUNXI_CHANNEL_MAX       4
#define SUNXI_MAX_MSG_FIFO      8

#define SUNXI_RPMSG_ID		"rpmsg_id"
#define SUNXI_RPMSG_AMP_CNT	"msgbox_amp_counts"
#define SUNXI_RPMSG_AMP_LOCAL	"msgbox_amp_local"
#define SUNXI_RPMSG_AMP_REMOTE	"rpmsg_amp_remote"
#define SUNXI_RPMSG_READ_CHAN   "rpmsg_read_channel"
#define SUNXI_RPMSG_WRITE_CHAN  "rpmsg_write_channel"

#define SX_MSG_OFFSET(n)		(0x100 * (n))
#define SX_MSG_READ_IRQ_ENABLE(n)	(0x20 + SX_MSG_OFFSET(n))
#define SX_MSG_READ_IRQ_STATUS(n)	(0x24 + SX_MSG_OFFSET(n))
#define SX_MSG_WRITE_IRQ_ENABLE(n)	(0x30 + SX_MSG_OFFSET(n))
#define SX_MSG_WRITE_IRQ_STATUS(n)	(0x34 + SX_MSG_OFFSET(n))
#define SX_MSG_DEBUG_REGISTER(n)	(0x40 + SX_MSG_OFFSET(n))
#define SX_MSG_FIFO_STATUS(n, p)	(0x50 + SX_MSG_OFFSET(n) + 0x4 * (p))
#define SX_MSG_MSG_STATUS(n, p)	        (0x60 + SX_MSG_OFFSET(n) + 0x4 * (p))
#define SX_MSG_MSG_FIFO(n, p)	        (0x70 + SX_MSG_OFFSET(n) + 0x4 * (p))
#define SX_MSG_WRITE_IRQ_THRESHOLD(n, p) (0x80 + SX_MSG_OFFSET(n) + 0x4 * (p))

#define SUNXI_CHANNEL_MSGBOX(x) (((x) >> 8) & 0xff)
#define SUNXI_CHANNEL_FIFO(x) ((x) & 0xff)

#define to_sx_rpmsg_dev(rpmsg_dev)                                             \
	container_of(rpmsg_dev, struct sx_rpmsg_dev, rp_dev)
#define to_sx_rpmsg_edp(rpmsg_edp)                                             \
	container_of(rpmsg_edp, struct sx_rpmsg_endpoint, ep)

struct sx_msgbox_dev {
	struct platform_device *pdev;
	struct list_head head;
	struct clk *clk;
	struct reset_control *rst;
	spinlock_t lock; /* for list protect */
	void __iomem *base[SX_MSG_MAX_CORE];
	int *irq;
	int irq_cnts;
	int amp_cnt;
	int amp_cur;
};

/*
 * @head: rpmsg_endpoint list
 */
struct sx_rpmsg_dev {
	struct rpmsg_device rp_dev;
	struct sx_msgbox_dev *msg_dev;
	struct list_head node;
	struct list_head head;
	int local_amp;
	int remote_amp;
	int read_channel;
	int write_channel;
};

struct sx_rpmsg_endpoint {
	struct rpmsg_endpoint ep;
	struct completion send_done;
	struct mutex s_mtx; /* protect send data */
	struct list_head node;
	int local_amp;
	int remote_amp;
	int read_channel;
	int write_channel;
	int idx;
	int len;
	u8 *d;
};

/**
 * local must not equal remote
 */
static inline int sx_calculate_n(int local, int remote)
{
	if (local < remote)
		return remote - 1;
	else
		return remote;
}

static inline void sx_write_irq_enable(void __iomem *base, int l, int r, int p)
{
	u32 d;
	int n = sx_calculate_n(r, l);

	base += SX_MSG_WRITE_IRQ_ENABLE(n);

	d = readl(base);
	d |= (1 << (p * 2 + 1));
	writel(d, base);
}

static inline void sx_write_irq_disable(void __iomem *base, int l, int r, int p)
{
	u32 d;
	int n = sx_calculate_n(r, l);

	base += SX_MSG_WRITE_IRQ_ENABLE(n);

	d = readl(base);
	d &= ~(1 << (p * 2 + 1));
	writel(d, base);
}

static inline void sx_write_irq_threshold(void __iomem *base, int l, int r,
					  int p, int th)
{
	int n = sx_calculate_n(r, l);

	base += SX_MSG_WRITE_IRQ_THRESHOLD(n, p);

	switch (th) {
	case 8:
		th = 3;
		break;
	case 4:
		th = 2;
		break;
	case 2:
		th = 1;
		break;
	case 1:
		th = 0;
		break;
	default:
		pr_warn("sunxi-msgbox:msgbox set threshold error, use 8\n");
		th = 0;
		break;
	}

	writel(th, base);
}

static inline void sx_read_irq_enable(void __iomem *base, int l, int r, int p)
{
	u32 d;
	int n = sx_calculate_n(l, r);

	base += SX_MSG_READ_IRQ_ENABLE(n);

	d = readl(base);
	d |= 1 << (p * 2);
	writel(d, base);
}

static inline void sx_read_irq_disable(void __iomem *base, int l, int r, int p)
{
	u32 d;
	int n = sx_calculate_n(l, r);

	base += SX_MSG_READ_IRQ_ENABLE(n);

	d = readl(base);
	d &= ~(1 << (p * 2));
	writel(d, base);
}

static inline void *sx_base(struct sx_msgbox_dev *mdev, int i)
{
	void *b;

	BUG_ON(i >= SX_MSG_MAX_CORE);

	b = mdev->base[i];
	BUG_ON(!b);

	return b;

}

static void sunxi_rpmsg_destroy_edp(struct rpmsg_endpoint *ept)
{
	struct sx_rpmsg_endpoint *sx_edp = to_sx_rpmsg_edp(ept);
	struct sx_rpmsg_dev *sx_rpmsg_dev = to_sx_rpmsg_dev(ept->rpdev);

	sx_read_irq_disable(sx_base(sx_rpmsg_dev->msg_dev, sx_edp->local_amp),
			    sx_edp->local_amp, sx_edp->remote_amp,
			    sx_edp->read_channel);
	sx_write_irq_disable(sx_base(sx_rpmsg_dev->msg_dev, sx_edp->remote_amp),
			     sx_edp->local_amp, sx_edp->remote_amp,
			     sx_edp->write_channel);

	list_del(&sx_edp->node);
	devm_kfree(&sx_rpmsg_dev->msg_dev->pdev->dev, sx_edp);
}

static int sunxi_rpmsg_start_send(struct sx_rpmsg_endpoint *medp)
{
	struct sx_rpmsg_dev *rpmsg_dev = to_sx_rpmsg_dev(medp->ep.rpdev);
	struct sx_msgbox_dev *msg_dev = rpmsg_dev->msg_dev;

	/* enable interrupt */
	sx_write_irq_enable(sx_base(msg_dev, medp->remote_amp), medp->local_amp,
			    medp->remote_amp, medp->write_channel);

	return 0;
}

static int sunxi_rpmsg_edp_send(struct rpmsg_endpoint *ept, void *data, int len)
{
	struct sx_rpmsg_endpoint *medp = to_sx_rpmsg_edp(ept);
	unsigned long ret;

	dev_dbg(&ept->rpdev->dev, "send msg start\n");

	mutex_lock(&medp->s_mtx);

	medp->d = data;
	medp->len = len;
	medp->idx = 0;

	reinit_completion(&medp->send_done);
	sunxi_rpmsg_start_send(medp);
	ret = wait_for_completion_timeout(&medp->send_done,
					  msecs_to_jiffies(15000));

	mutex_unlock(&medp->s_mtx);

	if (!ret)
		return -ERESTARTSYS;

	return 0;
}

static struct rpmsg_endpoint_ops sx_rpmsg_ep_ops = {
	.destroy_ept = sunxi_rpmsg_destroy_edp,
	.send = sunxi_rpmsg_edp_send,
};

static struct rpmsg_endpoint *
sunxi_rpmsg_create_ept(struct rpmsg_device *rpdev, rpmsg_rx_cb_t cb, void *priv,
		       struct rpmsg_channel_info chinfo)
{
	struct sx_rpmsg_dev *sx_rpmsg_dev = to_sx_rpmsg_dev(rpdev);
	struct sx_msgbox_dev *sx_msg_dev = sx_rpmsg_dev->msg_dev;
	struct sx_rpmsg_endpoint *sx_rpmsg_edp;
	int local, remote, ch_read, ch_write;

	local = SUNXI_CHANNEL_MSGBOX(chinfo.src);
	ch_read = SUNXI_CHANNEL_FIFO(chinfo.src);
	remote = SUNXI_CHANNEL_MSGBOX(chinfo.dst);
	ch_write = SUNXI_CHANNEL_FIFO(chinfo.dst);

	if (remote == 0xff)
		remote = sx_rpmsg_dev->remote_amp;

	if (ch_write == 0xff)
		ch_write = sx_rpmsg_dev->write_channel;

	if (local != sx_msg_dev->amp_cur || remote >= sx_msg_dev->amp_cnt ||
	    remote == local) {
		dev_err(&sx_msg_dev->pdev->dev, "create endpoint error amp\n");
		return NULL;
	}

	if (ch_read >= SUNXI_CHANNEL_MAX || ch_write >= SUNXI_CHANNEL_MAX) {
		dev_err(&sx_msg_dev->pdev->dev,
			"create endpoint error channel\n");
		return NULL;
	}

	list_for_each_entry(sx_rpmsg_edp, &sx_rpmsg_dev->head, node) {
		if (sx_rpmsg_edp->remote_amp == remote &&
		    sx_rpmsg_edp->read_channel == ch_read &&
		    sx_rpmsg_edp->write_channel == ch_write) {
			sx_rpmsg_edp->ep.cb = cb;
			sx_rpmsg_edp->ep.priv = priv;
			return &sx_rpmsg_edp->ep;
		}

		if (sx_rpmsg_edp->remote_amp == remote &&
		    (sx_rpmsg_edp->read_channel == ch_read ||
		     sx_rpmsg_edp->write_channel == ch_write)) {
			dev_err(&sx_msg_dev->pdev->dev,
				"channel is used for ohters endpoint\n");
			return NULL;
		}
	}

	sx_rpmsg_edp =
		devm_kzalloc(&sx_msg_dev->pdev->dev,
			     sizeof(struct sx_rpmsg_endpoint), GFP_KERNEL);
	if (!sx_rpmsg_edp) {
		dev_err(&sx_msg_dev->pdev->dev, "no memory for endpoint\n");
		return NULL;
	}

	sx_rpmsg_edp->local_amp = local;
	sx_rpmsg_edp->remote_amp = remote;
	sx_rpmsg_edp->read_channel = ch_read;
	sx_rpmsg_edp->write_channel = ch_write;
	sx_rpmsg_edp->ep.addr = (local << 8) | ch_read;
	sx_rpmsg_edp->ep.cb = cb;
	sx_rpmsg_edp->ep.priv = priv;
	sx_rpmsg_edp->ep.rpdev = &sx_rpmsg_dev->rp_dev;
	sx_rpmsg_edp->ep.ops = &sx_rpmsg_ep_ops;

	kref_init(&sx_rpmsg_edp->ep.refcount);
	mutex_init(&sx_rpmsg_edp->s_mtx);
	init_completion(&sx_rpmsg_edp->send_done);

	list_add(&sx_rpmsg_edp->node, &sx_rpmsg_dev->head);

	sx_write_irq_threshold(sx_base(sx_msg_dev, sx_rpmsg_edp->remote_amp),
			       sx_rpmsg_edp->local_amp,
			       sx_rpmsg_edp->remote_amp,
			       sx_rpmsg_edp->write_channel, 1);
	sx_read_irq_enable(sx_base(sx_msg_dev, sx_rpmsg_edp->local_amp),
			   sx_rpmsg_edp->local_amp, sx_rpmsg_edp->remote_amp,
			   sx_rpmsg_edp->read_channel);

	return &sx_rpmsg_edp->ep;
}

static struct rpmsg_device_ops sunxi_rpmsg_dev_ops = {
	.create_ept = sunxi_rpmsg_create_ept,
};

static void sunxi_rpmsg_create(struct sx_msgbox_dev *msg_dev, int idx)
{
	int ret;
	const char *rpmsg_id;
	char *pparm;
	struct platform_device *pdev = msg_dev->pdev;
	struct sx_rpmsg_dev *sx_rpdev;
	u32 remote, ch_read, ch_write;

	ret = of_property_read_string_index(pdev->dev.of_node, SUNXI_RPMSG_ID,
					    idx, &rpmsg_id);
	if (ret < 0) {
		dev_dbg(&pdev->dev, "create rpmsg %s failed\n", rpmsg_id);
		return;
	}

	pparm = kasprintf(GFP_KERNEL, SUNXI_RPMSG_AMP_REMOTE "-%d", idx);
	ret = of_property_read_u32(pdev->dev.of_node, pparm, &remote);
	if (ret < 0) {
		dev_warn(&pdev->dev, SUNXI_RPMSG_AMP_REMOTE "-%d get error\n",
			 idx);
		kfree(pparm);
		return;
	}
	kfree(pparm);

	pparm = kasprintf(GFP_KERNEL, SUNXI_RPMSG_READ_CHAN "-%d", idx);
	ret = of_property_read_u32(pdev->dev.of_node, pparm, &ch_read);
	if (ret < 0) {
		dev_warn(&pdev->dev, SUNXI_RPMSG_READ_CHAN "-%d get error\n",
			 idx);
		kfree(pparm);
		return;
	}
	kfree(pparm);

	pparm = kasprintf(GFP_KERNEL, SUNXI_RPMSG_WRITE_CHAN "-%d", idx);
	ret = of_property_read_u32(pdev->dev.of_node, pparm, &ch_write);
	if (ret < 0) {
		dev_warn(&pdev->dev, SUNXI_RPMSG_WRITE_CHAN "-%d get error\n",
			 idx);
		kfree(pparm);
		return;
	}
	kfree(pparm);

	if (remote >= msg_dev->amp_cnt || ch_read >= SUNXI_CHANNEL_MAX ||
	    ch_write >= SUNXI_CHANNEL_MAX) {
		dev_warn(&pdev->dev, "rpmsg param config error\n");
		return;
	}

	sx_rpdev = devm_kzalloc(&pdev->dev, sizeof(struct sx_rpmsg_dev),
				GFP_KERNEL);
	if (!sx_rpdev)
		return;

	sx_rpdev->local_amp = msg_dev->amp_cur;
	sx_rpdev->remote_amp = remote;
	sx_rpdev->read_channel = ch_read;
	sx_rpdev->write_channel = ch_write;
	sx_rpdev->rp_dev.ops = &sunxi_rpmsg_dev_ops;
	sx_rpdev->rp_dev.dst =
		(sx_rpdev->remote_amp << 8) | (sx_rpdev->write_channel << 0);
	sx_rpdev->rp_dev.src =
		(sx_rpdev->local_amp << 8) | (sx_rpdev->read_channel << 0);
	sx_rpdev->rp_dev.dev.parent = &pdev->dev;
	sx_rpdev->msg_dev = msg_dev;
	INIT_LIST_HEAD(&sx_rpdev->head);
	list_add(&sx_rpdev->node, &msg_dev->head);

	strncpy(sx_rpdev->rp_dev.id.name, rpmsg_id, RPMSG_NAME_SIZE - 1);
	ret = rpmsg_register_device(&sx_rpdev->rp_dev);
	if (ret) {
		dev_dbg(&pdev->dev, "register rpsmg dev error %d\n", ret);
		devm_kfree(&pdev->dev, sx_rpdev);
		return;
	}
}

static void sunxi_edp_irq_send(struct sx_msgbox_dev *mdev,
			       struct sx_rpmsg_endpoint *sx_rpmsg_edp)
{
	void __iomem *fifo, *msg_status;
	u32 l, r, c;
	u32 data, i;
	u8 d;

	l = sx_rpmsg_edp->local_amp;
	r = sx_rpmsg_edp->remote_amp;
	c = sx_rpmsg_edp->write_channel;
	fifo = sx_base(mdev, r) + SX_MSG_MSG_FIFO(sx_calculate_n(r, l), c);
	msg_status =
		sx_base(mdev, r) + SX_MSG_MSG_STATUS(sx_calculate_n(r, l), c);

	while (readl(msg_status) != SUNXI_MAX_MSG_FIFO) {
		if (sx_rpmsg_edp->idx >= sx_rpmsg_edp->len) {
			/* disable interrupt */
			sx_write_irq_disable(sx_base(mdev, r), l, r, c);

			complete(&sx_rpmsg_edp->send_done);

			break;
		}

		data = 0;
		for (i = 0; i < sizeof(uint32_t); i++) {
			if (sx_rpmsg_edp->idx < sx_rpmsg_edp->len) {
				d = sx_rpmsg_edp->d[sx_rpmsg_edp->idx++];
				data |= d << (i * 8);
			}
		}
		writel(data, fifo);
	}
}

static void sunxi_edp_irq_rec(struct sx_msgbox_dev *mdev,
			      struct sx_rpmsg_endpoint *sx_rpmsg_edp)
{
	void __iomem *fifo, *msg_status;
	u32 l, r, c;
	u32 data;
	u8 d[4];

	l = sx_rpmsg_edp->local_amp;
	r = sx_rpmsg_edp->remote_amp;
	c = sx_rpmsg_edp->read_channel;
	fifo = sx_base(mdev, l) + SX_MSG_MSG_FIFO(sx_calculate_n(l, r), c);
	msg_status =
		sx_base(mdev, l) + SX_MSG_MSG_STATUS(sx_calculate_n(l, r), c);

	while (readl(msg_status)) {
		data = readl(fifo);
		d[0] = data & 0xff;
		d[1] = (data >> 8) & 0xff;
		d[2] = (data >> 16) & 0xff;
		d[3] = (data >> 24) & 0xff;

		if (sx_rpmsg_edp->ep.cb) {
			sx_rpmsg_edp->ep.cb(sx_rpmsg_edp->ep.rpdev, d, 4,
					    sx_rpmsg_edp->ep.priv,
					    sx_rpmsg_edp->ep.addr);
		}
	}
}

static void sunxi_endpoint_irq(struct sx_msgbox_dev *mdev,
			       struct sx_rpmsg_endpoint *sx_rpmsg_edp)
{
	u32 l, r, cr, cw;
	void __iomem *r_e, *w_e, *r_s, *w_s;

	l = sx_rpmsg_edp->local_amp;
	r = sx_rpmsg_edp->remote_amp;
	cr = sx_rpmsg_edp->read_channel;
	cw = sx_rpmsg_edp->write_channel;

	r_e = sx_base(mdev, l) + SX_MSG_READ_IRQ_ENABLE(sx_calculate_n(l, r));
	w_e = sx_base(mdev, r) + SX_MSG_WRITE_IRQ_ENABLE(sx_calculate_n(r, l));
	r_s = sx_base(mdev, l) + SX_MSG_READ_IRQ_STATUS(sx_calculate_n(l, r));
	w_s = sx_base(mdev, r) + SX_MSG_WRITE_IRQ_STATUS(sx_calculate_n(r, l));

	if ((readl(r_e) & (1 << (cr * 2))) && readl(r_s) & (1 << (cr * 2))) {
		sunxi_edp_irq_rec(mdev, sx_rpmsg_edp);
		writel(1 << (cr * 2), r_s);
	}

	if (readl(w_e) & (1 << (cw * 2 + 1)) &&
	    readl(w_s) & (1 << (cw * 2 + 1))) {
		sunxi_edp_irq_send(mdev, sx_rpmsg_edp);
		writel(1 << (cw * 2 + 1), w_s);
	}
}

static irqreturn_t sunxi_msgbox_irq(int irq, void *data)
{
	struct sx_msgbox_dev *sx_msgbox_dev = data;
	struct sx_rpmsg_dev *sx_rpmsg_dev;
	struct sx_rpmsg_endpoint *sx_rpmsg_endpoint;
	unsigned long flag;

	spin_lock_irqsave(&sx_msgbox_dev->lock, flag);
	list_for_each_entry (sx_rpmsg_dev, &sx_msgbox_dev->head, node) {
		list_for_each_entry(sx_rpmsg_endpoint, &sx_rpmsg_dev->head,
				    node) {
			sunxi_endpoint_irq(sx_msgbox_dev, sx_rpmsg_endpoint);
		}
	}
	spin_unlock_irqrestore(&sx_msgbox_dev->lock, flag);

	return IRQ_HANDLED;
}

static int sunxi_msgbox_probe(struct platform_device *pdev)
{
	struct sx_msgbox_dev *msg_dev;
	u32 cnts, i, rst;

	msg_dev = devm_kzalloc(&pdev->dev, sizeof(struct sx_msgbox_dev),
			       GFP_KERNEL);
	if (!msg_dev)
		return -ENOMEM;

	platform_set_drvdata(pdev, msg_dev);
	msg_dev->pdev = pdev;

	for (i = 0; i < SX_MSG_MAX_CORE; i++) {
		msg_dev->base[i] = devm_platform_ioremap_resource(pdev, i);
		if (IS_ERR_OR_NULL(msg_dev->base[i]))
			break;
	}
	if (i == 0 && IS_ERR(msg_dev->base[0])) {
		dev_dbg(&pdev->dev, "ioremap resource failed\n");
		return PTR_ERR(msg_dev->base[0]);
	}

	msg_dev->clk = of_clk_get(pdev->dev.of_node, 0);
	if (IS_ERR(msg_dev->clk)) {
		dev_err(&pdev->dev, "error get clk\n");
		return -EINVAL;
	}
	clk_prepare_enable(msg_dev->clk);

	msg_dev->rst = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(msg_dev->rst)) {
		dev_err(&pdev->dev, "error get reset control\n");
		return -EINVAL;
	}
	reset_control_deassert(msg_dev->rst);

	cnts = of_irq_count(pdev->dev.of_node);
	msg_dev->irq_cnts = cnts;
	msg_dev->irq = devm_kcalloc(&pdev->dev, cnts, sizeof(int), GFP_KERNEL);

	for (i = 0; i < cnts; i++) {
		msg_dev->irq[i] = of_irq_get(pdev->dev.of_node, i);
		if (msg_dev->irq[i] < 0) {
			dev_dbg(&pdev->dev, "of_irq_get[%d] failed\n", i);
			return msg_dev->irq[i];
		} else if (msg_dev->irq == 0) {
			dev_dbg(&pdev->dev, "of_irq_get[%d] map error\n", i);
			return -EINVAL;
		}
	}

	rst = of_property_read_u32(pdev->dev.of_node, SUNXI_RPMSG_AMP_CNT,
				   &cnts);
	if (rst < 0) {
		dev_dbg(&pdev->dev, "no msgbox amp cnt, use 2 instead\n");
		cnts = 2;
	}
	msg_dev->amp_cnt = cnts;

	rst = of_property_read_u32(pdev->dev.of_node, SUNXI_RPMSG_AMP_LOCAL,
				   &cnts);
	if (rst < 0) {
		dev_warn(&pdev->dev, "get msgbox current amp failed\n");
		return -EINVAL;
	}
	msg_dev->amp_cur = cnts;

	if (msg_dev->amp_cur >= msg_dev->amp_cnt) {
		dev_warn(&pdev->dev, "amp cur config error\n");
		return -EINVAL;
	}

	INIT_LIST_HEAD(&msg_dev->head);
	cnts = of_property_count_strings(pdev->dev.of_node, SUNXI_RPMSG_ID);
	for (i = 0; i < cnts; i++)
		sunxi_rpmsg_create(msg_dev, i);

	spin_lock_init(&msg_dev->lock);
	cnts = of_irq_count(pdev->dev.of_node);
	for (i = 0; i < cnts; i++) {
		rst = devm_request_irq(&pdev->dev, msg_dev->irq[i],
				       sunxi_msgbox_irq, 0,
				       dev_name(&pdev->dev), msg_dev);
		if (rst) {
			dev_err(&pdev->dev, "error request irq\n");
			return rst;
		}
	}

	return 0;
}

static int sunxi_remove_device(struct device *dev, void *data)
{
	device_unregister(dev);

	return 0;
}

static int sunxi_msgbox_remove(struct platform_device *pdev)
{
	int i;
	struct sx_msgbox_dev *msg_dev = platform_get_drvdata(pdev);

	for (i = 0; i < msg_dev->irq_cnts; i++)
		disable_irq(msg_dev->irq[i]);

	device_for_each_child(&pdev->dev, NULL, sunxi_remove_device);

	return 0;
}

#ifdef CONFIG_PM_SLEEP

static int sunxi_msgbox_suspend(struct device *dev)
{
	return 0;
}

static int sunxi_msgbox_resume(struct device *dev)
{
	return 0;
}

static SIMPLE_DEV_PM_OPS(sx_msg_pm, sunxi_msgbox_suspend, sunxi_msgbox_resume);
#define SX_MSGBOX_PM (&sx_msg_pm)

#else
#define SX_MSGBOX_PM NULL
#endif /* CONFIG_PM_SLEEP */

static const struct of_device_id sunxi_msgbox_table[] = {
	{ .compatible = "sunxi,msgbox-amp" },
	{},
};

static struct platform_driver sunxi_msgbox_driver = {
	.probe = sunxi_msgbox_probe,
	.remove = sunxi_msgbox_remove,
	.driver = {
		.name = "sunxi-msgbox-amp",
		.of_match_table = sunxi_msgbox_table,
		.pm = SX_MSGBOX_PM,
	},
};

static int __init sunxi_msgbox_init(void)
{
	return platform_driver_register(&sunxi_msgbox_driver);
}
subsys_initcall(sunxi_msgbox_init);

static void __exit sunxi_msgbox_exit(void)
{
	platform_driver_unregister(&sunxi_msgbox_driver);
}
module_exit(sunxi_msgbox_exit);

MODULE_AUTHOR("fuyao <fuyao@allwinnertech.com>");
MODULE_VERSION("0.0.1");
MODULE_DESCRIPTION("msgbox amp communication channel");
MODULE_LICENSE("GPL v2");
