#ifndef __LINUX_BS83B16C_H
#define __LINUX_BS83B16C_H

#define REG_INTEG	0X0d
#define REG_INTC0	0x0e
#define REG_INTC1	0x0f
#define REG_MFI		0x12

#define REG_PA		0x14
#define REG_PAC		0x15
#define REG_PAPU	0x16
#define REG_PAWU	0x17
#define REG_PXRM	0x18
#define REG_PB		0x20
#define REG_PBC		0x21
#define REG_PBPU	0x22
#define REG_PC		0x38
#define REG_PCC		0x39
#define REG_PCPU	0x3a

/* read for data, write for output level */
#define PA_DATA		0x00
#define PB_DATA		0x01
#define PC_DATA		0x02

/* control input and output */
#define PA_STATE	0x03
#define PB_STATE	0x04
#define PC_STATE	0x05

/* control pull up */
#define PA_PULL		0x06
#define PB_PULL		0x07
#define PC_PULL		0x08

/* control irq enable */
#define PA_INT		0x09
#define PB_INT		0x0a
#define PC_INT		0x0b

#define BIT_EMI		BIT(0)

struct bs83b16c_platform_data {
	unsigned	gpio_base;
	unsigned        n_latch;
	int             (*setup)(struct i2c_client *client,
					int gpio, unsigned ngpio,
					void *context);
	int             (*teardown)(struct i2c_client *client,
					int gpio, unsigned ngpio,
					void *context);
	void            *context;
};

#endif /* __LINUX_BS83B16C_H */
