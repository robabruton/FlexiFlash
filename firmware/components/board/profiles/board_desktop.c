/**
 * @file board_desktop.c
 * @brief Benchtop / station board profile - ESP32-WROOM-32E-N16.
 *
 * Mains-powered bench and small-production form factor: production I/O and
 * target current sensing, no battery. Owns the desktop pin map and capability
 * flags.
 */

#include "board.h"

/** Compiled-in description of the benchtop board. */
static const ff_board_t k_board = {
    .name = "FlexiFlash desktop",
    .form_factor = FF_FORM_FACTOR_DESKTOP,
    .caps = {
        .has_battery = false,
        .has_rgb_led = true,
        .has_current_sense = true,
        .has_production_io = true,
    },
};

const ff_board_t *board_get(void)
{
    return &k_board;
}
