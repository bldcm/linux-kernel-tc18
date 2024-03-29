/*
 *  linux/arch/x86_64/kernel/head.S -- start in 32bit and switch to 64bit
 *
 *  Copyright (C) 2000 Andrea Arcangeli <andrea@suse.de> SuSE
 *  Copyright (C) 2000 Pavel Machek <pavel@suse.cz>
 *  Copyright (C) 2000 Karsten Keil <kkeil@suse.de>
 *  Copyright (C) 2001,2002 Andi Kleen <ak@suse.de>
 *
 *  $Id$
 */


#include <linux/linkage.h>
#include <linux/threads.h>
#include <asm/desc.h>
#include <asm/segment.h>
#include <asm/page.h>
#include <asm/msr.h>
#include <asm/cache.h>
	
/* we are not able to switch in one step to the final KERNEL ADRESS SPACE
 * because we need identity-mapped pages on setup so define __START_KERNEL to
 * 0x100000 for this stage
 * 
 */

	.text
	.code32
	.globl startup_32
/* %bx:	 1 if coming from smp trampoline on secondary cpu */ 
startup_32:
	
	/*
	 * At this point the CPU runs in 32bit protected mode (CS.D = 1) with
	 * paging disabled and the point of this file is to switch to 64bit
	 * long mode with a kernel mapping for kerneland to jump into the
	 * kernel virtual addresses.
 	 * There is no stack until we set one up.
	 */

	/* Initialize the %ds segment register */
	movl $__KERNEL_DS,%eax
	movl %eax,%ds

	/* Load new GDT with the 64bit segments using 32bit descriptor */
	lgdt	pGDT32 - __START_KERNEL_map

	/* If the CPU doesn't support CPUID this will double fault.
	 * Unfortunately it is hard to check for CPUID without a stack. 
	 */
	
	/* Check if extended functions are implemented */		
	movl	$0x80000000, %eax
	cpuid
	cmpl	$0x80000000, %eax
	jbe	no_long_mode
	/* Check if long mode is implemented */
	mov	$0x80000001, %eax
	cpuid
	btl	$29, %edx
	jnc	no_long_mode

	/*
	 * Prepare for entering 64bits mode
	 */

	/* Enable PAE mode */
	xorl	%eax, %eax
	btsl	$5, %eax
	movl	%eax, %cr4

	/* Setup early boot stage 4 level pagetables */
	movl	$(init_level4_pgt - __START_KERNEL_map), %eax
	movl	%eax, %cr3

	/* Setup EFER (Extended Feature Enable Register) */
	movl	$MSR_EFER, %ecx
	rdmsr

	/* Enable Long Mode */
	btsl	$_EFER_LME, %eax
				
	/* Make changes effective */
	wrmsr

	xorl	%eax, %eax
	btsl	$31, %eax			/* Enable paging and in turn activate Long Mode */
	btsl	$0, %eax			/* Enable protected mode */
	/* Make changes effective */
	movl	%eax, %cr0
	/*
	 * At this point we're in long mode but in 32bit compatibility mode
	 * with EFER.LME = 1, CS.L = 0, CS.D = 1 (and in turn
	 * EFER.LMA = 1). Now we want to jump in 64bit mode, to do that we use
	 * the new gdt/idt that has __KERNEL_CS with CS.L = 1.
	 */
	ljmp	$__KERNEL_CS, $(startup_64 - __START_KERNEL_map)

	.code64
	.org 0x100	
	.globl startup_64
startup_64:
	/* We come here either from startup_32
	 * or directly from a 64bit bootloader.
	 * Since we may have come directly from a bootloader we
	 * reload the page tables here.
	 */

	/* Enable PAE mode and PGE */
	xorq	%rax, %rax
	btsq	$5, %rax
	btsq	$7, %rax
	movq	%rax, %cr4

	/* Setup early boot stage 4 level pagetables. */
	movq	$(init_level4_pgt - __START_KERNEL_map), %rax
	movq	%rax, %cr3

	/* Check if nx is implemented */
	movl	$0x80000001, %eax
	cpuid
	movl	%edx,%edi

	/* Setup EFER (Extended Feature Enable Register) */
	movl	$MSR_EFER, %ecx
	rdmsr

	/* Enable System Call */
	btsl	$_EFER_SCE, %eax

	/* No Execute supported? */
	btl	$20,%edi
	jnc     1f
	btsl	$_EFER_NX, %eax
