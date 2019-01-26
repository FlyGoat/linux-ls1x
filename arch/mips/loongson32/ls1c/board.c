/*
 * Copyright (c) 2016 Yang Ling <gnaygnil@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include <linux/leds.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>

#include <loongson1.h>
#include <mtd/mtd-abi.h>
#include <platform.h>

static struct mtd_partition ls1x_spi_parts[] = {
	{
		.name		= "pmon",
		.offset	= 0,
		.size	= 1024 * 1024,
		.mask_flags = MTD_WRITEABLE,
	}, {
		.name		= "rootfs",
		.offset	= 1024 * 1024,
		.size	= 7 * 1024 * 1024,
	}
};

static struct flash_platform_data ls1x_spi_fpdata = {
	.name		= "spi-flash",
	.parts		= ls1x_spi_parts,
	.nr_parts	= ARRAY_SIZE(ls1x_spi_parts),
	.type		= "gd25q64",
};

static struct spi_board_info ls1x_spi_devices[] = {
	{
		.modalias	= "m25p80",
		.bus_num 	= 0,
		.chip_select	= SPI0_CS0,
		.max_speed_hz	= 60000000,
		.platform_data	= &ls1x_spi_fpdata,
	}
};

static struct plat_ls1x_spi ls1x_spi_pdata = {
	.cs_count = SPI0_CS3 + 1,
};

static const struct gpio_led ls1x_gpio_leds[] __initconst = {
	{
		.name			= "n19:red:status",
		.gpio			= 52,
		.active_low		= 1,
	}
};

static const struct gpio_led_platform_data ls1x_led_pdata __initconst = {
	.num_leds	= ARRAY_SIZE(ls1x_gpio_leds),
	.leds		= ls1x_gpio_leds,
};

static struct platform_device *ls1c_platform_devices[] __initdata = {
//	&ls1x_uart_pdev,
//	&ls1x_eth0_pdev,
//	&ls1x_ohci_pdev,
//	&ls1x_ehci_pdev,
	&ls1x_gpio0_pdev,
	&ls1x_gpio1_pdev,
//	&ls1x_wdt_pdev,
//	&ls1x_spi_pdev,
};

static int __init ls1c_platform_init(void)
{
	//ls1x_serial_set_uartclk(&ls1x_uart_pdev);

	ls1x_spi_set_platdata(&ls1x_spi_pdata);
	spi_register_board_info(ls1x_spi_devices,
				   ARRAY_SIZE(ls1x_spi_devices));

//	gpio_led_register_device(-1, &ls1x_led_pdata);

	return platform_add_devices(ls1c_platform_devices,
				   ARRAY_SIZE(ls1c_platform_devices));
}

arch_initcall(ls1c_platform_init);
