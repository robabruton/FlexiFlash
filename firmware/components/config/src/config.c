/**
 * @file config.c
 * @brief Product-wide build limit accessors.
 */

#include "config.h"

static const ff_config_limits_t limits = {
    .max_firmware_image_bytes = FF_CONFIG_MAX_FIRMWARE_IMAGE_BYTES,
    .firmware_image_chunk_bytes = FF_CONFIG_FIRMWARE_IMAGE_CHUNK_BYTES,
    .max_image_name_bytes = FF_CONFIG_MAX_IMAGE_NAME_BYTES,
    .max_image_metadata_bytes = FF_CONFIG_MAX_IMAGE_METADATA_BYTES,
    .max_operation_detail_bytes = FF_CONFIG_MAX_OPERATION_DETAIL_BYTES,
    .max_operation_report_bytes = FF_CONFIG_MAX_OPERATION_REPORT_BYTES,
};

const ff_config_limits_t *ff_config_limits(void)
{
    return &limits;
}

ff_status_t ff_config_validate_limits(void)
{
    if (limits.firmware_image_chunk_bytes == 0U) {
        return FF_STATUS_CHECK_FAILED;
    }

    if (limits.max_firmware_image_bytes < limits.firmware_image_chunk_bytes) {
        return FF_STATUS_CHECK_FAILED;
    }

    if (limits.max_image_name_bytes == 0U ||
        limits.max_image_metadata_bytes == 0U ||
        limits.max_operation_detail_bytes == 0U ||
        limits.max_operation_report_bytes == 0U) {
        return FF_STATUS_CHECK_FAILED;
    }

    return FF_STATUS_OK;
}
