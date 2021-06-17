/*
 * sunxi gorilla DE/TCON driver (For AW1855)
 *
 * Copyright (c) 2020, Martin <wuyan@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */
#define DEBUG	 /* Enable dev_dbg */
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include "gorilla-common.h"

/* Use TCON as hardware interrupt source. See AW1855_TCON_LCD_Spec_V0.1 */
#define LCD_GINT0_REG				(0x4)    /* LCD global interrupt register 0 */
#define LCD_VB_INT_EN				BIT(31)  /* Enable the VB interrupt */
#define LCD_VB_INT_FLAG				BIT(15)  /* Asserted during vertical no-display period every frame. Write 0 to clear it */

struct gorilla_de_tcon {
	struct device *dev;
	void __iomem *base;
	int irq;
};

static int de_tcon_init(struct gorilla_de_tcon *chip)
{
	// config DE
	// DE inner
	int data = 0x5;
	write_reg_by_address(0x06000000, data);
	write_reg_by_address(0x06000004, data);
	write_reg_by_address(0x06000008, data);
	//init_mem(sdram06.hex);// baseAddr, hexFileName, wrapSize, wrapEnable
	write_reg_by_address(0x06010000, 0x20000000); // Clock gate of scaler and DMA module

	write_reg_by_address(0x06100000, 0x00000000);
	write_reg_by_address(0x06101000, 0x00000f00);
	write_reg_by_address(0x06101004, 0x00000000);
	write_reg_by_address(0x06101008, 0x031e04ff);
	write_reg_by_address(0x0610100c, 0x00000000);
	write_reg_by_address(0x06101010, 0x00000000);
	write_reg_by_address(0x06101014, 0x00000000);
	write_reg_by_address(0x06101018, 0x031504ff);
	write_reg_by_address(0x0610101c, 0x00000000);
	write_reg_by_address(0x06101020, 0x00000000);
	write_reg_by_address(0x06101024, 0x00000000);
	write_reg_by_address(0x06101028, 0x031f04ff);
	write_reg_by_address(0x0610102c, 0x00000000);
	write_reg_by_address(0x06101030, 0x00000000);
	write_reg_by_address(0x06101034, 0x00000000);
	write_reg_by_address(0x06101038, 0x031f04ff);
	write_reg_by_address(0x0610103c, 0x00000000);
	write_reg_by_address(0x06101040, 0x00000000);
	write_reg_by_address(0x06101044, 0x00000000);
	write_reg_by_address(0x06101048, 0x00000000);
	write_reg_by_address(0x0610104c, 0x00000000);
	write_reg_by_address(0x06101050, 0x00000000);
	write_reg_by_address(0x06101054, 0x00000000);
	write_reg_by_address(0x06101058, 0x00000000);
	write_reg_by_address(0x0610105c, 0x00000000);
	write_reg_by_address(0x06101060, 0x00000000);
	write_reg_by_address(0x06101064, 0x00000000);
	write_reg_by_address(0x06101068, 0x00000000);
	write_reg_by_address(0x0610106c, 0x00000000);
	write_reg_by_address(0x06101070, 0x00000000);
	write_reg_by_address(0x06101074, 0x00000000);
	write_reg_by_address(0x06101078, 0x00000000);
	write_reg_by_address(0x0610107c, 0x00000000);
	write_reg_by_address(0x06101080, 0x00043210);
	write_reg_by_address(0x06101084, 0x00000000);
	write_reg_by_address(0x06101088, 0xff123456);
	write_reg_by_address(0x0610108c, 0x031f04ff);
	write_reg_by_address(0x06101090, 0x03010301);
	write_reg_by_address(0x06101094, 0x03010301);
	write_reg_by_address(0x06101098, 0x03010301);
	write_reg_by_address(0x0610109c, 0x03010301);
	write_reg_by_address(0x061010a0, 0x00000000);
	write_reg_by_address(0x061010a4, 0x00000000);
	write_reg_by_address(0x061010a8, 0x00000000);
	write_reg_by_address(0x061010ac, 0x00000000);
	write_reg_by_address(0x061010b0, 0x00000000);
	write_reg_by_address(0x061010b4, 0x00000000);
	write_reg_by_address(0x061010b8, 0x00000000);
	write_reg_by_address(0x061010bc, 0x00000000);
	write_reg_by_address(0x061010c0, 0x00000000);
	write_reg_by_address(0x061010c4, 0x00000000);
	write_reg_by_address(0x061010c8, 0x00000000);
	write_reg_by_address(0x061010cc, 0x00000000);
	write_reg_by_address(0x061010d0, 0x00000000);
	write_reg_by_address(0x061010d4, 0x00000000);
	write_reg_by_address(0x061010d8, 0x00000000);
	write_reg_by_address(0x061010dc, 0x00000000);
	write_reg_by_address(0x061010e0, 0x00000000);
	write_reg_by_address(0x061010e4, 0x00000000);
	write_reg_by_address(0x061010e8, 0x00000000);
	write_reg_by_address(0x061010ec, 0x00000000);
	write_reg_by_address(0x061010f0, 0x00000000);
	write_reg_by_address(0x061010f4, 0x00000000);
	write_reg_by_address(0x061010f8, 0x00000000);
	write_reg_by_address(0x061010fc, 0x00000000);
	write_reg_by_address(0x06102000, 0x00008001);
	write_reg_by_address(0x06102004, 0x031f04ff);
	write_reg_by_address(0x06102008, 0x00000000);
	write_reg_by_address(0x0610200c, 0x00001400);
	write_reg_by_address(0x06102010, 0x00000500);
	write_reg_by_address(0x06102014, 0x00000500);
	write_reg_by_address(0x06102018, 0x64000000);
	write_reg_by_address(0x0610201c, 0x00000000);
	write_reg_by_address(0x06102020, 0x00000000);
	write_reg_by_address(0x06102024, 0x00000000);
	write_reg_by_address(0x06102028, 0x00000000);
	write_reg_by_address(0x0610202c, 0x00000000);
	write_reg_by_address(0x06102030, 0x00008001);
	write_reg_by_address(0x06102034, 0x00c7018f);
	write_reg_by_address(0x06102038, 0x0006000c);
	write_reg_by_address(0x0610203c, 0x00000780);
	write_reg_by_address(0x06102040, 0x000001e0);
	write_reg_by_address(0x06102044, 0x000001e0);
	write_reg_by_address(0x06102048, 0x64803c18);
	write_reg_by_address(0x0610204c, 0x00000000);
	write_reg_by_address(0x06102050, 0x00000000);
	write_reg_by_address(0x06102054, 0x00000000);
	write_reg_by_address(0x06102058, 0x00000000);
	write_reg_by_address(0x0610205c, 0x00000000);
	write_reg_by_address(0x06102060, 0x00000000);
	write_reg_by_address(0x06102064, 0x00000000);
	write_reg_by_address(0x06102068, 0x00000000);
	write_reg_by_address(0x0610206c, 0x00000000);
	write_reg_by_address(0x06102070, 0x00000000);
	write_reg_by_address(0x06102074, 0x00000000);
	write_reg_by_address(0x06102078, 0x00000000);
	write_reg_by_address(0x0610207c, 0x00000000);
	write_reg_by_address(0x06102080, 0x00000000);
	write_reg_by_address(0x06102084, 0x00000000);
	write_reg_by_address(0x06102088, 0x00000000);
	write_reg_by_address(0x0610208c, 0x00000000);
	write_reg_by_address(0x06102090, 0x00000000);
	write_reg_by_address(0x06102094, 0x00000000);
	write_reg_by_address(0x06102098, 0x00000000);
	write_reg_by_address(0x0610209c, 0x00000000);
	write_reg_by_address(0x061020a0, 0x00000000);
	write_reg_by_address(0x061020a4, 0x00000000);
	write_reg_by_address(0x061020a8, 0x00000000);
	write_reg_by_address(0x061020ac, 0x00000000);
	write_reg_by_address(0x061020b0, 0x00000000);
	write_reg_by_address(0x061020b4, 0x00000000);
	write_reg_by_address(0x061020b8, 0x00000000);
	write_reg_by_address(0x061020bc, 0x00000000);
	write_reg_by_address(0x061020c0, 0x00000000);
	write_reg_by_address(0x061020c4, 0x00000000);
	write_reg_by_address(0x061020c8, 0x00000000);
	write_reg_by_address(0x061020cc, 0x00000000);
	write_reg_by_address(0x061020d0, 0x00000000);
	write_reg_by_address(0x061020d4, 0x00000000);
	write_reg_by_address(0x061020d8, 0x00000000);
	write_reg_by_address(0x061020dc, 0x00000000);
	write_reg_by_address(0x061020e0, 0x00000000);
	write_reg_by_address(0x061020e4, 0x00000000);
	write_reg_by_address(0x061020e8, 0x031f04ff);
	write_reg_by_address(0x061020ec, 0x00000000);
	write_reg_by_address(0x061020f0, 0x00000000);
	write_reg_by_address(0x061020f4, 0x00000000);
	write_reg_by_address(0x061020f8, 0x00000000);
	write_reg_by_address(0x061020fc, 0x00000000);
	write_reg_by_address(0x06103000, 0x9c008003);
	write_reg_by_address(0x06103004, 0x031f04ff);
	write_reg_by_address(0x06103008, 0x00000000);
	write_reg_by_address(0x0610300c, 0x00001400);
	write_reg_by_address(0x06103010, 0x00000500);
	write_reg_by_address(0x06103014, 0x00000500);
	write_reg_by_address(0x06103018, 0x65000000);
	write_reg_by_address(0x0610301c, 0x00000000);
	write_reg_by_address(0x06103020, 0x00000000);
	write_reg_by_address(0x06103024, 0x00000000);
	write_reg_by_address(0x06103028, 0x00000000);
	write_reg_by_address(0x0610302c, 0x00000000);
	write_reg_by_address(0x06103030, 0x9c008003);
	write_reg_by_address(0x06103034, 0x00c7018f);
	write_reg_by_address(0x06103038, 0x0006000c);
	write_reg_by_address(0x0610303c, 0x00000780);
	write_reg_by_address(0x06103040, 0x000001e0);
	write_reg_by_address(0x06103044, 0x000001e0);
	write_reg_by_address(0x06103048, 0x65803c18);
	write_reg_by_address(0x0610304c, 0x00000000);
	write_reg_by_address(0x06103050, 0x00000000);
	write_reg_by_address(0x06103054, 0x00000000);
	write_reg_by_address(0x06103058, 0x00000000);
	write_reg_by_address(0x0610305c, 0x00000000);
	write_reg_by_address(0x06103060, 0x00000000);
	write_reg_by_address(0x06103064, 0x00000000);
	write_reg_by_address(0x06103068, 0x00000000);
	write_reg_by_address(0x0610306c, 0x00000000);
	write_reg_by_address(0x06103070, 0x00000000);
	write_reg_by_address(0x06103074, 0x00000000);
	write_reg_by_address(0x06103078, 0x00000000);
	write_reg_by_address(0x0610307c, 0x00000000);
	write_reg_by_address(0x06103080, 0x00000000);
	write_reg_by_address(0x06103084, 0x00000000);
	write_reg_by_address(0x06103088, 0x00000000);
	write_reg_by_address(0x0610308c, 0x00000000);
	write_reg_by_address(0x06103090, 0x00000000);
	write_reg_by_address(0x06103094, 0x00000000);
	write_reg_by_address(0x06103098, 0x00000000);
	write_reg_by_address(0x0610309c, 0x00000000);
	write_reg_by_address(0x061030a0, 0x00000000);
	write_reg_by_address(0x061030a4, 0x00000000);
	write_reg_by_address(0x061030a8, 0x00000000);
	write_reg_by_address(0x061030ac, 0x00000000);
	write_reg_by_address(0x061030b0, 0x00000000);
	write_reg_by_address(0x061030b4, 0x00000000);
	write_reg_by_address(0x061030b8, 0x00000000);
	write_reg_by_address(0x061030bc, 0x00000000);
	write_reg_by_address(0x061030c0, 0x00000000);
	write_reg_by_address(0x061030c4, 0x00000000);
	write_reg_by_address(0x061030c8, 0x00000000);
	write_reg_by_address(0x061030cc, 0x00000000);
	write_reg_by_address(0x061030d0, 0x00000000);
	write_reg_by_address(0x061030d4, 0x00000000);
	write_reg_by_address(0x061030d8, 0x00000000);
	write_reg_by_address(0x061030dc, 0x00000000);
	write_reg_by_address(0x061030e0, 0x00000000);
	write_reg_by_address(0x061030e4, 0x00000000);
	write_reg_by_address(0x061030e8, 0x031f04ff);
	write_reg_by_address(0x061030ec, 0x00000000);
	write_reg_by_address(0x061030f0, 0x00000000);
	write_reg_by_address(0x061030f4, 0x00000000);
	write_reg_by_address(0x061030f8, 0x00000000);
	write_reg_by_address(0x061030fc, 0x00000000);
	write_reg_by_address(0x06104000, 0x98000003);
	write_reg_by_address(0x06104004, 0x031f04fe);
	write_reg_by_address(0x06104008, 0x00000000);
	write_reg_by_address(0x0610400c, 0x00001400);
	write_reg_by_address(0x06104010, 0x66000000);
	write_reg_by_address(0x06104014, 0x00000000);
	write_reg_by_address(0x06104018, 0x00000000);
	write_reg_by_address(0x0610401c, 0x00000000);
	write_reg_by_address(0x06104020, 0x00000000);
	write_reg_by_address(0x06104024, 0x00000000);
	write_reg_by_address(0x06104028, 0x00000000);
	write_reg_by_address(0x0610402c, 0x00000000);
	write_reg_by_address(0x06104030, 0x00000000);
	write_reg_by_address(0x06104034, 0x00000000);
	write_reg_by_address(0x06104038, 0x00000000);
	write_reg_by_address(0x0610403c, 0x00000000);
	write_reg_by_address(0x06104040, 0x00000000);
	write_reg_by_address(0x06104044, 0x00000000);
	write_reg_by_address(0x06104048, 0x00000000);
	write_reg_by_address(0x0610404c, 0x00000000);
	write_reg_by_address(0x06104050, 0x00000000);
	write_reg_by_address(0x06104054, 0x00000000);
	write_reg_by_address(0x06104058, 0x00000000);
	write_reg_by_address(0x0610405c, 0x00000000);
	write_reg_by_address(0x06104060, 0x00000000);
	write_reg_by_address(0x06104064, 0x00000000);
	write_reg_by_address(0x06104068, 0x00000000);
	write_reg_by_address(0x0610406c, 0x00000000);
	write_reg_by_address(0x06104070, 0x00000000);
	write_reg_by_address(0x06104074, 0x00000000);
	write_reg_by_address(0x06104078, 0x00000000);
	write_reg_by_address(0x0610407c, 0x00000000);
	write_reg_by_address(0x06104080, 0x00000000);
	write_reg_by_address(0x06104084, 0x00000000);
	write_reg_by_address(0x06104088, 0x031f04ff);
	write_reg_by_address(0x0610408c, 0x00000000);
	write_reg_by_address(0x06104090, 0x00000000);
	write_reg_by_address(0x06104094, 0x00000000);
	write_reg_by_address(0x06104098, 0x00000000);
	write_reg_by_address(0x0610409c, 0x00000000);
	write_reg_by_address(0x061040a0, 0x00000000);
	write_reg_by_address(0x061040a4, 0x00000000);
	write_reg_by_address(0x061040a8, 0x00000000);
	write_reg_by_address(0x061040ac, 0x00000000);
	write_reg_by_address(0x061040b0, 0x00000000);
	write_reg_by_address(0x061040b4, 0x00000000);
	write_reg_by_address(0x061040b8, 0x00000000);
	write_reg_by_address(0x061040bc, 0x00000000);
	write_reg_by_address(0x061040c0, 0x00000000);
	write_reg_by_address(0x061040c4, 0x00000000);
	write_reg_by_address(0x061040c8, 0x00000000);
	write_reg_by_address(0x061040cc, 0x00000000);
	write_reg_by_address(0x061040d0, 0x00000000);
	write_reg_by_address(0x061040d4, 0x00000000);
	write_reg_by_address(0x061040d8, 0x00000000);
	write_reg_by_address(0x061040dc, 0x00000000);
	write_reg_by_address(0x061040e0, 0x00000000);
	write_reg_by_address(0x061040e4, 0x00000000);
	write_reg_by_address(0x061040e8, 0x00000000);
	write_reg_by_address(0x061040ec, 0x00000000);
	write_reg_by_address(0x061040f0, 0x00000000);
	write_reg_by_address(0x061040f4, 0x00000000);
	write_reg_by_address(0x061040f8, 0x00000000);
	write_reg_by_address(0x061040fc, 0x00000000);
	write_reg_by_address(0x06105000, 0x98000003);
	write_reg_by_address(0x06105004, 0x031f04fe);
	write_reg_by_address(0x06105008, 0x00000000);
	write_reg_by_address(0x0610500c, 0x00001400);
	write_reg_by_address(0x06105010, 0x67000000);
	write_reg_by_address(0x06105014, 0x00000000);
	write_reg_by_address(0x06105018, 0x00000000);
	write_reg_by_address(0x0610501c, 0x00000000);
	write_reg_by_address(0x06105020, 0x00000000);
	write_reg_by_address(0x06105024, 0x00000000);
	write_reg_by_address(0x06105028, 0x00000000);
	write_reg_by_address(0x0610502c, 0x00000000);
	write_reg_by_address(0x06105030, 0x00000000);
	write_reg_by_address(0x06105034, 0x00000000);
	write_reg_by_address(0x06105038, 0x00000000);
	write_reg_by_address(0x0610503c, 0x00000000);
	write_reg_by_address(0x06105040, 0x00000000);
	write_reg_by_address(0x06105044, 0x00000000);
	write_reg_by_address(0x06105048, 0x00000000);
	write_reg_by_address(0x0610504c, 0x00000000);
	write_reg_by_address(0x06105050, 0x00000000);
	write_reg_by_address(0x06105054, 0x00000000);
	write_reg_by_address(0x06105058, 0x00000000);
	write_reg_by_address(0x0610505c, 0x00000000);
	write_reg_by_address(0x06105060, 0x00000000);
	write_reg_by_address(0x06105064, 0x00000000);
	write_reg_by_address(0x06105068, 0x00000000);
	write_reg_by_address(0x0610506c, 0x00000000);
	write_reg_by_address(0x06105070, 0x00000000);
	write_reg_by_address(0x06105074, 0x00000000);
	write_reg_by_address(0x06105078, 0x00000000);
	write_reg_by_address(0x0610507c, 0x00000000);
	write_reg_by_address(0x06105080, 0x00000000);
	write_reg_by_address(0x06105084, 0x00000000);
	write_reg_by_address(0x06105088, 0x031f04fe);
	write_reg_by_address(0x0610508c, 0x00000000);
	write_reg_by_address(0x06105090, 0x00000000);
	write_reg_by_address(0x06105094, 0x00000000);
	write_reg_by_address(0x06105098, 0x00000000);
	write_reg_by_address(0x0610509c, 0x00000000);
	write_reg_by_address(0x061050a0, 0x00000000);
	write_reg_by_address(0x061050a4, 0x00000000);
	write_reg_by_address(0x061050a8, 0x00000000);
	write_reg_by_address(0x061050ac, 0x00000000);
	write_reg_by_address(0x061050b0, 0x00000000);
	write_reg_by_address(0x061050b4, 0x00000000);
	write_reg_by_address(0x061050b8, 0x00000000);
	write_reg_by_address(0x061050bc, 0x00000000);
	write_reg_by_address(0x061050c0, 0x00000000);
	write_reg_by_address(0x061050c4, 0x00000000);
	write_reg_by_address(0x061050c8, 0x00000000);
	write_reg_by_address(0x061050cc, 0x00000000);
	write_reg_by_address(0x061050d0, 0x00000000);
	write_reg_by_address(0x061050d4, 0x00000000);
	write_reg_by_address(0x061050d8, 0x00000000);
	write_reg_by_address(0x061050dc, 0x00000000);
	write_reg_by_address(0x061050e0, 0x00000000);
	write_reg_by_address(0x061050e4, 0x00000000);
	write_reg_by_address(0x061050e8, 0x00000000);
	write_reg_by_address(0x061050ec, 0x00000000);
	write_reg_by_address(0x061050f0, 0x00000000);
	write_reg_by_address(0x061050f4, 0x00000000);
	write_reg_by_address(0x061050f8, 0x00000000);
	write_reg_by_address(0x061050fc, 0x00000000);
	write_reg_by_address(0x06100004, 0x00000000);
	write_reg_by_address(0x06100008, 0x00000001);
	write_reg_by_address(0x0610000c, 0x031f04ff);
	write_reg_by_address(0x06100000, 0x00000001);

	//[SCALERVIDEO_0]
	write_reg_by_address(0x0612000c, 0x00000000);
	write_reg_by_address(0x06120040, 0x031e04ff);
	write_reg_by_address(0x06120080, 0x031f04ff);
	write_reg_by_address(0x06120088, 0x00100000);
	write_reg_by_address(0x0612008c, 0x00100520);
	write_reg_by_address(0x06120090, 0x00000000);
	write_reg_by_address(0x06120098, 0x00000000);
	write_reg_by_address(0x0612009c, 0x00000000);
	write_reg_by_address(0x061200c0, 0x031f04ff);
	write_reg_by_address(0x061200c8, 0x00100000);
	write_reg_by_address(0x061200cc, 0x00100520);
	write_reg_by_address(0x061200d0, 0x00000000);
	write_reg_by_address(0x061200d8, 0x00000000);
	write_reg_by_address(0x061200dc, 0x00000000);
	write_reg_by_address(0x06120200, 0x40000000);
	write_reg_by_address(0x06120204, 0x40fe0000);
	write_reg_by_address(0x06120208, 0x3ffd0100);
	write_reg_by_address(0x0612020c, 0x3efc0100);
	write_reg_by_address(0x06120210, 0x3efb0100);
	write_reg_by_address(0x06120214, 0x3dfa0200);
	write_reg_by_address(0x06120218, 0x3cf90200);
	write_reg_by_address(0x0612021c, 0x3bf80200);
	write_reg_by_address(0x06120220, 0x39f70200);
	write_reg_by_address(0x06120224, 0x37f70200);
	write_reg_by_address(0x06120228, 0x35f70200);
	write_reg_by_address(0x0612022c, 0x33f70200);
	write_reg_by_address(0x06120230, 0x31f70200);
	write_reg_by_address(0x06120234, 0x2ef70200);
	write_reg_by_address(0x06120238, 0x2cf70200);
	write_reg_by_address(0x0612023c, 0x2af70200);
	write_reg_by_address(0x06120240, 0x27f70200);
	write_reg_by_address(0x06120244, 0x24f80100);
	write_reg_by_address(0x06120248, 0x22f80100);
	write_reg_by_address(0x0612024c, 0x1ef90100);
	write_reg_by_address(0x06120250, 0x1cf90100);
	write_reg_by_address(0x06120254, 0x19fa0100);
	write_reg_by_address(0x06120258, 0x17fa0100);
	write_reg_by_address(0x0612025c, 0x14fb0100);
	write_reg_by_address(0x06120260, 0x11fc0000);
	write_reg_by_address(0x06120264, 0x0ffc0000);
	write_reg_by_address(0x06120268, 0x0cfd0000);
	write_reg_by_address(0x0612026c, 0x0afd0000);
	write_reg_by_address(0x06120270, 0x08fe0000);
	write_reg_by_address(0x06120274, 0x05ff0000);
	write_reg_by_address(0x06120278, 0x03ff0000);
	write_reg_by_address(0x0612027c, 0x02000000);
	write_reg_by_address(0x06120300, 0x00000000);
	write_reg_by_address(0x06120304, 0x00000002);
	write_reg_by_address(0x06120308, 0x0000ff04);
	write_reg_by_address(0x0612030c, 0x0000ff06);
	write_reg_by_address(0x06120310, 0x0000fe08);
	write_reg_by_address(0x06120314, 0x0000fd0a);
	write_reg_by_address(0x06120318, 0x0000fd0c);
	write_reg_by_address(0x0612031c, 0x0000fc0f);
	write_reg_by_address(0x06120320, 0x0000fc12);
	write_reg_by_address(0x06120324, 0x0001fb14);
	write_reg_by_address(0x06120328, 0x0001fa17);
	write_reg_by_address(0x0612032c, 0x0001fa19);
	write_reg_by_address(0x06120330, 0x0001f91c);
	write_reg_by_address(0x06120334, 0x0001f91f);
	write_reg_by_address(0x06120338, 0x0001f822);
	write_reg_by_address(0x0612033c, 0x0001f824);
	write_reg_by_address(0x06120340, 0x0002f727);
	write_reg_by_address(0x06120344, 0x0002f72a);
	write_reg_by_address(0x06120348, 0x0002f72c);
	write_reg_by_address(0x0612034c, 0x0002f72f);
	write_reg_by_address(0x06120350, 0x0002f731);
	write_reg_by_address(0x06120354, 0x0002f733);
	write_reg_by_address(0x06120358, 0x0002f735);
	write_reg_by_address(0x0612035c, 0x0002f737);
	write_reg_by_address(0x06120360, 0x0002f73a);
	write_reg_by_address(0x06120364, 0x0002f83b);
	write_reg_by_address(0x06120368, 0x0002f93c);
	write_reg_by_address(0x0612036c, 0x0002fa3d);
	write_reg_by_address(0x06120370, 0x0001fb3e);
	write_reg_by_address(0x06120374, 0x0001fc3f);
	write_reg_by_address(0x06120378, 0x0001fd40);
	write_reg_by_address(0x0612037c, 0x0000fe40);
	write_reg_by_address(0x06120400, 0x00004000);
	write_reg_by_address(0x06120404, 0x000140ff);
	write_reg_by_address(0x06120408, 0x00033ffe);
	write_reg_by_address(0x0612040c, 0x00043ffd);
	write_reg_by_address(0x06120410, 0x00063efc);
	write_reg_by_address(0x06120414, 0xff083dfc);
	write_reg_by_address(0x06120418, 0x000a3bfb);
	write_reg_by_address(0x0612041c, 0xff0d39fb);
	write_reg_by_address(0x06120420, 0xff0f37fb);
	write_reg_by_address(0x06120424, 0xff1136fa);
	write_reg_by_address(0x06120428, 0xfe1433fb);
	write_reg_by_address(0x0612042c, 0xfe1631fb);
	write_reg_by_address(0x06120430, 0xfd192ffb);
	write_reg_by_address(0x06120434, 0xfd1c2cfb);
	write_reg_by_address(0x06120438, 0xfd1f29fb);
	write_reg_by_address(0x0612043c, 0xfc2127fc);
	write_reg_by_address(0x06120440, 0xfc2424fc);
	write_reg_by_address(0x06120444, 0xfc2721fc);
	write_reg_by_address(0x06120448, 0xfb291ffd);
	write_reg_by_address(0x0612044c, 0xfb2c1cfd);
	write_reg_by_address(0x06120450, 0xfb2f19fd);
	write_reg_by_address(0x06120454, 0xfb3116fe);
	write_reg_by_address(0x06120458, 0xfb3314fe);
	write_reg_by_address(0x0612045c, 0xfa3611ff);
	write_reg_by_address(0x06120460, 0xfb370fff);
	write_reg_by_address(0x06120464, 0xfb390dff);
	write_reg_by_address(0x06120468, 0xfb3b0a00);
	write_reg_by_address(0x0612046c, 0xfc3d08ff);
	write_reg_by_address(0x06120470, 0xfc3e0600);
	write_reg_by_address(0x06120474, 0xfd3f0400);
	write_reg_by_address(0x06120478, 0xfe3f0300);
	write_reg_by_address(0x0612047c, 0xff400100);
	write_reg_by_address(0x06120600, 0x40000000);
	write_reg_by_address(0x06120604, 0x40fe0000);
	write_reg_by_address(0x06120608, 0x3ffd0100);
	write_reg_by_address(0x0612060c, 0x3efc0100);
	write_reg_by_address(0x06120610, 0x3efb0100);
	write_reg_by_address(0x06120614, 0x3dfa0200);
	write_reg_by_address(0x06120618, 0x3cf90200);
	write_reg_by_address(0x0612061c, 0x3bf80200);
	write_reg_by_address(0x06120620, 0x39f70200);
	write_reg_by_address(0x06120624, 0x37f70200);
	write_reg_by_address(0x06120628, 0x35f70200);
	write_reg_by_address(0x0612062c, 0x33f70200);
	write_reg_by_address(0x06120630, 0x31f70200);
	write_reg_by_address(0x06120634, 0x2ef70200);
	write_reg_by_address(0x06120638, 0x2cf70200);
	write_reg_by_address(0x0612063c, 0x2af70200);
	write_reg_by_address(0x06120640, 0x27f70200);
	write_reg_by_address(0x06120644, 0x24f80100);
	write_reg_by_address(0x06120648, 0x22f80100);
	write_reg_by_address(0x0612064c, 0x1ef90100);
	write_reg_by_address(0x06120650, 0x1cf90100);
	write_reg_by_address(0x06120654, 0x19fa0100);
	write_reg_by_address(0x06120658, 0x17fa0100);
	write_reg_by_address(0x0612065c, 0x14fb0100);
	write_reg_by_address(0x06120660, 0x11fc0000);
	write_reg_by_address(0x06120664, 0x0ffc0000);
	write_reg_by_address(0x06120668, 0x0cfd0000);
	write_reg_by_address(0x0612066c, 0x0afd0000);
	write_reg_by_address(0x06120670, 0x08fe0000);
	write_reg_by_address(0x06120674, 0x05ff0000);
	write_reg_by_address(0x06120678, 0x03ff0000);
	write_reg_by_address(0x0612067c, 0x02000000);
	write_reg_by_address(0x06120700, 0x00000000);
	write_reg_by_address(0x06120704, 0x00000002);
	write_reg_by_address(0x06120708, 0x0000ff04);
	write_reg_by_address(0x0612070c, 0x0000ff06);
	write_reg_by_address(0x06120710, 0x0000fe08);
	write_reg_by_address(0x06120714, 0x0000fd0a);
	write_reg_by_address(0x06120718, 0x0000fd0c);
	write_reg_by_address(0x0612071c, 0x0000fc0f);
	write_reg_by_address(0x06120720, 0x0000fc12);
	write_reg_by_address(0x06120724, 0x0001fb14);
	write_reg_by_address(0x06120728, 0x0001fa17);
	write_reg_by_address(0x0612072c, 0x0001fa19);
	write_reg_by_address(0x06120730, 0x0001f91c);
	write_reg_by_address(0x06120734, 0x0001f91f);
	write_reg_by_address(0x06120738, 0x0001f822);
	write_reg_by_address(0x0612073c, 0x0001f824);
	write_reg_by_address(0x06120740, 0x0002f727);
	write_reg_by_address(0x06120744, 0x0002f72a);
	write_reg_by_address(0x06120748, 0x0002f72c);
	write_reg_by_address(0x0612074c, 0x0002f72f);
	write_reg_by_address(0x06120750, 0x0002f731);
	write_reg_by_address(0x06120754, 0x0002f733);
	write_reg_by_address(0x06120758, 0x0002f735);
	write_reg_by_address(0x0612075c, 0x0002f737);
	write_reg_by_address(0x06120760, 0x0002f73a);
	write_reg_by_address(0x06120764, 0x0002f83b);
	write_reg_by_address(0x06120768, 0x0002f93c);
	write_reg_by_address(0x0612076c, 0x0002fa3d);
	write_reg_by_address(0x06120770, 0x0001fb3e);
	write_reg_by_address(0x06120774, 0x0001fc3f);
	write_reg_by_address(0x06120778, 0x0001fd40);
	write_reg_by_address(0x0612077c, 0x0000fe40);
	write_reg_by_address(0x06120800, 0x00004000);
	write_reg_by_address(0x06120804, 0x000140ff);
	write_reg_by_address(0x06120808, 0x00033ffe);
	write_reg_by_address(0x0612080c, 0x00043ffd);
	write_reg_by_address(0x06120810, 0x00063efc);
	write_reg_by_address(0x06120814, 0xff083dfc);
	write_reg_by_address(0x06120818, 0x000a3bfb);
	write_reg_by_address(0x0612081c, 0xff0d39fb);
	write_reg_by_address(0x06120820, 0xff0f37fb);
	write_reg_by_address(0x06120824, 0xff1136fa);
	write_reg_by_address(0x06120828, 0xfe1433fb);
	write_reg_by_address(0x0612082c, 0xfe1631fb);
	write_reg_by_address(0x06120830, 0xfd192ffb);
	write_reg_by_address(0x06120834, 0xfd1c2cfb);
	write_reg_by_address(0x06120838, 0xfd1f29fb);
	write_reg_by_address(0x0612083c, 0xfc2127fc);
	write_reg_by_address(0x06120840, 0xfc2424fc);
	write_reg_by_address(0x06120844, 0xfc2721fc);
	write_reg_by_address(0x06120848, 0xfb291ffd);
	write_reg_by_address(0x0612084c, 0xfb2c1cfd);
	write_reg_by_address(0x06120850, 0xfb2f19fd);
	write_reg_by_address(0x06120854, 0xfb3116fe);
	write_reg_by_address(0x06120858, 0xfb3314fe);
	write_reg_by_address(0x0612085c, 0xfa3611ff);
	write_reg_by_address(0x06120860, 0xfb370fff);
	write_reg_by_address(0x06120864, 0xfb390dff);
	write_reg_by_address(0x06120868, 0xfb3b0a00);
	write_reg_by_address(0x0612086c, 0xfc3d08ff);
	write_reg_by_address(0x06120870, 0xfc3e0600);
	write_reg_by_address(0x06120874, 0xfd3f0400);
	write_reg_by_address(0x06120878, 0xfe3f0300);
	write_reg_by_address(0x0612087c, 0xff400100);
	write_reg_by_address(0x06120000, 0x00000011);

	//[SCALERVIDEO_1]
	write_reg_by_address(0x0614000c, 0x00000000);
	write_reg_by_address(0x06140040, 0x031504ff);
	write_reg_by_address(0x06140080, 0x031f04ff);
	write_reg_by_address(0x06140088, 0x00100000);
	write_reg_by_address(0x0614008c, 0x001033d8);
	write_reg_by_address(0x06140090, 0x00000000);
	write_reg_by_address(0x06140098, 0x00000000);
	write_reg_by_address(0x0614009c, 0x00000000);
	write_reg_by_address(0x061400c0, 0x031f04ff);
	write_reg_by_address(0x061400c8, 0x00100000);
	write_reg_by_address(0x061400cc, 0x001033d8);
	write_reg_by_address(0x061400d0, 0x00000000);
	write_reg_by_address(0x061400d8, 0x00000000);
	write_reg_by_address(0x061400dc, 0x00000000);
	write_reg_by_address(0x06140200, 0x40000000);
	write_reg_by_address(0x06140204, 0x40fe0000);
	write_reg_by_address(0x06140208, 0x3ffd0100);
	write_reg_by_address(0x0614020c, 0x3efc0100);
	write_reg_by_address(0x06140210, 0x3efb0100);
	write_reg_by_address(0x06140214, 0x3dfa0200);
	write_reg_by_address(0x06140218, 0x3cf90200);
	write_reg_by_address(0x0614021c, 0x3bf80200);
	write_reg_by_address(0x06140220, 0x39f70200);
	write_reg_by_address(0x06140224, 0x37f70200);
	write_reg_by_address(0x06140228, 0x35f70200);
	write_reg_by_address(0x0614022c, 0x33f70200);
	write_reg_by_address(0x06140230, 0x31f70200);
	write_reg_by_address(0x06140234, 0x2ef70200);
	write_reg_by_address(0x06140238, 0x2cf70200);
	write_reg_by_address(0x0614023c, 0x2af70200);
	write_reg_by_address(0x06140240, 0x27f70200);
	write_reg_by_address(0x06140244, 0x24f80100);
	write_reg_by_address(0x06140248, 0x22f80100);
	write_reg_by_address(0x0614024c, 0x1ef90100);
	write_reg_by_address(0x06140250, 0x1cf90100);
	write_reg_by_address(0x06140254, 0x19fa0100);
	write_reg_by_address(0x06140258, 0x17fa0100);
	write_reg_by_address(0x0614025c, 0x14fb0100);
	write_reg_by_address(0x06140260, 0x11fc0000);
	write_reg_by_address(0x06140264, 0x0ffc0000);
	write_reg_by_address(0x06140268, 0x0cfd0000);
	write_reg_by_address(0x0614026c, 0x0afd0000);
	write_reg_by_address(0x06140270, 0x08fe0000);
	write_reg_by_address(0x06140274, 0x05ff0000);
	write_reg_by_address(0x06140278, 0x03ff0000);
	write_reg_by_address(0x0614027c, 0x02000000);
	write_reg_by_address(0x06140300, 0x00000000);
	write_reg_by_address(0x06140304, 0x00000002);
	write_reg_by_address(0x06140308, 0x0000ff04);
	write_reg_by_address(0x0614030c, 0x0000ff06);
	write_reg_by_address(0x06140310, 0x0000fe08);
	write_reg_by_address(0x06140314, 0x0000fd0a);
	write_reg_by_address(0x06140318, 0x0000fd0c);
	write_reg_by_address(0x0614031c, 0x0000fc0f);
	write_reg_by_address(0x06140320, 0x0000fc12);
	write_reg_by_address(0x06140324, 0x0001fb14);
	write_reg_by_address(0x06140328, 0x0001fa17);
	write_reg_by_address(0x0614032c, 0x0001fa19);
	write_reg_by_address(0x06140330, 0x0001f91c);
	write_reg_by_address(0x06140334, 0x0001f91f);
	write_reg_by_address(0x06140338, 0x0001f822);
	write_reg_by_address(0x0614033c, 0x0001f824);
	write_reg_by_address(0x06140340, 0x0002f727);
	write_reg_by_address(0x06140344, 0x0002f72a);
	write_reg_by_address(0x06140348, 0x0002f72c);
	write_reg_by_address(0x0614034c, 0x0002f72f);
	write_reg_by_address(0x06140350, 0x0002f731);
	write_reg_by_address(0x06140354, 0x0002f733);
	write_reg_by_address(0x06140358, 0x0002f735);
	write_reg_by_address(0x0614035c, 0x0002f737);
	write_reg_by_address(0x06140360, 0x0002f73a);
	write_reg_by_address(0x06140364, 0x0002f83b);
	write_reg_by_address(0x06140368, 0x0002f93c);
	write_reg_by_address(0x0614036c, 0x0002fa3d);
	write_reg_by_address(0x06140370, 0x0001fb3e);
	write_reg_by_address(0x06140374, 0x0001fc3f);
	write_reg_by_address(0x06140378, 0x0001fd40);
	write_reg_by_address(0x0614037c, 0x0000fe40);
	write_reg_by_address(0x06140400, 0x00004000);
	write_reg_by_address(0x06140404, 0x000140ff);
	write_reg_by_address(0x06140408, 0x00033ffe);
	write_reg_by_address(0x0614040c, 0x00043ffd);
	write_reg_by_address(0x06140410, 0x00063efc);
	write_reg_by_address(0x06140414, 0xff083dfc);
	write_reg_by_address(0x06140418, 0x000a3bfb);
	write_reg_by_address(0x0614041c, 0xff0d39fb);
	write_reg_by_address(0x06140420, 0xff0f37fb);
	write_reg_by_address(0x06140424, 0xff1136fa);
	write_reg_by_address(0x06140428, 0xfe1433fb);
	write_reg_by_address(0x0614042c, 0xfe1631fb);
	write_reg_by_address(0x06140430, 0xfd192ffb);
	write_reg_by_address(0x06140434, 0xfd1c2cfb);
	write_reg_by_address(0x06140438, 0xfd1f29fb);
	write_reg_by_address(0x0614043c, 0xfc2127fc);
	write_reg_by_address(0x06140440, 0xfc2424fc);
	write_reg_by_address(0x06140444, 0xfc2721fc);
	write_reg_by_address(0x06140448, 0xfb291ffd);
	write_reg_by_address(0x0614044c, 0xfb2c1cfd);
	write_reg_by_address(0x06140450, 0xfb2f19fd);
	write_reg_by_address(0x06140454, 0xfb3116fe);
	write_reg_by_address(0x06140458, 0xfb3314fe);
	write_reg_by_address(0x0614045c, 0xfa3611ff);
	write_reg_by_address(0x06140460, 0xfb370fff);
	write_reg_by_address(0x06140464, 0xfb390dff);
	write_reg_by_address(0x06140468, 0xfb3b0a00);
	write_reg_by_address(0x0614046c, 0xfc3d08ff);
	write_reg_by_address(0x06140470, 0xfc3e0600);
	write_reg_by_address(0x06140474, 0xfd3f0400);
	write_reg_by_address(0x06140478, 0xfe3f0300);
	write_reg_by_address(0x0614047c, 0xff400100);
	write_reg_by_address(0x06140600, 0x40000000);
	write_reg_by_address(0x06140604, 0x40fe0000);
	write_reg_by_address(0x06140608, 0x3ffd0100);
	write_reg_by_address(0x0614060c, 0x3efc0100);
	write_reg_by_address(0x06140610, 0x3efb0100);
	write_reg_by_address(0x06140614, 0x3dfa0200);
	write_reg_by_address(0x06140618, 0x3cf90200);
	write_reg_by_address(0x0614061c, 0x3bf80200);
	write_reg_by_address(0x06140620, 0x39f70200);
	write_reg_by_address(0x06140624, 0x37f70200);
	write_reg_by_address(0x06140628, 0x35f70200);
	write_reg_by_address(0x0614062c, 0x33f70200);
	write_reg_by_address(0x06140630, 0x31f70200);
	write_reg_by_address(0x06140634, 0x2ef70200);
	write_reg_by_address(0x06140638, 0x2cf70200);
	write_reg_by_address(0x0614063c, 0x2af70200);
	write_reg_by_address(0x06140640, 0x27f70200);
	write_reg_by_address(0x06140644, 0x24f80100);
	write_reg_by_address(0x06140648, 0x22f80100);
	write_reg_by_address(0x0614064c, 0x1ef90100);
	write_reg_by_address(0x06140650, 0x1cf90100);
	write_reg_by_address(0x06140654, 0x19fa0100);
	write_reg_by_address(0x06140658, 0x17fa0100);
	write_reg_by_address(0x0614065c, 0x14fb0100);
	write_reg_by_address(0x06140660, 0x11fc0000);
	write_reg_by_address(0x06140664, 0x0ffc0000);
	write_reg_by_address(0x06140668, 0x0cfd0000);
	write_reg_by_address(0x0614066c, 0x0afd0000);
	write_reg_by_address(0x06140670, 0x08fe0000);
	write_reg_by_address(0x06140674, 0x05ff0000);
	write_reg_by_address(0x06140678, 0x03ff0000);
	write_reg_by_address(0x0614067c, 0x02000000);
	write_reg_by_address(0x06140700, 0x00000000);
	write_reg_by_address(0x06140704, 0x00000002);
	write_reg_by_address(0x06140708, 0x0000ff04);
	write_reg_by_address(0x0614070c, 0x0000ff06);
	write_reg_by_address(0x06140710, 0x0000fe08);
	write_reg_by_address(0x06140714, 0x0000fd0a);
	write_reg_by_address(0x06140718, 0x0000fd0c);
	write_reg_by_address(0x0614071c, 0x0000fc0f);
	write_reg_by_address(0x06140720, 0x0000fc12);
	write_reg_by_address(0x06140724, 0x0001fb14);
	write_reg_by_address(0x06140728, 0x0001fa17);
	write_reg_by_address(0x0614072c, 0x0001fa19);
	write_reg_by_address(0x06140730, 0x0001f91c);
	write_reg_by_address(0x06140734, 0x0001f91f);
	write_reg_by_address(0x06140738, 0x0001f822);
	write_reg_by_address(0x0614073c, 0x0001f824);
	write_reg_by_address(0x06140740, 0x0002f727);
	write_reg_by_address(0x06140744, 0x0002f72a);
	write_reg_by_address(0x06140748, 0x0002f72c);
	write_reg_by_address(0x0614074c, 0x0002f72f);
	write_reg_by_address(0x06140750, 0x0002f731);
	write_reg_by_address(0x06140754, 0x0002f733);
	write_reg_by_address(0x06140758, 0x0002f735);
	write_reg_by_address(0x0614075c, 0x0002f737);
	write_reg_by_address(0x06140760, 0x0002f73a);
	write_reg_by_address(0x06140764, 0x0002f83b);
	write_reg_by_address(0x06140768, 0x0002f93c);
	write_reg_by_address(0x0614076c, 0x0002fa3d);
	write_reg_by_address(0x06140770, 0x0001fb3e);
	write_reg_by_address(0x06140774, 0x0001fc3f);
	write_reg_by_address(0x06140778, 0x0001fd40);
	write_reg_by_address(0x0614077c, 0x0000fe40);
	write_reg_by_address(0x06140800, 0x00004000);
	write_reg_by_address(0x06140804, 0x000140ff);
	write_reg_by_address(0x06140808, 0x00033ffe);
	write_reg_by_address(0x0614080c, 0x00043ffd);
	write_reg_by_address(0x06140810, 0x00063efc);
	write_reg_by_address(0x06140814, 0xff083dfc);
	write_reg_by_address(0x06140818, 0x000a3bfb);
	write_reg_by_address(0x0614081c, 0xff0d39fb);
	write_reg_by_address(0x06140820, 0xff0f37fb);
	write_reg_by_address(0x06140824, 0xff1136fa);
	write_reg_by_address(0x06140828, 0xfe1433fb);
	write_reg_by_address(0x0614082c, 0xfe1631fb);
	write_reg_by_address(0x06140830, 0xfd192ffb);
	write_reg_by_address(0x06140834, 0xfd1c2cfb);
	write_reg_by_address(0x06140838, 0xfd1f29fb);
	write_reg_by_address(0x0614083c, 0xfc2127fc);
	write_reg_by_address(0x06140840, 0xfc2424fc);
	write_reg_by_address(0x06140844, 0xfc2721fc);
	write_reg_by_address(0x06140848, 0xfb291ffd);
	write_reg_by_address(0x0614084c, 0xfb2c1cfd);
	write_reg_by_address(0x06140850, 0xfb2f19fd);
	write_reg_by_address(0x06140854, 0xfb3116fe);
	write_reg_by_address(0x06140858, 0xfb3314fe);
	write_reg_by_address(0x0614085c, 0xfa3611ff);
	write_reg_by_address(0x06140860, 0xfb370fff);
	write_reg_by_address(0x06140864, 0xfb390dff);
	write_reg_by_address(0x06140868, 0xfb3b0a00);
	write_reg_by_address(0x0614086c, 0xfc3d08ff);
	write_reg_by_address(0x06140870, 0xfc3e0600);
	write_reg_by_address(0x06140874, 0xfd3f0400);
	write_reg_by_address(0x06140878, 0xfe3f0300);
	write_reg_by_address(0x0614087c, 0xff400100);
	write_reg_by_address(0x06140000, 0x00000011);

	//[CSC_1]
	write_reg_by_address(0x061f0000, 0x00000000);
	write_reg_by_address(0x061f0004, 0x00000000);
	write_reg_by_address(0x061f0008, 0x00000000);
	write_reg_by_address(0x061f000c, 0x00000000);
	write_reg_by_address(0x061f0010, 0x00000000);
	write_reg_by_address(0x061f0014, 0x00000000);
	write_reg_by_address(0x061f0018, 0x00000000);
	write_reg_by_address(0x061f001c, 0x00000000);
	write_reg_by_address(0x061f0020, 0x00000000);
	write_reg_by_address(0x061f0024, 0x00000000);
	write_reg_by_address(0x061f0028, 0x00000000);
	write_reg_by_address(0x061f002c, 0x00000000);
	write_reg_by_address(0x061f0030, 0x00000000);
	write_reg_by_address(0x061f0034, 0x00000000);
	write_reg_by_address(0x061f0038, 0x00000000);
	write_reg_by_address(0x061f003c, 0x00000000);
	write_reg_by_address(0x061f0040, 0xff000000);

	//[SCALERUI_2]
	write_reg_by_address(0x0616000c, 0x00000000);
	write_reg_by_address(0x06160010, 0x00000000);
	write_reg_by_address(0x06160040, 0x031f04ff);
	write_reg_by_address(0x06160080, 0x031f04ff);
	write_reg_by_address(0x06160088, 0x00100000);
	write_reg_by_address(0x0616008c, 0x00100000);
	write_reg_by_address(0x06160090, 0x00000000);
	write_reg_by_address(0x06160098, 0x00000000);
	write_reg_by_address(0x0616009c, 0x00000000);
	write_reg_by_address(0x06160200, 0x00004000);
	write_reg_by_address(0x06160204, 0x00033ffe);
	write_reg_by_address(0x06160208, 0x00063efc);
	write_reg_by_address(0x0616020c, 0x000a3bfb);
	write_reg_by_address(0x06160210, 0xff0f37fb);
	write_reg_by_address(0x06160214, 0xfe1433fb);
	write_reg_by_address(0x06160218, 0xfd192ffb);
	write_reg_by_address(0x0616021c, 0xfd1f29fb);
	write_reg_by_address(0x06160220, 0xfc2424fc);
	write_reg_by_address(0x06160224, 0xfb291ffd);
	write_reg_by_address(0x06160228, 0xfb2f19fd);
	write_reg_by_address(0x0616022c, 0xfb3314fe);
	write_reg_by_address(0x06160230, 0xfb370fff);
	write_reg_by_address(0x06160234, 0xfb3b0a00);
	write_reg_by_address(0x06160238, 0xfc3e0600);
	write_reg_by_address(0x0616023c, 0xfe3f0300);
	write_reg_by_address(0x06160000, 0x00000010);

	//[SCALERUI_3]
	write_reg_by_address(0x0617000c, 0x00000000);
	write_reg_by_address(0x06170010, 0x00000000);
	write_reg_by_address(0x06170040, 0x031f04ff);
	write_reg_by_address(0x06170080, 0x031f04fe);
	write_reg_by_address(0x06170088, 0x000ffccc);
	write_reg_by_address(0x0617008c, 0x00100000);
	write_reg_by_address(0x06170090, 0x00000000);
	write_reg_by_address(0x06170098, 0x00000000);
	write_reg_by_address(0x0617009c, 0x00000000);
	write_reg_by_address(0x06170200, 0x00004000);
	write_reg_by_address(0x06170204, 0x00033ffe);
	write_reg_by_address(0x06170208, 0x00063efc);
	write_reg_by_address(0x0617020c, 0x000a3bfb);
	write_reg_by_address(0x06170210, 0xff0f37fb);
	write_reg_by_address(0x06170214, 0xfe1433fb);
	write_reg_by_address(0x06170218, 0xfd192ffb);
	write_reg_by_address(0x0617021c, 0xfd1f29fb);
	write_reg_by_address(0x06170220, 0xfc2424fc);
	write_reg_by_address(0x06170224, 0xfb291ffd);
	write_reg_by_address(0x06170228, 0xfb2f19fd);
	write_reg_by_address(0x0617022c, 0xfb3314fe);
	write_reg_by_address(0x06170230, 0xfb370fff);
	write_reg_by_address(0x06170234, 0xfb3b0a00);
	write_reg_by_address(0x06170238, 0xfc3e0600);
	write_reg_by_address(0x0617023c, 0xfe3f0300);
	write_reg_by_address(0x06170000, 0x00000011);

	//[WB]
	//write_reg_by_address(0x01c0d000, 0x00000000);
	write_reg_by_address(0x06010004, 0x031f04ff);
	write_reg_by_address(0x06010008, 0x00000000);
	write_reg_by_address(0x0601000c, 0x031f04ff);
	write_reg_by_address(0x06010010, 0x673e8000);
	write_reg_by_address(0x06010014, 0x6743c604);
	write_reg_by_address(0x06010018, 0x67451788);
	write_reg_by_address(0x0601001c, 0x00000000);
	write_reg_by_address(0x06010020, 0x00000000);
	write_reg_by_address(0x06010024, 0x00000000);
	write_reg_by_address(0x06010028, 0x00000000);
	write_reg_by_address(0x0601002c, 0x00000000);
	write_reg_by_address(0x06010030, 0x000002d0);
	write_reg_by_address(0x06010034, 0x00000168);
	write_reg_by_address(0x06010040, 0x00000000);
	write_reg_by_address(0x06010044, 0x00000008);
	write_reg_by_address(0x06010048, 0x00000001);
	write_reg_by_address(0x0601004c, 0x00000000);
	write_reg_by_address(0x06010054, 0x00000005);
	write_reg_by_address(0x06010070, 0x00000000);
	write_reg_by_address(0x06010074, 0x00000000);
	write_reg_by_address(0x06010080, 0x031f04ff);
	write_reg_by_address(0x06010084, 0x01df02cf);
	write_reg_by_address(0x06010088, 0x001c71c4);
	write_reg_by_address(0x0601008c, 0x001aaaa8);
	write_reg_by_address(0x06010050, 0x00000020);
	write_reg_by_address(0x06010100, 0x000000bb);
	write_reg_by_address(0x06010104, 0x00000274);
	write_reg_by_address(0x06010108, 0x0000003f);
	write_reg_by_address(0x0601010c, 0x00004200);
	write_reg_by_address(0x06010110, 0x00001f98);
	write_reg_by_address(0x06010114, 0x00001ea5);
	write_reg_by_address(0x06010118, 0x000001c1);
	write_reg_by_address(0x0601011c, 0x00020200);
	write_reg_by_address(0x06010120, 0x000001c1);
	write_reg_by_address(0x06010124, 0x00001e67);
	write_reg_by_address(0x06010128, 0x00001fd7);
	write_reg_by_address(0x0601012c, 0x00020200);
	write_reg_by_address(0x06010200, 0x000e240e);
	write_reg_by_address(0x06010280, 0x000e240e);
	write_reg_by_address(0x06010204, 0x0010240c);
	write_reg_by_address(0x06010284, 0x0010240c);
	write_reg_by_address(0x06010208, 0x0013230a);
	write_reg_by_address(0x06010288, 0x0013230a);
	write_reg_by_address(0x0601020c, 0x00142309);
	write_reg_by_address(0x0601028c, 0x00142309);
	write_reg_by_address(0x06010210, 0x00162208);
	write_reg_by_address(0x06010290, 0x00162208);
	write_reg_by_address(0x06010214, 0x01182106);
	write_reg_by_address(0x06010294, 0x01182106);
	write_reg_by_address(0x06010218, 0x011a2005);
	write_reg_by_address(0x06010298, 0x011a2005);
	write_reg_by_address(0x0601021c, 0x021b1f04);
	write_reg_by_address(0x0601029c, 0x021b1f04);
	write_reg_by_address(0x06010220, 0x031d1d03);
	write_reg_by_address(0x060102a0, 0x031d1d03);
	write_reg_by_address(0x06010224, 0x041e1c02);
	write_reg_by_address(0x060102a4, 0x041e1c02);
	write_reg_by_address(0x06010228, 0x05201a01);
	write_reg_by_address(0x060102a8, 0x05201a01);
	write_reg_by_address(0x0601022c, 0x06211801);
	write_reg_by_address(0x060102ac, 0x06211801);
	write_reg_by_address(0x06010230, 0x07221601);
	write_reg_by_address(0x060102b0, 0x07221601);
	write_reg_by_address(0x06010234, 0x09231400);
	write_reg_by_address(0x060102b4, 0x09231400);
	write_reg_by_address(0x06010238, 0x0a231300);
	write_reg_by_address(0x060102b8, 0x0a231300);
	write_reg_by_address(0x0601023c, 0x0c231100);
	write_reg_by_address(0x060102bc, 0x0c231100);
	//write_reg_by_address(0x06010000, 0x20000001);
	write_reg_by_address(0x06100000, 0x00000001);
	write_reg_by_address(0x06000010, 0x00000000);
	//write_reg_by_address(0x01c0d0f8, 0x00000008);
	//write_reg_by_address(0x01c0d0fc, 0x00002000);
	//write_reg_by_address(0x01c0d000, 0x00000000);
	//write_reg_by_address(0x01c0d004, 0x00000000);
	//write_reg_by_address(0x01c0d008, 0x00000000);
	//write_reg_by_address(0x01c0d040, 0x00000040);
	//write_reg_by_address(0x01c0d044, 0x80000020);
	//write_reg_by_address(0x01c0d048, 0x04ff031f);
	//write_reg_by_address(0x01c0d04c, 0x05130003);
	//write_reg_by_address(0x01c0d050, 0x06660003);
	//write_reg_by_address(0x01c0d054, 0x00020002);
	//write_reg_by_address(0x01c0d058, 0x00000000);
	//write_reg_by_address(0x01c0d088, 0x00000000);
	//write_reg_by_address(0x01c0d08c, 0x00000000);
	//write_reg_by_address(0x01c0d090, 0x00000001);
	//write_reg_by_address(0x01c0d040, 0x80000040);
	//write_reg_by_address(0x01c0d000, 0x80000000);

	// config tcon
	write_reg_by_address(0x300B06C, 0x22222222);
	write_reg_by_address(0x300B070, 0x22222222);
	write_reg_by_address(0x300B074, 0x22222222);
	data = read_reg_by_address(0x0651001c); // (DISP0_IF_TOP_BASE+0x1c)
	data = data & 0xfffffff0;
	write_reg_by_address(0x0651001c, data);
	write_reg_by_address(0x06511000+0x000, 0x00000000);  //tcon_gctl
	write_reg_by_address(0x06511000+0x004, 0x80000000);  //tcon_gint0
	write_reg_by_address(0x06511000+0x008, 0x00000000);  //tcon_gint1
	write_reg_by_address(0x06511000+0x044, 0xf0000000 | 1); //tcon0_dclk
	write_reg_by_address(0x06511000+0x048, (800-1)<<16  | (1280-1));  //tcon0_basic0 x y
	write_reg_by_address(0x06511000+0x04c, (800+10-1)<<16 | (5-1));  //tcon0_basic1  ht hbp
	write_reg_by_address(0x06511000+0x050, ((1280+10)*2)<<16 | (5-1)); //tcon0_basic2	vt vbp
	write_reg_by_address(0x06511000+0x054, (2-1)<<16 | (2-1));  //tcon0_basic3	hspw vspw
	write_reg_by_address(0x06511000+0x058, 0x00000000);  //tcon0_hv
	write_reg_by_address(0x06511000+0x088, 0x00000000);  //tcon0_inv
	write_reg_by_address(0x06511000+0x08c, 0x00000000);  //tcon0_tri
	write_reg_by_address(0x06511000+0x040, 0x80000021);  //tcon0_enable
	write_reg_by_address(0x06511000+0x000, 0x80000000);  //tcon_enable

	return 0;
}

