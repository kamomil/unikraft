/* SPDX-License-Identifier: BSD-2-Clause */
/*-
 * Copyright (c) 2016 Martin Lucina.  All Rights Reserved.
 *
 * Based on rumprun/hw arch/amd64/locore.S, which is:
 * Copyright (c) 2014, 2015 Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <kvm-x86/multiboot_defs.h>
#include <kvm-x86/cpu_x86_64_defs.h>

#define ENTRY(x) .text; .globl x; .type x,%function; x:
#define END(x)   .size x, . - x

#define MYMULTIBOOT_FLAGS \
    (MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_AOUT_KLUDGE)

    /*
     * Exception entry point. This expects an error code/orig_rax on the stack
	 * and the exception handler in %rax.
	 */

#define RAX       80
#define RDI      112
#define ORIG_RAX 120       /* + error_code */
#define RIP      128
#define CS       136
#define RFLAGS   144
#define RSP      152

.macro RESTORE_ALL
movq (%rsp),%r15
movq 1*8(%rsp),%r14
movq 2*8(%rsp),%r13
movq 3*8(%rsp),%r12
movq 4*8(%rsp),%rbp
movq 5*8(%rsp),%rbx
movq 6*8(%rsp),%r11
movq 7*8(%rsp),%r10
movq 8*8(%rsp),%r9
movq 9*8(%rsp),%r8
movq 10*8(%rsp),%rax
movq 11*8(%rsp),%rcx
movq 12*8(%rsp),%rdx
movq 13*8(%rsp),%rsi
movq 14*8(%rsp),%rdi
addq $15*8+8,%rsp
.endm

.macro SAVE_ALL
/* rdi slot contains rax, oldrax contains error code */
cld
subq $14*8,%rsp
movq %rsi,13*8(%rsp)
movq 14*8(%rsp),%rsi	/* load rax from rdi slot */
movq %rdx,12*8(%rsp)
movq %rcx,11*8(%rsp)
movq %rsi,10*8(%rsp)	/* store rax */
movq %r8, 9*8(%rsp)
movq %r9, 8*8(%rsp)
movq %r10,7*8(%rsp)
movq %r11,6*8(%rsp)
movq %rbx,5*8(%rsp)
movq %rbp,4*8(%rsp)
movq %r12,3*8(%rsp)
movq %r13,2*8(%rsp)
movq %r14,1*8(%rsp)
movq %r15,(%rsp)
movq %rdi, RDI(%rsp)	/* put rdi into the slot */
.endm

error_entry:
SAVE_ALL

movq %rsp,%rdi
movq ORIG_RAX(%rsp),%rsi	# get error code
movq $-1,ORIG_RAX(%rsp)
call *%rax
    RESTORE_ALL
    popq %r10
#    add $0x4, %r10
    pushq %r10
    cli
    hlt
forever:
    jmp forever



iretq

/*jmp error_exit*/


.macro zeroentry sym
pushq $0	/* push error code/oldrax */
pushq %rax	/* push real oldrax to the rdi slot */
leaq  \sym(%rip),%rax
jmp error_entry
.endm

.macro errorentry sym
pushq %rax
leaq  \sym(%rip),%rax
jmp error_entry
.endm

ENTRY(coprocessor_error)
    zeroentry do_coprocessor_error
    iret
END(coprocessor_error)

ENTRY(simd_coprocessor_error)
        zeroentry do_simd_coprocessor_error
END(simd_coprocessor_error)

ENTRY(device_not_available)
        zeroentry do_device_not_available
END(device_not_available)

ENTRY(debug)
        zeroentry do_debug
END(debug)

ENTRY(int3)
        zeroentry do_int3
END(int3)

ENTRY(overflow)
        zeroentry do_overflow
END(overflow)

ENTRY(bounds)
        zeroentry do_bounds
END(bounds)

ENTRY(invalid_op)
        zeroentry do_invalid_op
END(invalid_op)

ENTRY(coprocessor_segment_overrun)
        zeroentry do_coprocessor_segment_overrun
END(coprocessor_segment_overrun)

ENTRY(invalid_TSS)
        errorentry do_invalid_TSS
END(invalid_TSS)

ENTRY(segment_not_present)
        errorentry do_segment_not_present
END(segment_not_present)

/* runs on exception stack */
ENTRY(stack_segment)
        errorentry do_stack_segment
END(stack_segment)

ENTRY(general_protection)
        errorentry do_general_protection
END(general_protection)

ENTRY(alignment_check)
        errorentry do_alignment_check