1:
	/* Make changes effective */
	wrmsr

	/* Setup cr0 */
	xorq	%rax, %rax
	btsq	$31, %rax			/* Enable paging */
	btsq	$0, %rax			/* Enable protected mode */
	btsq	$1, %rax			/* Enable MP */
	btsq	$4, %rax			/* Enable ET */
	btsq	$5, %rax			/* Enable NE */
	btsq	$16, %rax			/* Enable WP */
	btsq	$18, %rax			/* Enable AM */
	/* Make changes effective */
	movq	%rax, %cr0

	/* Setup a boot time stack */
	movq init_rsp(%rip),%rsp

	/* zero EFLAGS after setting rsp */
	pushq $0
	popfq

	/*
	 * We must switch to a new descriptor in kernel space for the GDT
	 * because soon the kernel won't have access anymore to the userspace
	 * addresses where we're currently running on. We have to do that here
	 * because in 32bit we couldn't load a 64bit linear address.
	 */
	lgdt	cpu_gdt_descr

	/* 
	 * Setup up a dummy PDA. this is just for some early bootup code
	 * that does in_interrupt() 
	 */ 
	movl	$MSR_GS_BASE,%ecx
	movq	$empty_zero_page,%rax
	movq    %rax,%rdx
	shrq	$32,%rdx
	wrmsr	

	/* set up data segments. actually 0 would do too */
	movl $__KERNEL_DS,%eax
	movl %eax,%ds	
	movl %eax,%ss
	movl %eax,%es
			
	/* esi is pointer to real mode structure with interesting info.
	   pass it to C */
	movl	%esi, %edi
	
	/* Finally jump to run C code and to be on real kernel address
	 * Since we are running on identity-mapped space we have to jump
	 * to the full 64bit address , this is only possible as indirect
	 * jump
	 */
	movq	initial_code(%rip),%rax
	jmp	*%rax

	/* SMP bootup changes these two */	
	.globl	initial_code
initial_code:
	.quad	x86_64_start_kernel
	.globl init_rsp
init_rsp:
	.quad  init_thread_union+THREAD_SIZE-8

ENTRY(early_idt_handler)
	cmpl $2,early_recursion_flag(%rip)
	jz  1f
	incl early_recursion_flag(%rip)
	xorl %eax,%eax
	movq 8(%rsp),%rsi	# get rip
	movq (%rsp),%rdx
	movq %cr2,%rcx
	leaq early_idt_msg(%rip),%rdi
	call early_printk
	cmpl $2,early_recursion_flag(%rip)
	jz  1f
	call dump_stack
1:	hlt
	jmp 1b
early_recursion_flag:
	.long 0

early_idt_msg:
	.asciz "PANIC: early exception rip %lx error %lx cr2 %lx\n"

.code32
ENTRY(no_long_mode)
	/* This isn't an x86-64 CPU so hang */
1:
	jmp	1b

.org 0xf00
	.globl pGDT32
pGDT32:
	.word	gdt_end-cpu_gdt_table
	.long	cpu_gdt_table-__START_KERNEL_map

.org 0xf10	
ljumpvector:
	.long	startup_64-__START_KERNEL_map
	.word	__KERNEL_CS

ENTRY(stext)
ENTRY(_stext)

	/*
	 * This default setting generates an ident mapping at address 0x100000
	 * and a mapping for the kernel that precisely maps virtual address
	 * 0xffffffff80000000 to physical address 0x000000. (always using
	 * 2Mbyte large pages provided by PAE mode)
	 */
.org 0x1000
ENTRY(init_level4_pgt)
	.quad	0x0000000000102007		/* -> level3_ident_pgt */
	.fill	255,8,0
	.quad	0x000000000010a007
	.fill	254,8,0
	/* (2^48-(2*1024*1024*1024))/(2^39) = 511 */
	.quad	0x0000000000103007		/* -> level3_kernel_pgt */

.org 0x2000
ENTRY(level3_ident_pgt)
	.quad	0x0000000000104007
	.fill	511,8,0

.org 0x3000
ENTRY(level3_kernel_pgt)
	.fill	510,8,0
	/* (2^48-(2*1024*1024*1024)-((2^39)*511))/(2^30) = 510 */
	.quad	0x0000000000105007		/* -> level2_kernel_pgt */
	.fill	1,8,0

