global _start                   ; the entry symbol for ELF

extern kmain           ; in kmain.c
extern page_directory  ; in paging_asm.s
extern page_table      ; in paging_asm.s

extern kernel_physical_start  ; from link.ld
extern kernel_physical_end
extern kernel_virtual_start
extern kernel_virtual_end

MAGIC_NUMBER equ 0x1BADB002     ; define the magic number constant
FLAGS        equ 0x0            ; multiboot flags
CHECKSUM     equ -MAGIC_NUMBER  ; calculate the checksum
                                ; (magic number + checksum + flags should equal 0)

KERNEL_STACK_SIZE equ 4096
UPPER_HALF_OFFSET equ 0xC0000000
LARGE_PAGE_SIZE   equ 0x400000
PAGE_SIZE         equ 0x1000
UPPER_HALF_INDEX  equ UPPER_HALF_OFFSET/LARGE_PAGE_SIZE

section .bss
align 4
kernel_stack:
    resb KERNEL_STACK_SIZE

section .text                   ; start of the text (code) section
align 4                         ; the code must be 4 byte aligned
    dd MAGIC_NUMBER             ; write the magic number to the machine code,
    dd FLAGS                    ; the flags,
    dd CHECKSUM                 ; and the checksum

_start:                              ; the loader label (defined as entry point in linker script)
    xchg bx, bx  ; TODO: MAGIC
    ; First set up bare bones paging, where the 0th and the upper half page frames are pointed at 0.
    lea eax, [page_table]            ; First we need to create the page table
    sub eax, UPPER_HALF_OFFSET       ; [page_table] is the virtual addr so we need to subtract the virtual offset
    mov ecx, 0x00000003              ; ecx will hold the physical address and PTE options
                                     ; 0x03 makes it a 4kb read/write page
    populate_page_table:
        mov [eax], ecx               ; Add the entry to the page table
        add eax, 4                   ; Increment our page table entry pointer to the next page table entry
        add ecx, PAGE_SIZE           ; ... and our physical memory pointer/options by 4kb
        cmp ecx, kernel_physical_end ; Continue until we've mapped through the kernel
                                     ; The kernel is aligned to pages, so the 12 config bits shouldn't interfere with the CMP
                                     ; TODO: This'll need to change if the kernel goes over 4MB (unlikely :P)
        jl populate_page_table

    lea ecx, [page_table]            ; Load the page table address so we can add it to the page directory
    sub ecx, UPPER_HALF_OFFSET
    or  ecx, 0x00000003              ; Set the config bits to say we're pointing at a r/w PTE

    lea eax, [page_directory]        ; Now for the page directory
    sub eax, UPPER_HALF_OFFSET       ; Same thing as the page table, still virtual
    mov [eax], ecx                   ; Load the address of the page table + options
    add eax, (4 * UPPER_HALF_INDEX)  ; Also for the page frame that the kernel is in
    mov [eax], ecx             

    lea eax, [page_directory]        ; Now put the physical address of the PD into cr3
    sub eax, UPPER_HALF_OFFSET
    mov cr3, eax

    mov eax, cr4
    or  eax, 0x10 ; set PSE for 4GB (4th bit)
    mov cr4, eax

    mov eax, cr0
    or  eax, 0x80000000 ; set PG to enable paging
    mov cr0, eax

    lea eax, [higher_half] ; load the address of the label in eax
    jmp eax                ; jump to the label
    higher_half:

    lea eax, [page_directory] ; No need to correct the virtual address this time
                              ; since we're actually in the upper half now
    mov [eax], dword 0        ; Clear the 0 page mapping
    invlpg [0]                ; and invalidate its entry in the TLB

    mov esp, kernel_stack + KERNEL_STACK_SIZE   ; point esp to the start of the
                                                ; stack (end of memory area)
    add  ebx, UPPER_HALF_OFFSET  ; Multiboot header address is in ebx, but is the physical
                                 ; address so we need to translate it to the virtual version.
    push kernel_virtual_end      ; Push the KernelLocation structure
    push kernel_virtual_start
    push kernel_physical_end
    push kernel_physical_start
    push ebx                     ; Push the address of the multiboot header
    call kmain                   ; start the kernel
.loop:
    jmp .loop                   ; loop forever
