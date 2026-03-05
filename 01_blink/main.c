/**
 * Blink bare-metal STM32 example
 */

#include <stdint.h>
#include "stm32f103xb.h"

#define IOPC_BIT_NR (4)
#define PLLRDY_BIT (25)
#define PLLON_BIT (24)
#define HSERDY_BIT (17)
#define HSEON_BIT (16)
#define LOW (0)
#define HIGH (1)
#define GPIO_MODE_Msk (GPIO_CRL_MODE0_Msk | GPIO_CRL_CNF0_Msk) // 4 bits
#define BLINK_PERIOD_MS (1000)
#define SYST ((SYST_TypeDef*)(0xE000E010))

static volatile uint32_t g_systick = 0;

typedef enum
{
    GPIO_MODE_INPUT,
    GPIO_MODE_OUTPUT_MED,  // Max 10 MHz
    GPIO_MODE_OUTPUT_SLOW, // Max 2 MHz
    GPIO_MODE_OUTPUT_FAST, // Max 50 MHz
} GPIOMode;

typedef enum
{
    PUSH_PULL,
    OPEN_DRAIN,
    AF_PUSH_PULL,
    AF_OPEN_DRAIN
} GPIOOutputConfig;

typedef enum
{
    ANALOG,
    FLOATING, // default
    PULLED,
} GPIOInputConfig;

typedef struct
{
    uint32_t CSR;
    uint32_t RVR;
    uint32_t CVR;
    uint32_t CALIB;
} SYST_TypeDef;

static inline void gpio_set_mode(GPIO_TypeDef* gpio_port, const uint8_t pin, const GPIOMode mode, const uint8_t cnf)
{
    if (pin < 8)
    {
        gpio_port->CRL &= ~((GPIO_MODE_Msk) << (pin * 4));                    // Clear existing setting
        gpio_port->CRL |= ((mode | (cnf << 2)) & GPIO_MODE_Msk) << (pin * 4); // Set new mode
    }
    else
    {
        gpio_port->CRH &= ~(GPIO_MODE_Msk << ((pin - 8) * 4));                                // Clear existing setting
        gpio_port->CRH |= ((uint32_t)(mode | (cnf << 2)) & GPIO_MODE_Msk) << ((pin - 8) * 4); // Set new mode
    }
}

static inline void gpio_write(GPIO_TypeDef* gpio_port, uint8_t pin, bool val)
{
    gpio_port->BSRR = (1U << pin) << (val ? 0 : 16);
}

static inline void spin(volatile uint32_t count)
{
    while (count--)
        (void)0;
}
#define is_pll_ready() (RCC->CR & (1UL << PLLRDY_BIT))
#define is_hse_ready() (RCC->CR & (1UL << HSERDY_BIT))

int main(void)
{
    // GPIO initialization
    RCC->APB2ENR |= 1UL << IOPC_BIT_NR; // Enable GPIO clock for PORTC
    gpio_write(GPIOC, 13, LOW);
    gpio_set_mode(GPIOC, 13, GPIO_MODE_OUTPUT_FAST, OPEN_DRAIN);

    // SysTick initialization
    SYST->RVR = 8000000 / 1000;
    SYST->CVR = 0;
    SYST->CSR = 7;
    RCC->CFGR |= 1UL << 16 & 1;

    RCC->CR = (1UL << PLLON_BIT) | (1UL << HSEON_BIT);
    while (!(is_pll_ready() && is_hse_ready()))
    {
        asm("nop");
    }
    gpio_write(GPIOC, 13, LOW);
    uint8_t toggle      = HIGH;
    uint32_t toggleTime = g_systick + BLINK_PERIOD_MS / 2;
    while (1)
    {
        if (g_systick > toggleTime)
        {
            gpio_write(GPIOC, 13, toggle);
            toggle = !toggle;
            toggleTime += (BLINK_PERIOD_MS / 2);
        }
    }
    return 0;
}

// Startup code
__attribute__((naked, noreturn)) void _reset(void)
{
    extern long _sbss, _ebss, _sdata, _edata, _sidata;

    for (long* dst = &_sbss; dst < &_ebss; dst++)
        *dst = 0;

    for (long *dst = &_sdata, *src = &_sidata; dst < &_edata;)
        *dst++ = *src++;

    main();

    for (;;)
        (void)0; // Infinite loop - should never be reached
}

extern void _estack(void); // Defined in link.ld

void _systick(void)
{
    g_systick++;
}

// 16 standard and 91 STM32-specific handlers
__attribute__((section(".vectors"))) void (*const tab[16 + 68])(void) = {
    _estack,  // 0
    _reset,   // 0x04
    0,        // 0x08
    0,        // 0x0C
    0,        // 0x10
    0,        // 0x14
    0,        // 0x18
    0,        // 0x1C
    0,        // 0x20
    0,        // 0x24
    0,        // 0x28
    0,        // 0x2C
    0,        // 0x30
    0,        // 0x34
    0,        // 0x38
    _systick, // 0x3C
};
