// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2012-2016 Zhang, Keguang <keguang.zhang@gmail.com>
 * Copyright (c) 2019 Jiaxun Yang <jiaxun.yang@flygoat.com>
 */

#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/err.h>

#include "clk.h"
#include <dt-bindings/clock/ls1b-clock.h>

#define LS1B_CLK_COUNT	9

#define DIV_APB		2
/* Clock PLL Divisor Register Bits */
#define DIV_DC_EN			BIT(31)
#define DIV_DC_RST			BIT(30)
#define DIV_CPU_EN			BIT(25)
#define DIV_CPU_RST			BIT(24)
#define DIV_DDR_EN			BIT(19)
#define DIV_DDR_RST			BIT(18)
#define RST_DC_EN			BIT(5)
#define RST_DC				BIT(4)
#define RST_DDR_EN			BIT(3)
#define RST_DDR				BIT(2)
#define RST_CPU_EN			BIT(1)
#define RST_CPU				BIT(0)

#define DIV_DC_SHIFT			26
#define DIV_CPU_SHIFT			20
#define DIV_DDR_SHIFT			14

#define DIV_DC_WIDTH			4
#define DIV_CPU_WIDTH			4
#define DIV_DDR_WIDTH			4

#define BYPASS_DC_SHIFT			12
#define BYPASS_DDR_SHIFT		10
#define BYPASS_CPU_SHIFT		8

#define BYPASS_DC_WIDTH			1
#define BYPASS_DDR_WIDTH		1
#define BYPASS_CPU_WIDTH		1

static DEFINE_SPINLOCK(_lock);

static unsigned long ls1x_pll_recalc_rate(struct clk_hw *hw,
					  unsigned long parent_rate)
{
	u32 pll, rate;

	pll = __raw_readl(CLK_PLL_FREQ_ADDR);
	rate = 12 + (pll & GENMASK(5, 0));
	rate *= parent_rate;
	rate >>= 1;

	return rate;
}

static const struct clk_ops ls1x_pll_clk_ops = {
	.recalc_rate = ls1x_pll_recalc_rate,
};


struct clk_hw_onecell_data __init *ls1b_clk_init_hw(const char *osc_name)
{
	struct clk_hw *hw;
	struct clk_hw_onecell_data *onecell;
	const char *parents[2];

	onecell = kzalloc(sizeof(*onecell) +
			  (LS1B_CLK_COUNT * sizeof(struct clk_hw *)),
			  GFP_KERNEL);

	if (!onecell)
		return NULL;

	onecell->num = LS1B_CLK_COUNT;

	parents[1] = osc_name;
	/* clock derived from 33 MHz OSC clk */
	hw = clk_hw_register_pll(NULL, "pll_clk", osc_name,
				 &ls1x_pll_clk_ops, 0);
	if (!hw)
		goto err;
	onecell->hws[LS1B_CLK_PLL] = hw;

	/* clock derived from PLL clk */
	/*                                 _____
	 *         _______________________|     |
	 * OSC ___/                       | MUX |___ CPU CLK
	 *        \___ PLL ___ CPU DIV ___|     |
	 *                                |_____|
	 */
	hw = clk_hw_register_divider(NULL, "cpu_clk_div", "pll_clk",
				   CLK_GET_RATE_NOCACHE, CLK_PLL_DIV_ADDR,
				   DIV_CPU_SHIFT, DIV_CPU_WIDTH,
				   CLK_DIVIDER_ONE_BASED |
				   CLK_DIVIDER_ROUND_CLOSEST, &_lock);
	if (!hw)
		goto err;
	onecell->hws[LS1B_CLK_CPU_DIV] = hw;

	parents[0] = "cpu_clk_div";
	hw = clk_hw_register_mux(NULL, "cpu_clk", parents,
			       ARRAY_SIZE(parents),
			       CLK_SET_RATE_NO_REPARENT, CLK_PLL_DIV_ADDR,
			       BYPASS_CPU_SHIFT, BYPASS_CPU_WIDTH, 0, &_lock);
	if (!hw)
		goto err;
	onecell->hws[LS1B_CLK_CPU] = hw;

	/*                                 _____
	 *         _______________________|     |
	 * OSC ___/                       | MUX |___ DC  CLK
	 *        \___ PLL ___ DC  DIV ___|     |
	 *                                |_____|
	 */
	hw = clk_hw_register_divider(NULL, "dc_clk_div", "pll_clk",
				   0, CLK_PLL_DIV_ADDR, DIV_DC_SHIFT,
				   DIV_DC_WIDTH, CLK_DIVIDER_ONE_BASED, &_lock);
	if (!hw)
		goto err;
	onecell->hws[LS1B_CLK_DC_DIV] = hw;

