/**
 * @file test_protocol_uart_boot.c
 * @brief Host-side tests for ESP UART bootloader framing helpers.
 */

#include "protocol_uart_boot.h"

#include <stdio.h>
#include <string.h>

static int s_failures;

static void expect_true(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        ++s_failures;
    }
}

static void expect_status(ff_status_t actual,
                          ff_status_t expected,
                          const char *message)
{
    if (actual != expected) {
        fprintf(stderr,
                "FAIL: %s: expected %s, got %s\n",
                message,
                ff_status_name(expected),
                ff_status_name(actual));
        ++s_failures;
    }
}

static void test_checksum(void)
{
    const uint8_t payload[] = {0x01U, 0x02U};

    expect_true(ff_uart_boot_checksum(NULL, 0U) == 0xEFU,
                "empty payload checksum uses bootloader seed");
    expect_true(ff_uart_boot_checksum(payload, sizeof(payload)) == 0xECU,
                "payload checksum XORs payload bytes");
}

static void test_slip_encode(void)
{
    const uint8_t input[] = {
        0x01U,
        FF_UART_BOOT_SLIP_END,
        FF_UART_BOOT_SLIP_ESC,
        0x02U,
    };
    const uint8_t expected[] = {
        FF_UART_BOOT_SLIP_END,
        0x01U,
        FF_UART_BOOT_SLIP_ESC,
        FF_UART_BOOT_SLIP_ESC_END,
        FF_UART_BOOT_SLIP_ESC,
        FF_UART_BOOT_SLIP_ESC_ESC,
        0x02U,
        FF_UART_BOOT_SLIP_END,
    };
    uint8_t output[sizeof(expected)] = {0};
    size_t output_bytes = 0U;

    expect_status(ff_uart_boot_slip_encode(input,
                                           sizeof(input),
                                           output,
                                           sizeof(output),
                                           &output_bytes),
                  FF_STATUS_OK,
                  "SLIP encode succeeds");
    expect_true(output_bytes == sizeof(expected),
                "SLIP encode reports encoded size");
    expect_true(memcmp(output, expected, sizeof(expected)) == 0,
                "SLIP encode escapes delimiter and escape bytes");

    expect_status(ff_uart_boot_slip_encode(input,
                                           sizeof(input),
                                           output,
                                           sizeof(output) - 1U,
                                           &output_bytes),
                  FF_STATUS_NO_MEMORY,
                  "SLIP encode reports small output");
    expect_status(ff_uart_boot_slip_encode(NULL,
                                           sizeof(input),
                                           output,
                                           sizeof(output),
                                           &output_bytes),
                  FF_STATUS_INVALID_ARGUMENT,
                  "SLIP encode rejects missing input");
}

static void test_slip_decode(void)
{
    const uint8_t input[] = {
        FF_UART_BOOT_SLIP_END,
        0x01U,
        FF_UART_BOOT_SLIP_ESC,
        FF_UART_BOOT_SLIP_ESC_END,
        FF_UART_BOOT_SLIP_ESC,
        FF_UART_BOOT_SLIP_ESC_ESC,
        0x02U,
        FF_UART_BOOT_SLIP_END,
    };
    const uint8_t expected[] = {
        0x01U,
        FF_UART_BOOT_SLIP_END,
        FF_UART_BOOT_SLIP_ESC,
        0x02U,
    };
    uint8_t output[sizeof(expected)] = {0};
    size_t output_bytes = 0U;

    expect_status(ff_uart_boot_slip_decode(input,
                                           sizeof(input),
                                           output,
                                           sizeof(output),
                                           &output_bytes),
                  FF_STATUS_OK,
                  "SLIP decode succeeds");
    expect_true(output_bytes == sizeof(expected),
                "SLIP decode reports decoded size");
    expect_true(memcmp(output, expected, sizeof(expected)) == 0,
                "SLIP decode restores escaped bytes");

    expect_status(ff_uart_boot_slip_decode(input,
                                           sizeof(input),
                                           output,
                                           sizeof(output) - 1U,
                                           &output_bytes),
                  FF_STATUS_NO_MEMORY,
                  "SLIP decode reports small output");

    const uint8_t missing_end[] = {
        FF_UART_BOOT_SLIP_END,
        0x01U,
    };
    expect_status(ff_uart_boot_slip_decode(missing_end,
                                           sizeof(missing_end),
                                           output,
                                           sizeof(output),
                                           &output_bytes),
                  FF_STATUS_CHECK_FAILED,
                  "SLIP decode rejects missing end delimiter");

    const uint8_t bad_escape[] = {
        FF_UART_BOOT_SLIP_END,
        FF_UART_BOOT_SLIP_ESC,
        0x00U,
        FF_UART_BOOT_SLIP_END,
    };
    expect_status(ff_uart_boot_slip_decode(bad_escape,
                                           sizeof(bad_escape),
                                           output,
                                           sizeof(output),
                                           &output_bytes),
                  FF_STATUS_CHECK_FAILED,
                  "SLIP decode rejects invalid escape");
}

