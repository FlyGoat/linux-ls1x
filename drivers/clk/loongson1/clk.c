// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2012-2016 Zhang, Keguang <keguang.zhang@gmail.com>
 * Copyright (c) 2019 Jiaxun Yang <jiaxun.yang@flygoat.com>
 */

#include <linux/clkdev.h>

#include <linux/clk-provider.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of_address.h>

#include <asm/mach-loongson32/platform.h>

void __iomem *clk_base;

#define LS1C_OSC		(24 * 1000000)
#define LS1B_OSC		(33 * 1000000)

#define LS1X_CLK_BASE	0x1fe78030

#include "clk.h"

struct clk_hw *__init clk_hw_register_pll(struct device *dev,
					  const char *name,
					  const char *parent_name,
					  const struct clk_ops *ops,
					  unsigned long flags)
{
	int ret;
	struct clk_hw *hw;
	struct clk_init_data init;

	/* allocate the divider */
	hw = kzalloc(sizeof(*hw), GFP_KERNEL);
	if (!hw)
		return ERR_PTR(-ENOMEM);

	init.name = name;
	init.ops = ops;
	init.flags = flags;
	init.parent_names = parent_name ? &parent_name : NULL;
	init.num_parents = parent_name ? 1 : 0;
	hw->init = &init;

	/* register the clock */
	ret = clk_hw_register(dev, hw);
	if (ret) {
		kfree(hw);
		hw = ERR_PTR(ret);
	}

	return hw;
}


static void __init ls1b_clk_of_setup(struct device_node *np)
{
	struct clk_hw_onecell_data *onecell;
	int err;
	const char *parent = of_clk_get_parent_name(np, 0);

	clk_base = of_iomap(np, 0);

	onecell = ls1b_clk_init_hw(parent);
	if (!onecell)
		pr_err("ls1b-clk: unable to register clk_hw");

	err = of_clk_add_hw_provider(np, of_clk_hw_onecell_get, onecell);
	if (err)
		pr_err("ls1b-clk: failed to add DT provider: %d\n", err);

	pr_info("ls1b-clk: driver registered");
}

CLK_OF_DECLARE(clk_ls1b, "loongson,ls1b-clock", ls1b_clk_of_setup);



void __init ls1x_clk_init(void)
{
	struct clk_hw *hw;
	struct clk_hw_onecell_data *onecell;

	clk_base = (void __iomem *)KSEG1ADDR(LS1X_CLK_BASE);
#ifdef CONFIG_LOONGSON1_LS1B
	hw = clk_hw_register_fixed_rate(NULL, "osc_clk", NULL, 0, LS1B_OSC);
	clk_hw_register_clkdev(hw, "osc_clk", NULL);
	onecell = ls1c_clk_init_hw("osc_clk");
	if (!onecell)
		panic("ls1x-clk: unable to register clk_hw");

	ls1c_register_clkdev(onecell);
#elif defined(CONFIG_LOONGSON1_LS1C)
	hw = clk_hw_register_fixed_rate(NULL, "osc_clk", NULL, 0, LS1C_OSC);
	clk_hw_register_clkdev(hw, "osc_clk", NULL);
	onecell = ls1c_clk_init_hw("osc_clk");
	if (!onecell)
		panic("ls1x-clk: unable to register clk_hw");

	ls1c_register_clkdev(onecell);
#else
	panic("ls1x-clk: not loongson platform");
#endif
}

