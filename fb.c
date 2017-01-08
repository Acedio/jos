#include "fb.h"

#include "io.h"

/* The I/O ports */
#define FB_COMMAND_PORT         0x3D4
#define FB_DATA_PORT            0x3D5

/* The I/O port commands */
#define FB_HIGH_BYTE_COMMAND    14
#define FB_LOW_BYTE_COMMAND     15

/* FB dimensions */
#define FB_WIDTH 80
#define FB_HEIGHT 25

char* fb = (char*)0x000B8000;

// TODO: Once we learn how to read back the cursor position, probably remove this.
// cursor pos refers to cell, so needs to be multiplied by two when indexing into fb.
unsigned short cursor_pos = 0;

// 1 bit for blink, 3 bits for bg, 4 bits for fg.
unsigned char fb_color = 0x07;  //light grey on black

void fb_move_cursor(unsigned short pos)
{
  outb(FB_COMMAND_PORT, FB_HIGH_BYTE_COMMAND);
  outb(FB_DATA_PORT,    ((pos >> 8) & 0x00FF));
  outb(FB_COMMAND_PORT, FB_LOW_BYTE_COMMAND);
  outb(FB_DATA_PORT,    pos & 0x00FF);
  cursor_pos = pos;
}

void fb_write_cell_internal(unsigned short pos, char c, unsigned char color) {
  fb[pos * 2] = c;
  fb[pos * 2 + 1] = color;
}

void fb_write_cell(unsigned short pos, char c, unsigned char fg, unsigned char bg) {
  fb_write_cell_internal(pos, c, (bg << 4) | (fg & 0x0F));
}

// Shift up one row. Does not change the cursor location.
void shift_up() {
  int row;
  // First copy the first height - 1 rows from the next row.
  for(row = 0; row < FB_HEIGHT - 1; ++row) {
    // Since each cell is actually two bytes, col is half a cell.
    for (int col = 0; col < FB_WIDTH * 2; ++col) {
      fb[row * FB_WIDTH * 2 + col] = fb[(row + 1) * FB_WIDTH * 2 + col];
    }
  }
  // Then clear the last row.
  for (int col = 0; col < FB_WIDTH; ++col) {
    int pos = row * FB_WIDTH + col;
    fb_write_cell_internal(pos, ' ', fb_color);
  }
}

void fb_set_color(unsigned char fg, unsigned char bg) {
  fb_color = (bg << 4) | (fg & 0x0F);
}

void fb_clear() {
  for(int row = 0; row < FB_HEIGHT; ++row) {
    unsigned short row_off = row * FB_WIDTH;
    for (int col = 0; col < FB_WIDTH; ++col) {
      fb_write_cell_internal(row_off + col, ' ', fb_color);
    }
  }
  fb_move_cursor(0);
}

void fb_putchar(char c) {
  unsigned short new_pos;
  if (c == '\n') {
    // There has to be a quicker way of doing this :P
    new_pos = (cursor_pos/FB_WIDTH + 1) * FB_WIDTH;
  } else {
    fb_write_cell_internal(cursor_pos, c, fb_color);
    new_pos = cursor_pos + 1;
  }
  if (new_pos >= (FB_WIDTH * FB_HEIGHT)) {
    shift_up();
    new_pos -= FB_WIDTH;
  }
  fb_move_cursor(new_pos);
}

void fb_write(const char* buf, unsigned int len) {
  for (unsigned int i = 0; i < len; ++i) {
    fb_putchar(buf[i]);
  }
}

void fb_puts(const char* str) {
  if (!str) {
    return;
  }
  while (*str) {
    fb_putchar(*str);
    ++str;
  }
}
