/**
 * @file board.h
 * @brief Board identity and capability contract.
 *
 * Every hardware fact that differs between boards lives behind this contract.
 * Shared components branch on capability flags, never on which board or form
 * factor is compiled, so common code adapts to each board and diverges only
 * where a capability actually differs.
 */

#ifndef FF_BOARD_H
#define FF_BOARD_H

#include <stdbool.h>
#include <stdint.h>

/** Product form factor a board profile presents. */
typedef enum {
    FF_FORM_FACTOR_HANDHELD,  /**< Portable pen device. */
    FF_FORM_FACTOR_DESKTOP,   /**< Benchtop / production station. */
} ff_form_factor_t;

/**
 * @brief Capability flags describing what a board physically supports.
 *
 * Each board profile owns its own values. Shared code reads these instead of
 * assuming a form factor, which is how a feature can be present on one variant
 * and absent on another without forking the firmware.
 */
typedef struct {
    bool has_battery;       /**< LiPo + fuel gauge present. */
    bool has_rgb_led;       /**< Controllable multicolor status LED present. */
    bool has_current_sense; /**< Target current measurement channel present. */
    /** External trigger and pass/fail outputs present. */
    bool has_production_io;
} ff_board_caps_t;

/**
 * @brief Static description of the active board.
 *
 * Populated by exactly one profile source selected at compile time.
 */
typedef struct {
    const char *name;             /**< Human-readable board name. */
    ff_form_factor_t form_factor; /**< Form factor this board presents. */
    ff_board_caps_t caps;         /**< Physical capability flags. */
} ff_board_t;

/**
 * @brief Returns the active board description.
 *
 * @return Pointer to the compiled-in board profile. Never NULL; the build
 *         fails if no profile is selected.
 */
const ff_board_t *board_get(void);

#endif /* FF_BOARD_H */
