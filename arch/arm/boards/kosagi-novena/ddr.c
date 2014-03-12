/*
 * Copyright (C) 2010-2011 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <mach/imx6-mmdc.h>
#include <mach/imx6-regs.h>
#include <mach/iomux-mx6.h>
#include <debug_ll.h>

#define SPD_NUMBYTES         0
#define SPD_REVISION         1
#define SPD_DRAM_DEVICE_TYPE 2
#define SPD_MODULE_TYPE      3
#define SPD_SDRAM_DENSITY_AND_BANKS 4
#define SPD_SDRAM_ADDRESSING 5
#define SPD_NOMINAL_VOLTAGE  6
#define SPD_ORGANIZATION     7
#define SPD_BUS_WIDTH        8

#define SPD_MTB_DIVIDEND     10
#define SPD_MTB_DIVISOR      11
#define SPD_TCKMIN           12

#define SPD_CL_LSB           14
#define SPD_CL_MSB           15
#define SPD_TAAMIN           16
#define SPD_TWRMIN           17
#define SPD_TRCDMIN          18
#define SPD_TRRDMIN          19
#define SPD_TRPMIN           20
#define SPD_TRAS_TRC_MSB     21
#define SPD_TRAS_LSB         22
#define SPD_TRC_LSB          23
#define SPD_TRFC_LSB         24
#define SPD_TRFC_MSB         25
#define SPD_WTRMIN           26
#define SPD_RTPMIN           27
#define SPD_TFAW_MSB         28
#define SPD_TFAW_LSB         29
#define SPD_OPTIONAL         30
#define SPD_THERMAL          31

#define SPD_VENDOR_ID_LSB    117
#define SPD_VENDOR_ID_MSB    118

#define SPD_NAME             128

#define MTB_PER_CYC          0xF  /* 15 * 0.125ns per 533MHz clock cycle */


#define MPDGCTRL0_PHY0 0x021b083c
#define MPDGCTRL1_PHY0 0x021b0840
#define MPDGCTRL0_PHY1 0x021b483c
#define MPDGCTRL1_PHY1 0x021b4840
#define MPRDDLCTL_PHY0 0x021b0848
#define MPRDDLCTL_PHY1 0x021b4848
#define MPWRDLCTL_PHY0 0x021b0850
#define MPWRDLCTL_PHY1 0x021b4850

#define MDPDC_OFFSET 0x0004
#define MDCFG0_OFFSET 0x000C
#define MDCFG1_OFFSET 0x0010
#define MDCFG2_OFFSET 0x0014
#define MAPSR_OFFSET 0x0404
#define MDREF_OFFSET 0x0020
#define MDASP_OFFSET 0x0040
#define MPZQHWCTRL_OFFSET 0x0800
#define MDSCR_OFFSET 0x001C
#define MPWLGCR_OFFSET 0x0808
#define MPWLDECTRL0_OFFSET 0x080c
#define MPWLDECTRL1_OFFSET 0x0810
#define MDCTL_OFFSET 0x0000
#define MDMISC_OFFSET 0x0018
#define MPPDCMPR1_OFFSET 0x088C
#define MPSWDAR_OFFSET 0x0894
#define MPRDDLCTL_OFFSET 0x0848
#define MPMUR_OFFSET 0x08B8
#define MPDGCTRL0_OFFSET 0x083C
#define MPDGHWST0_OFFSET 0x087C
#define MPDGHWST1_OFFSET 0x0880
#define MPDGHWST2_OFFSET 0x0884
#define MPDGHWST3_OFFSET 0x0888
#define MPDGCTRL1_OFFSET 0x0840
#define MPRDDLHWCTL_OFFSET 0x0860
#define MPWRDLCTL_OFFSET 0x0850
#define MPWRDLHWCTL_OFFSET 0x0864

static uint32_t mx6q_sw_pad_ctl_pad_dram_sdqs[] = {
	0x020E05A8,
	0x020E05B0,
	0x020E0524,
	0x020E051C,
	0x020E0518,
	0x020E050C,
	0x020E05B8,
	0x020E05C0,
};

static uint32_t mx6dl_sw_pad_ctl_pad_dram_sdqs[] = {
	0x020e04bc,
	0x020e04c0,
	0x020e04c4,
	0x020e04c8,
	0x020e04cc,
	0x020e04d0,
	0x020e04d4,
	0x020e04d8,
};

#define printf(...)
#define GET_BIT(n,x) ( ( (x) >> (n) ) & 1 )

#define DISP_LINE_LEN 16

#define I2C_SPEED 10000
extern void fsl_i2c_init(const void __iomem *i2c, int speed);
extern int fsl_i2c_read(const void __iomem *i2c, uint8_t dev,
			uint addr, int alen, uint8_t *data, int length);
extern void fsl_i2c_stop(const void __iomem *i2c);


struct ddr_spd {
	uint	density;
	uint	banks;
	uint	rows;
	uint	cols;
	uint	rank;
	uint	devwidth;
	uint	capacity;  /* in megabits */
	uint	clockrate; /* in MHz */
	uint	caslatency;
	uint	tAAmin;
	uint	tWRmin;
	uint	tRCDmin;
	uint	tRRDmin;
	uint	tRPmin;
	uint	tRAS;
	uint	tRC;
	uint	tRFCmin;
	uint	tWTRmin;
	uint	tRTPmin;
	uint	tFAW;
	uint	vendorID;
	u_char	name[19];
};

void reg32_write(unsigned int addr, unsigned int data) {
	*((unsigned int *) addr) = (unsigned int) data;
}

volatile unsigned int reg32_read(unsigned int addr) {
	unsigned int data;
	data = *((volatile unsigned int *)addr);
	return data;
}

void reg32setbit(unsigned int addr, unsigned int bit) {
	*((unsigned int *)addr) |= (1 << bit);
}
void reg32clrbit(unsigned int addr, unsigned int bit) {
	*((unsigned int *)addr) &= ~(1 << bit);
}


static void print_i2c_status(void)
{
	uint8_t arg;
	uint8_t hex_hi;
	uint8_t hex_lo;

	arg = readb(MX6_I2C1_BASE_ADDR + 0x8);
	hex_lo = (arg >> 4) & 0xf;
	hex_hi = (arg >> 0) & 0xf;
	putc_ll(' ');
	putc_ll('C');
	putc_ll('R');
	putc_ll(':');
	putc_ll(' ');
	putc_ll('0');
	putc_ll('x');
	putc_ll( (hex_lo < 10) ? (hex_lo + '0') : (hex_lo + 'A' - 10) );
	putc_ll( (hex_hi < 10) ? (hex_hi + '0') : (hex_hi + 'A' - 10) );

	arg = readb(MX6_I2C1_BASE_ADDR + 0xc);
	hex_lo = (arg >> 4) & 0xf;
	hex_hi = (arg >> 0) & 0xf;
	putc_ll(' ');
	putc_ll('S');
	putc_ll('R');
	putc_ll(':');
	putc_ll(' ');
	putc_ll('0');
	putc_ll('x');
	putc_ll( (hex_lo < 10) ? (hex_lo + '0') : (hex_lo + 'A' - 10) );
	putc_ll( (hex_hi < 10) ? (hex_hi + '0') : (hex_hi + 'A' - 10) );
}

static void dram_fatal(uint8_t arg)
{
	uint8_t hex_hi = (arg >> 0) & 0xf;
	uint8_t hex_lo = (arg >> 4) & 0xf;

	print_i2c_status();
	/* Reset in case it's a transient error reading SPD */
	putc_ll(' ');
	putc_ll('F');
	putc_ll('a');
	putc_ll('t');
	putc_ll('a');
	putc_ll('l');
	putc_ll(' ');
	putc_ll('D');
	putc_ll('D');
	putc_ll('R');
	putc_ll(' ');
	putc_ll('e');
	putc_ll('r');
	putc_ll('r');
	putc_ll('o');
	putc_ll('r');
	putc_ll(' ');
	putc_ll('0');
	putc_ll('x');
	putc_ll( (hex_lo < 10) ? (hex_lo + '0') : (hex_lo + 'A' - 10) );
	putc_ll( (hex_hi < 10) ? (hex_hi + '0') : (hex_hi + 'A' - 10) );
	putc_ll('\r');
	putc_ll('\n');
	while(1);
}

static unsigned int mtb_to_cycles(unsigned int mtbs) {
	return (mtbs / MTB_PER_CYC) + (((mtbs % MTB_PER_CYC) > 0) ? 1 : 0);
}



///////////////////////////////////////////////////////
///////////////////////////////////////////////////////



