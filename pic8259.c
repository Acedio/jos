#include "pic8259.h"

#include "io.h"
#include "log.h"
#include "string.h"

#define PIC1_BASE    0x20    /* IO base address for master PIC */
#define PIC2_BASE    0xA0    /* IO base address for slave PIC */
#define PIC1_COMMAND  PIC1_BASE
#define PIC1_DATA  (PIC1_BASE+1)
#define PIC2_COMMAND  PIC2_BASE
#define PIC2_DATA  (PIC2_BASE+1)

#define ICW1_ICW4  0x01    /* ICW4 (not) needed */
#define ICW1_SINGLE  0x02    /* Single (cascade) mode */
#define ICW1_INTERVAL4  0x04    /* Call address interval 4 (8) */
#define ICW1_LEVEL  0x08    /* Level triggered (edge) mode */
#define ICW1_INIT  0x10    /* Initialization - required! */
 
#define ICW4_8086  0x01    /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO  0x02    /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE  0x08    /* Buffered mode/slave */
#define ICW4_BUF_MASTER  0x0C    /* Buffered mode/master */
#define ICW4_SFNM  0x10    /* Special fully nested (not) */
 
#define PIC_ACK     0x20

/*
reinitialize the PIC controllers, giving them specified vector offsets
rather than 8h and 70h, as configured by default
arguments:
  offset1 - vector offset for master PIC
    vectors on the master become offset1..offset1+7
  offset2 - same for slave PIC: offset2..offset2+7
*/
void PicRemap(int offset1, int offset2)
{
  unsigned char a1, a2;
 
  a1 = inb(PIC1_DATA);                        // save masks
  a2 = inb(PIC2_DATA);

  outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
  outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
  outb(PIC1_DATA, offset1);                 // ICW2: Master PIC vector offset
  outb(PIC2_DATA, offset2);                 // ICW2: Slave PIC vector offset
  outb(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
  outb(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
 
  outb(PIC1_DATA, ICW4_8086);
  outb(PIC2_DATA, ICW4_8086);
 
  outb(PIC1_DATA, a1);   // restore saved masks.
  outb(PIC2_DATA, a2);
}

void PicSetMask(unsigned char mask1, unsigned char mask2) {
  outb(PIC1_DATA, mask1);
  outb(PIC2_DATA, mask2);
}

void PicInit() {
  PicRemap(PIC1_START_INTERRUPT, PIC2_START_INTERRUPT);
}

/** PicAck:
 *  Acknowledges an interrupt from either PIC 1 or PIC 2.
 *
 *  @param num The number of the interrupt
 */
void PicAck(unsigned int interrupt)
{
  if (interrupt < PIC1_START_INTERRUPT || interrupt > PIC2_END_INTERRUPT) {
    return;
  }

  if (interrupt < PIC2_START_INTERRUPT) {
    outb(PIC1_COMMAND, PIC_ACK);
  } else {
    outb(PIC2_COMMAND, PIC_ACK);
  }
}
