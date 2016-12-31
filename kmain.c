char* fb = (char*)0x000B8000;

void fb_write_cell(unsigned int i, char c, unsigned char fg, unsigned char bg) {
  fb[i] = c;
  fb[i+1] = (fg << 4) | (bg & 0x0F);
}

int cfunc(int arg1, int arg2, int arg3) {
  fb_write_cell(0, 'O', 1, 5);
  fb_write_cell(2, 'M', 2, 6);
  fb_write_cell(4, 'G', 3, 7);
  fb_write_cell(6, '!', 4, 8);
  return arg1 + 2 * arg2 + 3 * arg3;
}