static int do_tune_mww(int argc, char * const argv[])
{
	ulong	addr, writeval, count, bank, feedback, sum1;
	int	size;
	ulong	base_address = 0x10000000;

	if (argc > 1)
		bank = simple_strtoul(argv[1], NULL, 16);
	else
		bank = 0;

	/* Address is specified since argc > 1 */
	if (bank == 0)
		addr = base_address;
	else
		addr = base_address + 0x90000000;
	
	debug("write starting at %08lx\n", addr);

	/* Get the value to write.  */
	writeval = 0x0;

	count = 0x20000;
	size = 2;
	sum1 = 0;

	while (count-- > 0) {
		feedback = (GET_BIT(14,writeval) == GET_BIT(13,writeval));
		writeval = (writeval<<1) + feedback;
		writeval &= 0x7FFF;
		*((ushort  *)addr) = (ushort )writeval;
		sum1 += (ushort )writeval;
		addr += size;
	}
	debug("checksum: %08lx\n", sum1);
	return 0;
}

static int do_tune_mrr(int argc, char * const argv[])
{
	ulong	addr, writeval, count, bank, feedback, sum1, sum2;
	int	size;
	ulong	base_address = 0x10000000;

	if ( argc > 1)
		bank = simple_strtoul(argv[1], NULL, 16);
	else
		bank = 0;

	/* Address is specified since argc > 1 */
	if ( bank == 0 )
		addr = base_address;
	else
		addr = base_address + 0x90000000;
	debug("read starting at %08lx\n", addr);

	/* Get the value to write.  */
	writeval = 0x0;
	size = 2;

	sum1 = 0;
	sum2 = 0;
	count = 0x20000;
	feedback = 0;
	while (count-- > 0) {
		feedback = (GET_BIT(14,writeval) == GET_BIT(13,writeval));
		writeval = (writeval<<1) + feedback;
		writeval &= 0x7FFF;
		sum1 += (ushort )writeval;
		sum2 += *((ushort  *)addr);
		addr += size;
	}
	debug("computed: %08lx, readback: %08lx\n", sum1, sum2);
	return 0;
}


static int do_tune_wcal(int argc, char * const argv[])
{
	int temp1, temp2;
	int errorcount = 0;
	int ddr_mr1 = 0x04;
	int wldel0 = 0;
	int wldel1 = 0;
	int wldel2 = 0;
	int wldel3 = 0;
	int wldel4 = 0;
	int wldel5 = 0;
	int wldel6 = 0;
	int wldel7 = 0;
	int ldectrl[4];

	if (argc > 1)
		ddr_mr1 = (int) simple_strtoul(argv[1], NULL, 16);
	else
		ddr_mr1 = 0x04;

	if (argc > 3) {
		wldel0 = (int)simple_strtoul(argv[3], NULL, 16) & 0x3;
		wldel1 = (int)simple_strtoul(argv[4], NULL, 16) & 0x3;
		wldel2 = (int)simple_strtoul(argv[5], NULL, 16) & 0x3;
		wldel3 = (int)simple_strtoul(argv[6], NULL, 16) & 0x3;
		wldel4 = (int)simple_strtoul(argv[7], NULL, 16) & 0x3;
		wldel5 = (int)simple_strtoul(argv[8], NULL, 16) & 0x3;
		wldel6 = (int)simple_strtoul(argv[9], NULL, 16) & 0x3;
		wldel7 = (int)simple_strtoul(argv[10], NULL, 16) & 0x3;

		reg32_write(MX6_MMDC_P0_BASE_ADDR + MPWLDECTRL0_OFFSET,
			(reg32_read(MX6_MMDC_P0_BASE_ADDR + MPWLDECTRL0_OFFSET)
			& 0xF9FFF9FF)
			| (wldel0 << 9) | (wldel1 << 25));

		reg32_write(MX6_MMDC_P0_BASE_ADDR + MPWLDECTRL1_OFFSET,
			(reg32_read(MX6_MMDC_P0_BASE_ADDR + MPWLDECTRL1_OFFSET)
			& 0xF9FFF9FF)
			| (wldel2 << 9) | (wldel3 << 25));

		reg32_write(MX6_MMDC_P1_BASE_ADDR + MPWLDECTRL0_OFFSET,
			(reg32_read(MX6_MMDC_P0_BASE_ADDR + MPWLDECTRL0_OFFSET)
			& 0xF9FFF9FF)
			| (wldel4 << 9) | (wldel5 << 25));

		reg32_write(MX6_MMDC_P1_BASE_ADDR + MPWLDECTRL1_OFFSET,
		(reg32_read(MX6_MMDC_P0_BASE_ADDR + MPWLDECTRL1_OFFSET)
			& 0xF9FFF9FF)
			| (wldel6 << 9) | (wldel7 << 25));

		debug("MMDC_MPWLDECTRL0 before write level cal: 0x%08X\n",
			reg32_read(MX6_MMDC_P0_BASE_ADDR + MPWLDECTRL0_OFFSET));
		debug("MMDC_MPWLDECTRL1 before write level cal: 0x%08X\n",
			reg32_read(MX6_MMDC_P0_BASE_ADDR + MPWLDECTRL1_OFFSET));
		debug("MMDC_MPWLDECTRL0 before write level cal: 0x%08X\n",
			reg32_read(MX6_MMDC_P1_BASE_ADDR + MPWLDECTRL0_OFFSET));
		debug("MMDC_MPWLDECTRL1 before write level cal: 0x%08X\n",
			reg32_read(MX6_MMDC_P1_BASE_ADDR + MPWLDECTRL1_OFFSET));
	}

	/*
	 * Stash old values in case calibration fails,
	 * we need to restore them
	 */
	ldectrl[0] = reg32_read(MX6_MMDC_P0_BASE_ADDR + MPWLDECTRL0_OFFSET);
	ldectrl[1] = reg32_read(MX6_MMDC_P0_BASE_ADDR + MPWLDECTRL1_OFFSET);
	ldectrl[2] = reg32_read(MX6_MMDC_P1_BASE_ADDR + MPWLDECTRL0_OFFSET);
	ldectrl[3] = reg32_read(MX6_MMDC_P1_BASE_ADDR + MPWLDECTRL1_OFFSET);

	/* disable DDR logic power down timer */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MDPDC_OFFSET),
		reg32_read((MX6_MMDC_P0_BASE_ADDR + MDPDC_OFFSET)) & 0xffff00ff);
	/* disable Adopt power down timer */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MAPSR_OFFSET),
		reg32_read((MX6_MMDC_P0_BASE_ADDR + MAPSR_OFFSET)) | 0x1);

	debug("Start write leveling calibration\n");
	/*
	 * 2. disable auto refresh and ZQ calibration
	 * before proceeding with Write Leveling calibration
	 */
	temp1 = reg32_read(MX6_MMDC_P0_BASE_ADDR + MDREF_OFFSET);
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MDREF_OFFSET), 0x0000C000);
	temp2 = reg32_read(MX6_MMDC_P0_BASE_ADDR + MPZQHWCTRL_OFFSET);
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MPZQHWCTRL_OFFSET), temp2 & ~(0x3));

	/* 3. increase walat and ralat to maximum */
	reg32setbit((MX6_MMDC_P0_BASE_ADDR + MDMISC_OFFSET), 6); //set RALAT to max
	reg32setbit((MX6_MMDC_P0_BASE_ADDR + MDMISC_OFFSET), 7);
	reg32setbit((MX6_MMDC_P0_BASE_ADDR + MDMISC_OFFSET), 8);
	reg32setbit((MX6_MMDC_P0_BASE_ADDR + MDMISC_OFFSET), 16); //set WALAT to max
	reg32setbit((MX6_MMDC_P0_BASE_ADDR + MDMISC_OFFSET), 17);

	reg32setbit((MX6_MMDC_P1_BASE_ADDR + MDMISC_OFFSET), 6); //set RALAT to max
	reg32setbit((MX6_MMDC_P1_BASE_ADDR + MDMISC_OFFSET), 7);
	reg32setbit((MX6_MMDC_P1_BASE_ADDR + MDMISC_OFFSET), 8);
	reg32setbit((MX6_MMDC_P1_BASE_ADDR + MDMISC_OFFSET), 16); //set WALAT to max
	reg32setbit((MX6_MMDC_P1_BASE_ADDR + MDMISC_OFFSET), 17);

	/*
	 * 4 & 5. Configure the external DDR device to enter write-leveling
	 * mode through Load Mode Register command.
	 * Register setting:
	 * Bits[31:16] MR1 value (0x0080 write leveling enable)
	 * Bit[9] set WL_EN to enable MMDC DQS output
	 * Bits[6:4] set CMD bits for Load Mode Register programming
	 * Bits[2:0] set CMD_BA to 0x1 for DDR MR1 programming
	 */
	reg32_write(MX6_MMDC_P0_BASE_ADDR + MDSCR_OFFSET, 0x00808231);

	/* 6. Activate automatic calibration by setting MPWLGCR[HW_WL_EN] */
	reg32_write(MX6_MMDC_P0_BASE_ADDR + MPWLGCR_OFFSET, 0x00000001);

	/*
	 * 7. Upon completion of this process the MMDC de-asserts
	 * the MPWLGCR[HW_WL_EN]
	 */
	while (reg32_read(MX6_MMDC_P0_BASE_ADDR + MPWLGCR_OFFSET) & 0x00000001)
		debug(".");

	/*
	 * 8. check for any errors: check both PHYs for x64 configuration,
	 * if x32, check only PHY0
	 */
	if ((reg32_read(MX6_MMDC_P0_BASE_ADDR + MPWLGCR_OFFSET) & 0x00000F00) ||
	    (reg32_read(MX6_MMDC_P1_BASE_ADDR + MPWLGCR_OFFSET) & 0x00000F00))
		errorcount++;
	debug("Write leveling calibration completed, errcount: %d\n",
			errorcount);

	/* check to see if cal failed */
	if ((reg32_read(MX6_MMDC_P0_BASE_ADDR + MPWLDECTRL0_OFFSET)) == 0x001F001F
	&& (reg32_read(MX6_MMDC_P0_BASE_ADDR + MPWLDECTRL1_OFFSET)) == 0x001F001F
	&& (reg32_read(MX6_MMDC_P1_BASE_ADDR + MPWLDECTRL0_OFFSET)) == 0x001F001F
	&& (reg32_read(MX6_MMDC_P1_BASE_ADDR + MPWLDECTRL1_OFFSET)) == 0x001F001F) {
		debug("Cal seems to have soft-failed due to memory "
			"not supporting write leveling on all channels. "
			"Restoring original write leveling values.\n");
		reg32_write(MX6_MMDC_P0_BASE_ADDR + MPWLDECTRL0_OFFSET, ldectrl[0]);
		reg32_write(MX6_MMDC_P0_BASE_ADDR + MPWLDECTRL1_OFFSET, ldectrl[1]);
		reg32_write(MX6_MMDC_P1_BASE_ADDR + MPWLDECTRL0_OFFSET, ldectrl[2]);
		reg32_write(MX6_MMDC_P1_BASE_ADDR + MPWLDECTRL1_OFFSET, ldectrl[3]);
		errorcount++;
	}

	/*
	 * User should issue MRS command to exit write leveling mode
	 * through Load Mode Register command
	 * Register setting:
	 * Bits[31:16] MR1 value "ddr_mr1" value from initialization
	 * Bit[9] clear WL_EN to disable MMDC DQS output
	 * Bits[6:4] set CMD bits for Load Mode Register programming
	 * Bits[2:0] set CMD_BA to 0x1 for DDR MR1 programming
	 */
	reg32_write( MX6_MMDC_P0_BASE_ADDR + MDSCR_OFFSET,((ddr_mr1 << 16)+0x8031));
	/* re-enable to auto refresh and zq cal */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MDREF_OFFSET), temp1);
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MPZQHWCTRL_OFFSET), temp2);
#ifdef DEBUG
	debug("MMDC_MPWLDECTRL0 after write level cal: 0x%08X\n",
		reg32_read(MX6_MMDC_P0_BASE_ADDR + MPWLDECTRL0_OFFSET));
	debug("MMDC_MPWLDECTRL1 after write level cal: 0x%08X\n",
		reg32_read(MX6_MMDC_P0_BASE_ADDR + MPWLDECTRL1_OFFSET));
	debug("MMDC_MPWLDECTRL0 after write level cal: 0x%08X\n",
		reg32_read(MX6_MMDC_P1_BASE_ADDR + MPWLDECTRL0_OFFSET));
	debug("MMDC_MPWLDECTRL1 after write level cal: 0x%08X\n",
		reg32_read(MX6_MMDC_P1_BASE_ADDR + MPWLDECTRL1_OFFSET));
