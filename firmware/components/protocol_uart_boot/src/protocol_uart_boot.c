/**
 * @file protocol_uart_boot.c
 * @brief ESP UART bootloader framing helper implementations.
 */

#include "protocol_uart_boot.h"

#include <stdbool.h>

/** ROM SYNC command payload used to establish bootloader communication. */
static const uint8_t k_sync_payload[FF_UART_BOOT_SYNC_PAYLOAD_BYTES] = {
    0x07U, 0x07U, 0x12U, 0x20U,
    0x55U, 0x55U, 0x55U, 0x55U,
    0x55U, 0x55U, 0x55U, 0x55U,
    0x55U, 0x55U, 0x55U, 0x55U,
    0x55U, 0x55U, 0x55U, 0x55U,
    0x55U, 0x55U, 0x55U, 0x55U,
    0x55U, 0x55U, 0x55U, 0x55U,
    0x55U, 0x55U, 0x55U, 0x55U,
    0x55U, 0x55U, 0x55U, 0x55U,
};

static bool payload_pointer_is_valid(const uint8_t *payload,
                                     size_t payload_bytes)
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

static void frame_accumulator_clear(
    ff_uart_boot_frame_accumulator_t *accumulator)
{
    accumulator->frame_bytes = 0U;
    accumulator->in_frame = false;
    accumulator->escaping = false;
}

static ff_status_t frame_accumulator_append(
    ff_uart_boot_frame_accumulator_t *accumulator,
    uint8_t value)
{
    if (accumulator->frame_bytes >= accumulator->capacity) {
        frame_accumulator_clear(accumulator);
        return FF_STATUS_NO_MEMORY;
    }

    accumulator->frame[accumulator->frame_bytes] = value;
    ++accumulator->frame_bytes;
    return FF_STATUS_OK;
}

