global lgdt

; lgdt - load gdt, assumes the following gdt layout:
;     0: null segment descriptor
;     1: code segment descriptor
;     2: data segment descriptor
;   [esp+4] location of the gdt struct
lgdt:
  mov eax, [esp + 4]
  lgdt [eax]
  mov ax, 0x0010
  mov ss, ax
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  ; other data segments?
  ; far jump to update cs to GDT:0x08
  jmp 0x08:flush_cs
  flush_cs:
  ret