#else
	/* We must force a readback of these values, to get them to stick */
	reg32_read(MX6_MMDC_P0_BASE_ADDR + MPWLDECTRL0_OFFSET);
	reg32_read(MX6_MMDC_P0_BASE_ADDR + MPWLDECTRL1_OFFSET);
	reg32_read(MX6_MMDC_P1_BASE_ADDR + MPWLDECTRL0_OFFSET);
	reg32_read(MX6_MMDC_P1_BASE_ADDR + MPWLDECTRL1_OFFSET);
#endif
	/* enable DDR logic power down timer: */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MDPDC_OFFSET),
	reg32_read((MX6_MMDC_P0_BASE_ADDR + MDPDC_OFFSET)) | 0x00005500);
	/* enable Adopt power down timer: */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MAPSR_OFFSET),
	reg32_read((MX6_MMDC_P0_BASE_ADDR + MAPSR_OFFSET)) & 0xfffffff7);
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MDSCR_OFFSET),0); //clear CON_REQ
	return errorcount;
}


static int modify_dg_result(int reg_st0, int reg_st1, int reg_ctrl)
{
	/*
	 * DQS gating absolute offset should be modified from reflecting
	 * (HW_DG_LOWx + HW_DG_UPx)/2 to reflecting (HW_DG_UPx - 0x80)
	 */
	int dg_tmp_val0,dg_tmp_val1, dg_tmp_val2;
	int dg_dl_abs_offset0, dg_dl_abs_offset1;
	int dg_hc_del0, dg_hc_del1;
	dg_tmp_val0 = ((reg32_read(reg_st0) & 0x07ff0000) >>16) - 0xc0;
	dg_tmp_val1 = ((reg32_read(reg_st1) & 0x07ff0000) >>16) - 0xc0;
	dg_dl_abs_offset0 = dg_tmp_val0 & 0x7f;
	dg_hc_del0 = (dg_tmp_val0 & 0x780) << 1;
	dg_dl_abs_offset1 = dg_tmp_val1 & 0x7f;
	dg_hc_del1 = (dg_tmp_val1 & 0x780) << 1;
	dg_tmp_val2 = dg_dl_abs_offset0 + dg_hc_del0 + ((dg_dl_abs_offset1 +
						   dg_hc_del1) << 16);
	reg32_write((reg_ctrl),
		reg32_read((reg_ctrl)) & 0xf0000000);
	reg32_write((reg_ctrl),
		reg32_read((reg_ctrl)) & 0xf0000000);
	reg32_write((reg_ctrl),
		reg32_read((reg_ctrl)) | dg_tmp_val2);
	return 0;
}

