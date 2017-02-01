; set eax to some distinguishable number, to read from the log afterwards
[BITS 32]
fn: 
  mov eax, dword 0xDEADBEEF
  ret