static void response_clear(ff_uart_boot_response_t *response)
{
    response->command = 0U;
    response->value = 0U;
    response->payload = NULL;
    response->payload_bytes = 0U;
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

static uint16_t read_u16_le(const uint8_t *input)
{
    return (uint16_t)input[0] | ((uint16_t)input[1] << 8U);
}

static uint32_t read_u32_le(const uint8_t *input)
{
    return (uint32_t)input[0] |
           ((uint32_t)input[1] << 8U) |
           ((uint32_t)input[2] << 16U) |
           ((uint32_t)input[3] << 24U);
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

    ff_status_t status = append_byte(FF_UART_BOOT_SLIP_END,
                                     output,
                                     output_capacity,
                                     output_bytes);
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
            status = append_byte(input[i],
                                 output,
                                 output_capacity,
                                 output_bytes);
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

ff_status_t ff_uart_boot_frame_accumulator_init(
    ff_uart_boot_frame_accumulator_t *accumulator,
    uint8_t *frame,
    size_t frame_capacity)
{
    if (accumulator == NULL || frame == NULL || frame_capacity == 0U) {
        return FF_STATUS_INVALID_ARGUMENT;
    }

    accumulator->frame = frame;
    accumulator->capacity = frame_capacity;
    frame_accumulator_clear(accumulator);
    return FF_STATUS_OK;
}

ff_status_t ff_uart_boot_frame_accumulator_reset(
    ff_uart_boot_frame_accumulator_t *accumulator)
{
    if (accumulator == NULL || accumulator->frame == NULL) {
        return FF_STATUS_INVALID_ARGUMENT;
    }

    frame_accumulator_clear(accumulator);
    return FF_STATUS_OK;
}

ff_status_t ff_uart_boot_frame_accumulator_push(
    ff_uart_boot_frame_accumulator_t *accumulator,
    uint8_t byte,
    bool *frame_ready,
    size_t *frame_bytes)
{
    if (accumulator == NULL ||
        accumulator->frame == NULL ||
        accumulator->capacity == 0U ||
        frame_ready == NULL ||
        frame_bytes == NULL) {
        return FF_STATUS_INVALID_ARGUMENT;
    }

    *frame_ready = false;
    *frame_bytes = 0U;

    if (byte == FF_UART_BOOT_SLIP_END) {
        if (!accumulator->in_frame) {
            accumulator->in_frame = true;
            accumulator->escaping = false;
            accumulator->frame_bytes = 0U;
            return FF_STATUS_OK;
        }

        if (accumulator->escaping) {
            frame_accumulator_clear(accumulator);
            return FF_STATUS_CHECK_FAILED;
        }

        accumulator->in_frame = false;
        *frame_ready = true;
        *frame_bytes = accumulator->frame_bytes;
        return FF_STATUS_OK;
    }

    if (!accumulator->in_frame) {
        return FF_STATUS_OK;
    }

    if (accumulator->escaping) {
        accumulator->escaping = false;

        if (byte == FF_UART_BOOT_SLIP_ESC_END) {
            byte = FF_UART_BOOT_SLIP_END;
        } else if (byte == FF_UART_BOOT_SLIP_ESC_ESC) {
            byte = FF_UART_BOOT_SLIP_ESC;
        } else {
            frame_accumulator_clear(accumulator);
            return FF_STATUS_CHECK_FAILED;
        }
    } else if (byte == FF_UART_BOOT_SLIP_ESC) {
        accumulator->escaping = true;
        return FF_STATUS_OK;
    }

    return frame_accumulator_append(accumulator, byte);
}

ff_status_t ff_uart_boot_response_reader_init(
    ff_uart_boot_response_reader_t *reader,
    uint8_t *frame,
    size_t frame_capacity)
{
    if (reader == NULL) {
        return FF_STATUS_INVALID_ARGUMENT;
    }

    return ff_uart_boot_frame_accumulator_init(&reader->accumulator,
                                               frame,
                                               frame_capacity);
}

ff_status_t ff_uart_boot_response_reader_reset(
    ff_uart_boot_response_reader_t *reader)
{
    if (reader == NULL) {
        return FF_STATUS_INVALID_ARGUMENT;
    }

    return ff_uart_boot_frame_accumulator_reset(&reader->accumulator);
}

ff_status_t ff_uart_boot_response_reader_push(
    ff_uart_boot_response_reader_t *reader,
    uint8_t byte,
    bool *response_ready,
    ff_uart_boot_response_t *response)
{
    if (reader == NULL || response_ready == NULL || response == NULL) {
        return FF_STATUS_INVALID_ARGUMENT;
    }

    *response_ready = false;
    response_clear(response);

    bool frame_ready = false;
    size_t frame_bytes = 0U;
    ff_status_t status =
        ff_uart_boot_frame_accumulator_push(&reader->accumulator,
                                            byte,
                                            &frame_ready,
                                            &frame_bytes);
    if (status != FF_STATUS_OK) {
        (void)ff_uart_boot_response_reader_reset(reader);
        return status;
    }

    if (!frame_ready) {
        return FF_STATUS_OK;
    }

    status = ff_uart_boot_parse_response(reader->accumulator.frame,
                                         frame_bytes,
                                         response);
    if (status != FF_STATUS_OK) {
        (void)ff_uart_boot_response_reader_reset(reader);
        response_clear(response);
        return status;
    }

    *response_ready = true;
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

    const size_t frame_bytes =
        FF_UART_BOOT_COMMAND_HEADER_BYTES + payload_bytes;
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

ff_status_t ff_uart_boot_build_sync_command(uint8_t *output,
                                            size_t output_capacity,
                                            size_t *output_bytes)
{
    return ff_uart_boot_build_command(FF_UART_BOOT_COMMAND_SYNC,
                                      k_sync_payload,
                                      sizeof(k_sync_payload),
                                      output,
                                      output_capacity,
                                      output_bytes);
}

ff_status_t ff_uart_boot_parse_response(const uint8_t *frame,
                                        size_t frame_bytes,
                                        ff_uart_boot_response_t *response)
{
    if (frame == NULL || response == NULL) {
        return FF_STATUS_INVALID_ARGUMENT;
    }

    response_clear(response);

    if (frame_bytes < FF_UART_BOOT_RESPONSE_HEADER_BYTES ||
        frame[0] != FF_UART_BOOT_FRAME_DIRECTION_RESPONSE) {
        return FF_STATUS_CHECK_FAILED;
    }

    const uint16_t payload_bytes = read_u16_le(&frame[2]);
    if ((size_t)payload_bytes !=
        frame_bytes - FF_UART_BOOT_RESPONSE_HEADER_BYTES) {
        return FF_STATUS_CHECK_FAILED;
    }

    response->command = (ff_uart_boot_command_t)frame[1];
    response->payload_bytes = payload_bytes;
    response->value = read_u32_le(&frame[4]);
    response->payload = &frame[FF_UART_BOOT_RESPONSE_HEADER_BYTES];

    return FF_STATUS_OK;
}

ff_status_t ff_uart_boot_response_status(
    const ff_uart_boot_response_t *response,
    size_t data_bytes,
    uint8_t *status,
    uint8_t *error)
{
    if (response == NULL || status == NULL || error == NULL) {
        return FF_STATUS_INVALID_ARGUMENT;
    }

    if (response->payload == NULL ||
        response->payload_bytes < data_bytes ||
        response->payload_bytes - data_bytes < FF_UART_BOOT_STATUS_BYTES) {
        return FF_STATUS_CHECK_FAILED;
    }

    *status = response->payload[data_bytes];
    *error = response->payload[data_bytes + 1U];
    return FF_STATUS_OK;
}

ff_status_t ff_uart_boot_validate_sync_response(
    const ff_uart_boot_response_t *response,
    uint32_t *value)
{
    if (response == NULL) {
        return FF_STATUS_INVALID_ARGUMENT;
    }

    if (response->command != FF_UART_BOOT_COMMAND_SYNC) {
        return FF_STATUS_CHECK_FAILED;
    }

    uint8_t status = 0U;
    uint8_t error = 0U;
    ff_status_t result =
        ff_uart_boot_response_status(response, 0U, &status, &error);
    if (result != FF_STATUS_OK) {
        return result;
    }

    if (status != FF_UART_BOOT_STATUS_SUCCESS ||
        error != FF_UART_BOOT_STATUS_SUCCESS) {
        return FF_STATUS_CHECK_FAILED;
    }

    if (value != NULL) {
        *value = response->value;
    }

    return FF_STATUS_OK;
}