END(alignment_check)

ENTRY(divide_error)
    zeroentry do_divide_error
    iret
END(divide_error)

ENTRY(spurious_interrupt_bug)
        zeroentry do_spurious_interrupt_bug
END(spurious_interrupt_bug)

ENTRY(page_fault)
        errorentry do_page_fault
END(page_fault)

.section .data.multiboot

.align 4
_multiboot_header:
.long MULTIBOOT_HEADER_MAGIC
.long MYMULTIBOOT_FLAGS
.long -(MULTIBOOT_HEADER_MAGIC+MYMULTIBOOT_FLAGS)
.long _multiboot_header
.long 0x100000
.long _edata
.long _ebss
.long _libkvmplat_start32

.section .bss

.space 4096
bootstack:

/*
 * Bootloader entry point.
 *
 * Bootstrap is slightly different from i386.  Multiboot puts us only
 * in 32bit mode, so it's our responsibility to install a page table
 * and switch to long mode.  Notably, we can't call C code until
 * we've switched to long mode.
 */
.code32

ENTRY(_libkvmplat_start32)
	cld
	movl $bootstack, %esp

	/* save multiboot info pointer at top of stack, we pop it in 64bit */
	pushl $0
	pushl %ebx

	/* only multiboot is supported for now */
	cmpl $MULTIBOOT_BOOTLOADER_MAGIC, %eax
	jne nomultiboot

	lgdt (gdt64_ptr)
	pushl $0x0
	pushw $0x10
	pushl $1f
	lret

1:	movl $0x18, %eax
	movl %eax, %ds
	movl %eax, %es
	movl %eax, %ss

	xorl %eax, %eax
	movl %eax, %fs
	movl %eax, %gs

	/*
	 * x86_64 switch to long mode
	 */

	/* 1: enable pae */
	movl %cr4, %eax
	orl $X86_CR4_PAE, %eax
	movl %eax, %cr4

	/* 2: enable long mode */
	movl $0xc0000080, %ecx
	rdmsr
	orl $X86_EFER_LME, %eax
	wrmsr

	/* 3: load pml4 pointer */
	movl $cpu_pml4, %eax
	movl %eax, %cr3

	/* 4: enable paging */
	movl %cr0, %eax
	orl $X86_CR0_PG, %eax
	movl %eax, %cr0

	/* 5: poetically longjump to longmode */
	pushw $0x08
	pushl $_libkvmplat_start64
	lret

	/* NOTREACHED */
	jmp haltme

nomultiboot:

haltme:
	cli
	hlt
	jmp haltme
END(_libkvmplat_start32)

/*
 * amd64 programmer's manual:
 *
 * "In long mode, segmentation is not used ... except for a few exceptions."
 *
 * Uuuyea, exceptions.
 */

.data
.align 64
gdt64:
	.quad 0x0000000000000000
	.quad GDT_DESC_CODE_VAL		/* 64bit CS		*/
	.quad 0x00cf9b000000ffff	/* 32bit CS		*/
	.quad GDT_DESC_DATA_VAL		/* DS			*/
	.quad 0x0000000000000000	/* TSS part 1 (via C)	*/
	.quad 0x0000000000000000	/* TSS part 2 (via C)	*/
gdt64_end:
.align 64

.type gdt64_ptr, @object
gdt64_ptr:
	.word gdt64_end-gdt64-1
	.quad gdt64

.type mxcsr_ptr, @object
mxcsr_ptr:
	.word 0x1f80			/* Intel SDM power-on default */



#include "pagetable.S"

.code64

ENTRY(_libkvmplat_start64)
	movq $bootstack, %rsp
	xorq %rbp, %rbp

	/* enable FPU and SSE units */
	movq %cr0, %rax
	andq $(~X86_CR0_EM), %rax
	orq $(X86_CR0_MP | X86_CR0_NE), %rax
	movq %rax, %cr0
	movq %cr4, %rax
	orq $(X86_CR4_OSXMMEXCPT | X86_CR4_OSFXSR), %rax
	movq %rax, %cr4
	ldmxcsr (mxcsr_ptr)

	/* read multiboot info pointer */
	movq -8(%rsp), %rdi

	pushq $0x0
	pushq $0x0

	call _libkvmplat_entry

	cli
	hlt
END(_libkvmplat_start64)

ENTRY(_libkvmplat_newstack)
	movq %rdi, %rsp
	movq %rdx, %rdi

	pushq $0x0
	pushq $0x0

	call *%rsi

	cli
	hlt
END(_libkvmplat_newstack)
