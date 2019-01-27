// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2012-2016 Zhang, Keguang <keguang.zhang@gmail.com>
 * Copyright (c) 2019 Jiaxun Yang <jiaxun.yang@flygoat.com>
 */

#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/of_address.h>

#include "clk.h"

#include <dt-bindings/clock/ls1c-clock.h>

#define LS1C_CLK_COUNT	6

#define DIV_APB		1

/* PLL/SDRAM Frequency configuration register Bits */
#define PLL_VALID			BIT(31)
#define FRAC_N				GENMASK(23, 16)
#define RST_TIME			GENMASK(3, 2)
#define SDRAM_DIV			GENMASK(1, 0)

/* CPU/CAMERA/DC Frequency configuration register Bits */
#define DIV_DC_EN			BIT(31)
#define DIV_DC				GENMASK(30, 24)
#define DIV_CAM_EN			BIT(23)
#define DIV_CAM				GENMASK(22, 16)
#define DIV_CPU_EN			BIT(15)
#define DIV_CPU				GENMASK(14, 8)
#define DIV_DC_SEL_EN			BIT(5)
#define DIV_DC_SEL			BIT(4)
#define DIV_CAM_SEL_EN			BIT(3)
#define DIV_CAM_SEL			BIT(2)
#define DIV_CPU_SEL_EN			BIT(1)
#define DIV_CPU_SEL			BIT(0)

#define DIV_DC_SHIFT			24
#define DIV_CAM_SHIFT			16
#define DIV_CPU_SHIFT			8
#define DIV_DDR_SHIFT			0

#define DIV_DC_WIDTH			7
#define DIV_CAM_WIDTH			7
#define DIV_CPU_WIDTH			7
#define DIV_DDR_WIDTH			2

static DEFINE_SPINLOCK(_lock);

static unsigned long ls1x_pll_recalc_rate(struct clk_hw *hw,
					  unsigned long parent_rate)
{
	u32 pll, rate;

	pll = __raw_readl(CLK_PLL_FREQ_ADDR);
	rate = ((pll >> 8) & 0xff) + ((pll >> 16) & 0xff);
	rate *= parent_rate;
	rate >>= 2;

	return rate;
}

static const struct clk_ops ls1x_pll_clk_ops = {
	.recalc_rate = ls1x_pll_recalc_rate,
};

static const struct clk_div_table ahb_div_table[] = {
	[0] = { .val = 0, .div = 2 },
	[1] = { .val = 1, .div = 4 },
	[2] = { .val = 2, .div = 3 },
	[3] = { .val = 3, .div = 3 },
};

struct clk_hw_onecell_data __init *ls1c_clk_init_hw(const char *osc_name)
{
	struct clk_hw *hw;
	struct clk_hw_onecell_data *onecell;

	onecell = kzalloc(sizeof(*onecell) +
			  (LS1C_CLK_COUNT * sizeof(struct clk_hw *)),
			  GFP_KERNEL);

	if (!onecell)
		return NULL;

	onecell->num = LS1C_CLK_COUNT;

	/* clock derived from OSC clk */
	hw = clk_hw_register_pll(NULL, "pll_clk", osc_name,
				&ls1x_pll_clk_ops, 0);
	if (!hw)
		goto err;
	onecell->hws[LS1C_CLK_PLL] = hw;

	hw = clk_hw_register_divider(NULL, "cpu_clk", "pll_clk",
				   CLK_GET_RATE_NOCACHE, CLK_PLL_DIV_ADDR,
				   DIV_CPU_SHIFT, DIV_CPU_WIDTH,
				   CLK_DIVIDER_ONE_BASED |
				   CLK_DIVIDER_ROUND_CLOSEST, &_lock);
	if (!hw)
		goto err;
	onecell->hws[LS1C_CLK_CPU] = hw;


	hw = clk_hw_register_divider(NULL, "dc_clk", "pll_clk",
				   0, CLK_PLL_DIV_ADDR, DIV_DC_SHIFT,
				   DIV_DC_WIDTH, CLK_DIVIDER_ONE_BASED, &_lock);
	if (!hw)
		goto err;
	onecell->hws[LS1C_CLK_DC] = hw;

	hw = clk_hw_register_divider_table(NULL, "ddr_clk", "cpu_clk",
				0, CLK_PLL_FREQ_ADDR, DIV_DDR_SHIFT,
				DIV_DDR_WIDTH, CLK_DIVIDER_ALLOW_ZERO,
				ahb_div_table, &_lock);
	if (!hw)
		goto err;
	onecell->hws[LS1C_CLK_DDR] = hw;


	hw = clk_hw_register_fixed_factor(NULL, "ahb_clk", "ddr_clk",
					0, 1, 1);
	if (!hw)
		goto err;
	onecell->hws[LS1C_CLK_AHB] = hw;


	hw = clk_hw_register_fixed_factor(NULL, "apb_clk", "ahb_clk", 0, 1,
					DIV_APB);
	if (!hw)
		goto err;
	onecell->hws[LS1C_CLK_APB] = hw;

	return onecell;

err:
	kfree(onecell);
	return NULL;
}

void __init ls1c_register_clkdev(struct clk_hw_onecell_data *onecell)
{
	struct clk_hw *hw;

	hw = onecell->hws[LS1C_CLK_PLL];
	clk_hw_register_clkdev(hw, "pll_clk", NULL);

	hw = onecell->hws[LS1C_CLK_CPU];
	clk_hw_register_clkdev(hw, "cpu_clk", NULL);

	hw = onecell->hws[LS1C_CLK_DC];
	clk_hw_register_clkdev(hw, "dc_clk", NULL);

	hw = onecell->hws[LS1C_CLK_DDR];
	clk_hw_register_clkdev(hw, "ddr_clk", NULL);

	hw = onecell->hws[LS1C_CLK_AHB];
	clk_hw_register_clkdev(hw, "ahb_clk", NULL);
	clk_hw_register_clkdev(hw, "ls1x-dma", NULL);
	clk_hw_register_clkdev(hw, "stmmaceth", NULL);

	hw = onecell->hws[LS1C_CLK_APB];
	clk_hw_register_clkdev(hw, "apb_clk", NULL);
	clk_hw_register_clkdev(hw, "ls1x-ac97", NULL);
	clk_hw_register_clkdev(hw, "ls1x-i2c", NULL);
	clk_hw_register_clkdev(hw, "ls1x-nand", NULL);
	clk_hw_register_clkdev(hw, "ls1x-pwmtimer", NULL);
	clk_hw_register_clkdev(hw, "ls1x-spi", NULL);
	clk_hw_register_clkdev(hw, "ls1x-wdt", NULL);
	clk_hw_register_clkdev(hw, "serial8250", NULL);
}

static void __init ls1c_clk_of_setup(struct device_node *np)
{
	struct clk_hw_onecell_data *onecell;
	int err;
	const char *parent = of_clk_get_parent_name(np, 0);

	clk_base = of_iomap(np, 0);
	pr_info("ls1c-clk: driver reg start");
	onecell = ls1c_clk_init_hw(parent);
	if (!onecell)
		pr_err("ls1c-clk: unable to register clk_hw");

	err = of_clk_add_hw_provider(np, of_clk_hw_onecell_get, onecell);
	if (err)
		pr_err("ls1c-clk: failed to add DT provider: %d\n", err);

	pr_info("ls1c-clk: driver registered");
}
CLK_OF_DECLARE(clk_ls1c, "loongson,ls1c-clock", ls1c_clk_of_setup);