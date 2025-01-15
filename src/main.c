#include "mprintf.h"

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

static void UART_init(void);
static void sysclk_init(void);
static void Error_Handler(void);


#define BUFFER_SIZE 0x200
#define ENTRY_LEN 100
#define SRC_OFFSET 0x40
#define DEST_OFFSET 0x3F


extern void* memset_orig(void *ptr, uint32_t value, uint32_t num);
extern void* memmove_orig(void *destination, const void *source, size_t num);
extern void* memmove_new(void *destination, const void *source, size_t num);

static void test(void* (*f)(void *, const void *, size_t), char * func_name);
static inline void enable_cycle_count(void);
static inline uint32_t get_cycle_count(void);
static inline uint32_t get_LSU_count(void);
static void clear_buffer(uint8_t *buf, uint32_t size);
static void set_buffer(uint8_t *buf);



int main(void)
{
  sysclk_init();
  UART_init();

  enable_cycle_count();

  test(memmove_new, "memmove_new");
  test(memmove_orig, "memmove_orig");
  test(memmove, "lib_memmove");

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
  memset(buf, 0, size);
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




/* Communication Functions */

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


static void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif /* USE_FULL_ASSERT */
