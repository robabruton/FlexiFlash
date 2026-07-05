/**
 * @file protocol_uart_boot.c
 * @brief ESP UART bootloader framing helper implementations.
 */

#include "protocol_uart_boot.h"

#include <stdbool.h>

static bool payload_pointer_is_valid(const uint8_t *payload, size_t payload_bytes)
{
    return payload != NULL || payload_bytes == 0U;
}

static ff_status_t append_byte(uint8_t value,
                               uint8_t *output,
                               size_t output_capacity,
                               size_t *output_bytes)
{
    if (*output_bytes >= output_capacity) {
        return FF_STATUS_NO_MEMORY;
    }

    output[*output_bytes] = value;
    ++(*output_bytes);
    return FF_STATUS_OK;
}

static void write_u16_le(uint16_t value, uint8_t *output)
{
    output[0] = (uint8_t)(value & 0xFFU);
    output[1] = (uint8_t)((value >> 8U) & 0xFFU);
}

static void write_u32_le(uint32_t value, uint8_t *output)
{
    output[0] = (uint8_t)(value & 0xFFU);
    output[1] = (uint8_t)((value >> 8U) & 0xFFU);
    output[2] = (uint8_t)((value >> 16U) & 0xFFU);
    output[3] = (uint8_t)((value >> 24U) & 0xFFU);
}

uint32_t ff_uart_boot_checksum(const uint8_t *payload, size_t payload_bytes)
{
    uint8_t checksum = 0xEFU;

    if (!payload_pointer_is_valid(payload, payload_bytes)) {
        return checksum;
    }

    for (size_t i = 0U; i < payload_bytes; ++i) {
        checksum ^= payload[i];
    }

    return checksum;
}

ff_status_t ff_uart_boot_slip_encode(const uint8_t *input,
                                     size_t input_bytes,
                                     uint8_t *output,
                                     size_t output_capacity,
                                     size_t *output_bytes)
{
    if (!payload_pointer_is_valid(input, input_bytes) ||
        output == NULL ||
        output_bytes == NULL) {
        return FF_STATUS_INVALID_ARGUMENT;
    }

    *output_bytes = 0U;

    ff_status_t status =
        append_byte(FF_UART_BOOT_SLIP_END, output, output_capacity, output_bytes);
    if (status != FF_STATUS_OK) {
        return status;
    }

    for (size_t i = 0U; i < input_bytes; ++i) {
        switch (input[i]) {
        case FF_UART_BOOT_SLIP_END:
            status = append_byte(FF_UART_BOOT_SLIP_ESC,
                                 output,
                                 output_capacity,
                                 output_bytes);
            if (status == FF_STATUS_OK) {
                status = append_byte(FF_UART_BOOT_SLIP_ESC_END,
                                     output,
                                     output_capacity,
                                     output_bytes);
            }
            break;
        case FF_UART_BOOT_SLIP_ESC:
            status = append_byte(FF_UART_BOOT_SLIP_ESC,
                                 output,
                                 output_capacity,
                                 output_bytes);
            if (status == FF_STATUS_OK) {
                status = append_byte(FF_UART_BOOT_SLIP_ESC_ESC,
                                     output,
                                     output_capacity,
                                     output_bytes);
            }
            break;
        default:
            status = append_byte(input[i], output, output_capacity, output_bytes);
            break;
        }

        if (status != FF_STATUS_OK) {
            return status;
        }
    }

    return append_byte(FF_UART_BOOT_SLIP_END,
                       output,
                       output_capacity,
                       output_bytes);
}

ff_status_t ff_uart_boot_slip_decode(const uint8_t *input,
                                     size_t input_bytes,
                                     uint8_t *output,
                                     size_t output_capacity,
                                     size_t *output_bytes)
{
    if (input == NULL || output == NULL || output_bytes == NULL) {
        return FF_STATUS_INVALID_ARGUMENT;
    }

    *output_bytes = 0U;

    if (input_bytes < 2U ||
        input[0] != FF_UART_BOOT_SLIP_END ||
        input[input_bytes - 1U] != FF_UART_BOOT_SLIP_END) {
        return FF_STATUS_CHECK_FAILED;
    }

    for (size_t i = 1U; i + 1U < input_bytes; ++i) {
        uint8_t value = input[i];

        if (value == FF_UART_BOOT_SLIP_END) {
            return FF_STATUS_CHECK_FAILED;
        }

        if (value == FF_UART_BOOT_SLIP_ESC) {
            ++i;
            if (i + 1U >= input_bytes) {
                return FF_STATUS_CHECK_FAILED;
            }

            if (input[i] == FF_UART_BOOT_SLIP_ESC_END) {
                value = FF_UART_BOOT_SLIP_END;
            } else if (input[i] == FF_UART_BOOT_SLIP_ESC_ESC) {
                value = FF_UART_BOOT_SLIP_ESC;
            } else {
                return FF_STATUS_CHECK_FAILED;
            }
        }

        ff_status_t status =
            append_byte(value, output, output_capacity, output_bytes);
        if (status != FF_STATUS_OK) {
            return status;
        }
    }

    return FF_STATUS_OK;
}

ff_status_t ff_uart_boot_build_command(ff_uart_boot_command_t command,
                                       const uint8_t *payload,
                                       size_t payload_bytes,
                                       uint8_t *output,
                                       size_t output_capacity,
                                       size_t *output_bytes)
{
    if (!payload_pointer_is_valid(payload, payload_bytes) ||
        output == NULL ||
        output_bytes == NULL ||
        payload_bytes > UINT16_MAX) {
        return FF_STATUS_INVALID_ARGUMENT;
    }

    const size_t frame_bytes = FF_UART_BOOT_COMMAND_HEADER_BYTES + payload_bytes;
    if (output_capacity < frame_bytes) {
        *output_bytes = 0U;
        return FF_STATUS_NO_MEMORY;
    }

    output[0] = FF_UART_BOOT_FRAME_DIRECTION_COMMAND;
    output[1] = (uint8_t)command;
    write_u16_le((uint16_t)payload_bytes, &output[2]);
    write_u32_le(ff_uart_boot_checksum(payload, payload_bytes), &output[4]);

    for (size_t i = 0U; i < payload_bytes; ++i) {
        output[FF_UART_BOOT_COMMAND_HEADER_BYTES + i] = payload[i];
    }

    *output_bytes = frame_bytes;
    return FF_STATUS_OK;
}