static void tcon_irq_enable(struct gorilla_de_tcon *chip)
{
	u32 val;

	dev_dbg(chip->dev, "%s()\n", __func__);
	val = readl(chip->base + LCD_GINT0_REG) | LCD_VB_INT_EN;
	writel(val, chip->base + LCD_GINT0_REG);
}

static u32 tcon_irq_check(struct gorilla_de_tcon *chip)
{
	u32 val;

	val = readl(chip->base + LCD_GINT0_REG) & LCD_VB_INT_FLAG;
	if (val) {
		//dev_dbg(chip->dev, "%s(): IRQ asserted\n", __func__);
	} else {
		dev_warn(chip->dev, "%s(): IRQ NOT asserted\n", __func__);
	}

	return val;
}

static void tcon_irq_clear(struct gorilla_de_tcon *chip)
{
	u32 val;

	//dev_dbg(chip->dev, "%s()\n", __func__);

	val = readl(chip->base + LCD_GINT0_REG) & ~LCD_VB_INT_FLAG;
	writel(val, chip->base + LCD_GINT0_REG);
#if 1
	/* FIXME: TCON IRQ has to be cleared twice. This is a hardware issue.
	 * The workaround below is dangerous... */
	while (readl(chip->base + LCD_GINT0_REG) | LCD_VB_INT_FLAG) {
		dev_dbg(chip->dev, "%s(): re-clear irq\n", __func__);
		val = readl(chip->base + LCD_GINT0_REG) & ~LCD_VB_INT_FLAG;
		writel(val, chip->base + LCD_GINT0_REG);
	}
#endif
}

