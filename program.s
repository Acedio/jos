; set eax to some distinguishable number, to read from the log afterwards
fn: 
  mov eax, 0xDEADBEEF
  mov ebx, 0xDEADBEEF
  mov ecx, 0xDEADBEEF
  mov edx, 0xDEADBEEF

; enter infinite loop, nothing more to do
jmp fn
