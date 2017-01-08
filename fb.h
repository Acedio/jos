// pos is the actual position, so will be multiplied by 2 before being used as
// an index.
void fb_write_cell(unsigned short pos, char c, unsigned char fg,
                   unsigned char bg);

/** fb_move_cursor:
 *  Moves the cursor of the framebuffer to the given position
 *
 *  @param pos The new position of the cursor (in cells, not bytes)
 */
void fb_move_cursor(unsigned short pos);

// Black	0	Red	4	Dark grey	8	Light red	12
// Blue	1	Magenta	5	Light blue	9	Light magenta	13
// Green	2	Brown	6	Light green	10	Light brown	14
// Cyan	3	Light grey	7	Light cyan	11	White	15
//
// Looks like using values >7 for bg makes it blink, annoyingly :P
void fb_set_color(unsigned char fg, unsigned char bg);

// Clears FB to ' ' in the current color.
void fb_clear();

void fb_write(const char* buf, unsigned int len);

void fb_puts(const char* str);
