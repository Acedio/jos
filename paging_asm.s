section .bss
align 4096

; The global page directory for all of vram.
global page_directory
page_directory:
  resb 4096

; The page table used by the OS.
global os_page_table
os_page_table:
  resb 4096
