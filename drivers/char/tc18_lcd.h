#ifndef __TC16_LCD_H
#define __TC16_LCD_H

#define LCD_WIDTH 16
#define LCD_HEIGHT 2

#define LCD_MASK0 0xfffffff0
#define LCD_MASK1 0x3fffffff

#define  INST       (unsigned long) 0           /* Instruction register      */
#define  DATA       (unsigned long) 0x80000000  /* Daten register (GPIO32)   */

#define  DSBL       (unsigned long) 0           /* Disable                   */
#define  ENBL       (unsigned long) 0x40000000  /* Enable (GPIO33)           */


/* Addresses of GPIO registers in memory on the PPC 440GR */

#define GPIO_MASK 0x007f81f3

#define GPIO0_BASE      0xef600b00
#define GPIO1_BASE	0xef600c00

#define GPIO0_OR        0x0
#define GPIO0_TCR       0x4
#define GPIO0_OSRH      0xc
#define GPIO0_OSRL      0x8
#define GPIO0_TSRH      0x14
#define GPIO0_TSRL      0x10
#define GPIO0_ODR       0x18
#define GPIO0_IR        0x1c
#define GPIO0_RR1       0x20
#define GPIO0_ISR1H     0x34
#define GPIO0_ISR1L     0x30


#endif

