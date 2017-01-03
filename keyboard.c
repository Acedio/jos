#include "io.h"
#include "keyboard.h"

char ScancodeToAscii(unsigned char scancode) {
  switch (scancode) {
    case 0x02:
      return '1';
    case 0x03:
      return '2';
    case 0x04:
      return '3';
    case 0x05:
      return '4';
    case 0x06:
      return '5';
    case 0x07:
      return '6';
    case 0x08:
      return '7';
    case 0x09:
      return '8';
    case 0x0a:
      return '9';
    case 0x0b:
      return '0';
    case 0x0c:
      return '-';
    case 0x0d:
      return '=';
//    case 0x0e:
//      return 'Backspace';
    case 0x0f:
      return '\t';
    case 0x10:
      return 'Q';
    case 0x11:
      return 'W';
    case 0x12:
      return 'E';
    case 0x13:
      return 'R';
    case 0x14:
      return 'T';
    case 0x15:
      return 'Y';
    case 0x16:
      return 'U';
    case 0x17:
      return 'I';
    case 0x18:
      return 'O';
    case 0x19:
      return 'P';
    case 0x1a:
      return '[';
    case 0x1b:
      return ']';
    case 0x1c:
      return '\n';
//    case 0x1d:
//      return 'LCtrl';
    case 0x1e:
      return 'A';
    case 0x1f:
      return 'S';
    case 0x20:
      return 'D';
    case 0x21:
      return 'F';
    case 0x22:
      return 'G';
    case 0x23:
      return 'H';
    case 0x24:
      return 'J';
    case 0x25:
      return 'K';
    case 0x26:
      return 'L';
    case 0x27:
      return ';';
    case 0x28:
      return '\'';
    case 0x29:
      return '`';
//    case 0x2a:
//      return 'LShift';
    case 0x2b:
      return '\\';
    case 0x2c:
      return 'Z';
    case 0x2d:
      return 'X';
    case 0x2e:
      return 'C';
    case 0x2f:
      return 'V';
    case 0x30:
      return 'B';
    case 0x31:
      return 'N';
    case 0x32:
      return 'M';
    case 0x33:
      return ',';
    case 0x34:
      return '.';
    case 0x35:
      return '/';
//    case 0x36:
//      return 'RShift';
//    case 0x37:
//      return 'Keypad-*) or (*/PrtScn) on a 83/84-key keyboard
//    case 0x38:
//      return 'LAlt';
    case 0x39:
      return ' ';
//    case 0x3a:
//      return 'CapsLock';
//    case 0x3b:
//      return 'F1';
//    case 0x3c:
//      return 'F2';
//    case 0x3d:
//      return 'F3';
//    case 0x3e:
//      return 'F4';
//    case 0x3f:
//      return 'F5';
//    case 0x40:
//      return 'F6';
//    case 0x41:
//      return 'F7';
//    case 0x42:
//      return 'F8';
//    case 0x43:
//      return 'F9';
//    case 0x44:
//      return 'F10';
//    case 0x45:
//      return 'NumLock';
//    case 0x46:
//      return 'ScrollLock';
//    case 0x47:
//      return 'Keypad-7/Home';
//    case 0x48:
//      return 'Keypad-8/Up';
//    case 0x49:
//      return 'Keypad-9/PgUp';
//    case 0x4a:
//      return 'Keypad--';
//    case 0x4b:
//      return 'Keypad-4/Left';
//    case 0x4c:
//      return 'Keypad-5';
//    case 0x4d:
//      return 'Keypad-6/Right';
//    case 0x4e:
//      return 'Keypad-+';
//    case 0x4f:
//      return 'Keypad-1/End';
//    case 0x50:
//      return 'Keypad-2/Down';
//    case 0x51:
//      return 'Keypad-3/PgDn';
//    case 0x52:
//      return 'Keypad-0/Ins';
//    case 0x53:
//      return 'Keypad-./Del';
//    case 0x54:
//      return 'Alt-SysRq) on a 84+ key keyboard
//    case 0x55 is less common; occurs e.g. as F11 on a Cherry G80-0777 keyboard
//    case 0xas F12 on a Telerate keyboard
//    case 0xas PF1 on a Focus 9000 keyboard
//    case 0xand as FN on an IBM ThinkPad.
//    case 0x56 mostly on non-US keyboards. It is often an unlabelled key to the left or to the right of the left Alt key.
//    case 0x57:
//      return 'F11';
//    case 0x58:
//      return 'F12) both on a 101+ key keyboard
    default:
      return ' ';
  }
}

#define KBD_DATA_PORT   0x60

/** read_scan_code:
 *  Reads a scan code from the keyboard
 *
 *  @return The scan code (NOT an ASCII character!)
 */
unsigned char ReadScanCode()
{
  return inb(KBD_DATA_PORT);
}

char GetAscii() {
  return ScancodeToAscii(ReadScanCode());
}
