/**
 * @file board_handheld.c
 * @brief Handheld (pen) board profile - ESP32-WROOM-32E-N16.
 *
 * Portable form factor: battery powered with an RGB status LED. Owns the
 * handheld pin map and capability flags.
 */

#include "board.h"

/** Compiled-in description of the handheld board. */
static const ff_board_t k_board = {
    .name = "FlexiFlash handheld",
    .form_factor = FF_FORM_FACTOR_HANDHELD,
    .caps = {
        .has_battery = true,
        .has_rgb_led = true,
        .has_current_sense = false,
        .has_production_io = false,
    },
};

const ff_board_t *board_get(void)
{
    return &k_board;
}
