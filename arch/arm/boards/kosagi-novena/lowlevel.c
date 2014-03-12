/*
 * Copyright (C) 2013 Sascha Hauer <s.hauer@pengutronix.de>
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
 */
#include <common.h>
#include <sizes.h>
#include <io.h>
#include <asm/sections.h>
#include <asm/mmu.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/imx6-mmdc.h>
#include <mach/imx6-regs.h>

#include <debug_ll.h>


static inline void early_uart_init(void)
{
	void __iomem *ccmbase = (void *)MX6_CCM_BASE_ADDR;
	void __iomem *uartbase = (void *)MX6_UART2_BASE_ADDR;

	writel(0xffffffff, ccmbase + 0x68);
	writel(0xffffffff, ccmbase + 0x6c);
	writel(0xffffffff, ccmbase + 0x70);
	writel(0xffffffff, ccmbase + 0x74);
	writel(0xffffffff, ccmbase + 0x78);
	writel(0xffffffff, ccmbase + 0x7c);
	writel(0xffffffff, ccmbase + 0x80);

	writel(0x00000000, uartbase + 0x80);
	writel(0x00004027, uartbase + 0x84);
	writel(0x00000704, uartbase + 0x88);
	writel(0x00000a81, uartbase + 0x90);
	writel(0x0000002b, uartbase + 0x9c);
	writel(0x00013880, uartbase + 0xb0);
	writel(0x0000047f, uartbase + 0xa4);
	writel(0x0000c34f, uartbase + 0xa8);
	writel(0x00000001, uartbase + 0x80);

	putc_ll('>');
}

static inline void early_uart_init_6q(void)
{
	writel(0x4, MX6_IOMUXC_BASE_ADDR + 0xbc);
	writel(0x4, MX6_IOMUXC_BASE_ADDR + 0xc0);
	writel(0x1, MX6_IOMUXC_BASE_ADDR + 0x928);
	writel(0x0001b0b1, MX6_IOMUXC_BASE_ADDR + 0x3d0);
	writel(0x0001b0b1, MX6_IOMUXC_BASE_ADDR + 0x3d4);

	early_uart_init();
}

static inline void early_uart_init_6dl(void)
{
	writel(0x4, MX6_IOMUXC_BASE_ADDR + 0x16c);
	writel(0x4, MX6_IOMUXC_BASE_ADDR + 0x170);
	writel(0x1, MX6_IOMUXC_BASE_ADDR + 0x904);
	writel(0x0001b0b1, MX6_IOMUXC_BASE_ADDR + 0x53c);
	writel(0x0001b0b1, MX6_IOMUXC_BASE_ADDR + 0x540);

	early_uart_init();
}

extern char __dtb_imx6q_kosagi_novena_start[];

ENTRY_FUNCTION(start_imx6q_kosagi_novena_6q, r0, r1, r2)
{
	uint32_t fdt;
	uint32_t ram_size;

	ram_size = r0;
	ram_size = SZ_2G;
	fdt = (uint32_t)__dtb_imx6q_kosagi_novena_start - get_runtime_offset();
	barebox_arm_entry(0x10800000, ram_size, fdt);
}

extern char __dtb_imx6dl_kosagi_novena_start[];

ENTRY_FUNCTION(start_imx6dl_kosagi_novena_6dl, r0, r1, r2)
{
	uint32_t fdt;
	uint32_t ram_size;

	ram_size = r0;
	ram_size = SZ_2G;
	fdt = (uint32_t)__dtb_imx6dl_kosagi_novena_start - get_runtime_offset();
	barebox_arm_entry(0x10800000, ram_size, fdt);
}