	parents[0] = "dc_clk_div";
	hw = clk_hw_register_mux(NULL, "dc_clk", parents,
			       ARRAY_SIZE(parents),
			       CLK_SET_RATE_NO_REPARENT, CLK_PLL_DIV_ADDR,
			       BYPASS_DC_SHIFT, BYPASS_DC_WIDTH, 0, &_lock);
	if (!hw)
		goto err;
	onecell->hws[LS1B_CLK_DC] = hw;

	/*                                 _____
	 *         _______________________|     |
	 * OSC ___/                       | MUX |___ DDR CLK
	 *        \___ PLL ___ DDR DIV ___|     |
	 *                                |_____|
	 */
	hw = clk_hw_register_divider(NULL, "ddr_clk_div", "pll_clk",
				   0, CLK_PLL_DIV_ADDR, DIV_DDR_SHIFT,
				   DIV_DDR_WIDTH, CLK_DIVIDER_ONE_BASED,
				   &_lock);
	if (!hw)
		goto err;
	onecell->hws[LS1B_CLK_DDR_DIV] = hw;

	parents[0] = "ddr_clk_div";
	hw = clk_hw_register_mux(NULL, "ddr_clk", parents,
			       ARRAY_SIZE(parents),
			       CLK_SET_RATE_NO_REPARENT, CLK_PLL_DIV_ADDR,
			       BYPASS_DDR_SHIFT, BYPASS_DDR_WIDTH, 0, &_lock);
	if (!hw)
		goto err;
	onecell->hws[LS1B_CLK_DDR] = hw;

	hw = clk_hw_register_fixed_factor(NULL, "ahb_clk", "ddr_clk", 0, 1,
					1);
	if (!hw)
		goto err;
	onecell->hws[LS1B_CLK_AHB] = hw;

	/* clock derived from AHB clk */
	/* APB clk is always half of the AHB clk */
	hw = clk_hw_register_fixed_factor(NULL, "apb_clk", "ahb_clk", 0, 1,
					DIV_APB);
	if (!hw)
		goto err;
	onecell->hws[LS1B_CLK_APB] = hw;

	return onecell;

err:
	kfree(onecell);
	return NULL;
}

void __init ls1b_register_clkdev(struct clk_hw_onecell_data *onecell)
{
	struct clk_hw *hw;

	hw = onecell->hws[LS1B_CLK_PLL];
	clk_hw_register_clkdev(hw, "pll_clk", NULL);

	hw = onecell->hws[LS1B_CLK_CPU_DIV];
	clk_hw_register_clkdev(hw, "cpu_clk_div", NULL);

	hw = onecell->hws[LS1B_CLK_CPU];
	clk_hw_register_clkdev(hw, "cpu_clk", NULL);


	hw = onecell->hws[LS1B_CLK_DC_DIV];
	clk_hw_register_clkdev(hw, "dc_clk_div", NULL);

	hw = onecell->hws[LS1B_CLK_DC];
	clk_hw_register_clkdev(hw, "dc_clk", NULL);

	hw = onecell->hws[LS1B_CLK_DDR_DIV];
	clk_hw_register_clkdev(hw, "ddr_clk_div", NULL);

	hw = onecell->hws[LS1B_CLK_DDR];
	clk_hw_register_clkdev(hw, "ddr_clk_div", NULL);

	hw = onecell->hws[LS1B_CLK_AHB];
	clk_hw_register_clkdev(hw, "ahb_clk", NULL);
	clk_hw_register_clkdev(hw, "ls1x-dma", NULL);
	clk_hw_register_clkdev(hw, "stmmaceth", NULL);

	hw = onecell->hws[LS1B_CLK_APB];
	clk_hw_register_clkdev(hw, "apb_clk", NULL);
	clk_hw_register_clkdev(hw, "ls1x-ac97", NULL);
	clk_hw_register_clkdev(hw, "ls1x-i2c", NULL);
	clk_hw_register_clkdev(hw, "ls1x-nand", NULL);
	clk_hw_register_clkdev(hw, "ls1x-pwmtimer", NULL);
	clk_hw_register_clkdev(hw, "ls1x-spi", NULL);
	clk_hw_register_clkdev(hw, "ls1x-wdt", NULL);
	clk_hw_register_clkdev(hw, "serial8250", NULL);
}