static void test_build_command(void)
{
    const uint8_t payload[] = {0x01U, 0x02U};
    uint8_t output[FF_UART_BOOT_COMMAND_HEADER_BYTES + sizeof(payload)] = {0};
    size_t output_bytes = 0U;

    expect_status(ff_uart_boot_build_command(FF_UART_BOOT_COMMAND_SYNC,
                                             payload,
                                             sizeof(payload),
                                             output,
                                             sizeof(output),
                                             &output_bytes),
                  FF_STATUS_OK,
                  "command build succeeds");
    expect_true(output_bytes == sizeof(output),
                "command build reports frame size");
    expect_true(output[0] == FF_UART_BOOT_FRAME_DIRECTION_COMMAND,
                "command frame direction is command");
    expect_true(output[1] == FF_UART_BOOT_COMMAND_SYNC,
                "command frame stores operation");
    expect_true(output[2] == sizeof(payload) && output[3] == 0U,
                "command frame stores little-endian payload length");
    expect_true(output[4] == 0xECU &&
                    output[5] == 0U &&
                    output[6] == 0U &&
                    output[7] == 0U,
                "command frame stores little-endian checksum");
    expect_true(memcmp(&output[FF_UART_BOOT_COMMAND_HEADER_BYTES],
                       payload,
                       sizeof(payload)) == 0,
                "command frame appends payload");

    expect_status(ff_uart_boot_build_command(FF_UART_BOOT_COMMAND_SYNC,
                                             payload,
                                             sizeof(payload),
                                             output,
                                             sizeof(output) - 1U,
                                             &output_bytes),
                  FF_STATUS_NO_MEMORY,
                  "command build reports small output");
    expect_status(ff_uart_boot_build_command(FF_UART_BOOT_COMMAND_SYNC,
                                             NULL,
                                             sizeof(payload),
                                             output,
                                             sizeof(output),
                                             &output_bytes),
                  FF_STATUS_INVALID_ARGUMENT,
                  "command build rejects missing payload");
}

static void test_build_sync_command(void)
{
    uint8_t output[FF_UART_BOOT_COMMAND_HEADER_BYTES +
                   FF_UART_BOOT_SYNC_PAYLOAD_BYTES] = {0};
    size_t output_bytes = 0U;

    expect_status(ff_uart_boot_build_sync_command(output,
                                                  sizeof(output),
                                                  &output_bytes),
                  FF_STATUS_OK,
                  "sync command build succeeds");
    expect_true(output_bytes == sizeof(output),
                "sync command reports frame size");
    expect_true(output[0] == FF_UART_BOOT_FRAME_DIRECTION_COMMAND,
                "sync command frame direction is command");
    expect_true(output[1] == FF_UART_BOOT_COMMAND_SYNC,
                "sync command stores operation");
    expect_true(output[2] == FF_UART_BOOT_SYNC_PAYLOAD_BYTES &&
                    output[3] == 0U,
                "sync command stores payload length");
    expect_true(output[4] == 0xDDU &&
                    output[5] == 0U &&
                    output[6] == 0U &&
                    output[7] == 0U,
                "sync command stores payload checksum");
    expect_true(output[FF_UART_BOOT_COMMAND_HEADER_BYTES] == 0x07U &&
                    output[FF_UART_BOOT_COMMAND_HEADER_BYTES + 1U] == 0x07U &&
                    output[FF_UART_BOOT_COMMAND_HEADER_BYTES + 2U] == 0x12U &&
                    output[FF_UART_BOOT_COMMAND_HEADER_BYTES + 3U] == 0x20U,
                "sync command stores payload prefix");

    for (size_t i = 4U; i < FF_UART_BOOT_SYNC_PAYLOAD_BYTES; ++i) {
        expect_true(output[FF_UART_BOOT_COMMAND_HEADER_BYTES + i] == 0x55U,
                    "sync command stores payload fill bytes");
    }

    expect_status(ff_uart_boot_build_sync_command(output,
                                                  sizeof(output) - 1U,
                                                  &output_bytes),
                  FF_STATUS_NO_MEMORY,
                  "sync command reports small output");
    expect_status(ff_uart_boot_build_sync_command(NULL,
                                                  sizeof(output),
                                                  &output_bytes),
                  FF_STATUS_INVALID_ARGUMENT,
                  "sync command rejects missing output");
}

