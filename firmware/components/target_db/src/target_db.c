/**
 * @file target_db.c
 * @brief Target descriptor validation helpers.
 */

#include "target_db.h"

#include <string.h>

static bool text_field_is_valid(const char *value, size_t max_bytes)
{
    return value != NULL &&
           value[0] != '\0' &&
           strnlen(value, max_bytes + 1U) <= max_bytes;
}

static bool match_kind_is_valid(ff_target_match_kind_t kind)
{
    switch (kind) {
    case FF_TARGET_MATCH_ESP_CHIP_ID:
        return true;
    default:
        return false;
    }
}

static bool protocol_matches_family(ff_target_family_t family,
                                    ff_target_protocol_t protocol)
{
    switch (family) {
    case FF_TARGET_FAMILY_ESP32:
        return protocol == FF_TARGET_PROTOCOL_ESP_UART_BOOT;
    default:
        return false;
    }
}

static bool match_matches_protocol(ff_target_protocol_t protocol,
                                   const ff_target_match_t *match)
{
    switch (protocol) {
    case FF_TARGET_PROTOCOL_ESP_UART_BOOT:
        return match->kind == FF_TARGET_MATCH_ESP_CHIP_ID && match->value != 0U;
    default:
        return false;
    }
}

static bool flash_geometry_is_valid(const ff_target_flash_t *flash)
{
    if (flash->size_bytes == 0U ||
        flash->erase_block_bytes == 0U ||
        flash->write_block_bytes == 0U) {
        return false;
    }

    if ((flash->size_bytes % flash->erase_block_bytes) != 0U ||
        (flash->size_bytes % flash->write_block_bytes) != 0U) {
        return false;
    }

    return flash->erase_block_bytes >= flash->write_block_bytes;
}

static bool voltage_range_is_valid(const ff_target_voltage_t *voltage)
{
    return voltage->min_mv > 0U && voltage->min_mv <= voltage->max_mv;
}

static bool rate_limits_are_valid(ff_target_protocol_t protocol,
                                  const ff_target_rate_limits_t *rates)
{
    switch (protocol) {
    case FF_TARGET_PROTOCOL_ESP_UART_BOOT:
        return rates->connect_baud > 0U &&
               rates->connect_baud <= rates->max_baud &&
               rates->connect_bit_clock_hz == 0U &&
               rates->max_bit_clock_hz == 0U;
    default:
        return false;
    }
}

bool ff_target_family_is_valid(ff_target_family_t family)
{
    switch (family) {
    case FF_TARGET_FAMILY_ESP32:
        return true;
    default:
        return false;
    }
}

bool ff_target_protocol_is_valid(ff_target_protocol_t protocol)
{
    switch (protocol) {
    case FF_TARGET_PROTOCOL_ESP_UART_BOOT:
        return true;
    default:
        return false;
    }
}

const char *ff_target_family_name(ff_target_family_t family)
{
    switch (family) {
    case FF_TARGET_FAMILY_ESP32:
        return "FF_TARGET_FAMILY_ESP32";
    default:
        return "FF_TARGET_FAMILY_UNKNOWN";
    }
}

const char *ff_target_protocol_name(ff_target_protocol_t protocol)
{
    switch (protocol) {
    case FF_TARGET_PROTOCOL_ESP_UART_BOOT:
        return "FF_TARGET_PROTOCOL_ESP_UART_BOOT";
    default:
        return "FF_TARGET_PROTOCOL_UNKNOWN";
    }
}

ff_status_t ff_target_descriptor_validate(const ff_target_descriptor_t *descriptor)
{
    if (descriptor == NULL) {
        return FF_STATUS_INVALID_ARGUMENT;
    }

    if (!text_field_is_valid(descriptor->id, FF_TARGET_DB_MAX_ID_BYTES) ||
        !text_field_is_valid(descriptor->name, FF_TARGET_DB_MAX_NAME_BYTES)) {
        return FF_STATUS_INVALID_ARGUMENT;
    }

    if (!ff_target_family_is_valid(descriptor->family) ||
        !ff_target_protocol_is_valid(descriptor->protocol) ||
        !match_kind_is_valid(descriptor->match.kind)) {
        return FF_STATUS_INVALID_ARGUMENT;
    }

    if (!protocol_matches_family(descriptor->family, descriptor->protocol) ||
        !match_matches_protocol(descriptor->protocol, &descriptor->match) ||
        !flash_geometry_is_valid(&descriptor->flash) ||
        !voltage_range_is_valid(&descriptor->voltage) ||
        !rate_limits_are_valid(descriptor->protocol, &descriptor->rates)) {
        return FF_STATUS_CHECK_FAILED;
    }

    return FF_STATUS_OK;
}
