#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/can.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>
#include <stddef.h>
#include <bootloader.h>
#include <boot_arg.h>
#include <platform/mcu/armv7-m/timeout_timer.h>
#include "platform.h"

#define GPIOA_LED_ERROR GPIO0
#define GPIOA_LED_DEBUG GPIO0
/* LED_STATUS is different between the two board revisions so dont use it */

#define GPIOA_LED_ALL (GPIOA_LED_ERROR | GPIOA_LED_DEBUG)

#define GPIOI_CAN1_RX GPIO9
#define GPIOH_CAN1_TX GPIO13

// page buffer used by config commands.
uint8_t config_page_buffer[CONFIG_PAGE_SIZE];

void can_interface_init(void)
{
    rcc_periph_clock_enable(RCC_CAN1);

    /*
       STM32F4 CAN1 on 42MHz configured APB1 peripheral clock
       42MHz / 6 -> 7MHz
       7MHz / (1tq + 11tq + 2tq) = 500Hz => 500kbit
     */
    can_init(CAN1, // Interface
             false, // Time triggered communication mode.
             true, // Automatic bus-off management.
             false, // Automatic wakeup mode.
             false, // No automatic retransmission.
             false, // Receive FIFO locked mode.
             true, // Transmit FIFO priority.
             CAN_BTR_SJW_1TQ, // Resynchronization time quanta jump width
             CAN_BTR_TS1_11TQ, // Time segment 1 time quanta width
             CAN_BTR_TS2_2TQ, // Time segment 2 time quanta width
             6, // Prescaler
             false, // Loopback
             false); // Silent

    // filter to match any standard id
    // mask bits: 0 = Don't care, 1 = mute match corresponding id bit
    can_filter_id_mask_32bit_init(
        CAN1,
        0, // filter nr
        0, // id: only std id, no rtr
        6 | (7 << 29), // mask: match only std id[10:8] = 0 (bootloader frames)
        0, // assign to fifo0
        true // enable
    );
}

void fault_handler(void)
{
    gpio_clear(GPIOA, GPIOA_LED_ALL);
    gpio_set(GPIOA, GPIOA_LED_ERROR);
    // while(1); // debug
    reboot_system(BOOT_ARG_START_BOOTLOADER_NO_TIMEOUT);
}

static void delay(int n)
{
    for (volatile int i = 0; i < n; i++) {
        asm volatile("nop");
    }
}

static void reboot_blink(void)
{
    /* arbitrary period obtained by trial and error. No units. */
    const int period = 700 * 1000;
    for (int i = 0; i < 10; i++) {
        gpio_clear(GPIOA, GPIOA_LED_ALL);
        gpio_set(GPIOA, GPIOA_LED_DEBUG);
        delay(period);

        gpio_clear(GPIOA, GPIOA_LED_ALL);
        gpio_set(GPIOA, GPIOA_LED_ERROR);
        delay(period);
    }
    gpio_set(GPIOA, GPIOB_LED_ALL);
}

void platform_main(int arg)
{   
    //8 MHz External Clock
    rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_168MHZ]);

    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOH);
    rcc_periph_clock_enable(RCC_GPIOI);


    // CAN pin TX PH13
    gpio_mode_setup(GPIOH, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIOH_CAN1_TX);
    gpio_set_output_options(GPIOH, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIOH_CAN1_TX);
    gpio_set_af(GPIOH, GPIO_AF9 , GPIOH_CAN1_TX); //AF9 is CAN


    // CAN pin RX PI9
    gpio_mode_setup(GPIOI, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIOI_CAN1_RX);
    gpio_set_output_options(GPIOI, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIOI_CAN1_RX);
    gpio_set_af(GPIOH, GPIO_AF9 , GPIOI_CAN1_RX); //AF9 is CAN

    // LED on
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIOA_LED_ALL);
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIOA_LED_ALL);
    gpio_set(GPIOA, GPIOA_LED_ERROR);

    // Signal entering the bootloader
    reboot_blink();

    // configure timeout of 10000 milliseconds
    timeout_timer_init(168000000, 10000);

    can_interface_init();

    bootloader_main(arg);

    reboot_system(BOOT_ARG_START_BOOTLOADER);
}
