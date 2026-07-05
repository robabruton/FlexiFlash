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

int main(void)
{
    test_checksum();
    test_slip_encode();
    test_slip_decode();
    test_build_command();

    if (s_failures != 0) {
        fprintf(stderr,
                "%d protocol_uart_boot host test failure(s)\n",
                s_failures);
        return 1;
    }

    puts("protocol_uart_boot host tests passed");
    return 0;
}
