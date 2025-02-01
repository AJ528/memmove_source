
  @ tells the assembler to use the unified instruction set
  .syntax unified
  @ this directive selects the thumb (16-bit) instruction set
  .thumb
  @ this directive specifies the following symbol is a thumb-encoded function
  .thumb_func
  @ align the next variable or instruction on a 2-byte boundary
  .align 2
  @ make the symbol visible to the linker
  .global memmove_new
  @ marks the symbol as being a function name
  .type memmove_new, STT_FUNC
memmove_new:
@ r0 = destination addr
@ r1 = source addr
@ r2 = num bytes
@ returns destination addr in r0

  push  {r4, r5, r6, r7}  @ store r4-r7 values on stack
  cmp   r2, #0            @ if there are 0 bytes to move
  beq   exit              @ exit

  add   r3, r1, r2      @ calculate final source address + 1 and store in r3
  cmp   r0, r1          @ determine if source addr is ahead or behind destination
  beq   exit            @ if source=destination, nothing to do
  blo   copy_f          @ if destination < source (source ahead), copy forward

@ source is behind destination, check for overlap
  cmp   r0, r3          @ compare first destination addr against final source address + 1
  bhs   copy_f          @ if the first destination addr is >= final source addr + 1, there is no overlap

@ otherwise we must copy backwards
  add   r4, r0, r2      @ calculate final destination addr + 1 and store in r4
  cmp   r2, #4          @ check if there are 4 or more bytes to copy
  blo   copy_bck_single @ if not, copy one at a time

quad_b_copy:            @ copy backwards 4 bytes at a time
  sub   r2, r2, #4      @ decrement remaining bytes by 4
  ldrb  r5, [r3, #-1]!  @ load byte from memory[r3-1] into r5. r3 is updated to r3-1
  strb  r5, [r4, #-1]!  @ store r5 byte into memory[r4-1]. r4 is updated to r4-1
  ldrb  r5, [r3, #-1]!  @ repeat for the next 3 bytes
  strb  r5, [r4, #-1]!
  ldrb  r5, [r3, #-1]!
  strb  r5, [r4, #-1]!
  ldrb  r5, [r3, #-1]!
  strb  r5, [r4, #-1]!
  cmp   r2, #4          @ check if there are 4 or more bytes to copy
  bhs   quad_b_copy     @ if so, quad copy again
  
  cmp   r3, r1          @ check if we are at the final source address
  beq     exit          @ if so, exit
@ otherwise, there are <4 bytes left to copy

copy_bck_single: 
  ldrb  r5, [r3, #-1]!  @ load byte from memory[r3-1] into r5. r3 is updated to r3-1
  cmp   r1, r3          @ check if we are at the first source address
  strb  r5, [r4, #-1]!  @ store r5 byte into memory[r4-1]. r4 is updated to r4-1
  bne   copy_bck_single @ if not done, repeat
  b     exit

@ copy forwards
copy_f:                 @ copy from beginning of source and work forwards
  subs  r4, r0, #1      @ subtract 1 from the first destination address and store in r4
  cmp   r2, #4          @ check if there are 4 or more bytes to copy
  blo   copy_fwd_single @ if not, copy one at a time

quad_f_copy:
  sub   r2, r2, #4      @ decrement remaining bytes by 4
  ldrb  r5, [r1], #1    @ load byte from memory[r1] into r5. r1 is updated to r1+1
  strb  r5, [r4, #1]!   @ store r5 byte into memory[r4+1]. r4 is updated to r4+1
  ldrb  r5, [r1], #1    @ repeat for the next 3 bytes
  strb  r5, [r4, #1]!
  ldrb  r5, [r1], #1
  strb  r5, [r4, #1]!
  ldrb  r5, [r1], #1
  strb  r5, [r4, #1]!
  cmp   r2, #4          @ check if there are 4 or more bytes to copy
  bhs   quad_f_copy     @ if so, quad copy again

  cmp   r3, r1          @ check if we are at the final source address
  beq     exit          @ if so, exit
@ otherwise, there are <4 bytes left to copy forward

copy_fwd_single:
  ldrb  r5, [r1], #1    @ load byte from memory[r1] into r5. r1 is updated to r1+1
  cmp   r3, r1          @ check if we are at the final source address
  strb  r5, [r4, #1]!   @ store r5 byte into memory[r4+1]. r4 is updated to r4+1
  bne   copy_fwd_single @ if not done, repeat

exit:
  pop   {r4, r5, r6, r7}  @ restore previous value of r4-r7
  bx    lr                @ exit function

  .size memmove_new, . - memmove_new 