static int do_tune_delays(uint32_t *dram_sdqs_pad_ctls)
{
	int temp1;
	int data_bus_size;
	int temp_ref;
	int cs0_enable = 0;
	int cs1_enable = 0;
	int cs0_enable_initial = 0;
	int cs1_enable_initial = 0;

	int PDDWord = 0x00FFFF00; /* best so far, place into MPPDCMPR1 */
	int errorcount = 0;
	unsigned int initdelay = 0x40404040;

	/* check to see which chip selects are enabled */
	cs0_enable_initial = (reg32_read(MX6_MMDC_P0_BASE_ADDR + MDCTL_OFFSET)
			& 0x80000000) >> 31;
	cs1_enable_initial = (reg32_read(MX6_MMDC_P0_BASE_ADDR + MDCTL_OFFSET)
			& 0x40000000) >> 30;
	//debug("init cs0: %d cs1: %d\n", cs0_enable_initial, cs1_enable_initial);
	/* disable DDR logic power down timer: */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MDPDC_OFFSET),
	reg32_read((MX6_MMDC_P0_BASE_ADDR + MDPDC_OFFSET)) & 0xffff00ff);

	/* disable Adopt power down timer: */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MAPSR_OFFSET),
	reg32_read((MX6_MMDC_P0_BASE_ADDR + MAPSR_OFFSET)) | 0x1);

	/* set DQS pull ups */
	reg32_write(dram_sdqs_pad_ctls[0],
		reg32_read(dram_sdqs_pad_ctls[0]) | 0x7000);
	reg32_write(dram_sdqs_pad_ctls[1],
		reg32_read(dram_sdqs_pad_ctls[1]) | 0x7000);
	reg32_write(dram_sdqs_pad_ctls[2],
		reg32_read(dram_sdqs_pad_ctls[2]) | 0x7000);
	reg32_write(dram_sdqs_pad_ctls[3],
		reg32_read(dram_sdqs_pad_ctls[3]) | 0x7000);
	reg32_write(dram_sdqs_pad_ctls[4],
		reg32_read(dram_sdqs_pad_ctls[4]) | 0x7000);
	reg32_write(dram_sdqs_pad_ctls[5],
		reg32_read(dram_sdqs_pad_ctls[5]) | 0x7000);
	reg32_write(dram_sdqs_pad_ctls[6],
		reg32_read(dram_sdqs_pad_ctls[6]) | 0x7000);
	reg32_write(dram_sdqs_pad_ctls[7],
		reg32_read(dram_sdqs_pad_ctls[7]) | 0x7000);
	/* set RALAT to max */
	temp1 = reg32_read(MX6_MMDC_P0_BASE_ADDR + MDMISC_OFFSET);
		reg32setbit((MX6_MMDC_P0_BASE_ADDR + MDMISC_OFFSET), 6);
	reg32setbit((MX6_MMDC_P0_BASE_ADDR + MDMISC_OFFSET), 7);
	reg32setbit((MX6_MMDC_P0_BASE_ADDR + MDMISC_OFFSET), 8);
	/* set WALAT to max */
	reg32setbit((MX6_MMDC_P0_BASE_ADDR + MDMISC_OFFSET), 16);
	reg32setbit((MX6_MMDC_P0_BASE_ADDR + MDMISC_OFFSET), 17);

	/* disable auto refresh before proceeding with calibration */
	temp_ref = reg32_read(MX6_MMDC_P0_BASE_ADDR + MDREF_OFFSET);
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MDREF_OFFSET), 0x0000C000);
	/*
	 * Per the ref manual, issue one refresh cycle MDSCR[CMD]= 0x2,
	 * this also sets the CON_REQ bit.
	 */
	if (cs0_enable_initial == 1)
		reg32_write((MX6_MMDC_P0_BASE_ADDR + MDSCR_OFFSET), 0x00008020);
	if (cs1_enable_initial == 1)
		reg32_write((MX6_MMDC_P0_BASE_ADDR + MDSCR_OFFSET), 0x00008028);
	/* poll to make sure the con_ack bit was asserted */
	while (!(reg32_read((MX6_MMDC_P0_BASE_ADDR + MDSCR_OFFSET)) & 0x00004000))
		debug(".");
	/*
	 * Check MDMISC register CALIB_PER_CS to see which CS calibration
	 * is targeted to (under normal cases, it should be cleared
	 * as this is the default value, indicating calibration is directed
	 * to CS0).
	 * Disable the other chip select not being target for calibration
	 * to avoid any potential issues.  This will get re-enabled at end
	 * of calibration.
	 */
	if ((reg32_read(MX6_MMDC_P0_BASE_ADDR + MDMISC_OFFSET) & 0x00100000) == 0)
		/* clear SDE_1 */
		reg32clrbit((MX6_MMDC_P0_BASE_ADDR + MDCTL_OFFSET), 30);
	else
		/* clear SDE_0 */
		reg32clrbit((MX6_MMDC_P0_BASE_ADDR + MDCTL_OFFSET), 31);
	/*
	 * Check to see which chip selects are now enabled for
	 * the remainder of the calibration.
	 */
	cs0_enable = (reg32_read(MX6_MMDC_P0_BASE_ADDR + MDCTL_OFFSET)
			& 0x80000000) >> 31;
	cs1_enable = (reg32_read(MX6_MMDC_P0_BASE_ADDR + MDCTL_OFFSET)
			& 0x40000000) >> 30;
	//debug("cal cs0: %d cs1: %d\n", cs0_enable, cs1_enable);
	/* check to see what is the data bus size: */
	data_bus_size = (reg32_read(MX6_MMDC_P0_BASE_ADDR + MDCTL_OFFSET)
			& 0x30000) >> 16;
	debug("db size: %d\n", data_bus_size);
	/*
	 * Issue the Precharge-All command to the DDR device for both
	 * chip selects.  Note, CON_REQ bit should also remain set.
	 * If only using one chip select, then precharge only the desired
	 * chip select.
	 */
	if (cs0_enable == 1)
		/* CS0 */
		reg32_write((MX6_MMDC_P0_BASE_ADDR + MDSCR_OFFSET), 0x04008050);
	if (cs1_enable == 1)
		/* CS1 */
		reg32_write((MX6_MMDC_P0_BASE_ADDR + MDSCR_OFFSET), 0x04008058);

	/* Write the pre-defined value into MPPDCMPR1 */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MPPDCMPR1_OFFSET), PDDWord);
	/*
	 * Issue a write access to the external DDR device by setting
	 * the bit SW_DUMMY_WR (bit 0) in the MPSWDAR0 and then poll
	 * this bit until it clears to indicate completion of the write access.
	 */
	reg32setbit((MX6_MMDC_P0_BASE_ADDR + MPSWDAR_OFFSET), 0);
	while (reg32_read((MX6_MMDC_P0_BASE_ADDR + MPSWDAR_OFFSET)) & 0x00000001)
		debug(".");

	/* Set the RD_DL_ABS_OFFSET# bits to their default values
	 * (will be calibrated later in the read delay-line calibration).
	 * Both PHYs for x64 configuration, if x32, do only PHY0.
	 */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MPRDDLCTL_OFFSET), 0x40404040);
	if (data_bus_size == 0x2)
		reg32_write((MX6_MMDC_P1_BASE_ADDR + MPRDDLCTL_OFFSET), 0x40404040);
	/* Force a measurment, for previous delay setup to take effect. */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MPMUR_OFFSET), 0x800);
	if (data_bus_size == 0x2)
		reg32_write((MX6_MMDC_P1_BASE_ADDR + MPMUR_OFFSET), 0x800);

	/*
	 * @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	 * Read DQS Gating calibration
	 * @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	 */
	debug("Starting DQS gating calibration...\n");
	/*
	 * Reset the read data FIFOs (two resets); only need to issue reset
	 * to PHY0 since in x64 mode, the reset will also go to PHY1.
	 */

	 /* Read data FIFOs reset1.  */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET),
	reg32_read((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET)) | 0x80000000);
	while (reg32_read((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET)) & 0x80000000)
		debug(".");

	/* Read data FIFOs reset2 */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET),
	reg32_read((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET)) | 0x80000000);
	while (reg32_read((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET)) & 0x80000000)
		debug(".");

	/*
	 * Start the automatic read DQS gating calibration process by
	 * asserting MPDGCTRL0[HW_DG_EN] and MPDGCTRL0[DG_CMP_CYC]
	 * and then poll MPDGCTRL0[HW_DG_EN]] until this bit clears
	 * to indicate completion.
	 * Also, ensure that MPDGCTRL0[HW_DG_ERR] is clear to indicate
	 * no errors were seen during calibration.
	 */

	/*
	 * Set bit 30: chooses option to wait 32 cycles instead of
	 * 16 before comparing read data.
	 */
	reg32setbit((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET), 30);
	/* Set bit 28 to start automatic read DQS gating calibration */
	reg32setbit((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET), 28);
	/* Poll for completion.  MPDGCTRL0[HW_DG_EN] should be 0 */
	while (reg32_read((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET))
			& 0x10000000)
		debug( "." );

	/*
	 * Check to see if any errors were encountered during calibration
	 * (check MPDGCTRL0[HW_DG_ERR]).
	 * Check both PHYs for x64 configuration, if x32, check only PHY0.
	 */
	if (data_bus_size == 0x2) {
		if ((reg32_read(MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET)
					& 0x00001000) ||
			(reg32_read(MX6_MMDC_P1_BASE_ADDR + MPDGCTRL0_OFFSET)
					& 0x00001000)) {
			errorcount++;
		}
	} else {
		if (reg32_read(MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET)
				& 0x00001000) {
			errorcount++;
		}
	}
	debug("errorcount: %d\n", errorcount);
	/*
	 * DQS gating absolute offset should be modified from
	 * reflecting (HW_DG_LOWx + HW_DG_UPx)/2 to
	 * reflecting (HW_DG_UPx - 0x80)
	 */
	modify_dg_result(MX6_MMDC_P0_BASE_ADDR + MPDGHWST0_OFFSET,
		MX6_MMDC_P0_BASE_ADDR + MPDGHWST1_OFFSET,
		MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET);
	modify_dg_result(MX6_MMDC_P0_BASE_ADDR + MPDGHWST2_OFFSET,
		MX6_MMDC_P0_BASE_ADDR + MPDGHWST3_OFFSET,
		MX6_MMDC_P0_BASE_ADDR + MPDGCTRL1_OFFSET);
	if (data_bus_size == 0x2) {
		modify_dg_result((MX6_MMDC_P1_BASE_ADDR + MPDGHWST0_OFFSET),
			(MX6_MMDC_P1_BASE_ADDR + MPDGHWST1_OFFSET),
			(MX6_MMDC_P1_BASE_ADDR + MPDGCTRL0_OFFSET));
		modify_dg_result((MX6_MMDC_P1_BASE_ADDR + MPDGHWST2_OFFSET),
			(MX6_MMDC_P1_BASE_ADDR + MPDGHWST3_OFFSET),
			(MX6_MMDC_P1_BASE_ADDR + MPDGCTRL1_OFFSET));
	}
	debug("DQS gating calibration completed, continuing...        \n");



	/*
	 * @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	 * Read delay Calibration
	 * @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	 */
	debug("Starting read calibration...\n");
	/*
	 * Reset the read data FIFOs (two resets); only need to issue reset
	 * to PHY0 since in x64 mode, the reset will also go to PHY1.
	 */

	/* Read data FIFOs reset1 */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET),
		reg32_read((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET))
		| 0x80000000);
	while (reg32_read((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET)) & 0x80000000)
		debug(".");
	
	/* Read data FIFOs reset2 */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET),
	reg32_read((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET)) | 0x80000000);
	while (reg32_read((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET)) & 0x80000000)
		debug(".");
	
	/*
	 * 4. Issue the Precharge-All command to the DDR device for both
	 * chip selects.  If only using one chip select, then precharge
	 * only the desired chip select.
	 */
	if (cs0_enable == 1)
		/* CS0 */
		reg32_write((MX6_MMDC_P0_BASE_ADDR + MDSCR_OFFSET), 0x04008050);
	while (!(reg32_read(MX6_MMDC_P0_BASE_ADDR + MDSCR_OFFSET) & 0x4000))
		debug( "x" );
	if (cs1_enable == 1)
		/* CS1 */
		reg32_write((MX6_MMDC_P0_BASE_ADDR + MDSCR_OFFSET), 0x04008058);
	while (!(reg32_read(MX6_MMDC_P0_BASE_ADDR + MDSCR_OFFSET) & 0x4000))
		debug( "x" );

	/* *********** 5. 6. 7. set the pre-defined word ************ */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MPPDCMPR1_OFFSET), PDDWord);
	/*
	 * Issue a write access to the external DDR device by setting
	 * the bit SW_DUMMY_WR (bit 0) in the MPSWDAR0 and then poll
	 * this bit until it clears to indicate completion of the write access.
	 */
	reg32setbit((MX6_MMDC_P0_BASE_ADDR + MPSWDAR_OFFSET), 0);
	while (reg32_read((MX6_MMDC_P0_BASE_ADDR + MPSWDAR_OFFSET)) & 0x00000001)
		debug( "." );

	/* 8. set initial delays to center up dq in clock */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MPRDDLCTL_OFFSET), initdelay);
	if (data_bus_size == 0x2)
		reg32_write((MX6_MMDC_P1_BASE_ADDR + MPRDDLCTL_OFFSET), initdelay);
	debug("intdel0: %08x / intdel1: %08x\n", 
		reg32_read(MX6_MMDC_P0_BASE_ADDR + MPRDDLCTL_OFFSET),
		reg32_read(MX6_MMDC_P1_BASE_ADDR + MPRDDLCTL_OFFSET));

	/*
	 * 9. Read delay-line calibration
	 * Start the automatic read calibration process by asserting
	 * MPRDDLHWCTL[HW_RD_DL_EN].
	 */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MPRDDLHWCTL_OFFSET), 0x00000030);

	/*
	 * 10. poll for completion
	 * MMDC indicates that the write data calibration had finished by
	 * setting MPRDDLHWCTL[HW_RD_DL_EN] = 0.   Also, ensure that
	 * no error bits were set.
	 */
	while (reg32_read((MX6_MMDC_P0_BASE_ADDR + MPRDDLHWCTL_OFFSET))
			& 0x00000010)
		debug( "." );

	/* check both PHYs for x64 configuration, if x32, check only PHY0 */
	if (data_bus_size == 0x2) {
		if ((reg32_read(MX6_MMDC_P0_BASE_ADDR + MPRDDLHWCTL_OFFSET)
					& 0x0000000f) ||
			(reg32_read(MX6_MMDC_P1_BASE_ADDR + MPRDDLHWCTL_OFFSET)
					& 0x0000000f)) {
			errorcount++;
		}
	} else {
		if (reg32_read(MX6_MMDC_P0_BASE_ADDR + MPRDDLHWCTL_OFFSET)
				& 0x0000000f) {
			errorcount++;
		}
	}
	debug("errorcount: %d\n", errorcount);
	debug("Read calibration completed, continuing...        \n");


	/*
	 * @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	 * Write delay Calibration
	 * @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	 */
	debug("Starting write calibration...\n");
	/*
	 * 3. Reset the read data FIFOs (two resets); only need to issue
	 * reset to PHY0 since in x64 mode, the reset will also go to PHY1.
	 */

	/* read data FIFOs reset1 */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET),
	reg32_read((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET)) | 0x80000000);
	while (reg32_read((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET)) & 0x80000000)
		debug(".");

	/* read data FIFOs reset2 */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET),
	reg32_read((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET)) | 0x80000000);
	while (reg32_read((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET)) & 0x80000000)
		debug(".");

	/*
	 * 4. Issue the Precharge-All command to the DDR device for both
	 * chip selects. If only using one chip select, then precharge
	 * only the desired chip select.
	 */
	if (cs0_enable == 1)
		/* CS0 */
		reg32_write((MX6_MMDC_P0_BASE_ADDR + MDSCR_OFFSET), 0x04008050);
	if (cs1_enable == 1)
		/* CS1 */
		reg32_write((MX6_MMDC_P0_BASE_ADDR + MDSCR_OFFSET), 0x04008058);

	/* *********** 5. 6. 7. set the pre-defined word ************ */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MPPDCMPR1_OFFSET), PDDWord);
	/*
	 * Issue a write access to the external DDR device by setting
	 * the bit SW_DUMMY_WR (bit 0) in the MPSWDAR0 and then poll this bit
	 * until it clears to indicate completion of the write access.
	 */
	reg32setbit((MX6_MMDC_P0_BASE_ADDR + MPSWDAR_OFFSET), 0);
	while (reg32_read((MX6_MMDC_P0_BASE_ADDR + MPSWDAR_OFFSET)) & 0x00000001)
		debug(".");

	/*
	 * 8. Set the WR_DL_ABS_OFFSET# bits to their default values.
	 * Both PHYs for x64 configuration, if x32, do only PHY0.
	 */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MPWRDLCTL_OFFSET), initdelay);
	if (data_bus_size == 0x2)
		reg32_write((MX6_MMDC_P1_BASE_ADDR + MPWRDLCTL_OFFSET), initdelay);
	debug("intdel0: %08x / intdel1: %08x\n", 
		reg32_read(MX6_MMDC_P0_BASE_ADDR + MPWRDLCTL_OFFSET),
		reg32_read(MX6_MMDC_P1_BASE_ADDR + MPWRDLCTL_OFFSET));

	/*
	 * XXX This isn't in the manual. Force a measurment,
	 * for previous delay setup to effect.
	 */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MPMUR_OFFSET), 0x800);
	if (data_bus_size == 0x2)
		reg32_write((MX6_MMDC_P1_BASE_ADDR + MPMUR_OFFSET), 0x800);

	/*
	 * 9. 10. Start the automatic write calibration process
	 * by asserting MPWRDLHWCTL0[HW_WR_DL_EN].
	 */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MPWRDLHWCTL_OFFSET), 0x00000030);

	/*
	 * Poll for completion.
	 * MMDC indicates that the write data calibration had finished
	 * by setting MPWRDLHWCTL[HW_WR_DL_EN] = 0.
	 * Also, ensure that no error bits were set.
	 */
	while (reg32_read((MX6_MMDC_P0_BASE_ADDR + MPWRDLHWCTL_OFFSET))
			& 0x00000010)
		debug(".");
	/* Check both PHYs for x64 configuration, if x32, check only PHY0 */
	if (data_bus_size == 0x2) {
		if ((reg32_read(MX6_MMDC_P0_BASE_ADDR + MPWRDLHWCTL_OFFSET)
					& 0x0000000f) ||
			(reg32_read(MX6_MMDC_P1_BASE_ADDR + MPWRDLHWCTL_OFFSET)
					& 0x0000000f)) {
			errorcount++;
		}
	} else {
		if (reg32_read(MX6_MMDC_P0_BASE_ADDR + MPWRDLHWCTL_OFFSET)
				& 0x0000000f) {
			errorcount++;
		}
	}
	debug("errorcount: %d\n", errorcount);
	debug("Write calibration completed, continuing...        \n");

	/*
	 * Reset the read data FIFOs (two resets); only need to issue
	 * reset to PHY0 since in x64 mode, the reset will also go to PHY1.
	 */

	/* read data FIFOs reset1 */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET),
	reg32_read((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET)) | 0x80000000);
	while (reg32_read((MX6_MMDC_P0_BASE_ADDR + MPDGCTRL0_OFFSET)) & 0x80000000)
		debug(".");

	/* read data FIFOs reset2 */
	reg32_write((MX6_MMDC_P1_BASE_ADDR + MPDGCTRL0_OFFSET),
	reg32_read((MX6_MMDC_P1_BASE_ADDR + MPDGCTRL0_OFFSET)) | 0x80000000);
	while (reg32_read((MX6_MMDC_P1_BASE_ADDR + MPDGCTRL0_OFFSET)) & 0x80000000)
		debug(".");

	/* Enable DDR logic power down timer: */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MDPDC_OFFSET),
	reg32_read((MX6_MMDC_P0_BASE_ADDR + MDPDC_OFFSET)) | 0x00005500);

	/* Enable Adopt power down timer: */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MAPSR_OFFSET),
	reg32_read((MX6_MMDC_P0_BASE_ADDR + MAPSR_OFFSET)) & 0xfffffff7);

	/* Restore MDMISC value (RALAT, WALAT) */
	reg32_write((MX6_MMDC_P1_BASE_ADDR + MDMISC_OFFSET), temp1);

	/* Clear DQS pull ups */
	reg32_write(dram_sdqs_pad_ctls[0],
		reg32_read(dram_sdqs_pad_ctls[0]) & 0xffff0fff);
	reg32_write(dram_sdqs_pad_ctls[1],
		reg32_read(dram_sdqs_pad_ctls[1]) & 0xffff0fff);
	reg32_write(dram_sdqs_pad_ctls[2],
		reg32_read(dram_sdqs_pad_ctls[2]) & 0xffff0fff);
	reg32_write(dram_sdqs_pad_ctls[3],
		reg32_read(dram_sdqs_pad_ctls[3]) & 0xffff0fff);
	reg32_write(dram_sdqs_pad_ctls[4],
		reg32_read(dram_sdqs_pad_ctls[4]) & 0xffff0fff);
	reg32_write(dram_sdqs_pad_ctls[5],
		reg32_read(dram_sdqs_pad_ctls[5]) & 0xffff0fff);
	reg32_write(dram_sdqs_pad_ctls[6],
		reg32_read(dram_sdqs_pad_ctls[6]) & 0xffff0fff);
	reg32_write(dram_sdqs_pad_ctls[7],
		reg32_read(dram_sdqs_pad_ctls[7]) & 0xffff0fff);

	/* Re-enable SDE (chip selects) if they were set initially */
	if (cs1_enable_initial == 1)
		/* Set SDE_1 */
		reg32setbit((MX6_MMDC_P0_BASE_ADDR + MDCTL_OFFSET), 30);

	if (cs0_enable_initial == 1)
		/* Set SDE_0 */
		reg32setbit((MX6_MMDC_P0_BASE_ADDR + MDCTL_OFFSET), 31);

	/* Re-enable to auto refresh */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MDREF_OFFSET), temp_ref);

	/* Clear the MDSCR (including the con_req bit) */
	reg32_write((MX6_MMDC_P0_BASE_ADDR + MDSCR_OFFSET), 0x0); // CS0

	/* Poll to make sure the con_ack bit is clear */
	while ((reg32_read(MX6_MMDC_P0_BASE_ADDR + MDSCR_OFFSET) & 0x00004000))
		debug(".");

	/*
	 * Print out the registers that were updated as a result
	 * of the calibration process.
	 */
	debug("MMDC registers updated from calibration \n");
	debug("\nRead DQS Gating calibration\n");
	debug("MPDGCTRL0 PHY0 (0x021b083c) = 0x%08X\n",
			reg32_read(MPDGCTRL0_PHY0));
	debug("MPDGCTRL1 PHY0 (0x021b0840) = 0x%08X\n",
			reg32_read(MPDGCTRL1_PHY0));
	debug("MPDGCTRL0 PHY1 (0x021b483c) = 0x%08X\n",
			reg32_read(MPDGCTRL0_PHY1));
	debug("MPDGCTRL1 PHY1 (0x021b4840) = 0x%08X\n",
			reg32_read(MPDGCTRL1_PHY1));
	debug("\nRead calibration\n");
	debug("MPRDDLCTL PHY0 (0x021b0848) = 0x%08X\n",
			reg32_read(MPRDDLCTL_PHY0));
	debug("MPRDDLCTL PHY1 (0x021b4848) = 0x%08X\n",
			reg32_read(MPRDDLCTL_PHY1));
	debug("\nWrite calibration\n");
	debug("MPWRDLCTL PHY0 (0x021b0850) = 0x%08X\n",
			reg32_read(MPWRDLCTL_PHY0));
	debug("MPWRDLCTL PHY1 (0x021b4850) = 0x%08X\n",
			reg32_read(MPWRDLCTL_PHY1));
	debug("\n");
	/*
	 * Registers below are for debugging purposes.  These print out
	 * the upper and lower boundaries captured during
	 * read DQS gating calibration.
	 */
	debug("Status registers, upper and lower bounds, "
			"for read DQS gating.\n");
	debug("MPDGHWST0 PHY0 (0x021b087c) = 0x%08X\n", reg32_read(0x021b087c));
	debug("MPDGHWST1 PHY0 (0x021b0880) = 0x%08X\n", reg32_read(0x021b0880));
	debug("MPDGHWST2 PHY0 (0x021b0884) = 0x%08X\n", reg32_read(0x021b0884));
	debug("MPDGHWST3 PHY0 (0x021b0888) = 0x%08X\n", reg32_read(0x021b0888));
	debug("MPDGHWST0 PHY1 (0x021b487c) = 0x%08X\n", reg32_read(0x021b487c));
	debug("MPDGHWST1 PHY1 (0x021b4880) = 0x%08X\n", reg32_read(0x021b4880));
	debug("MPDGHWST2 PHY1 (0x021b4884) = 0x%08X\n", reg32_read(0x021b4884));
	debug("MPDGHWST3 PHY1 (0x021b4888) = 0x%08X\n", reg32_read(0x021b4888));

	debug("errorcount: %d\n", errorcount);

	return 0;
}