static irqreturn_t gorilla_tcon_isr(int irq, void *id)
{
	struct gorilla_de_tcon *chip = (struct gorilla_de_tcon *)id;

	dev_dbg(chip->dev, "%s()\n", __func__);
	tcon_irq_check(chip);
	tcon_irq_clear(chip);
	//dev_dbg(chip->dev, "%s(): IRQ_HANDLED\n", __func__);
	return IRQ_HANDLED;
}

static const struct of_device_id gorilla_de_tcon_dt_ids[] = {
	{.compatible = "sunxi,gorilla-de-tcon"},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, gorilla_de_tcon_dt_ids);

static int gorilla_de_tcon_probe(struct platform_device *pdev)
{
	struct gorilla_de_tcon *chip;
	struct device *dev = &pdev->dev;
	struct resource *res;
	int err;

	dev_dbg(dev, "%s(): BEGIN\n", __func__);

	/* Initialize private data structure `chip` */
	chip = devm_kzalloc(dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;
	platform_set_drvdata(pdev, chip);
	chip->dev = dev;

	/* Initialize hardware */
	de_tcon_init(chip);

	/* Map IRQ registers */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "Fail to get IORESOURCE_MEM\n");
		return -EINVAL;
	}
	chip->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(chip->base)) {
		dev_err(dev, "Fail to map IO resource\n");
		return PTR_ERR(chip->base);
	}
	//chip->res = res;

	/* Setup IRQ */
	chip->irq = platform_get_irq(pdev, 0);
	if (chip->irq < 0) {
		dev_err(dev, "No IRQ resource\n");
		return -EINVAL;
	}
	err = devm_request_irq(dev, chip->irq, gorilla_tcon_isr, 0,
				dev_name(dev), chip);
	if (err) {
		dev_err(dev, "Could not request IRQ\n");
		return err;
	}
	tcon_irq_enable(chip);  /* already enabled in de_tcon_init() */

	dev_dbg(dev, "%s(): END\n", __func__);
	return 0;
}

static struct platform_driver gorilla_de_tcon_driver = {
	.probe    = gorilla_de_tcon_probe,
	.driver   = {
		.name  = "gorilla-de-tcon",
		.owner = THIS_MODULE,
		.of_match_table = gorilla_de_tcon_dt_ids,
	},
};

module_platform_driver(gorilla_de_tcon_driver);

MODULE_DESCRIPTION("sunxi gorilla DE/TCON driver");
MODULE_AUTHOR("Martin <wuyan@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
