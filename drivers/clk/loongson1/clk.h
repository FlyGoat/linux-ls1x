// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2012-2016 Zhang, Keguang <keguang.zhang@gmail.com>
 * Copyright (c) 2019 Jiaxun Yang <jiaxun.yang@flygoat.com>
 */

#ifndef __LOONGSON1_CLK_H
#define __LOONGSON1_CLK_H

struct clk_hw *clk_hw_register_pll(struct device *dev,
				   const char *name,
				   const char *parent_name,
				   const struct clk_ops *ops,
				   unsigned long flags);

struct clk_hw_onecell_data __init *ls1c_clk_init_hw(const char *osc_name);
struct clk_hw_onecell_data __init *ls1b_clk_init_hw(const char *osc_name);

void __init ls1c_register_clkdev(struct clk_hw_onecell_data *onecell);
void __init ls1b_register_clkdev(struct clk_hw_onecell_data *onecell);

extern void __iomem *clk_base;

#define CLK_PLL_FREQ_ADDR	clk_base
#define CLK_PLL_DIV_ADDR	(clk_base + 0x4)

#endif /* __LOONGSON1_CLK_H */
