#include "interrupts.h"

#include "log.h"
#include "string.h"

typedef struct __attribute__((packed)) {
  unsigned short size;  // in bytes, not descriptors
  unsigned int address;
} IDTSpec;

typedef struct __attribute__((packed)) {
  unsigned short offset_low;
  unsigned short segment;
  unsigned short flags;
  unsigned short offset_high;
} InterruptDescriptor;

InterruptDescriptor idt[21];

typedef struct __attribute__((packed)) {
  unsigned int eax;
  unsigned int ebx;
  unsigned int ecx;
  unsigned int edx;
  unsigned int esi;
  unsigned int edi;
  unsigned int ebp;
  unsigned int esp;
} CpuState;

typedef struct __attribute__((packed)) {
  unsigned int error_code;
  unsigned int eip;
  unsigned int cs;
  unsigned int eflags;
} StackState;

void interrupt_handler(CpuState cpu, StackState stack, unsigned int interrupt) {
  LOG(INFO, "interrupt!");
  cpu.eax = cpu.eax;
  stack.error_code = stack.error_code;
  interrupt = interrupt;
}

void populate_interrupt_descriptor(InterruptDescriptor* id, unsigned int fn_addr) {
  id->offset_low = fn_addr & 0xFFFF;
  id->offset_high = (fn_addr >> 16) & 0xFFFF;
  // 0b1000  Present in memory, 0 privilege, constant (0)
  // 0b1111  32 bit trap gate (doesn't prevent other interrupts by default)
  // 0b00000000  Constant/reserved
  id->flags = 0x8F00;
  id->segment = 0x0008;
}

// See interrupts_asm.s
extern void load_idt(IDTSpec* idt);
extern void interrupt_handler_0();
extern void interrupt_handler_1();
extern void interrupt_handler_2();
extern void interrupt_handler_3();
extern void interrupt_handler_4();
extern void interrupt_handler_5();
extern void interrupt_handler_6();
extern void interrupt_handler_7();
extern void interrupt_handler_8();
extern void interrupt_handler_9();
extern void interrupt_handler_10();
extern void interrupt_handler_11();
extern void interrupt_handler_12();
extern void interrupt_handler_13();
extern void interrupt_handler_14();
extern void interrupt_handler_15();
extern void interrupt_handler_16();
extern void interrupt_handler_17();
extern void interrupt_handler_18();
extern void interrupt_handler_19();
extern void interrupt_handler_20();

void init_interrupts() {
  populate_interrupt_descriptor(&idt[0], (unsigned int)interrupt_handler_0);
  populate_interrupt_descriptor(&idt[1], (unsigned int)interrupt_handler_1);
  populate_interrupt_descriptor(&idt[2], (unsigned int)interrupt_handler_2);
  populate_interrupt_descriptor(&idt[3], (unsigned int)interrupt_handler_3);
  populate_interrupt_descriptor(&idt[4], (unsigned int)interrupt_handler_4);
  populate_interrupt_descriptor(&idt[5], (unsigned int)interrupt_handler_5);
  populate_interrupt_descriptor(&idt[6], (unsigned int)interrupt_handler_6);
  populate_interrupt_descriptor(&idt[7], (unsigned int)interrupt_handler_7);
  populate_interrupt_descriptor(&idt[8], (unsigned int)interrupt_handler_8);
  populate_interrupt_descriptor(&idt[9], (unsigned int)interrupt_handler_9);
  populate_interrupt_descriptor(&idt[10], (unsigned int)interrupt_handler_10);
  populate_interrupt_descriptor(&idt[11], (unsigned int)interrupt_handler_11);
  populate_interrupt_descriptor(&idt[12], (unsigned int)interrupt_handler_12);
  populate_interrupt_descriptor(&idt[13], (unsigned int)interrupt_handler_13);
  populate_interrupt_descriptor(&idt[14], (unsigned int)interrupt_handler_14);
  populate_interrupt_descriptor(&idt[15], (unsigned int)interrupt_handler_15);
  populate_interrupt_descriptor(&idt[16], (unsigned int)interrupt_handler_16);
  populate_interrupt_descriptor(&idt[17], (unsigned int)interrupt_handler_17);
  populate_interrupt_descriptor(&idt[18], (unsigned int)interrupt_handler_18);
  populate_interrupt_descriptor(&idt[19], (unsigned int)interrupt_handler_19);
  populate_interrupt_descriptor(&idt[20], (unsigned int)interrupt_handler_20);

  char dec[12];

  IDTSpec idt_spec;
  idt_spec.address = (unsigned int)idt;
  idt_spec.size = sizeof(idt);
  int_to_dec(idt_spec.address, dec);
  LOG(INFO, "idt_spec.address: ");
  LOG(INFO, dec);
  int_to_dec(idt_spec.size, dec);
  LOG(INFO, "idt_spec.size: ");
  LOG(INFO, dec);
  LOG(INFO, "flush");
  load_idt(&idt_spec);
}
