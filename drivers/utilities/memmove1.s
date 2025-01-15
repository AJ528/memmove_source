
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

  cmp   r0, r1            @ determine if source addr is ahead or behind destination
  bls   copy_f            @ if source is ahead of destination (or same), copy forward

@ source is behind destination
  adds  r3, r1, r2      @ calculate final source address + 1 and store in r3
  cmp   r0, r3          @ compare first destination addr against final source address + 1
  bcs   copy_f          @ if the first destination addr is >= final source addr + 1, there is no overlap

@ copy backwards
  adds  r1, r0, r2      @ calculate final destination addr + 1 and store in r1
  subs  r2, r3, r2      @ calculate first source address and store in r2



copy_bck_single:        @ now we copy from the end of source and work backwards
  ldrb  r4, [r3, #-1]!  @ load byte from memory[r3-1] into r4. r3 is updated to r3-1
  cmp   r2, r3          @ check if we are at the first source address
  strb  r4, [r1, #-1]!  @ store r4 byte into memory[r1-1]. r1 is updated to r1-1
  bne   copy_bck_single @ if not done, repeat
  b     exit

@ copy forwards
copy_f:                 @ copy from beginning of source and work forwards
  add   r3, r2, r1      @ calculate the final source address + 1 and store in r3
  subs  r5, r0, #1      @ subtract 1 from the first destination address and store in r5

quad_check:
  cmp   r2, #3          @ if there are 3 or less bytes to copy
  bls   copy_fwd_single @ copy one at a time
  sub   r2, r2, #4      @ decrement remaining bytes by 4

  @ *** Option A: ***
  ldrb  r4, [r1], #1    @ load byte from memory[r1] into r4. r1 is updated to r1+1
  strb  r4, [r5, #1]!   @ store r4 byte into memory[r5+1]. r5 is updated to r5+1
  ldrb  r4, [r1], #1    @ load byte from memory[r1] into r4. r1 is updated to r1+1
  strb  r4, [r5, #1]!   @ store r4 byte into memory[r5+1]. r5 is updated to r5+1
  ldrb  r4, [r1], #1    @ load byte from memory[r1] into r4. r1 is updated to r1+1
  strb  r4, [r5, #1]!   @ store r4 byte into memory[r5+1]. r5 is updated to r5+1
  ldrb  r4, [r1], #1    @ load byte from memory[r1] into r4. r1 is updated to r1+1
  strb  r4, [r5, #1]!   @ store r4 byte into memory[r5+1]. r5 is updated to r5+1

  @ *** Option B: ***
  @ ldrb  r4, [r1], #1    @ load byte from memory[r1] into r4. r1 is updated to r1+1
  @ ldrb  r5, [r1], #1    @ load byte from memory[r1] into r5. r1 is updated to r1+1
  @ ldrb  r6, [r1], #1    @ load byte from memory[r1] into r6. r1 is updated to r1+1
  @ ldrb  r7, [r1], #1    @ load byte from memory[r1] into r7. r1 is updated to r1+1
  @ strb  r4, [r5, #1]!   @ store r4 byte into memory[r5+1]. r5 is updated to r5+1
  @ strb  r5, [r5, #1]!   @ store r5 byte into memory[r5+1]. r5 is updated to r5+1
  @ strb  r6, [r5, #1]!   @ store r6 byte into memory[r5+1]. r5 is updated to r5+1
  @ strb  r7, [r5, #1]!   @ store r7 byte into memory[r5+1]. r5 is updated to r5+1


  cmp   r3, r1          @ check if we are at the final source address
  bne   quad_check      @ if not done, repeat
  b     exit

copy_fwd_single:
  ldrb  r4, [r1], #1    @ load byte from memory[r1] into r4. r1 is updated to r1+1
  cmp   r3, r1          @ check if we are at the final source address
  strb  r4, [r5, #1]!   @ store r4 byte into memory[r5+1]. r5 is updated to r5+1
  bne   copy_fwd_single @ if not done, repeat

exit:
  pop   {r4, r5, r6, r7}  @ restore previous value of r4-r7
  bx    lr                @ exit function

  .size memmove_new, . - memmove_new

