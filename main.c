/*********************************************************************
 *  nRF52 DK (PCA10040, nRF52832) + ADS8324E – EasyDMA SPIM example  *
 *                                                                   *
 *  ▸ LED1  (P0.17)  ON while running                                *
 *  ▸ P0.18 → SCLK   (1 MHz, Mode 0)                                 *
 *  ▸ P0.19 → CS     (driver toggles automatically)                  *
 *  ▸ P0.16 ← MISO   (ADS DOUT)                                      *
 *                                                                   *
 *  Build: nRF5 SDK 17.x – **nrfx spim only** (no nrf_drv_* macros)  *
 *********************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrfx_spim.h"
#include "nrf_delay.h"
#include "app_error.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

/* ---------------- Pin map ------------------------------------------------ */
#define LED1_PIN            17      // P0.17 – LED1 on DK
#define ADS8324_SCK_PIN     18      // P0.18 → SCLK
#define ADS8324_MISO_PIN    16      // P0.16 ← DOUT
#define ADS8324_CS_PIN      19      // P0.19 → CS  (handled by SPIM)
#define ADS8324_MOSI_UNUSED NRF_SPIM_PIN_NOT_CONNECTED

/* ADC reference voltage on VREF pin (edit to match your hardware) */
#define ADS_VREF_V          2.943f

/* ---------------- SPIM instance ----------------------------------------- */
static const nrfx_spim_t spim = NRFX_SPIM_INSTANCE(0);   // use SPIM0

/* ---------------- GPIO & SPIM init -------------------------------------- */
static void gpio_init(void)
{
    nrf_gpio_cfg_output(LED1_PIN);
    nrf_gpio_pin_clear(LED1_PIN);                      // LED ON (active-low)
}

static void spim_init(void)
{
    nrfx_spim_config_t cfg = {
        .sck_pin      = ADS8324_SCK_PIN,
        .mosi_pin     = ADS8324_MOSI_UNUSED,
        .miso_pin     = ADS8324_MISO_PIN,
        .ss_pin       = ADS8324_CS_PIN,               // SPIM toggles CS
        .irq_priority = NRFX_SPIM_DEFAULT_CONFIG_IRQ_PRIORITY,
        .orc          = 0x00,
        .frequency    = NRF_SPIM_FREQ_1M,             // 1 MHz SCLK
        .mode         = NRF_SPIM_MODE_0,              // CPOL 0 | CPHA 0
        .bit_order    = NRF_SPIM_BIT_ORDER_MSB_FIRST
    };
    APP_ERROR_CHECK(nrfx_spim_init(&spim, &cfg, NULL, NULL));
}

/* ---------------- Single ADS8324E conversion ---------------------------- */
static uint16_t ads8324_sample(void)
{
    uint8_t tx_dummy[2] = { 0x00, 0x00 };   // any data, just clocks
    uint8_t rx_buf[2];

    nrfx_spim_xfer_desc_t xfer = {
        .p_tx_buffer = tx_dummy,
        .tx_length   = sizeof(tx_dummy),
        .p_rx_buffer = rx_buf,
        .rx_length   = sizeof(rx_buf)
    };

    APP_ERROR_CHECK(nrfx_spim_xfer(&spim, &xfer, 0));  // blocking

    return ((uint16_t)rx_buf[0] << 8) | rx_buf[1];     // MSB-first
}

/* ---------------- Main -------------------------------------------------- */
int main(void)
{
    /* Start RTT logging (brings up 16 MHz crystal) */
    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    gpio_init();
    spim_init();

    NRF_LOG_INFO("ADS8324E demo (nrfx_spim, 1 MHz, Vref = %.3f V)", ADS_VREF_V);

    while (true)
    {
        uint16_t raw   = ads8324_sample();
        float    volts = (raw / 65535.0f) * ADS_VREF_V;

        NRF_LOG_INFO("ADC raw: %5u  |  %.3f V", raw, volts);
        NRF_LOG_FLUSH();

        nrf_delay_ms(500);
    }
}