/* init ddr3, do calibrations */
static uint32_t imx6qdl_ddr_init(uint32_t *dram_sdqs_pad_ctls)
{
	uint8_t spd[256];
	uint8_t	chip;
	uint	addr, alen, length;
	int	j, nbytes, linebytes;
	struct ddr_spd ddrSPD;
	unsigned short cl_support = 0;
	unsigned int cfgval = 0;
	int errorcount = 0;
	int  i;
	uint32_t ram_size;

	debug("\nSPD dump:\n");

	chip   = 0x50;	/* Address of the SPD I2C device */
	addr   = 0x0;	/* Start of SPD address */
	alen   = 0x1;	/* Address is one-byte long */
	length = 0x100; /* length to display */

	putc_ll('I');
	fsl_i2c_init((void *)MX6_I2C1_BASE_ADDR, I2C_SPEED);
	putc_ll('2');

	nbytes = length;
	i = 0;
	do {
		unsigned char	linebuf[DISP_LINE_LEN];
		unsigned char	*cp;
		int ret;

		linebytes = (nbytes > DISP_LINE_LEN) ? DISP_LINE_LEN : nbytes;

		putc_ll('C');
		print_i2c_status();
		ret = fsl_i2c_read((void *)MX6_I2C1_BASE_ADDR, chip, addr,
				alen, linebuf, linebytes);
		if (ret != 0) {
			printf("Error reading SPD on DDR3.\n");
			dram_fatal(-ret);
		}
#ifdef DEBUG
		else {
			debug("%04x:", addr);
			cp = linebuf;
			for (j=0; j<linebytes; j++) {
				spd[i++] = *cp;
				debug(" %02x", *cp++);
				addr++;
			}
			debug("    ");
			cp = linebuf;
			for (j=0; j<linebytes; j++) {
				if ((*cp < 0x20) || (*cp > 0x7e))
					debug(".");
				else
					debug("%c", *cp);
				cp++;
			}
			debug("\n");
		}
#else
		cp = linebuf;
		for (j=0; j<linebytes; j++) {
			spd[i++] = *cp++;
			addr++;
		}
#endif
		nbytes -= linebytes;
	} while (nbytes > 0);
	fsl_i2c_stop((void *)MX6_I2C1_BASE_ADDR);

	debug("\nRaw DDR3 characteristics based on SPD:\n");
	if ((spd[SPD_DRAM_DEVICE_TYPE] != 0xB) || (spd[SPD_MODULE_TYPE] != 3)) {
		printf("Unrecognized DIMM type installed: %d / %d\n",
				spd[SPD_DRAM_DEVICE_TYPE],
				spd[SPD_MODULE_TYPE]);
		dram_fatal(0x01);
	}

	if ((spd[SPD_SDRAM_DENSITY_AND_BANKS] & 0x30) != 0)
		debug("  Warning: memory has an unsupported bank size\n");
	else
		debug("  8 banks\n");
	ddrSPD.banks = 8;

	ddrSPD.density = 256 * (1 << (spd[SPD_SDRAM_DENSITY_AND_BANKS] & 0xF));
	debug("  Individual chip density is %d Mib\n", ddrSPD.density);

	ddrSPD.rows = ((spd[SPD_SDRAM_ADDRESSING] & 0x38) >> 3) + 12;
	ddrSPD.cols = (spd[SPD_SDRAM_ADDRESSING] & 0x7) + 9;
	debug("  Rows: %d, Cols: %d\n", ddrSPD.rows, ddrSPD.cols);

	if (spd[SPD_NOMINAL_VOLTAGE] & 0x1) {
		printf("Module not operable at 1.5V, fatal error.\n");
		dram_fatal(0x02);
	} else
		debug("  Supports 1.5V operation.\n");

	ddrSPD.rank = ((spd[SPD_ORGANIZATION] >> 3) & 0x7) + 1;
	debug("  Module has %d rank(s)\n", ddrSPD.rank);

	ddrSPD.devwidth = (1 << (spd[SPD_ORGANIZATION] & 0x7)) * 4;
	debug("  Chips have a width of %d bits\n", ddrSPD.devwidth);

	if (spd[SPD_BUS_WIDTH] != 0x3) {
		printf("Unsupported device width, fatal.\n");
		dram_fatal(0x03);
	} else
		debug("  Module width is 64 bits, no ECC\n");

	ddrSPD.capacity = (64 / ddrSPD.devwidth) * ddrSPD.rank * ddrSPD.density;
	debug("  Module capacity is %d GiB\n", ddrSPD.capacity / 8192);

	if ((spd[SPD_MTB_DIVIDEND] != 1) || (spd[SPD_MTB_DIVISOR] != 8)) {
		printf( "Module has non-standard MTB for timing calculation. "
			"Doesn't mean the module is bad, just means this "
			"bootloader can't derive timing information based "
			"on the units coded in the SPD. This is, "
			"unfortunately, a fatal error. File a bug to "
			"get it fixed.\n");
		dram_fatal(0x04);
	}

	switch (spd[SPD_TCKMIN]) {
	case 0x14:
		ddrSPD.clockrate = 400;
		break;
	case 0x0F:
		ddrSPD.clockrate = 533;
		break;
	case 0x0C:
		ddrSPD.clockrate = 667;
		break;
	case 0x0A:
		ddrSPD.clockrate = 800;
		break;
	case 0x09:
		ddrSPD.clockrate = 933;
		break;
	case 0x08:
		ddrSPD.clockrate = 1067;
		break;
	default:
		if (spd[SPD_TCKMIN] <= 0xF) {
			debug("**undecodable but sufficiently fast "
					"clock rate detected\n");
			ddrSPD.clockrate = 533;
		} else {
			debug("**undecodable but too slow "
					"clock rate detected\n");
			ddrSPD.clockrate = 400;
		}
		break;
	}
	debug("  DDR3-%d speed rating detected\n", ddrSPD.clockrate * 2);
	if (ddrSPD.clockrate < 533) {
		printf("memory is too slow.\n");
		dram_fatal(0x05);
	}

	/*
	 * cl_support is a bit vector with bit 0 set <-> CL=4,
	 * bit 1 set <-> CL=5, etc.  These are just supported rates,
	 * not the actual rate computed.
	 */
	cl_support = (spd[SPD_CL_MSB] << 8) | spd[SPD_CL_LSB];

	ddrSPD.caslatency = mtb_to_cycles((unsigned int) spd[SPD_TAAMIN]);

	while ( !((1 << (ddrSPD.caslatency - 4)) & cl_support) ) {
		if (ddrSPD.caslatency > 18) {
			printf("no module-supported CAS latencies found\n");
			dram_fatal(0x06);
		}
		ddrSPD.caslatency++;
	}

	if (ddrSPD.caslatency > 11) {
		printf("cas latency larger than supported by i.MX6\n");
		dram_fatal(0x07);
	}
	if (ddrSPD.caslatency < 3) {
		printf("cas latency shorter than supported by i.MX6\n");
		dram_fatal(0x08);
	}
	debug("Derived optimal timing parameters, in 533MHz cycles:\n");
	debug("  CAS latency: %d\n", ddrSPD.caslatency);

	ddrSPD.tWRmin = mtb_to_cycles((unsigned int) spd[SPD_TWRMIN]);
	if (ddrSPD.tWRmin > 8) {
		debug( "  optimal tWRmin greater than supported by i.MX6, "
				"value saturated.\n");
		ddrSPD.tWRmin = 8;
	}
	debug( "  tWRmin: %d\n", ddrSPD.tWRmin );

	ddrSPD.tRCDmin = mtb_to_cycles((unsigned int) spd[SPD_TRCDMIN]);
	if (ddrSPD.tRCDmin > 8) {
		debug("  optimal tRCDmin greater than supported by i.MX6, "
				"value saturated.\n");
		ddrSPD.tRCDmin = 8;
	}
	debug("  tRCDmin: %d\n", ddrSPD.tRCDmin);

	ddrSPD.tRRDmin = mtb_to_cycles((unsigned int) spd[SPD_TRRDMIN]);
	if (ddrSPD.tRRDmin > 0x8) {
		debug("  optimal tRRDmin greater than supported by i.MX6, "
				"value saturated.\n" );
		ddrSPD.tRRDmin = 0x8;
	}
	debug("  tRRDmin: %d\n", ddrSPD.tRRDmin);

	ddrSPD.tRPmin = mtb_to_cycles((unsigned int) spd[SPD_TRPMIN]);
	debug("  tRPmin: %d\n", ddrSPD.tRPmin);
	if (ddrSPD.tRPmin > 8) {
		debug("  optimal tRPmin greater than supported by i.MX6, "
				"value saturated.\n");
		ddrSPD.tRPmin = 8;
	}

	ddrSPD.tRAS = mtb_to_cycles((unsigned int) spd[SPD_TRAS_LSB] | 
			(((unsigned int) spd[SPD_TRAS_TRC_MSB] & 0xF) << 8));
	if (ddrSPD.tRAS > 0x20) {
		debug("  optimal tRAS greater than supported by i.MX6, "
				"value saturated.\n");
		ddrSPD.tRAS = 0x20;
	}
	debug("  tRAS: %d\n", ddrSPD.tRAS);

	ddrSPD.tRC = mtb_to_cycles((unsigned int) spd[SPD_TRC_LSB] |
			(((unsigned int) spd[SPD_TRAS_TRC_MSB] & 0xF0) << 4));
	if (ddrSPD.tRC > 0x20) {
		debug("  optimal tRC greater than supported by i.MX6, "
				"value saturated.\n");
		ddrSPD.tRC = 0x20;
	}
	debug("  tRC: %d\n", ddrSPD.tRC);

	ddrSPD.tRFCmin = mtb_to_cycles((unsigned int) spd[SPD_TRFC_LSB] | 
				((unsigned int) spd[SPD_TRFC_MSB]) << 8 );
	if (ddrSPD.tRFCmin > 0x100) {
		ddrSPD.tRFCmin = 0x100;
		debug("  Info: derived tRFCmin exceeded max allowed value "
				"by i.MX6\n");
	}
	debug("  tRFCmin: %d\n", ddrSPD.tRFCmin);

	ddrSPD.tWTRmin = mtb_to_cycles((unsigned int) spd[SPD_WTRMIN]);
	if (ddrSPD.tWTRmin > 0x8) {
		debug("  optimal tWTRmin greater than supported by i.MX6, "
				"value saturated.\n");
		ddrSPD.tWTRmin = 0x8;
	}
	debug("  tWTRmin: %d\n", ddrSPD.tWTRmin);

	ddrSPD.tRTPmin = mtb_to_cycles((unsigned int) spd[SPD_RTPMIN]);
	if (ddrSPD.tRTPmin > 0x8) {
		debug("  optimal tRTPmin greater than supported by i.MX6, "
				"value saturated.\n");
		ddrSPD.tRTPmin = 0x8;
	}
	debug("  tRTPmin: %d\n", ddrSPD.tRTPmin);

	ddrSPD.tFAW = mtb_to_cycles((unsigned int) spd[SPD_TFAW_LSB] |
				((unsigned int) spd[SPD_TFAW_MSB]) << 8);
	if (ddrSPD.tFAW > 0x20) {
		ddrSPD.tFAW = 0x20;
		debug("  Info: derived tFAW exceeded max allowed value "
				"by i.MX6\n");
	}
	debug("  tFAW: %d\n", ddrSPD.tFAW);

	if (spd[SPD_THERMAL] & 0x80)
		debug("Info: thermal sensor exists on this module\n");
	else
		debug("Info: no thermal sensor on-module\n");

	ddrSPD.vendorID = spd[SPD_VENDOR_ID_LSB] 
			| (spd[SPD_VENDOR_ID_MSB] << 8);
	debug("Vendor ID: 0x%04x\n", ddrSPD.vendorID);

	for (i = 0; i < 18; i++)
		ddrSPD.name[i] = spd[SPD_NAME + i];
	ddrSPD.name[i] = '\0';
	debug("Module name: %s\n", ddrSPD.name);

	debug("\nReprogramming DDR timings...\n" );

	cfgval = reg32_read(MX6_MMDC_P0_BASE_ADDR + MDCTL_OFFSET);
	debug("Original CTL: %08x\n", cfgval);
	cfgval = 0x80000000;
	if(ddrSPD.rank == 2) 
		cfgval |= 0x40000000;
	cfgval |= (ddrSPD.rows - 11) << 24;
	cfgval |= (ddrSPD.cols - 9) << 20;
	cfgval |= 1 << 19; /* burst length = 8 */
	cfgval |= 2 << 16; /* data size is 64 bits */
	debug("Optimal CTL: %08x\n", cfgval);
	reg32_write(MX6_MMDC_P0_BASE_ADDR + MDCTL_OFFSET, cfgval);

	cfgval = reg32_read(MX6_MMDC_P0_BASE_ADDR + MDASP_OFFSET);
	debug("Original ASP: %08x\n", cfgval);
	cfgval = (ddrSPD.capacity / (256 * ddrSPD.rank)) - 1;
	debug("Optimal ASP: %08x\n", cfgval);
	reg32_write(MX6_MMDC_P0_BASE_ADDR + MDASP_OFFSET, cfgval );

	cfgval = reg32_read(MX6_MMDC_P0_BASE_ADDR + MDCFG0_OFFSET);
	debug("Original CFG0: %08x\n", cfgval);
	cfgval &= 0x00FFFE00;
	cfgval |= ((ddrSPD.tRFCmin - 1) << 24);
	cfgval |= ((ddrSPD.tFAW - 1) & 0x1F << 4);
	cfgval |= ddrSPD.caslatency - 3;
	debug("Optimal CFG0: %08x\n", cfgval);
	reg32_write(MX6_MMDC_P0_BASE_ADDR + MDCFG0_OFFSET,  cfgval );

	cfgval = reg32_read(MX6_MMDC_P0_BASE_ADDR + MDCFG1_OFFSET);
	debug("Original CFG1: %08x\n", cfgval);
	cfgval &= 0x000081FF;
	cfgval |= ((ddrSPD.tRCDmin - 1) << 29);
	cfgval |= ((ddrSPD.tRPmin - 1) << 26);
	cfgval |= ((ddrSPD.tRC - 1) << 21);
	cfgval |= ((ddrSPD.tRAS -1) << 16);
	cfgval |= ((ddrSPD.tWRmin -1) << 9);
	if ((cfgval & 0x7) + 2 < ddrSPD.caslatency) {
		debug( "Original CFG1 tCWL shorter "
				"than supported cas latency, fixing...\n");
		cfgval &= 0xFFFFFFF8;
		if( ddrSPD.caslatency > 7 )
			cfgval |= 0x6;
		else
			cfgval |= (ddrSPD.caslatency - 2);
	}
	debug("Optimal CFG1: %08x\n", cfgval);
	reg32_write(MX6_MMDC_P0_BASE_ADDR + MDCFG1_OFFSET,  cfgval);

	cfgval = reg32_read(MX6_MMDC_P0_BASE_ADDR + MDCFG2_OFFSET);
	debug("Original CFG2: %08x\n", cfgval);
	cfgval &= 0xFFFF0000;
	cfgval |= ((ddrSPD.tRTPmin - 1) << 6);
	cfgval |= ((ddrSPD.tWTRmin - 1) << 3);
	cfgval |= ((ddrSPD.tRRDmin - 1) << 0);
	debug("Optimal CFG2: %08x\n", cfgval);
	reg32_write(MX6_MMDC_P0_BASE_ADDR + MDCFG2_OFFSET, cfgval);

	/*
	 * Write and read back some dummy data to demonstrate 
	 * that ddr3 is not broken
	 */
	debug("\nReference read/write test prior to tuning\n");
	do_tune_mww(1, NULL);
	do_tune_mrr(1, NULL);

	/* do write (fly-by) calibration */
	debug("\nFly-by calibration\n");
	errorcount = do_tune_wcal(1, NULL);
	early_udelay(100000);
	/* let it settle in...seems it's necessary */
	if (errorcount != 0) {
		debug("Fly-by calibration seems to have failed. "
			"Guessing values for wcal based on rank...\n");
		if (ddrSPD.rank == 1) {
			/* Parameters for boards built at King Credie */
			reg32_write(MX6_MMDC_P0_BASE_ADDR + MPWLDECTRL0_OFFSET,
					0x00390042);
			reg32_write(MX6_MMDC_P0_BASE_ADDR + MPWLDECTRL1_OFFSET,
					0x00650057);
			reg32_write(MX6_MMDC_P1_BASE_ADDR + MPWLDECTRL0_OFFSET,
					0x00630106);
			reg32_write(MX6_MMDC_P1_BASE_ADDR + MPWLDECTRL1_OFFSET,
					0x01060116);
		} else {
			/* Parameters for boards built at King Credie */
			reg32_write(MX6_MMDC_P0_BASE_ADDR + MPWLDECTRL0_OFFSET,
					0x00290039);
			reg32_write(MX6_MMDC_P0_BASE_ADDR + MPWLDECTRL1_OFFSET,
					0x00160057);
			reg32_write(MX6_MMDC_P1_BASE_ADDR + MPWLDECTRL0_OFFSET,
					0x00640158);
			reg32_write(MX6_MMDC_P1_BASE_ADDR + MPWLDECTRL1_OFFSET,
					0x0111012C);
		}
	}

	/* Tune DQS delays. For some reason, has to be run twice. */
	debug("\nDQS delay calibration\n");
	do_tune_delays(dram_sdqs_pad_ctls);
	errorcount = do_tune_delays(dram_sdqs_pad_ctls);
	if (errorcount != 0) {
		debug("DQS delay calibration has failed. Guessing values "
				"for delay cal based on rank...\n");
		if (ddrSPD.rank == 1) {
			/* Parameters for boards built at King Credie */
			reg32_write(MPDGCTRL0_PHY0, 0x456B057A);
			reg32_write(MPDGCTRL1_PHY0, 0x057F0607);
			reg32_write(MPDGCTRL0_PHY1, 0x46470645);
			reg32_write(MPDGCTRL1_PHY1, 0x0651061D);
			reg32_write(MPRDDLCTL_PHY0, 0x48444249);
			reg32_write(MPRDDLCTL_PHY1, 0x4D4D424E);
			reg32_write(MPWRDLCTL_PHY0, 0x322D4132);
			reg32_write(MPWRDLCTL_PHY1, 0x3D2E3F37);
		} else {
			/* Parameters for boards built at King Credie */
			reg32_write(MPDGCTRL0_PHY0, 0x4604061F);
			reg32_write(MPDGCTRL1_PHY0, 0x0555062B);
			reg32_write(MPDGCTRL0_PHY1, 0x4672073F);
			reg32_write(MPDGCTRL1_PHY1, 0x07010665);
			reg32_write(MPRDDLCTL_PHY0, 0x4B3F4145);
			reg32_write(MPRDDLCTL_PHY1, 0x48423F47);
			reg32_write(MPWRDLCTL_PHY0, 0x39354132);
			reg32_write(MPWRDLCTL_PHY1, 0x3C323840);
		}
	}

	/* 
	 * Confirm that the memory is working by read/write demo.
	 * Confirmation currently read out on terminal.
	 */
	debug("\nReference read/write test post-tuning\n");
	do_tune_mww(1, NULL);
	do_tune_mrr(1, NULL);

	ram_size = ((ddrSPD.capacity / 8) - 256) * 1024 * 1024;
	debug("ddrSPD.capacity: %08x\n", ddrSPD.capacity);
	debug("Ramsize according to SPD: %08x\n", ram_size);

	return ram_size;
}

