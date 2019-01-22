/*
 * Copyright (c) 2011 Zhang, Keguang <keguang.zhang@gmail.com>
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General	 Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/init.h>
#include <linux/io.h>
#include <linux/irqchip.h>
#include <linux/kernel.h>
#include <linux/libfdt.h>
#include <linux/of_fdt.h>
#include <asm/prom.h>

#include <asm/bootinfo.h>
#include <prom.h>

void __init plat_mem_setup(void)
{
	add_memory_region(0x0, (memsize << 20), BOOT_MEM_RAM);
	__dt_setup_arch(__dtb_start);
}

void __init device_tree_init(void)
{
	unflatten_and_copy_device_tree();
}

void __init arch_init_irq(void)
{
	irqchip_init();
}


const char *get_system_type(void)
{
	const char *str;
	int err;

	err = of_property_read_string(of_root, "model", &str);
	if (!err)
		return str;

	err = of_property_read_string_index(of_root, "compatible", 0, &str);
	if (!err)
		return str;

	return "Unknown Loongson-1 System";
}
