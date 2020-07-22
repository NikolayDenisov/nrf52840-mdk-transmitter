#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bsp.h"
#include "nrf_drv_qspi.h"
#include "nrf_delay.h"
#include "app_util_platform.h"
#include "app_error.h"
#include "boards.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "sdk_config.h"
#include "radio_config.h"
#include "app_timer.h"
#include "bsp_config.h"

static uint32_t packet;

void send_packet() {
    NRF_RADIO->EVENTS_READY = 0U;
    NRF_RADIO->TASKS_TXEN = 1;
    while (NRF_RADIO->EVENTS_READY == 0U) {
        // wait
    }
    NRF_RADIO->EVENTS_END = 0U;
    NRF_RADIO->TASKS_START = 1U;
    while (NRF_RADIO->EVENTS_END == 0U) {
        // wait
    }
    uint32_t err_code = bsp_indication_set(BSP_INDICATE_SENT_OK);
    NRF_LOG_INFO("The packet was sent");
    APP_ERROR_CHECK(err_code);
    NRF_RADIO->EVENTS_DISABLED = 0U;
    NRF_RADIO->TASKS_DISABLE = 1U;
    while (NRF_RADIO->EVENTS_DISABLED == 0U) {
        // wait
    }
}

void bsp_evt_handler(bsp_event_t evt) {
    uint32_t prep_packet = 0;
    switch (evt) {
        case BSP_EVENT_KEY_0:
            /* Fall through. */
        case BSP_EVENT_KEY_1:
            /* Fall through. */
        case BSP_EVENT_KEY_2:
            /* Fall through. */
        case BSP_EVENT_KEY_3:
            /* Fall through. */
        case BSP_EVENT_KEY_4:
            /* Fall through. */
        case BSP_EVENT_KEY_5:
            /* Fall through. */
        case BSP_EVENT_KEY_6:
            /* Fall through. */
        case BSP_EVENT_KEY_7:
            /* Get actual button state. */
            for (int i = 0; i < BUTTONS_NUMBER; i++) {
                prep_packet |= (bsp_board_button_state_get(i) ? (1 << i) : 0);
            }
            break;
        default:
            /* No implementation needed. */
            break;
    }
    packet = prep_packet;
}

void clock_initialization() {
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0) {
        // wait
    }
    NRF_CLOCK->LFCLKSRC = (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos);
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_LFCLKSTART = 1;
    while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0) {
        // wait
    }
}

int main(void) {
    uint32_t err_code = NRF_SUCCESS;
    packet = 1;
    clock_initialization();
    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_DEFAULT_BACKENDS_INIT();
    err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, bsp_evt_handler);
    APP_ERROR_CHECK(err_code);
    radio_configure();
    NRF_RADIO->PACKETPTR = (uint32_t) &packet;
    err_code = bsp_indication_set(BSP_INDICATE_USER_STATE_OFF);
    NRF_LOG_INFO("Radio transmitter example started.");
    while (true) {
        if (packet != 0) {
            send_packet();
            NRF_LOG_INFO("The contents of the package was %u", (unsigned int) packet);
            packet = 0;
        }
        NRF_LOG_FLUSH();
        __WFE();
    }
}
