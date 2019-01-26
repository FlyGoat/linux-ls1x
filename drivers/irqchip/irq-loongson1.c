// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2019, Jiaxun Yang <jiaxun.yang@flygoat.com>
 *  Loongson-1 platform IRQ support
 */

#include <linux/errno.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/irqchip.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/io.h>
#include <linux/irqchip/chained_irq.h>

#include <asm/mach-loongson32/irq.h>


#define MAX_CHIPS	5
#define LS_REG_INTC_STATUS	0x00
#define LS_REG_INTC_EN	0x04
#define LS_REG_INTC_SET	0x08
#define LS_REG_INTC_CLR	0x0c
#define LS_REG_INTC_POL	0x10
#define LS_REG_INTC_EDGE	0x14
#define CHIP_SIZE	0x18

static void ls_chained_handle_irq(struct irq_desc *desc)
{
	struct irq_chip_generic *gc =irq_desc_get_handler_data(desc);
	struct irq_chip *chip = irq_desc_get_chip(desc);
	u32 pending;
	
	chained_irq_enter(chip, desc);
	pending = readl(gc->reg_base + LS_REG_INTC_STATUS) &
			readl(gc->reg_base + LS_REG_INTC_EN);

	if (!pending) {
		spurious_interrupt();
		chained_irq_exit(chip, desc);
		return;
	}

	while (pending) {
		int bit = __ffs(pending);
		generic_handle_irq(gc->irq_base + bit);
		pending &= ~BIT(bit);
	}

	chained_irq_exit(chip, desc);
}

static void ls_intc_set_bit(struct irq_chip_generic *gc,
							unsigned int offset, 
							u32 mask, bool set)
{
	if (set)
		writel(readl(gc->reg_base + offset) | mask,
		gc->reg_base + offset);
	else
		writel(readl(gc->reg_base + offset) & ~mask,
		gc->reg_base + offset);
}

static int ls_intc_set_type(struct irq_data *data, unsigned int type)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(data);
	u32 mask = data->mask;

	switch (type) {
	case IRQ_TYPE_LEVEL_HIGH:
		ls_intc_set_bit(gc, LS_REG_INTC_EDGE, mask, false);
		ls_intc_set_bit(gc, LS_REG_INTC_POL, mask, true);
		break;
	case IRQ_TYPE_LEVEL_LOW:
		ls_intc_set_bit(gc, LS_REG_INTC_EDGE, mask, false);
		ls_intc_set_bit(gc, LS_REG_INTC_POL, mask, false);
		break;
	case IRQ_TYPE_EDGE_RISING:
		ls_intc_set_bit(gc, LS_REG_INTC_EDGE, mask, true);
		ls_intc_set_bit(gc, LS_REG_INTC_POL, mask, true);
		break;
	case IRQ_TYPE_EDGE_FALLING:
		ls_intc_set_bit(gc, LS_REG_INTC_EDGE, mask, true);
		ls_intc_set_bit(gc, LS_REG_INTC_POL, mask, false);
		break;
	case IRQ_TYPE_NONE:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}


static int __init ls_intc_of_init(struct device_node *node,
				       struct device_node *parent)
{
	struct irq_chip_generic *gc[MAX_CHIPS];
	struct irq_chip_type *ct;
	struct irq_domain *domain;
	void __iomem *base;
	int parent_irq[MAX_CHIPS], err = 0;
	unsigned int i = 0;

	unsigned int num_chips = of_irq_count(node);

	base = of_iomap(node, 0);
	if (!base)
		return -ENODEV;

	for (i = 0; i < num_chips; i++) {
		/* Mask all irqs */
		writel(0x0, base + (i * CHIP_SIZE) +
		       LS_REG_INTC_EN);

		/* Set all irqs to high level triggered */
		writel(0xffffffff, base + (i * CHIP_SIZE) +
		       LS_REG_INTC_POL);

		parent_irq[i] = irq_of_parse_and_map(node, i);

		if (!parent_irq[i]) {
			pr_warn("unable to get parent irq for irqchip %u\n", i);
			goto out_err;
		}

		gc[i] = irq_alloc_generic_chip("INTC", 1,
					    LS1X_IRQ_BASE + (i * 32),
					    base + (i * CHIP_SIZE),
					    handle_level_irq);

		ct = gc[i]->chip_types;
		ct->regs.mask = LS_REG_INTC_EN;
		ct->regs.ack = LS_REG_INTC_CLR;
		ct->chip.irq_unmask = irq_gc_mask_set_bit;
		ct->chip.irq_mask = irq_gc_mask_clr_bit;
		ct->chip.irq_ack = irq_gc_ack_set_bit;
		ct->chip.irq_set_type = ls_intc_set_type;

		irq_setup_generic_chip(gc[i], IRQ_MSK(32), 0, 0,
				       IRQ_NOPROBE | IRQ_LEVEL);
	}

	domain = irq_domain_add_legacy(node, num_chips * 32, LS1X_IRQ_BASE, 0,
		&irq_domain_simple_ops, NULL);
	if (!domain) {
		pr_warn("unable to register IRQ domain\n");
		err = -EINVAL;
		goto out_err;
	}

	for (i = 0; i < num_chips; i++)
		irq_set_chained_handler_and_data(parent_irq[i],
		ls_chained_handle_irq, gc[i]);

	pr_info("ls1x-irq: %u chips registered\n", num_chips);

	return 0;

out_err:
	for (i = 0; i < MAX_CHIPS; i++) {
		if(gc[i])
		irq_destroy_generic_chip(gc[i], IRQ_MSK(32),
				       IRQ_NOPROBE | IRQ_LEVEL, 0);
		if (parent_irq[i])
			irq_dispose_mapping(parent_irq[i]);
	}
	return err;
}

IRQCHIP_DECLARE(ls1x_intc, "loongson,ls1x-intc", ls_intc_of_init);