static void test_parse_response(void)
{
    const uint8_t frame[] = {
        FF_UART_BOOT_FRAME_DIRECTION_RESPONSE,
        FF_UART_BOOT_COMMAND_SYNC,
        0x04U,
        0x00U,
        0x78U,
        0x56U,
        0x34U,
        0x12U,
        0xAAU,
        0xBBU,
        0x00U,
        0x00U,
    };
    ff_uart_boot_response_t response = {0};

    expect_status(ff_uart_boot_parse_response(frame,
                                              sizeof(frame),
                                              &response),
                  FF_STATUS_OK,
                  "response parser accepts valid frame");
    expect_true(response.command == FF_UART_BOOT_COMMAND_SYNC,
                "response parser stores opcode");
    expect_true(response.value == 0x12345678U,
                "response parser stores little-endian value");
    expect_true(response.payload_bytes == 4U,
                "response parser stores payload length");
    expect_true(response.payload == &frame[FF_UART_BOOT_RESPONSE_HEADER_BYTES],
                "response parser points into input payload");
    expect_true(response.payload[0] == 0xAAU && response.payload[1] == 0xBBU,
                "response parser exposes payload bytes");

    uint8_t status = 0xFFU;
    uint8_t error = 0xFFU;
    expect_status(ff_uart_boot_response_status(&response,
                                               2U,
                                               &status,
                                               &error),
                  FF_STATUS_OK,
                  "response status reads after command data");
    expect_true(status == 0U && error == 0U,
                "response status reports success bytes");

    uint8_t bad_direction[sizeof(frame)] = {0};
    memcpy(bad_direction, frame, sizeof(frame));
    bad_direction[0] = FF_UART_BOOT_FRAME_DIRECTION_COMMAND;
    expect_status(ff_uart_boot_parse_response(bad_direction,
                                              sizeof(bad_direction),
                                              &response),
                  FF_STATUS_CHECK_FAILED,
                  "response parser rejects command-direction frames");

    uint8_t bad_length[sizeof(frame)] = {0};
    memcpy(bad_length, frame, sizeof(frame));
    bad_length[2] = 0x05U;
    expect_status(ff_uart_boot_parse_response(bad_length,
                                              sizeof(bad_length),
                                              &response),
                  FF_STATUS_CHECK_FAILED,
                  "response parser rejects inconsistent lengths");

    expect_status(ff_uart_boot_parse_response(frame,
                                              FF_UART_BOOT_RESPONSE_HEADER_BYTES - 1U,
                                              &response),
                  FF_STATUS_CHECK_FAILED,
                  "response parser rejects short frames");
    expect_status(ff_uart_boot_parse_response(NULL,
                                              sizeof(frame),
                                              &response),
                  FF_STATUS_INVALID_ARGUMENT,
                  "response parser rejects missing frame");
    expect_status(ff_uart_boot_parse_response(frame,
                                              sizeof(frame),
                                              NULL),
                  FF_STATUS_INVALID_ARGUMENT,
                  "response parser rejects missing output");
}

static void test_response_status(void)
{
    const uint8_t frame[] = {
        FF_UART_BOOT_FRAME_DIRECTION_RESPONSE,
        FF_UART_BOOT_COMMAND_SYNC,
        0x02U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x05U,
        0x06U,
    };
    ff_uart_boot_response_t response = {0};
    uint8_t status = 0U;
    uint8_t error = 0U;

    expect_status(ff_uart_boot_parse_response(frame,
                                              sizeof(frame),
                                              &response),
                  FF_STATUS_OK,
                  "response parser accepts status-only frame");
    expect_status(ff_uart_boot_response_status(&response,
                                               0U,
                                               &status,
                                               &error),
                  FF_STATUS_OK,
                  "response status reads status-only payload");
    expect_true(status == 0x05U && error == 0x06U,
                "response status exposes failure bytes");
    expect_status(ff_uart_boot_response_status(&response,
                                               1U,
                                               &status,
                                               &error),
                  FF_STATUS_CHECK_FAILED,
                  "response status rejects missing status bytes");
    expect_status(ff_uart_boot_response_status(NULL,
                                               0U,
                                               &status,
                                               &error),
                  FF_STATUS_INVALID_ARGUMENT,
                  "response status rejects missing response");
    expect_status(ff_uart_boot_response_status(&response,
                                               0U,
                                               NULL,
                                               &error),
                  FF_STATUS_INVALID_ARGUMENT,
                  "response status rejects missing status output");
}

int main(void)
{
    test_checksum();
    test_slip_encode();
    test_slip_decode();
    test_build_command();
    test_build_sync_command();
    test_parse_response();
    test_response_status();

    if (s_failures != 0) {
        fprintf(stderr,
                "%d protocol_uart_boot host test failure(s)\n",
                s_failures);
        return 1;
    }

    puts("protocol_uart_boot host tests passed");
    return 0;
}
