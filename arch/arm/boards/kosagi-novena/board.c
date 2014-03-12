/*
 * Copyright (C) 2013 Sascha Hauer, Pengutronix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation.
 *
 */
#define pr_fmt(fmt)  "kosagi-novena: " fmt

#include <generated/mach-types.h>
#include <common.h>
#include <sizes.h>
#include <envfs.h>
#include <init.h>
#include <gpio.h>
#include <net.h>
#include <linux/phy.h>

#include <asm/armlinux.h>
#include <asm/memory.h>
#include <asm/mmu.h>
#include <asm/io.h>

#include <mach/imx6-regs.h>
#include <mach/generic.h>
#include <mach/bbu.h>

static int kosagi_novena_init(void)
{
	if (!of_machine_is_compatible("kosagi,novena"))
		return 0;

	return 0;
}
device_initcall(kosagi_novena_init);
