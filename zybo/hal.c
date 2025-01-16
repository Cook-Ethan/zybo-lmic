/*
 * SPI  | Pins
 * mosi | MIO 10
 * miso | MIO 11
 * sclk | MIO 12
 * ss   | MIO 13
 * 
 * GPIO | Pins   | Direction
 * rst  | MIO 9  | Out
 * DIO0 | MIO 14 | In
 * DIO1 | MIO 15 | In
 */


#include "../lmic/lmic.h"
#include "xparameters.h"
#include "xgpiops.h"
#include "xspips.h"

static struct {
    int irqlevel;
    u4_t ticks;
} HAL;

// ------------------------------------------------------------------------
// GPIO
#define RST_PIN 9
#define DIO0_PIN 14
#define DIO1_PIN 15

static XGpioPs gpio_ps;

extern void radio_irq_handler (u1_t dio);

static void irq_handler (void *CallbackRef, u4_t bank, u4_t status) {
    u4_t dio0_status, dio1_status;
    dio0_status = XGpioPs_ReadPin(&gpio_ps, DIO0_PIN);
    dio1_status = XGpioPs_ReadPin(&gpio_ps, DIO1_PIN);

    // DIO0 Interrupt
    if (dio0_status != 0) {
        XGpioPs_IntrClearPin(&gpio_ps, DIO0_PIN);
        radio_irq_handler(0);
    }

    // DIO1 Interrupt
    if (dio1_status != 0) {
        XGpioPs_IntrClearPin(&gpio_ps, DIO1_PIN);
        radio_irq_handler(1);
    }

    return;
}

static void hal_io_init () {
    XGpioPs_Config *cfg_ptr;

    cfg_ptr = XGpioPs_LookupConfig(XPAR_XGPIOPS_0_BASEADDR);

    XGpioPs_CfgInitialize((&gpio_ps), cfg_ptr, cfg_ptr->BaseAddr);

    XGpioPs_SetDirectionPin(&gpio_ps, RST_PIN, 1);
    XGpioPs_SetDirectionPin(&gpio_ps, DIO0_PIN, 0);
    XGpioPs_SetDirectionPin(&gpio_ps, DIO1_PIN, 0);

    XGpioPs_WritePin(&gpio_ps, RST_PIN, 0x00000000);

    XGpioPs_SetIntrTypePin(&gpio_ps, DIO0_PIN, XGPIOPS_IRQ_TYPE_EDGE_RISING);
    XGpioPs_SetIntrTypePin(&gpio_ps, DIO1_PIN, XGPIOPS_IRQ_TYPE_EDGE_RISING);
    
    XGpioPs_SetCallbackHandler(&gpio_ps, (void *)&gpio_ps, irq_handler);

    XGpioPs_IntrEnablePin(&gpio_ps, DIO0_PIN);
    XGpioPs_IntrEnablePin(&gpio_ps, DIO1_PIN);

    XSetupInterruptSystem(&gpio_ps, &XGpioPs_IntrHandler, cfg_ptr->IntrId, cfg_ptr->IntrParent, XINTERRUPT_DEFAULT_PRIORITY);
}

void hal_pin_rxtx (u1_t val) {
    return;
}

void hal_pin_rst (u1_t val) {
    if (val == 0 || val == 1) {
        XGpioPs_WritePin(&gpio_ps, RST_PIN, val);
    }
}

// ------------------------------------------------------------------------


// ------------------------------------------------------------------------
// SPI
static XSpiPs spi_ps;

static void hal_spi_init () {
    XSpiPs_Config *cfg_ptr;

    cfg_ptr = XSpiPs_LookupConfig(XPAR_XSPIPS_0_BASEADDR);

    XSpiPs_CfgInitialize((&spi_ps), cfg_ptr, cfg_ptr->BaseAddr);

    XSpiPs_SetOptions(&spi_ps, XSPIPS_MASTER_OPTION | XSPIPS_FORCE_SSELECT_OPTION);
}

u1_t hal_spi (u1_t out) {
    u1_t send = out;
    u1_t recv;
    XSpiPs_PolledTransfer(&spi_ps, &send, &recv, 1); // Transfer one byte
    return recv;
}

void hal_pin_nss (u1_t val) {
    XSpiPs_SetSlaveSelect(&spi_ps, val);
}

// ------------------------------------------------------------------------


// ------------------------------------------------------------------------
// IRQ
void hal_disableIRQs () {

}

void hal_enableIRQs () {

}

void hal_sleep () {

}
// ------------------------------------------------------------------------


// ------------------------------------------------------------------------
// Timer

static void hal_time_init () {
}

u4_t hal_ticks () {
}

void hal_waitUntil (u4_t time) {

}

u1_t hal_checkTimer (u4_t targettime) {

}

// ------------------------------------------------------------------------

void hal_init () {

}

void hal_failed () {

}

void hal_sleep