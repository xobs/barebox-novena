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
	printf("Hello there\n");
	if (!of_machine_is_compatible("kosagi,novena"))
		return 0;

	imx6_bbu_internal_mmc_register_handler("mmc", "/dev/mmc3.boot0",
		BBU_HANDLER_FLAG_DEFAULT, NULL, 0, 0);

	armlinux_set_bootparams((void *)0x10000100);
	armlinux_set_architecture(MACH_TYPE_MX6Q_SABRESD);

	return 0;
}
device_initcall(kosagi_novena_init);
