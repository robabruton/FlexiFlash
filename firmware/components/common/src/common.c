/**
 * @file common.c
 * @brief Shared status helper implementations.
 */

#include "common.h"

bool ff_status_is_ok(ff_status_t status)
{
    return status == FF_STATUS_OK;
}

const char *ff_status_name(ff_status_t status)
{
    switch (status) {
    case FF_STATUS_OK:
        return "FF_STATUS_OK";
    case FF_STATUS_INVALID_ARGUMENT:
        return "FF_STATUS_INVALID_ARGUMENT";
    case FF_STATUS_UNSUPPORTED:
        return "FF_STATUS_UNSUPPORTED";
    case FF_STATUS_BUSY:
        return "FF_STATUS_BUSY";
    case FF_STATUS_TIMEOUT:
        return "FF_STATUS_TIMEOUT";
    case FF_STATUS_IO_ERROR:
        return "FF_STATUS_IO_ERROR";
    case FF_STATUS_NO_MEMORY:
        return "FF_STATUS_NO_MEMORY";
    case FF_STATUS_NOT_FOUND:
        return "FF_STATUS_NOT_FOUND";
    case FF_STATUS_INVALID_STATE:
        return "FF_STATUS_INVALID_STATE";
    case FF_STATUS_CHECK_FAILED:
        return "FF_STATUS_CHECK_FAILED";
    case FF_STATUS_CANCELLED:
        return "FF_STATUS_CANCELLED";
    case FF_STATUS_INTERNAL_ERROR:
        return "FF_STATUS_INTERNAL_ERROR";
    default:
        return "FF_STATUS_UNKNOWN";
    }
}
