section .bss
align 4096

global page_directory
page_directory:
  resb 4096

global page_table
page_table:
  resb 4096

section .text
align 4
global init_paging

init_paging:
  mov eax, [esp + 4]
  mov cr3, eax

  mov ebx, cr4
  or  ebx, 0x10 ; set PSE for 4GB (4th bit)
  mov cr4, ebx

  mov ebx, cr0
  or  ebx, 0x80000000 ; set PG to enable paging
  mov cr0, ebx
  ret
