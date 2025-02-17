# mmemmove
`memmove()` written in ARM Cortex-M Assembly. Designed for Cortex M4 and M0+ cores. Probably works on other ARMv6-M and ARMv7-M architectures.

### Status 
Does everything I need it to. Will fix bugs as I come across them.

### How to use this in an embedded project
1. Copy `mmemmove.s` from the src folder into your project
2. Add a function declaration for `memmove_()` somewhere in your project. The function declaration should look something like `extern void* memmove_(void *destination, const void *source, size_t num);`.
3. Use `memmove_()` just like you would the normal `memmove()` function.
4. Done!

### How to measure the size of this project
1. copy these files into an embedded project.
2. compile the project.
3. measure the size of the `mmemmove.o` object file using `size -A`.
4. the total size is found from summing all relevant sections together.


### Details

Compiles down to under 230 bytes on `arm-none-eabi-gcc`. memmove is about 130 lines of code (per David A. Wheeler's `SLOCCount`).

In my testing, `memmove_()` is always faster than my standard `memmove()`. Sometimes it's only a few cycles faster, other times it is 7x faster.

This repo is designed to run on an STM32WLxx microcontroller. It uses the cycle counter of the Cortex-M4 microcontroller to profile the functions during testing.
Note: the environment this repo was developed in is only important to know if you want to test the `memmove_()` function like I did. If you just want to use it, follow the instructions in [How to use this in an embedded project](#how-to-use-this-in-an-embedded-project).

I also included testing using Greatest from https://github.com/silentbicycle/greatest. There's a number of tests that compare `memmove_()` against the standard `memmove()`.
