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
#include "xttcps.h"

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
    } else {
        XGpioPs_SetDirectionPin(&gpio_ps, RST_PIN, 0);
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
    // Disable irqs
    XTtcPs_DisableInterrupts(&ttc_ps, XTTCPS_IXR_MATCH_0_MASK | XTTCPS_IXR_CNT_OVR_MASK);
    XGpioPs_IntrDisablePin(&gpio_ps, DIO0_PIN);
    XGpioPs_IntrDisablePin(&gpio_ps, DIO1_PIN);
    HAL.irqlevel++;
}

void hal_enableIRQs () {
    HAL.irqlevel--;
    if (HAL.irqlevel == 0) {
        // enable irqs
        XTtcPs_EnableInterrupts(&ttc_ps, XTTCPS_IXR_MATCH_0_MASK | XTTCPS_IXR_CNT_OVR_MASK);
        XGpioPs_IntrEnablePin(&gpio_ps, DIO0_PIN);
        XGpioPs_IntrEnablePin(&gpio_ps, DIO1_PIN);
    }
}

void hal_sleep () {
    // Sleep not configured
    (void);
}
// ------------------------------------------------------------------------


// ------------------------------------------------------------------------
// Timer
static XTtcPs ttc_ps;

static void timer_irq_handler(void *CallBackRef, u32 StatusEvent) {
    if (XTtcPs_GetInterruptStatus(&ttc_ps) & XTTCPS_IXR_CNT_OVR_MASK) {
        HAL.ticks++;
    }
    if (XTtcPs_GetInterruptStatus(&ttc_ps) & XTTCPS_IXR_MATCH_0_MASK) {
        // do nothing, only wake up cpu
    }
    XTtcPs_ClearInterruptStatus(&ttc_ps, XTTCPS_IXR_CNT_OVR_MASK & XTTCPS_IXR_MATCH_0_MASK);
}

static void hal_time_init () {
    XTtcPs_Config *cfg_ptr;

    cfg_ptr = XTtcPs_LookupConfig(XPAR_XTTCPS_0_BASEADDR);
    
    XTtcPs_CfgInitialize(&ttc_ps, cfg_ptr, cfg_ptr->BaseAddr);

    XTtcPs_SetOptions(&ttc_ps, XTTCPS_OPTION_WAVE_DISABLE | XTTCPS_OPTION_MATCH_MODE);
    XTtcPs_SetPrescaler(&ttc_ps, 11); // CPU_Clk / 2^(11 + 1) = 111MHz / 4096 = 27.127 MHz

    XTtcPs_SetStatusHandler(&ttc_ps, &ttc_ps, (XTtcPs_StatusHandler) timer_irq_handler);

    XSetupInterruptSystem(&ttc_ps, XTtcPs_InterruptHandler, cfg_ptr->IntrId[0], cfg_ptr->IntrParent, XINTERRUPT_DEFAULT_PRIORITY);
    XTtcPs_EnableInterrupts(&ttc_ps, XTTCPS_IXR_CNT_OVR_MASK);

    XTtcPs_Start(&ttc_ps);
}

u4_t hal_ticks () {
    hal_disableIRQs();
    u4_t t = HAL.ticks;
    u2_t cnt = XTtcPs_GetCounterValue(&ttc_ps);
    if (XTtcPs_GetInterruptStatus(&ttc_ps) & XTTCPS_IXR_CNT_OVR_MASK) {
        cnt = XTtcPs_GetCounterValue(&ttc_ps);
        t++;
    }
    hal_enableIRQs();
    return (t<<16)|cnt;    
}

static u2_t deltaticks (u4_t time) {
    u4_t t = hal_ticks();
    s4_t d = time - t;
    if (d <= 0) return 0; // in the past
    if ((d >> 16) != 0) return 0xFFFF; // far ahead
    return (u2_t) d;
}

void hal_waitUntil (u4_t time) {
    while(deltaticks(time) != 0); // busy wait until timestamp
}

u1_t hal_checkTimer (u4_t time) {
    u2_t dt;
    XTtcPs_ClearInterruptStatus(&ttc_ps, XTTCPS_IXR_CNT_OVR_MASK);
    dt = deltaticks(time);
    if (dt < 5) { // event is now
        XTtcPs_DisableInterrupts(&ttc_ps, XTTCPS_IXR_MATCH_0_MASK);
        return 1;
    } else {
        u2_t cnt = XTtcPs_GetCounterValue(&ttc_ps);
        XTtcPs_SetMatchValue(&ttc_ps, 0, cnt + dt);
        XTtcPs_EnableInterrupts(&ttc_ps, XTTCPS_IXR_MATCH_0_MASK);
        return 0;
    }
}

// ------------------------------------------------------------------------

void hal_init () {
    memset(&HAL, 0x00, sizeof(HAL));
    hal_disableIRQs();

    hal_io_init();

    hal_spi_init();

    hal_time_init();

    hal_enableIRQs();
}

void hal_failed () {
    hal_disableIRQs();
    hal_sleep();
    while(1);
}

void hal_sleep