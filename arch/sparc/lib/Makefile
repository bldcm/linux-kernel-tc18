# $Id$
# Makefile for Sparc library files..
#

EXTRA_AFLAGS := -ansi -DST_DIV0=0x02

lib-y := mul.o rem.o sdiv.o udiv.o umul.o urem.o ashrdi3.o memcpy.o memset.o \
         strlen.o checksum.o blockops.o memscan.o memcmp.o strncmp.o \
	 strncpy_from_user.o divdi3.o udivdi3.o strlen_user.o \
	 copy_user.o locks.o atomic.o atomic32.o bitops.o \
	 lshrdi3.o ashldi3.o rwsem.o muldi3.o bitext.o

lib-$(CONFIG_DEBUG_SPINLOCK) +=	debuglocks.o
