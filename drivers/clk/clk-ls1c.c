#define PLL_MUL_A_SHIFT 8
#define PLL_MUL_B_SHIFT 16
#define PLL_MUL_MASK    0xff

#define PLL_MUL_A       GENMASK(15, 8)
#define PLL_MUL_B       GENMASK(23, 16)

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

static void __iomem *base __initdata;

#define DIV_REG    (base + 0x4)


static DEFINE_SPINLOCK(_pll_lock);

static unsigned long ls1x_pll_recalc_rate(struct clk_hw *hw,
					  unsigned long parent_rate)
{
	u32 pll;

	pll = readl(DIV_BASE);
	return ((pll >> PLL_MUL_A_SHIFT) & PLL_MUL_MASK) +  \
    ((pll >> PLL_MUL_B_SHIFT) & PLL_MUL_MASK) * parent_rate / 4;
}