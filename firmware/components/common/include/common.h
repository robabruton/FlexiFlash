/**
 * @file common.h
 * @brief Project-wide status and result primitives.
 */

#ifndef FF_COMMON_H
#define FF_COMMON_H

#include <stdbool.h>

/**
 * @brief Shared operation status codes.
 *
 * Status values are stable programmatic results. Components may attach
 * contextual detail through ff_result_t when a caller needs more diagnostic
 * information than the status alone provides.
 */
typedef enum {
    FF_STATUS_OK = 0,           /**< Operation completed successfully. */
    FF_STATUS_INVALID_ARGUMENT, /**< Caller supplied an invalid argument. */
    FF_STATUS_UNSUPPORTED,      /**< Requested capability is not supported. */
    FF_STATUS_BUSY,             /**< Resource or operation is already in use. */
    FF_STATUS_TIMEOUT,          /**< Operation exceeded its time limit. */
    FF_STATUS_IO_ERROR,         /**< Transport or storage I/O failed. */
    FF_STATUS_NO_MEMORY,        /**< Allocation or fixed buffer failed. */
    FF_STATUS_NOT_FOUND,        /**< Requested object was absent. */
    FF_STATUS_INVALID_STATE,    /**< Operation is invalid in this state. */
    FF_STATUS_CHECK_FAILED,     /**< Integrity or validation check failed. */
    FF_STATUS_CANCELLED,        /**< Operation was cancelled. */
    FF_STATUS_INTERNAL_ERROR,   /**< Unexpected internal failure. */
} ff_status_t;

/**
 * @brief Result value with optional diagnostic context.
 */
typedef struct {
    ff_status_t status; /**< Programmatic status code. */
    /** Optional static diagnostic detail, or NULL. */
    const char *detail;
} ff_result_t;

/**
 * @brief Reports whether a status represents success.
 *
 * @param[in] status Status code to classify.
 *
 * @return true when status is FF_STATUS_OK; false otherwise.
 */
bool ff_status_is_ok(ff_status_t status);

/**
 * @brief Returns the stable symbolic name for a status code.
 *
 * @param[in] status Status code to name.
 *
 * @return Constant string for the status, or "FF_STATUS_UNKNOWN" for values
 *         outside the defined enum range.
 */
const char *ff_status_name(ff_status_t status);

#endif /* FF_COMMON_H */
