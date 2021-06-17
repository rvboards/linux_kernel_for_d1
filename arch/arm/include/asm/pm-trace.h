/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_ARM_PM_TRACE_H
#define _ASM_ARM_PM_TRACE_H


#define TRACE_RESUME(user)                                                     \
	do {                                                                   \
		if (pm_trace_enabled) {                                        \
			generate_pm_trace(0, user);                            \
		}                                                              \
	} while (0)

#define TRACE_SUSPEND(user)	TRACE_RESUME(user)

#endif /* _ASM_ARM_PM_TRACE_H */