.org 0x4000
ENTRY(level2_ident_pgt)
	/* 40MB for bootup. 	*/
	.quad	0x0000000000000283
	.quad	0x0000000000200183
	.quad	0x0000000000400183
	.quad	0x0000000000600183
	.quad	0x0000000000800183
	.quad	0x0000000000A00183
	.quad	0x0000000000C00183
	.quad	0x0000000000E00183
	.quad	0x0000000001000183
	.quad	0x0000000001200183
	.quad	0x0000000001400183
	.quad	0x0000000001600183
	.quad	0x0000000001800183
	.quad	0x0000000001A00183
	.quad	0x0000000001C00183
	.quad	0x0000000001E00183
	.quad	0x0000000002000183
	.quad	0x0000000002200183
	.quad	0x0000000002400183
	.quad	0x0000000002600183
	/* Temporary mappings for the super early allocator in arch/x86_64/mm/init.c */
	.globl temp_boot_pmds
temp_boot_pmds:
	.fill	492,8,0
	
.org 0x5000
ENTRY(level2_kernel_pgt)
	/* 40MB kernel mapping. The kernel code cannot be bigger than that.
	   When you change this change KERNEL_TEXT_SIZE in page.h too. */
	/* (2^48-(2*1024*1024*1024)-((2^39)*511)-((2^30)*510)) = 0 */
	.quad	0x0000000000000183
	.quad	0x0000000000200183
	.quad	0x0000000000400183
	.quad	0x0000000000600183
	.quad	0x0000000000800183
	.quad	0x0000000000A00183
	.quad	0x0000000000C00183
	.quad	0x0000000000E00183
	.quad	0x0000000001000183
	.quad	0x0000000001200183
	.quad	0x0000000001400183
	.quad	0x0000000001600183
	.quad	0x0000000001800183
	.quad	0x0000000001A00183
	.quad	0x0000000001C00183
	.quad	0x0000000001E00183
	.quad	0x0000000002000183
	.quad	0x0000000002200183
	.quad	0x0000000002400183
	.quad	0x0000000002600183
	/* Module mapping starts here */
	.fill	492,8,0

.org 0x6000
ENTRY(empty_zero_page)

.org 0x7000
ENTRY(empty_bad_page)

.org 0x8000
ENTRY(empty_bad_pte_table)

.org 0x9000
ENTRY(empty_bad_pmd_table)

.org 0xa000
ENTRY(level3_physmem_pgt)
	.quad	0x0000000000105007		/* -> level2_kernel_pgt (so that __va works even before pagetable_init) */

	.org 0xb000
#ifdef CONFIG_ACPI_SLEEP
ENTRY(wakeup_level4_pgt)
	.quad	0x0000000000102007		/* -> level3_ident_pgt */
	.fill	255,8,0
	.quad	0x000000000010a007
	.fill	254,8,0
	/* (2^48-(2*1024*1024*1024))/(2^39) = 511 */
	.quad	0x0000000000103007		/* -> level3_kernel_pgt */
#endif

	.data

	.align 16
	.globl cpu_gdt_descr
cpu_gdt_descr:
	.word	gdt_end-cpu_gdt_table
gdt:
	.quad	cpu_gdt_table
#ifdef CONFIG_SMP
	.rept	NR_CPUS-1
	.word	0
	.quad	0
	.endr
#endif

/* We need valid kernel segments for data and code in long mode too
 * IRET will check the segment types  kkeil 2000/10/28
 * Also sysret mandates a special GDT layout 
 */
		 		
.align L1_CACHE_BYTES

/* The TLS descriptors are currently at a different place compared to i386.
   Hopefully nobody expects them at a fixed place (Wine?) */
	
ENTRY(cpu_gdt_table)
	.quad	0x0000000000000000	/* NULL descriptor */
	.quad	0x008f9a000000ffff	/* __KERNEL_COMPAT32_CS */	
	.quad	0x00af9a000000ffff	/* __KERNEL_CS */
	.quad	0x00cf92000000ffff	/* __KERNEL_DS */
	.quad	0x00cffa000000ffff	/* __USER32_CS */
	.quad	0x00cff2000000ffff	/* __USER_DS, __USER32_DS  */		
	.quad	0x00affa000000ffff	/* __USER_CS */
	.quad	0x00cf9a000000ffff	/* __KERNEL32_CS */
	.quad	0,0			/* TSS */
	.quad	0,0			/* LDT */
	.quad   0,0,0			/* three TLS descriptors */ 
	.quad	0x00009a000000ffff	/* __KERNEL16_CS - 16bit PM for S3 wakeup. */
					/* base must be patched for real base address. */
gdt_end:	
	/* asm/segment.h:GDT_ENTRIES must match this */	
	/* This should be a multiple of the cache line size */
	/* GDTs of other CPUs: */	
	.fill (GDT_SIZE * NR_CPUS) - (gdt_end - cpu_gdt_table)

	.align  L1_CACHE_BYTES
ENTRY(idt_table)	
	.rept   256
	.quad   0
	.quad 	0
	.endr

