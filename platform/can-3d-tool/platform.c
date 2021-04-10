#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/can.h>
#include <stddef.h>
#include <bootloader.h>
#include <boot_arg.h>
#include <platform/mcu/armv7-m/timeout_timer.h>
#include "platform.h"

// page buffer used by config commands.
uint8_t config_page_buffer[CONFIG_PAGE_SIZE];

void can_interface_init(void)
{
    rcc_periph_clock_enable(RCC_CAN1);

    /*
    STM32F1 CAN1 on 18MHz configured APB1 peripheral clock
    36MHz / 4 -> 9MHz
    9MHz / (1tq + 15tq + 2tq) = 500kHz => 500kbit
    */
    can_init(CAN1, // Interface
             false, // Time triggered communication mode.
             true, // Automatic bus-off management.
             false, // Automatic wakeup mode.
             false, // No automatic retransmission.
             false, // Receive FIFO locked mode.
             true, // Transmit FIFO priority.
             CAN_BTR_SJW_1TQ, // Resynchronization time quanta jump width
             CAN_BTR_TS1_15TQ, // Time segment 1 time quanta width
             CAN_BTR_TS2_2TQ, // Time segment 2 time quanta width
             4, // Prescaler
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
    // while(1); // debug
    reboot_system(BOOT_ARG_START_BOOTLOADER_NO_TIMEOUT);
}

void platform_main(int arg)
{
    //If external 8MHz on PC14/PC15
    rcc_clock_setup_in_hse_8mhz_out_72mhz();

    //Activate PORTA
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_AFIO);

    // CAN pin
    /* init PA12 (TX) to Alternativ function output */
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_CAN1_TX); //TX output
    /* init PA11 (RX) to input pull up */
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO_CAN1_RX); //RX input pull up/down
    gpio_set(GPIOA, GPIO_CAN1_RX); //pull up
    /* Remap the can to pin PA11 and PA12 , sould already be by default */
    gpio_primary_remap(AFIO_MAPR_SWJ_CFG_FULL_SWJ, AFIO_MAPR_CAN1_REMAP_PORTA); //Can sur port A pin 11/12

    // configure timeout of 10000 milliseconds on a 72Mhz
    timeout_timer_init(72000000, 10000);
    can_interface_init();
    bootloader_main(arg);

    reboot_system(BOOT_ARG_START_BOOTLOADER);
}
