#include "../../include/isr.h"

#include<stddef.h>

ISRHandler _isrHandlers[256];

static const char* const _exceptions[] = {
    "Divide by zero Ady-error",           "Ady-Debug",
    "Non-maskable Ady-Interrupt",         "Ady-Breakpoint",
    "Ady-Overflow",                       "Ady-Bound Range Exceeded",
    "Invalid Ady-Opcode",                 "Ady-Device Not Available",
    "Double Ady-Fault",                   "Coprocessor Ady-Segment Overrun",
    "Invalid Ady-TSS",                    "Ady-Segment Not Present",
    "Ady-Stack-Segment Fault",            "General Ady-Protection Fault",
    "Ady-Page Fault", "",                 "x87 Floating-Point Ady-Exception",
    "Ady-Alignment Check",                "Ady-Machine Check",
    "SIMD Floating-Point Ady-Exception",  "Virtualization Ady-Exception",
    "Control Protection Ady-Exception ",  "", "", "", "", "", "",
    "Hypervisor Injection Ady-Exception", "VMM Communication Ady-Exception",
    "Security Ady-Exception", ""
};

void i386_ISR_InitializeGates();

void i386_isr_initialize() {
    i386_ISR_InitializeGates();
    for (int i = 0; i < 256; i++)
        i386_idt_enableGate(i);
}

void __attribute__((cdecl)) i386_isr_handler(Registers* regs) {
    if (_isrHandlers[regs->interrupt] != NULL) _isrHandlers[regs->interrupt](regs);
    else if (regs->interrupt > 32) kprintf("Unhandled interrupt! Interrupt: %d\n", regs->interrupt);
    else {
        kprintf("Unhandled exception %d %s\n", regs->interrupt, _exceptions[regs->interrupt]);
        kprintf("  eax=%x  ebx=%x  ecx=%x  edx=%x  esi=%x  edi=%x\n",
               regs->eax, regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);
        kprintf("  esp=%x  ebp=%x  eip=%x  eflags=%x  cs=%x  ds=%x  ss=%x\n",
               regs->esp, regs->ebp, regs->eip, regs->eflag, regs->cs, regs->ds, regs->ss);
        kprintf("  interrupt=%x  errorcode=%x\n", regs->interrupt, regs->error);
        kernel_panic("\nKERNEL PANIC\n");
    }
}

void i386_isr_registerHandler(int interrupt, ISRHandler handler) {
    _isrHandlers[interrupt] = handler;
    i386_idt_enableGate(interrupt);
}