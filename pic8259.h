#ifndef PIC8259_H
#define PIC8259_H

/* The PIC interrupts have been remapped */
#define PIC1_START_INTERRUPT 0x20
#define PIC2_START_INTERRUPT 0x28
#define PIC2_END_INTERRUPT   PIC2_START_INTERRUPT + 7

void PicInit();

void PicSetMask(unsigned char mask1, unsigned char mask2);

void PicAck(unsigned int interrupt);

#endif  // PIC8259_H
