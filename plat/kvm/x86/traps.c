/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* Taken from Mini-OS */

/* 16-byte gate - Long mode IDT entry. */
#include <uk/essentials.h>
#include <uk/arch/types.h>
#include <stdint.h>
#include <kvm-x86/cpu_x86_64_defs.h>
#include <string.h>
#include <kvm-x86/cpu_x86_64.h>

struct __packed seg_gate64 {
    union {
        struct {
            uint64_t lo, hi;
        };
        struct {
            uint16_t offset0;
            uint16_t selector;
            unsigned ist: 3, _r0: 5, type: 4, s: 1, dpl: 2, p: 1;
            uint16_t offset1;
            uint32_t offset2;
            uint32_t _r1;
        };
    };
};

struct seg_gate64 idt[256] = { };

#include <kvm-x86/traps.h>
#include <kvm-x86/os.h>
#include <uk/print.h>
#include <uk/arch/limits.h>

/*
 * These are assembler stubs in entry.S.
 * They are the actual entry points for virtual exceptions.
 */
void divide_error(void);
void debug(void);
void int3(void);
void overflow(void);
void bounds(void);
void invalid_op(void);
void device_not_available(void);
void coprocessor_segment_overrun(void);
void invalid_TSS(void);
void segment_not_present(void);
void stack_segment(void);
void general_protection(void);
void page_fault(void);
void coprocessor_error(void);
void simd_coprocessor_error(void);
void alignment_check(void);
void spurious_interrupt_bug(void);
void machine_check(void);

#define do_exit()                                                              \
	for (;;) {                                                             \
	}

void dump_regs(struct __regs *regs)
{
// uk_printk("Thread: %s\n", current ? current->name : "*NONE*");
#ifdef __X86_64__
	uk_printk("RIP: %04lx:[<%016lx>] ", regs->cs & 0xffff, regs->rip);
	uk_printk("\nRSP: %04lx:%016lx  EFLAGS: %08lx\n", regs->ss, regs->rsp,
		  regs->eflags);
	uk_printk("RAX: %016lx RBX: %016lx RCX: %016lx\n", regs->rax, regs->rbx,
		  regs->rcx);
	uk_printk("RDX: %016lx RSI: %016lx RDI: %016lx\n", regs->rdx, regs->rsi,
		  regs->rdi);
	uk_printk("RBP: %016lx R08: %016lx R09: %016lx\n", regs->rbp, regs->r8,
		  regs->r9);
	uk_printk("R10: %016lx R11: %016lx R12: %016lx\n", regs->r10, regs->r11,
		  regs->r12);
	uk_printk("R13: %016lx R14: %016lx R15: %016lx\n", regs->r13, regs->r14,
		  regs->r15);
#else
	uk_printk("EIP: %lx, EFLAGS %lx.\n", regs->eip, regs->eflags);
	uk_printk("EBX: %08lx ECX: %08lx EDX: %08lx\n", regs->ebx, regs->ecx,
		  regs->edx);
	uk_printk("ESI: %08lx EDI: %08lx EBP: %08lx EAX: %08lx\n", regs->esi,
		  regs->edi, regs->ebp, regs->eax);
	uk_printk("DS: %04x ES: %04x orig_eax: %08lx, eip: %08lx\n", regs->xds,
		  regs->xes, regs->orig_eax, regs->eip);
	uk_printk("CS: %04x EFLAGS: %08lx esp: %08lx ss: %04x\n", regs->xcs,
		  regs->eflags, regs->esp, regs->xss);
#endif
}

static void do_trap(int trapnr, char *str, struct __regs *regs,
		    unsigned long error_code)
{
	uk_printk("FATAL:  Unhandled Trap %d (%s), error code=0x%lx\n", trapnr,
		  str, error_code);
	uk_printk("Regs address %p\n", regs);
    long l;
    asm("\t movq %%rbp,%0" : "=r"(l));
    printf("rbp = 0x%x\n",l);
	dump_regs(regs);
}

#define DO_ERROR(trapnr, str, name)                                            \
	void do_##name(struct __regs *regs, unsigned long error_code)          \
	{                                                                      \
		do_trap(trapnr, str, regs, error_code);                        \
	}

#define DO_ERROR_INFO(trapnr, str, name, sicode, siaddr)                       \
	void do_##name(struct __regs *regs, unsigned long error_code)          \
	{                                                                      \
		do_trap(trapnr, str, regs, error_code);                        \
	}

