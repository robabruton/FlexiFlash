/**
 * @file config.h
 * @brief Product-wide build limits for shared firmware components.
 */

#ifndef FF_CONFIG_H
#define FF_CONFIG_H

#include <stdint.h>

#include "common.h"

/** Maximum bytes accepted for a stored target firmware image. */
#define FF_CONFIG_MAX_FIRMWARE_IMAGE_BYTES (8U * 1024U * 1024U)

/** Maximum bytes handled in one firmware-image stream chunk. */
#define FF_CONFIG_FIRMWARE_IMAGE_CHUNK_BYTES 1024U

/** Maximum bytes reserved for a stored firmware-image display name. */
#define FF_CONFIG_MAX_IMAGE_NAME_BYTES 96U

/** Maximum bytes reserved for one serialized image metadata record. */
#define FF_CONFIG_MAX_IMAGE_METADATA_BYTES 4096U

/** Maximum bytes reserved for human-readable operation detail. */
#define FF_CONFIG_MAX_OPERATION_DETAIL_BYTES 160U

/** Maximum bytes reserved for one serialized operation report record. */
#define FF_CONFIG_MAX_OPERATION_REPORT_BYTES 2048U

/**
 * @brief Product-wide build limits exposed as data.
 */
typedef struct {
    uint32_t max_firmware_image_bytes;   /**< Maximum stored image size. */
    uint32_t firmware_image_chunk_bytes; /**< Maximum stream chunk size. */
    uint32_t max_image_name_bytes;       /**< Maximum image name size. */
    uint32_t max_image_metadata_bytes;   /**< Maximum metadata record size. */
    uint32_t max_operation_detail_bytes; /**< Maximum operation detail size. */
    uint32_t max_operation_report_bytes; /**< Maximum report record size. */
} ff_config_limits_t;

/**
 * @brief Returns the compiled product-wide build limits.
 *
 * @return Pointer to static limits data. Never NULL.
 */
const ff_config_limits_t *ff_config_limits(void);

/**
 * @brief Validates internal relationships between compiled build limits.
 *
 * @return FF_STATUS_OK when the build limits are coherent; otherwise a
 *         validation failure status.
 */
ff_status_t ff_config_validate_limits(void);

#endif /* FF_CONFIG_H */
