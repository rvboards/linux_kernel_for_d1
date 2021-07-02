/* SPDX-License-Identifier: GPL-2.0 */

#include <asm/proc-fns.h>
#include <linux/kernel.h>

void cpu_do_idle(void)
{
	__asm__ __volatile__ ("wfi");

}
