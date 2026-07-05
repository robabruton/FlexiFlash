/**
 * @file target_db.h
 * @brief Target descriptor contract for supported programmable devices.
 */

#ifndef FF_TARGET_DB_H
#define FF_TARGET_DB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "common.h"

/** Maximum bytes allowed for a stable target descriptor identifier. */
#define FF_TARGET_DB_MAX_ID_BYTES 48U

/** Maximum bytes allowed for a target display name. */
#define FF_TARGET_DB_MAX_NAME_BYTES 96U

/** Flag for target-specific behavior that does not fit a common field. */
typedef uint32_t ff_target_quirks_t;

/** Broad device family a target descriptor represents. */
typedef enum {
    FF_TARGET_FAMILY_ESP32 = 0,  /**< ESP32 target devices. */
} ff_target_family_t;

/** Programming protocol family used to communicate with a target. */
typedef enum {
    FF_TARGET_PROTOCOL_ESP_UART_BOOT = 0,  /**< ESP UART ROM bootloader protocol. */
} ff_target_protocol_t;

/** Target identification mechanism used during descriptor matching. */
typedef enum {
    FF_TARGET_MATCH_ESP_CHIP_ID = 0,  /**< ESP ROM/chip identifier. */
} ff_target_match_kind_t;

/**
 * @brief Target identification values used to match a connected device.
 */
typedef struct {
    ff_target_match_kind_t kind;   /**< Matching mechanism. */
    uint32_t               value;  /**< Numeric chip identifier. */
} ff_target_match_t;

/**
 * @brief Flash memory geometry used by protocol implementations.
 */
typedef struct {
    uint32_t base_address;       /**< First programmable flash address. */
    uint32_t size_bytes;         /**< Total programmable flash size. */
    uint32_t erase_block_bytes;  /**< Smallest erasable flash block. */
    uint32_t write_block_bytes;  /**< Smallest writable flash block. */
    uint8_t  erased_byte;        /**< Value read from erased flash. */
} ff_target_flash_t;

/**
 * @brief Electrical limits expected by a target descriptor.
 */
typedef struct {
    uint16_t min_mv;  /**< Minimum supported target voltage in millivolts. */
    uint16_t max_mv;  /**< Maximum supported target voltage in millivolts. */
} ff_target_voltage_t;

/**
 * @brief Conservative and maximum programming rates for a target.
 */
typedef struct {
    uint32_t connect_bit_clock_hz;  /**< Initial bit-clocked protocol rate. */
    uint32_t max_bit_clock_hz;      /**< Maximum bit-clocked protocol rate. */
    uint32_t connect_baud;          /**< Initial UART-style baud rate. */
    uint32_t max_baud;              /**< Maximum UART-style baud rate. */
} ff_target_rate_limits_t;

/**
 * @brief Static facts required to program one supported target.
 */
typedef struct {
    const char             *id;        /**< Stable lowercase descriptor ID. */
    const char             *name;      /**< Human-readable target name. */
    ff_target_family_t      family;    /**< Broad device family. */
    ff_target_protocol_t    protocol;  /**< Programming protocol. */
    ff_target_match_t       match;     /**< Device identification rule. */
    ff_target_flash_t       flash;     /**< Main flash geometry. */
    ff_target_voltage_t     voltage;   /**< Supported target voltage range. */
    ff_target_rate_limits_t rates;     /**< Safe connection and programming rates. */
    ff_target_quirks_t      quirks;    /**< Target-specific behavior flags. */
} ff_target_descriptor_t;

/**
 * @brief Reports whether a target family value is defined.
 *
 * @param[in] family Target family to classify.
 *
 * @return true when family is a defined ff_target_family_t value.
 */
bool ff_target_family_is_valid(ff_target_family_t family);

/**
 * @brief Reports whether a target protocol value is defined.
 *
 * @param[in] protocol Target protocol to classify.
 *
 * @return true when protocol is a defined ff_target_protocol_t value.
 */
bool ff_target_protocol_is_valid(ff_target_protocol_t protocol);

/**
 * @brief Returns the stable symbolic name for a target family.
 *
 * @param[in] family Target family to name.
 *
 * @return Constant string for the family, or "FF_TARGET_FAMILY_UNKNOWN".
 */
const char *ff_target_family_name(ff_target_family_t family);

/**
 * @brief Returns the stable symbolic name for a target protocol.
 *
 * @param[in] protocol Target protocol to name.
 *
 * @return Constant string for the protocol, or "FF_TARGET_PROTOCOL_UNKNOWN".
 */
const char *ff_target_protocol_name(ff_target_protocol_t protocol);

/**
 * @brief Validates that a descriptor is internally coherent.
 *
 * @param[in] descriptor Target descriptor to validate.
 *
 * @return FF_STATUS_OK when the descriptor is coherent; otherwise a status
 *         describing the failed validation class.
 */
ff_status_t ff_target_descriptor_validate(const ff_target_descriptor_t *descriptor);

#endif /* FF_TARGET_DB_H */
