global loader                   ; the entry symbol for ELF

extern kmain           ; in kmain.c
extern page_directory  ; in paging_asm.s

MAGIC_NUMBER equ 0x1BADB002     ; define the magic number constant
FLAGS        equ 0x0            ; multiboot flags
CHECKSUM     equ -MAGIC_NUMBER  ; calculate the checksum
                                ; (magic number + checksum + flags should equal 0)

KERNEL_STACK_SIZE equ 4096
UPPER_HALF_OFFSET equ 0xC0000000
FOUR_MB_BYTES     equ 0x400000
UPPER_HALF_INDEX  equ UPPER_HALF_OFFSET/FOUR_MB_BYTES

section .bss
align 4
kernel_stack:
    resb KERNEL_STACK_SIZE

section .text                   ; start of the text (code) section
align 4                         ; the code must be 4 byte aligned
    dd MAGIC_NUMBER             ; write the magic number to the machine code,
    dd FLAGS                    ; the flags,
    dd CHECKSUM                 ; and the checksum

loader:                         ; the loader label (defined as entry point in linker script)
    ; First set up bare bones paging, where the 0th and the upper half page frames are pointed at 0.
    lea eax, [page_directory]        ; This is the virtual address
    sub eax, UPPER_HALF_OFFSET       ; so we need to subtract the virtual offset
    mov [eax], dword 0x00000083      ; Identity map the first page frame. 0x83 = 4 MB r/w page
    add eax, (4 * UPPER_HALF_INDEX)  ; Also for the page frame that the kernel is in
    mov [eax], dword 0x00000083

    lea eax, [page_directory]        ; Now put the physical address of the PD into cr3
    sub eax, UPPER_HALF_OFFSET
    mov cr3, eax

    mov ebx, cr4
    or  ebx, 0x10 ; set PSE for 4GB (4th bit)
    mov cr4, ebx

    mov ebx, cr0
    or  ebx, 0x80000000 ; set PG to enable paging
    mov cr0, ebx

    lea ebx, [higher_half] ; load the address of the label in ebx
    jmp ebx                ; jump to the label
    higher_half:

    lea eax, [page_directory] ; No need to correct the virtual address this time
                              ; since we're actually in the upper half now
    mov [eax], dword 0        ; Clear the 0 page mapping
    invlpg [0]                ; and invalidate its entry in the TLB

    mov esp, kernel_stack + KERNEL_STACK_SIZE   ; point esp to the start of the
                                                ; stack (end of memory area)
    call kmain                  ; start the kernel
.loop:
    jmp .loop                   ; loop forever
