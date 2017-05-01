#include "interrupts.h"

#include "fb.h"
#include "io.h"
#include "keyboard.h"
#include "log.h"
#include "pic8259.h"
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

InterruptDescriptor idt[48];

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
extern void interrupt_handler_21();
extern void interrupt_handler_22();
extern void interrupt_handler_23();
extern void interrupt_handler_24();
extern void interrupt_handler_25();
extern void interrupt_handler_26();
extern void interrupt_handler_27();
extern void interrupt_handler_28();
extern void interrupt_handler_29();
extern void interrupt_handler_30();
extern void interrupt_handler_31();
extern void interrupt_handler_32();
extern void interrupt_handler_33();
extern void interrupt_handler_34();
extern void interrupt_handler_35();
extern void interrupt_handler_36();
extern void interrupt_handler_37();
extern void interrupt_handler_38();
extern void interrupt_handler_39();
extern void interrupt_handler_40();
extern void interrupt_handler_41();
extern void interrupt_handler_42();
extern void interrupt_handler_43();
extern void interrupt_handler_44();
extern void interrupt_handler_45();
extern void interrupt_handler_46();
extern void interrupt_handler_47();
extern void load_idt(IDTSpec* idt);
extern int reg_cr2();

void interrupt_handler(CpuState cpu, unsigned int interrupt, StackState stack) {
  cpu.eax = cpu.eax;
  stack.error_code = stack.error_code;
  interrupt = interrupt;
  switch (interrupt) {
    case 0x0E:  // page fault
      LOG_HEX(ERROR, "Page fault accessing ", reg_cr2());
      LOG_HEX(ERROR, "Error codes: ", stack.error_code);
      magic_bp();
      break;
    case 0x21:  // keyboard
      PushScancode();
      PicAck(0x21);
      break;
    default:
      LOG_HEX(INFO, "interrupt#: ", interrupt);
  }
}

void init_interrupts() {
  cli();  // disable interrupts
  PicInit();
  PicSetMask(0xFD, 0xFF);

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
  populate_interrupt_descriptor(&idt[21], (unsigned int)interrupt_handler_21);
  populate_interrupt_descriptor(&idt[22], (unsigned int)interrupt_handler_22);
  populate_interrupt_descriptor(&idt[23], (unsigned int)interrupt_handler_23);
  populate_interrupt_descriptor(&idt[24], (unsigned int)interrupt_handler_24);
  populate_interrupt_descriptor(&idt[25], (unsigned int)interrupt_handler_25);
  populate_interrupt_descriptor(&idt[26], (unsigned int)interrupt_handler_26);
  populate_interrupt_descriptor(&idt[27], (unsigned int)interrupt_handler_27);
  populate_interrupt_descriptor(&idt[28], (unsigned int)interrupt_handler_28);
  populate_interrupt_descriptor(&idt[29], (unsigned int)interrupt_handler_29);
  populate_interrupt_descriptor(&idt[30], (unsigned int)interrupt_handler_30);
  populate_interrupt_descriptor(&idt[31], (unsigned int)interrupt_handler_31);
  populate_interrupt_descriptor(&idt[32], (unsigned int)interrupt_handler_32);
  populate_interrupt_descriptor(&idt[33], (unsigned int)interrupt_handler_33);
  populate_interrupt_descriptor(&idt[34], (unsigned int)interrupt_handler_34);
  populate_interrupt_descriptor(&idt[35], (unsigned int)interrupt_handler_35);
  populate_interrupt_descriptor(&idt[36], (unsigned int)interrupt_handler_36);
  populate_interrupt_descriptor(&idt[37], (unsigned int)interrupt_handler_37);
  populate_interrupt_descriptor(&idt[38], (unsigned int)interrupt_handler_38);
  populate_interrupt_descriptor(&idt[39], (unsigned int)interrupt_handler_39);
  populate_interrupt_descriptor(&idt[40], (unsigned int)interrupt_handler_40);
  populate_interrupt_descriptor(&idt[41], (unsigned int)interrupt_handler_41);
  populate_interrupt_descriptor(&idt[42], (unsigned int)interrupt_handler_42);
  populate_interrupt_descriptor(&idt[43], (unsigned int)interrupt_handler_43);
  populate_interrupt_descriptor(&idt[44], (unsigned int)interrupt_handler_44);
  populate_interrupt_descriptor(&idt[45], (unsigned int)interrupt_handler_45);
  populate_interrupt_descriptor(&idt[46], (unsigned int)interrupt_handler_46);
  populate_interrupt_descriptor(&idt[47], (unsigned int)interrupt_handler_47);

  IDTSpec idt_spec;
  idt_spec.address = (unsigned int)idt;
  idt_spec.size = sizeof(idt);
  load_idt(&idt_spec);
  sti();  // enable interrupts
}
