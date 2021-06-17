/*
 * Generic definitions for Allwinner SunXi SoCs
 *
 * Copyright (C) 2012 Maxime Ripard
 *
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __MACH_SUNXI_H
#define __MACH_SUNXI_H

#if defined(CONFIG_ARCH_SUN8IW15P1)
#define ARISC_MESSAGE_POOL_PBASE  (0x48105000)
#define ARISC_MESSAGE_POOL_RANGE  (0x1000)
#endif

#if !defined(SUNXI_UART_PBASE)
#define SUNXI_UART_PBASE		CONFIG_DEBUG_UART_PHYS
#define SUNXI_UART_SIZE			UL(0x2000)
#endif

#if !defined(UARTIO_ADDRESS)
#define UARTIO_ADDRESS(x)  IOMEM((x) + 0xf0000000)
#endif

#endif /* __MACH_SUNXI_H */
