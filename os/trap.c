#include "trap.h"
#include "defs.h"
#include "loader.h"
#include "syscall.h"

// trampoline和uservec两个符号在trampoline.S定义
extern char trampoline[], uservec[], boot_stack_top[];
extern void *userret(uint64);

// set up to take exceptions and traps while in the kernel.
// uservec函数位于trampoline.S中，实现内核栈和用户栈的切换
void trap_init(void)
{	
	// 设置kernel处理trap的函数
	// 在 Trap 触发的一瞬间， CPU 就会切换到 S 特权级并跳转到 stvec 所指示的位置
	w_stvec((uint64)uservec & ~0x3); // 写 stvec, 最后两位表明跳转模式，该实验始终为 0
}

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void usertrap(struct trapframe *trapframe)
{
	if ((r_sstatus() & SSTATUS_SPP) != 0)
		panic("usertrap: not from user mode");

	uint64 cause = r_scause();
	if (cause == UserEnvCall) {
		trapframe->epc += 4;
		syscall();
		return usertrapret(trapframe, (uint64)boot_stack_top);
	}
	switch (cause) {
	case StoreMisaligned:
	case StorePageFault:
	case LoadMisaligned:
	case LoadPageFault:
	case InstructionMisaligned:
	case InstructionPageFault:
		errorf("%d in application, bad addr = %p, bad instruction = %p, core "
		       "dumped.",
		       cause, r_stval(), trapframe->epc);
		break;
	case IllegalInstruction:
		errorf("IllegalInstruction in application, epc = %p, core dumped.",
		       trapframe->epc);
		break;
	default:
		errorf("unknown trap: %p, stval = %p sepc = %p", r_scause(),
		       r_stval(), r_sepc());
		break;
	}
	infof("switch to next app");
	run_next_app();
	printf("ALL DONE\n");
	shutdown();
}

//
// return to user space
//
// 从S态返回U态是由 usertrapret 函数实现的
void usertrapret(struct trapframe *trapframe, uint64 kstack)
{
	trapframe->kernel_satp = r_satp(); // kernel page table
	trapframe->kernel_sp = kstack + PGSIZE; // process's kernel stack
	trapframe->kernel_trap = (uint64)usertrap;
	trapframe->kernel_hartid = r_tp(); // hartid for cpuid()

	w_sepc(trapframe->epc);
	// set up the registers that trampoline.S's sret will use
	// to get to user space.

	// set S Previous Privilege mode to User.
	uint64 x = r_sstatus();
	x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
	x |= SSTATUS_SPIE; // enable interrupts in user mode
	w_sstatus(x);

	// tell trampoline.S the user page table to switch to.
	// uint64 satp = MAKE_SATP(p->pagetable);
	userret((uint64)trapframe);
}