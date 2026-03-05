/**
 * Blink bare-metal STM32 example
 */

#include <stdint.h>
#include "stm32f103xb.h"

#define IOPA_BIT_NR (2)
#define IOPC_BIT_NR (4)
#define PLLRDY_BIT_NR (25)
#define PLLON_BIT_NR (24)
#define HSERDY_BIT_NR (17)
#define HSEON_BIT_NR (16)
#define LOW (0)
#define HIGH (1)
#define GPIO_MODE_Msk (0xFUL) // 4 bits
#define BLINK_PERIOD_MS (100)
#define SYST ((SYST_TypeDef*)(0xE000E010))
#define CRYSTAL_FREQ_HZ (8'000'000)
#define SYSTICK_FREQ_HZ (1'000)
#define is_pll_ready() (RCC->CR & (1UL << PLLRDY_BIT_NR))
#define is_hse_ready() (RCC->CR & (1UL << HSERDY_BIT_NR))

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

/* It won't compile if `__pin` is a number literal */
#define gpio_write(__port, __pin, __val) GPIO##__port->BSRR = (1U << __pin) << (__val ? 0 : 16);

/* It won't compile if `__pin` is a number literal */
#define gpio_init(__port, __pin, __mode, __cnf, __val)                                             \
    {                                                                                              \
        RCC->APB2ENR |= RCC_APB2ENR_IOP##__port##EN;                                               \
        gpio_write(__port, __pin, __val);                                                          \
        if (__pin < 8U)                                                                            \
        {                                                                                          \
            GPIO##__port->CRL &= ~((GPIO_MODE_Msk) << (__pin * 4));                                \
            GPIO##__port->CRL |= (((__mode | (__cnf << 2)) & GPIO_MODE_Msk) << (__pin * 4));       \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            GPIO##__port->CRH &= ~(GPIO_MODE_Msk << ((__pin - 8) * 4));                            \
            GPIO##__port->CRH |= (((__mode | (__cnf << 2)) & GPIO_MODE_Msk) << ((__pin - 8) * 4)); \
        }                                                                                          \
    }

static inline void systick_init(void)
{
    SYST->RVR = CRYSTAL_FREQ_HZ / SYSTICK_FREQ_HZ;
    SYST->CVR = 0;
    SYST->CSR = 7;
    RCC->CR   = (1UL << PLLON_BIT_NR) | (1UL << HSEON_BIT_NR);
    while (!(is_pll_ready() && is_hse_ready()))
    {
        asm("nop");
    }
}

// static inline void pwm_init(uint16_t freq, uint16_t duty_cycle)
int main(void)
{
    uint8_t blink_pin = 13;
    gpio_init(C, blink_pin, GPIO_MODE_OUTPUT_FAST, OPEN_DRAIN, HIGH);
    systick_init();
    uint8_t toggle      = HIGH;
    uint32_t toggleTime = g_systick + BLINK_PERIOD_MS / 2;
    while (1)
    {
        if (g_systick > toggleTime)
        {
            gpio_write(C, 13, toggle);
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
