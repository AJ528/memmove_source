#include "mprintf.h"
#include "greatest.h"

#include "stm32wlxx_ll_bus.h"
#include "stm32wlxx_ll_rcc.h"
#include "stm32wlxx_ll_gpio.h"
#include "stm32wlxx_ll_lpuart.h"
#include "stm32wlxx_ll_utils.h"
#include "stm32wlxx.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>

static void UART_init(void);
static void sysclk_init(void);


#define BUFFER_SIZE 0x200
#define ENTRY_LEN 100
#define SRC_OFFSET 0x40
#define DEST_OFFSET 0x41


extern void* memset_orig(void *ptr, uint32_t value, uint32_t num);
extern void* memmove_orig(void *destination, const void *source, size_t num);
extern void* memmove_new(void *destination, const void *source, size_t num);

static void test(void* (*f)(void *, const void *, size_t), char * func_name);
static inline void enable_cycle_count(void);
static inline uint32_t get_cycle_count(void);
static inline uint32_t get_LSU_count(void);
static void clear_buffer(uint8_t *buf, uint32_t size);
static void set_buffer(uint8_t *buf);

extern void initialise_monitor_handles(void);


TEST memmove_test(uint32_t data_len, uint32_t src_offset, uint32_t dest_offset)
{
    uint8_t expected[BUFFER_SIZE] = {0};
    uint8_t actual[BUFFER_SIZE] = {0};

    if(((data_len + src_offset) >= BUFFER_SIZE) || ((data_len + dest_offset) >= BUFFER_SIZE)){
      FAIL();
    }


    uint32_t i;

    for(i = 0; i < data_len; i++){
      expected[src_offset + i] = i;
      actual[src_offset + i] = i;
    }

    memmove(&(expected[dest_offset]), &(expected[src_offset]), data_len);
    memmove_new(&(actual[dest_offset]), &(actual[src_offset]), data_len);

    // force fail
    // actual[dest_offset+16] = 0;

    ASSERT_MEM_EQ(expected, actual, BUFFER_SIZE);

    PASS();
}

TEST memmove_iterate(uint32_t data_len_limit)
{
  uint32_t data_len;
  uint32_t src_offset;
  uint32_t dest_offset;
  uint32_t offset_limit;

  for(data_len = 0; data_len < data_len_limit; data_len++){
    offset_limit = (data_len * 2)+5;
    for(src_offset = 0; src_offset < offset_limit; src_offset++){
      for(dest_offset = 0; dest_offset < offset_limit; dest_offset++){
        CHECK_CALL(memmove_test(data_len, src_offset, dest_offset));
      }
    }
  }

  PASS();
}

// Add definitions that need to be in the test runner's main file.
GREATEST_MAIN_DEFS();

int main(void)
{
  initialise_monitor_handles();

  sysclk_init();
  UART_init();

  enable_cycle_count();

  test(memmove_new, "memmove_new");
  test(memmove_orig, "memmove_orig");
  test(memmove, "lib_memmove");
  print_newline();

  GREATEST_MAIN_BEGIN();  // command-line options, initialization.
  
  RUN_TESTp(memmove_test, 42, 5, 70);
  RUN_TEST1(memmove_iterate, 32);

  GREATEST_MAIN_END();    // display results

  while (1)
  {
  	// LL_mDelay(1000);
  }
}


static void test(void* (*f)(void *, const void *, size_t), char * func_name)
{
  uint8_t buffer[BUFFER_SIZE] = {0};
  set_buffer(buffer);

  uint32_t LSU_start = get_LSU_count();
  uint32_t start = get_cycle_count();

  f(&buffer[DEST_OFFSET], &buffer[SRC_OFFSET], ENTRY_LEN);

  uint32_t stop = get_cycle_count();
  uint32_t LSU_stop = get_LSU_count();

  printfln_("%s took %u cycles and %u LSU cycles", func_name, stop-start, (uint8_t)(LSU_stop-LSU_start));
}



static void clear_buffer(uint8_t *buf, uint32_t size)
{
  memset_orig(buf, 0, size);
}

static void set_buffer(uint8_t *buf)
{
  for(int i=0; i < ENTRY_LEN; i++){
    buf[SRC_OFFSET + i] = i;
  }
}

static inline void enable_cycle_count(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->LSUCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk | DWT_CTRL_LSUEVTENA_Msk;
}

static inline uint32_t get_cycle_count(void)
{
  return DWT->CYCCNT;
}

static inline uint32_t get_LSU_count(void)
{
  return (DWT->LSUCNT) & 0xFF;
}




/******* Communication Functions *******/

int32_t putchar_(char c)
{
  // loop while the LPUART_TDR register is full
  while(LL_LPUART_IsActiveFlag_TXE_TXFNF(LPUART1) != 1);
  // once the LPUART_TDR register is empty, fill it with char c
  LL_LPUART_TransmitData8(LPUART1, (uint8_t)c);
  return (c);
}

static void sysclk_init(void)
{
  // update the global variable SystemCoreClock
  SystemCoreClockUpdate();

  // configure 1ms systick for easy delays
  LL_RCC_ClocksTypeDef clk_struct;
  LL_RCC_GetSystemClocksFreq(&clk_struct);
  LL_Init1msTick(clk_struct.HCLK1_Frequency);
}

static void UART_init(void)
{
  // enable the UART GPIO port clock
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);

  // set the LPUART clock source to the peripheral clock
  LL_RCC_SetLPUARTClockSource(LL_RCC_LPUART1_CLKSOURCE_PCLK1);

  // enable clock for LPUART
  LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_LPUART1);

  // configure GPIO pins for LPUART1 communication
  // TX Pin is PA2, RX Pin is PA3
  LL_GPIO_InitTypeDef GPIO_InitStruct = {
  .Pin = LL_GPIO_PIN_2 | LL_GPIO_PIN_3,
  .Mode = LL_GPIO_MODE_ALTERNATE,
  .Pull = LL_GPIO_PULL_NO,
  .Speed = LL_GPIO_SPEED_FREQ_MEDIUM,
  .OutputType = LL_GPIO_OUTPUT_PUSHPULL,
  .Alternate = LL_GPIO_AF_8
};
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // configure the LPUART to transmit with the following settings:
  // baud = 115200, data bits = 8, stop bits = 1, parity bits = 0
  LL_LPUART_InitTypeDef LPUART_InitStruct = {
      .PrescalerValue = LL_LPUART_PRESCALER_DIV1,
      .BaudRate = 115200,
      .DataWidth = LL_LPUART_DATAWIDTH_8B,
      .StopBits = LL_LPUART_STOPBITS_1,
      .Parity = LL_LPUART_PARITY_NONE,
      .TransferDirection = LL_LPUART_DIRECTION_TX_RX,
      .HardwareFlowControl = LL_LPUART_HWCONTROL_NONE
  };
  LL_LPUART_Init(LPUART1, &LPUART_InitStruct);
  LL_LPUART_Enable(LPUART1);

  // wait for the LPUART module to send an idle frame and finish initialization
  while(!(LL_LPUART_IsActiveFlag_TEACK(LPUART1)) || !(LL_LPUART_IsActiveFlag_REACK(LPUART1)));
}