DO_ERROR_INFO(0, "divide error", divide_error, FPE_INTDIV, regs->eip)
DO_ERROR(3, "int3", int3)
DO_ERROR(4, "overflow", overflow)
DO_ERROR(5, "bounds", bounds)
DO_ERROR_INFO(6, "invalid operand", invalid_op, ILL_ILLOPN, regs->eip)
DO_ERROR(7, "device not available", device_not_available)
DO_ERROR(9, "coprocessor segment overrun", coprocessor_segment_overrun)
DO_ERROR(10, "invalid TSS", invalid_TSS)
DO_ERROR(11, "segment not present", segment_not_present)
DO_ERROR(12, "stack segment", stack_segment)
DO_ERROR_INFO(17, "alignment check", alignment_check, BUS_ADRALN, 0)
DO_ERROR(18, "machine check", machine_check)

static void do_stack_walk(unsigned long frame_base)
{
	unsigned long *frame = (void *)frame_base;

	uk_printk("base is %#lx ", frame_base);
	uk_printk("caller is %#lx\n", frame[1]);
	if (frame[0])
		do_stack_walk(frame[0]);
}

void stack_walk(void)
{
	unsigned long bp;
#ifdef __x86_64__
	asm("movq %%rbp, %0" : "=r"(bp));
#else
	asm("movl %%ebp, %0" : "=r"(bp));
#endif
	do_stack_walk(bp);
}

static void dump_mem(unsigned long addr)
{
	unsigned long i;

	if (addr < __PAGE_SIZE)
		return;

	for (i = ((addr)-16) & ~15; i < (((addr) + 48) & ~15); i++) {
		if (!(i % 16))
			uk_printk("\n%lx:", i);
		uk_printk(" %02x", *(unsigned char *)i);
	}
	uk_printk("\n");
}

static int handling_pg_fault;

void do_page_fault(struct __regs *regs, unsigned long error_code)
{
	unsigned long addr = 0;//read_cr2();
	//struct sched_shutdown sched_shutdown = {.reason = SHUTDOWN_crash};

	/* If we are already handling a page fault, and got another one
	 * that means we faulted in pagetable walk. Continuing here would cause
	 * a recursive fault
	 */
	if (handling_pg_fault == 1) {
		uk_printk("Page fault in pagetable walk (access to invalid memory?).\n");
		//HYPERVISOR_sched_op(SCHEDOP_shutdown, &sched_shutdown);
	}
	handling_pg_fault++;
	barrier();

#ifdef __X86_64__
	uk_printk("Page fault at linear address %lx, rip %lx, regs %p, sp %lx, our_sp %p, code %lx\n",
		  addr, regs->rip, regs, regs->rsp, &addr, error_code);
#else
	uk_printk("Page fault at linear address %lx, eip %lx, regs %p, sp %lx, our_sp %p, code %lx\n",
		  addr, regs->eip, regs, regs->esp, &addr, error_code);
#endif

	dump_regs(regs);
#ifdef __X86_64__
	do_stack_walk(regs->rbp);
	dump_mem(regs->rsp);
	dump_mem(regs->rbp);
	dump_mem(regs->rip);
#else
	do_stack_walk(regs->ebp);
	dump_mem(regs->esp);
	dump_mem(regs->ebp);
	dump_mem(regs->eip);
#endif
	//HYPERVISOR_sched_op(SCHEDOP_shutdown, &sched_shutdown);
	/* We should never get here ... but still */
	handling_pg_fault--;
}

void do_general_protection(struct __regs *regs, long error_code)
{
	//struct sched_shutdown sched_shutdown = {.reason = SHUTDOWN_crash};
#ifdef __X86_64__
	uk_printk("GPF rip: %lx, error_code=%lx\n", regs->rip, error_code);
#else
	uk_printk("GPF eip: %lx, error_code=%lx\n", regs->eip, error_code);
#endif
	dump_regs(regs);
#ifdef __X86_64__
	do_stack_walk(regs->rbp);
	dump_mem(regs->rsp);
	dump_mem(regs->rbp);
	dump_mem(regs->rip);
#else
	do_stack_walk(regs->ebp);
	dump_mem(regs->esp);
	dump_mem(regs->ebp);
	dump_mem(regs->eip);
#endif
	//HYPERVISOR_sched_op(SCHEDOP_shutdown, &sched_shutdown);
}

void do_debug(struct __regs *regs)
{
	uk_printk("Debug exception\n");
#define TF_MASK 0x100
	regs->eflags &= ~TF_MASK;
	dump_regs(regs);
	do_exit();
}

void do_coprocessor_error(struct __regs *regs)
{
	uk_printk("Copro error\n");
	dump_regs(regs);
	do_exit();
}

void simd_math_error(void *eip __unused)
{
	uk_printk("SIMD error\n");
}

void do_simd_coprocessor_error(struct __regs *regs __unused)
{
	uk_printk("SIMD copro error\n");
}

void do_spurious_interrupt_bug(struct __regs *regs __unused)
{
}

/* Assembler interface fns in entry.S. */
//void hypervisor_callback(void);
void failsafe_callback(void);

