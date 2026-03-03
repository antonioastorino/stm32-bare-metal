/**
 * Blink bare-metal STM32 example
 */

#include <stdint.h>
#include "stm32f103xb.h"

#define IOPC_BIT_NR (4)
#define LOW (0)
#define HIGH (1)
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

static inline void gpio_set_mode(GPIO_TypeDef* gpio_port, uint8_t pin, GPIOMode mode, uint8_t cnf)
{
    if (pin < 8)
    {
        gpio_port->CRL &= ~(15UL << (pin * 4));                      // Clear existing setting
        gpio_port->CRL |= ((mode | (cnf << 2)) & 15UL) << (pin * 4); // Set new mode
    }
    else
    {
        gpio_port->CRH &= ~(15UL << ((pin - 8) * 4));                                // Clear existing setting
        gpio_port->CRH |= ((uint32_t)(mode | (cnf << 2)) & 15UL) << ((pin - 8) * 4); // Set new mode
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

int main(void)
{
    RCC->APB2ENR |= 1UL << IOPC_BIT_NR; // Enable GPIO clock for PORTC
    gpio_write(GPIOC, 13, LOW);
    gpio_set_mode(GPIOC, 13, GPIO_MODE_OUTPUT_FAST, OPEN_DRAIN); // Set blue LED to output mode
    while (1)
    {
        gpio_write(GPIOC, 13, HIGH);
        spin(99099);
        gpio_write(GPIOC, 13, LOW);
        spin(999999);
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

// 16 standard and 91 STM32-specific handlers
__attribute__((section(".vectors"))) void (*const tab[16 + 68])(void) = {
    _estack,
    _reset};
