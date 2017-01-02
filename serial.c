#include "io.h" /* io.h is implement in the section "Moving the cursor" */

/* The I/O ports */

/* All the I/O ports are calculated relative to the data port. This is because
 * all serial ports (COM1, COM2, COM3, COM4) have their ports in the same
 * order, but they start at different values.
 */

#define SERIAL_COM1_BASE                0x3F8      /* COM1 base port */

#define SERIAL_DATA_PORT(base)          (base)
#define SERIAL_FIFO_COMMAND_PORT(base)  (base + 2)
#define SERIAL_LINE_COMMAND_PORT(base)  (base + 3)
#define SERIAL_MODEM_COMMAND_PORT(base) (base + 4)
#define SERIAL_LINE_STATUS_PORT(base)   (base + 5)

/* The I/O port commands */

/* SERIAL_LINE_ENABLE_DLAB:
 * Tells the serial port to expect first the highest 8 bits on the data port,
 * then the lowest 8 bits will follow
 * DLAB = Divisor Latch Access Bit
 */
#define SERIAL_LINE_ENABLE_DLAB         0x80

/** serial_configure_baud_rate:
 *  Sets the speed of the data being sent. The default speed of a serial
 *  port is 115200 bits/s. The argument is a divisor of that number, hence
 *  the resulting speed becomes (115200 / divisor) bits/s.
 *
 *  @param com      The base address of the COM port to configure
 *  @param divisor  The divisor
 */
void serial_configure_baud_rate(unsigned short com, unsigned short divisor) {
  // Enable DLAB so we can set the divisor.
  outb(SERIAL_LINE_COMMAND_PORT(com),
       SERIAL_LINE_ENABLE_DLAB);
  // I think this was incorrectly pushing both the MSB and LSB to DATA_PORT.
  // According to http://wiki.osdev.org/Serial_Ports DATA_PORT is for the LSB
  // and DATA_PORT+1 is for the MSB. As long as the MSB was 0 (i.e. for small
  // divisors) this would work by chance. Fixed.
  outb(SERIAL_DATA_PORT(com) + 1,
       (divisor >> 8) & 0x00FF);
  outb(SERIAL_DATA_PORT(com),
       divisor & 0x00FF);
}

/** serial_configure_line:
 *  Configures the line of the given serial port. The port is set to have a
 *  data length of 8 bits, no parity bits, one stop bit and break control
 *  disabled. Also sets up the FIFO buffers as 14 bytes and clears the send and
 *  receive queues. This also permanently sets RTS and DTR.
 *
 *  @param com  The serial port to configure
 */
void serial_configure_line(unsigned short com)
{
  /* Bit:     | 7 | 6 | 5 4 3 | 2 | 1 0 |
   * Content: | d | b | prty  | s | dl  |
   * Value:   | 0 | 0 | 0 0 0 | 0 | 1 1 | = 0x03
   */
  // Data lenth of 8 bits, no parity bits, one stop bit.
  outb(SERIAL_LINE_COMMAND_PORT(com), 0x03);
  // Enable 14 byte FIFOs, clear the send and receive queues.
  outb(SERIAL_FIFO_COMMAND_PORT(com), 0xC7);
  // Permanently set RTS and DTR so we're always sending.
  outb(SERIAL_MODEM_COMMAND_PORT(com), 0x03);
}

int serial_is_transmit_fifo_empty(unsigned short com) {
  return inb(SERIAL_LINE_STATUS_PORT(com)) & 0x20;
}

void serial_init() {
  serial_configure_baud_rate(SERIAL_COM1_BASE, 4);
  serial_configure_line(SERIAL_COM1_BASE);
}

void serial_write_internal(unsigned short com, const char* buf, unsigned int len) {
  for (unsigned int i = 0; i < len; ++i) {
    while (!serial_is_transmit_fifo_empty(com)) {
      // thumbs.twiddle();
    }
    outb(SERIAL_DATA_PORT(com), buf[i]);
  }
}

void serial_write(const char* buf, unsigned int len) {
  serial_write_internal(SERIAL_COM1_BASE, buf, len);
}

void serial_puts_internal(unsigned short com, const char* str) {
  while (*str != '\0') {
    while (!serial_is_transmit_fifo_empty(com)) {
      // thumbs.twiddle();
    }
    outb(SERIAL_DATA_PORT(com), *str);
    ++str;
  }
}

void serial_puts(const char* str) {
  serial_puts_internal(SERIAL_COM1_BASE, str);
}