#define INTR_STACK_SIZE __PAGE_SIZE
static unsigned char intr_stack[INTR_STACK_SIZE] __attribute__((aligned(16)));

/*
hw_tss tss __attribute__((aligned(16))) = {
#ifdef __X86_64__
	.rsp0 = (unsigned long)&intr_stack[INTR_STACK_SIZE],
#else
	.esp0 = (unsigned long)&intr_stack[INTR_STACK_SIZE],
	.ss0 = __KERN_DS,
#endif
	.iopb = X86_TSS_INVALID_IO_BITMAP,
};
*/

/* 16-byte gate - Long mode IDT entry. */

/*
struct __packed seg_gate64 {
    union {
        struct {
            uint64_t lo, hi;
        };
        struct {
            uint16_t offset0;
            uint16_t selector;
            unsigned ist: 3, _r0: 5, type: 4, s: 1, dpl: 2, p: 1;
            uint16_t offset1;
            uint32_t offset2;
            uint32_t _r1;
        };
    };
};
*/

static void setup_gate(unsigned int entry, void *addr, unsigned int dpl)
{
	idt[entry].offset0 = (unsigned long)addr & 0xffff;
	idt[entry].selector = GDT_DESC_CODE * 8;
	idt[entry]._r0 = 0;
	idt[entry].type = 15;
	idt[entry].s = 0;
	idt[entry].dpl = dpl;
	idt[entry].p = 1;
	idt[entry].offset1 = ((unsigned long)addr >> 16) & 0xffff;
#ifdef __X86_64__
	idt[entry].ist = 0;
	idt[entry].offset2 = ((unsigned long)addr >> 32) & 0xffffffffu;
	idt[entry]._r1 = 0;
#endif
}

/*
  #define TRAP_divide_error      0
  #define TRAP_debug             1
  #define TRAP_nmi               2
  #define TRAP_int3              3
  #define TRAP_overflow          4
  #define TRAP_bounds            5
  #define TRAP_invalid_op        6
  #define TRAP_no_device         7
  #define TRAP_double_fault      8
  #define TRAP_copro_seg         9
  #define TRAP_invalid_tss      10
  #define TRAP_no_segment       11
  #define TRAP_stack_error      12
  #define TRAP_gp_fault         13
  #define TRAP_page_fault       14
  #define TRAP_spurious_int     15
  #define TRAP_copro_error      16
  #define TRAP_alignment_check  17
  #define TRAP_machine_check    18
  #define TRAP_simd_error       19
  #define TRAP_deferred_nmi     31
  #define TRAP_xen_callback     32

 */


void trap_init(void)
{

    memset(idt,0,sizeof(idt));

	setup_gate(TRAP_divide_error, &divide_error, 0);
	setup_gate(TRAP_debug, &debug, 0);
	setup_gate(TRAP_int3, &int3, 3);
	setup_gate(TRAP_overflow, &overflow, 3);
	setup_gate(TRAP_bounds, &bounds, 0);
	setup_gate(TRAP_invalid_op, &invalid_op, 0);
	setup_gate(TRAP_no_device, &device_not_available, 0);
	setup_gate(TRAP_copro_seg, &coprocessor_segment_overrun, 0);
	setup_gate(TRAP_invalid_tss, &invalid_TSS, 0);
	setup_gate(TRAP_no_segment, &segment_not_present, 0);
	setup_gate(TRAP_stack_error, &stack_segment, 0);
	setup_gate(TRAP_gp_fault, &general_protection, 0);
	setup_gate(TRAP_page_fault, &page_fault, 0);
	setup_gate(TRAP_spurious_int, &spurious_interrupt_bug, 0);
	setup_gate(TRAP_copro_error, &coprocessor_error, 0);
	setup_gate(TRAP_alignment_check, &alignment_check, 0);
	setup_gate(TRAP_simd_error, &simd_coprocessor_error, 0);
	//setup_gate(TRAP_xen_callback, hypervisor_callback, 0);


    /*
      struct gdtptr {
      uint16_t limit;
      uint64_t base;
      } __packed;

     */
    volatile struct gdtptr idtptr;

	idtptr.limit = sizeof(idt) - 1;
	idtptr.base = (uint64_t)&idt;
	__asm__ __volatile__("lidt (%0)" ::"r"(&idtptr));

    uk_printk("setting idt %p %p\n",&idtptr,&idt);

    /*
	gdt[GDTE_TSS] =
	    (typeof(*gdt))INIT_GDTE((unsigned long)&tss, 0x67, 0x89);
	asm volatile("ltr %w0" ::"rm"(GDTE_TSS * 8));

	if (hvm_set_parameter(HVM_PARAM_CALLBACK_IRQ,
			      (2ULL << 56) | TRAP_xen_callback)) {
		uk_printk("Request for Xen HVM callback vector failed\n");
		do_exit();
	}
    */
}

void trap_fini(void)
{
}
