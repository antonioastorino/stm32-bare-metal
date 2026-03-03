/**
 * Blink bare-metal STM32 example
 */

#include <stdint.h>
#include <stdbool.h>
#define IOPC_BIT_NR (4)
#define GPIO(port) ((GPIOBase*)(0x40010800 + 0x400 * (port - 'A')))
#define RCC ((struct rcc*)0x40021000)

struct rcc
{
    volatile uint32_t CR;       // offset 0x00
    volatile uint32_t CFGR;     // offset 0x04
    volatile uint32_t CIR;      // offset 0x08
    volatile uint32_t AHB2RSTR; // offset 0x0C
    volatile uint32_t AHB1RSTR; // offset 0x10
    volatile uint32_t AHBENR;   // offset 0x14
    volatile uint32_t APB2ENR;  // offset 0x18
    volatile uint32_t APB1ENR;  // offset 0x1C
    volatile uint32_t BDCR;     // offset 0x20
    volatile uint32_t CSR;      // offset 0x24
};

typedef struct
{
    volatile uint32_t CRL;  // offset 0x00
    volatile uint32_t CRH;  // offset 0x04
    volatile uint32_t IDR;  // offset 0x08
    volatile uint32_t ODR;  // offset 0x0C
    volatile uint32_t BSRR; // offset 0x10
    volatile uint16_t BRR;  // offset 0x14
    volatile uint32_t LCKR; // offset 0x18
} GPIOBase;

// Enum values are per datasheet: 0, 1, 2, 3
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

static inline void gpio_set_mode(char port, uint8_t pin, GPIOMode mode, uint8_t cnf)
{
    GPIOBase* gpio_port = GPIO(port); // GPIO port
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

static inline void gpio_write(char port, uint8_t pin, bool val)
{
    GPIOBase* gpio_port = GPIO(port);
    gpio_port->BSRR     = (1U << pin) << (val ? 0 : 16);
}

static inline void spin(volatile uint32_t count)
{
    while (count--)
        (void)0;
}

int main(void)
{
    RCC->APB2ENR |= 1UL << IOPC_BIT_NR; // Enable GPIO clock for PORTC
    gpio_write('C', 13, false);
    gpio_set_mode('C', 13, GPIO_MODE_OUTPUT_FAST, OPEN_DRAIN); // Set blue LED to output mode
    while (true)
    {
        gpio_write('C', 13, true);
        spin(99099);
        gpio_write('C', 13, false);
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