uint32_t imx6q_ddr_init(void)
{
	/* IOMUXC_SW_PAD_CTL_PAD_EIM_D21 */
	writel(
		MX6_PAD_CTL_PKE | MX6_PAD_CTL_PUE | MX6_PAD_CTL_PUS_100K_UP 
	      | MX6_PAD_CTL_SPEED_LOW | MX6_PAD_CTL_DSE_240ohm
	      | MX6_PAD_CTL_HYS | MX6_PAD_CTL_ODE, 0x020e03b8);

	/* IOMUXC_SW_PAD_CTL_PAD_EIM_D28 */
	writel(
		MX6_PAD_CTL_PKE | MX6_PAD_CTL_PUE | MX6_PAD_CTL_PUS_100K_UP 
	      | MX6_PAD_CTL_SPEED_LOW | MX6_PAD_CTL_DSE_240ohm
	      | MX6_PAD_CTL_HYS | MX6_PAD_CTL_ODE, 0x020e03d8);

	/* IOMUXC_SW_MUX_CTL_PAD_EIM_D21 */
	writel(0x6, 0x020e00a4); /* SCLK */

	/* IOMUXC_SW_MUX_CTL_PAD_EIM_D28 */
	writel(0x1, 0x020e00c4); /* SDA */

	/* IOMUXC_I2C1_IPP_SCL_IN_SELECT_INPUT */
	writel(0, 0x020e0898);

	/* IOMUXC_I2C1_IPP_SDA_IN_SELECT_INUPT */
	writel(0, 0x020e089c);

	return imx6qdl_ddr_init(mx6q_sw_pad_ctl_pad_dram_sdqs);
}

uint32_t imx6dl_ddr_init(void)
{
	return imx6qdl_ddr_init(mx6dl_sw_pad_ctl_pad_dram_sdqs);
}
