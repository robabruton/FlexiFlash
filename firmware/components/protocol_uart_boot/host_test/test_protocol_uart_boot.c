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

static void expect_accumulator_push(
    ff_uart_boot_frame_accumulator_t *accumulator,
    uint8_t byte,
    ff_status_t expected_status,
    bool expected_ready,
    size_t expected_frame_bytes,
    const char *message)
{
    bool frame_ready = true;
    size_t frame_bytes = 0xFFFFU;
    ff_status_t status =
        ff_uart_boot_frame_accumulator_push(accumulator,
                                            byte,
                                            &frame_ready,
                                            &frame_bytes);

    expect_status(status, expected_status, message);
    expect_true(frame_ready == expected_ready,
                "accumulator frame-ready flag matches expectation");
    expect_true(frame_bytes == expected_frame_bytes,
                "accumulator frame byte count matches expectation");
}

static void expect_reader_push(ff_uart_boot_response_reader_t *reader,
                               uint8_t byte,
                               ff_status_t expected_status,
                               bool expected_ready,
                               const char *message)
{
    bool response_ready = true;
    ff_uart_boot_response_t response = {
        .command = FF_UART_BOOT_COMMAND_SYNC,
        .value = 0xFFFFFFFFU,
        .payload = (const uint8_t *)0x1,
        .payload_bytes = 0xFFFFU,
    };
    ff_status_t status =
        ff_uart_boot_response_reader_push(reader,
                                          byte,
                                          &response_ready,
                                          &response);

    expect_status(status, expected_status, message);
    expect_true(response_ready == expected_ready,
                "reader response-ready flag matches expectation");

    if (!expected_ready) {
        expect_true(response.payload == NULL && response.payload_bytes == 0U,
                    "reader clears response view while incomplete");
    }
}

static void test_checksum(void)
{
    const uint8_t payload[] = {0x01U, 0x02U};

    expect_true(
        ff_uart_boot_checksum(NULL, 0U) == 0xEFU,
        "empty payload checksum uses bootloader seed");
    expect_true(
        ff_uart_boot_checksum(payload, sizeof(payload)) == 0xECU,
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

    expect_status(
        ff_uart_boot_slip_encode(input,
                                 sizeof(input),
                                 output,
                                 sizeof(output),
                                 &output_bytes),
        FF_STATUS_OK,
        "SLIP encode succeeds");
    expect_true(output_bytes == sizeof(expected),
                "SLIP encode reports encoded size");
    expect_true(
        memcmp(output, expected, sizeof(expected)) == 0,
        "SLIP encode escapes delimiter and escape bytes");

    expect_status(
        ff_uart_boot_slip_encode(input,
                                 sizeof(input),
                                 output,
                                 sizeof(output) - 1U,
                                 &output_bytes),
        FF_STATUS_NO_MEMORY,
        "SLIP encode reports small output");
    expect_status(
        ff_uart_boot_slip_encode(NULL,
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

    expect_status(
        ff_uart_boot_slip_decode(input,
                                 sizeof(input),
                                 output,
                                 sizeof(output),
                                 &output_bytes),
        FF_STATUS_OK,
        "SLIP decode succeeds");
    expect_true(output_bytes == sizeof(expected),
                "SLIP decode reports decoded size");
    expect_true(
        memcmp(output, expected, sizeof(expected)) == 0,
        "SLIP decode restores escaped bytes");

    expect_status(
        ff_uart_boot_slip_decode(input,
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
    expect_status(
        ff_uart_boot_slip_decode(missing_end,
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
    expect_status(
        ff_uart_boot_slip_decode(bad_escape,
                                 sizeof(bad_escape),
                                 output,
                                 sizeof(output),
                                 &output_bytes),
        FF_STATUS_CHECK_FAILED,
        "SLIP decode rejects invalid escape");
}

static void test_frame_accumulator_init_reset(void)
{
    uint8_t frame[4] = {0};
    ff_uart_boot_frame_accumulator_t accumulator = {0};

    expect_status(
        ff_uart_boot_frame_accumulator_init(&accumulator,
                                            frame,
                                            sizeof(frame)),
        FF_STATUS_OK,
        "accumulator init succeeds");
    expect_true(accumulator.frame == frame,
                "accumulator stores caller frame buffer");
    expect_true(accumulator.capacity == sizeof(frame),
                "accumulator stores caller frame capacity");
    expect_true(accumulator.frame_bytes == 0U,
                "accumulator starts with no decoded bytes");
    expect_true(!accumulator.in_frame && !accumulator.escaping,
                "accumulator starts outside a frame");

    accumulator.frame_bytes = 2U;
    accumulator.in_frame = true;
    accumulator.escaping = true;
    expect_status(
        ff_uart_boot_frame_accumulator_reset(&accumulator),
        FF_STATUS_OK,
        "accumulator reset succeeds");
    expect_true(accumulator.frame_bytes == 0U,
                "accumulator reset clears decoded bytes");
    expect_true(!accumulator.in_frame && !accumulator.escaping,
                "accumulator reset clears receive state");

    expect_status(
        ff_uart_boot_frame_accumulator_init(NULL,
                                            frame,
                                            sizeof(frame)),
        FF_STATUS_INVALID_ARGUMENT,
        "accumulator init rejects missing state");
    expect_status(
        ff_uart_boot_frame_accumulator_init(&accumulator, NULL, sizeof(frame)),
        FF_STATUS_INVALID_ARGUMENT,
        "accumulator init rejects missing frame buffer");
    expect_status(
        ff_uart_boot_frame_accumulator_init(&accumulator, frame, 0U),
        FF_STATUS_INVALID_ARGUMENT,
        "accumulator init rejects empty frame buffer");
    expect_status(
        ff_uart_boot_frame_accumulator_reset(NULL),
        FF_STATUS_INVALID_ARGUMENT,
        "accumulator reset rejects missing state");
}

static void test_frame_accumulator_packet(void)
{
    uint8_t frame[4] = {0};
    ff_uart_boot_frame_accumulator_t accumulator = {0};

    expect_status(
        ff_uart_boot_frame_accumulator_init(&accumulator,
                                            frame,
                                            sizeof(frame)),
        FF_STATUS_OK,
        "accumulator init succeeds for packet test");

    expect_accumulator_push(&accumulator,
                            0x99U,
                            FF_STATUS_OK,
                            false,
                            0U,
                            "accumulator ignores pre-frame bytes");
    expect_accumulator_push(&accumulator,
                            FF_UART_BOOT_SLIP_END,
                            FF_STATUS_OK,
                            false,
                            0U,
                            "accumulator starts on delimiter");
    expect_accumulator_push(&accumulator,
                            0x01U,
                            FF_STATUS_OK,
                            false,
                            0U,
                            "accumulator stores raw frame byte");
    expect_accumulator_push(&accumulator,
                            FF_UART_BOOT_SLIP_ESC,
                            FF_STATUS_OK,
                            false,
                            0U,
                            "accumulator starts escape sequence");
    expect_accumulator_push(&accumulator,
                            FF_UART_BOOT_SLIP_ESC_END,
                            FF_STATUS_OK,
                            false,
                            0U,
                            "accumulator decodes escaped delimiter");
    expect_accumulator_push(&accumulator,
                            FF_UART_BOOT_SLIP_ESC,
                            FF_STATUS_OK,
                            false,
                            0U,
                            "accumulator starts second escape sequence");
    expect_accumulator_push(&accumulator,
                            FF_UART_BOOT_SLIP_ESC_ESC,
                            FF_STATUS_OK,
                            false,
                            0U,
                            "accumulator decodes escaped escape byte");
    expect_accumulator_push(&accumulator,
                            FF_UART_BOOT_SLIP_END,
                            FF_STATUS_OK,
                            true,
                            3U,
                            "accumulator reports complete frame");

    expect_true(frame[0] == 0x01U &&
                    frame[1] == FF_UART_BOOT_SLIP_END &&
                    frame[2] == FF_UART_BOOT_SLIP_ESC,
                "accumulator stores decoded frame bytes");
}

static void test_frame_accumulator_errors(void)
{
    uint8_t frame[2] = {0};
    ff_uart_boot_frame_accumulator_t accumulator = {0};

    expect_status(
        ff_uart_boot_frame_accumulator_init(&accumulator,
                                            frame,
                                            sizeof(frame)),
        FF_STATUS_OK,
        "accumulator init succeeds for error test");

    expect_accumulator_push(&accumulator,
                            FF_UART_BOOT_SLIP_END,
                            FF_STATUS_OK,
                            false,
                            0U,
                            "accumulator starts malformed frame");
    expect_accumulator_push(&accumulator,
                            FF_UART_BOOT_SLIP_ESC,
                            FF_STATUS_OK,
                            false,
                            0U,
                            "accumulator records pending escape");
    expect_accumulator_push(&accumulator,
                            0x00U,
                            FF_STATUS_CHECK_FAILED,
                            false,
                            0U,
                            "accumulator rejects malformed escape");
    expect_true(!accumulator.in_frame && !accumulator.escaping,
                "accumulator clears state after malformed escape");

    expect_accumulator_push(&accumulator,
                            FF_UART_BOOT_SLIP_END,
                            FF_STATUS_OK,
                            false,
                            0U,
                            "accumulator restarts after malformed escape");
    expect_accumulator_push(&accumulator,
                            0x11U,
                            FF_STATUS_OK,
                            false,
                            0U,
                            "accumulator stores first capacity byte");
    expect_accumulator_push(&accumulator,
                            0x22U,
                            FF_STATUS_OK,
                            false,
                            0U,
                            "accumulator stores second capacity byte");
    expect_accumulator_push(&accumulator,
                            0x33U,
                            FF_STATUS_NO_MEMORY,
                            false,
                            0U,
                            "accumulator rejects oversized frame");
    expect_true(!accumulator.in_frame && accumulator.frame_bytes == 0U,
                "accumulator clears state after oversized frame");

    bool frame_ready = false;
    size_t frame_bytes = 0U;
    expect_status(
        ff_uart_boot_frame_accumulator_push(&accumulator,
                                            FF_UART_BOOT_SLIP_END,
                                            NULL,
                                            &frame_bytes),
        FF_STATUS_INVALID_ARGUMENT,
        "accumulator push rejects missing ready output");
    expect_status(
        ff_uart_boot_frame_accumulator_push(&accumulator,
                                            FF_UART_BOOT_SLIP_END,
                                            &frame_ready,
                                            NULL),
        FF_STATUS_INVALID_ARGUMENT,
        "accumulator push rejects missing byte count output");
}

static void test_response_reader_init_reset(void)
{
    uint8_t frame[16] = {0};
    ff_uart_boot_response_reader_t reader = {0};

    expect_status(
        ff_uart_boot_response_reader_init(&reader, frame, sizeof(frame)),
        FF_STATUS_OK,
        "response reader init succeeds");
    expect_true(reader.accumulator.frame == frame,
                "response reader stores frame buffer");
    expect_true(reader.accumulator.capacity == sizeof(frame),
                "response reader stores frame capacity");

    reader.accumulator.frame_bytes = 2U;
    reader.accumulator.in_frame = true;
    reader.accumulator.escaping = true;
    expect_status(
        ff_uart_boot_response_reader_reset(&reader),
        FF_STATUS_OK,
        "response reader reset succeeds");
    expect_true(reader.accumulator.frame_bytes == 0U,
                "response reader reset clears decoded bytes");
    expect_true(!reader.accumulator.in_frame &&
                    !reader.accumulator.escaping,
                "response reader reset clears receive state");

    expect_status(
        ff_uart_boot_response_reader_init(NULL, frame, sizeof(frame)),
        FF_STATUS_INVALID_ARGUMENT,
        "response reader init rejects missing state");
    expect_status(
        ff_uart_boot_response_reader_init(&reader, NULL, sizeof(frame)),
        FF_STATUS_INVALID_ARGUMENT,
        "response reader init rejects missing frame buffer");
    expect_status(
        ff_uart_boot_response_reader_reset(NULL),
        FF_STATUS_INVALID_ARGUMENT,
        "response reader reset rejects missing state");
}

static void test_response_reader_packet(void)
{
    const uint8_t raw_response[] = {
        FF_UART_BOOT_FRAME_DIRECTION_RESPONSE,
        FF_UART_BOOT_COMMAND_SYNC,
        0x02U,
        0x00U,
        0x78U,
        0x56U,
        0x34U,
        0x12U,
        FF_UART_BOOT_STATUS_SUCCESS,
        FF_UART_BOOT_STATUS_SUCCESS,
    };
    uint8_t encoded[32] = {0};
    size_t encoded_bytes = 0U;
    uint8_t frame[sizeof(raw_response)] = {0};
    ff_uart_boot_response_reader_t reader = {0};
    ff_uart_boot_response_t response = {0};
    bool response_ready = false;

    expect_status(
        ff_uart_boot_slip_encode(raw_response,
                                 sizeof(raw_response),
                                 encoded,
                                 sizeof(encoded),
                                 &encoded_bytes),
        FF_STATUS_OK,
        "response reader fixture encodes response");
    expect_status(
        ff_uart_boot_response_reader_init(&reader, frame, sizeof(frame)),
        FF_STATUS_OK,
        "response reader init succeeds for packet test");

    for (size_t i = 0U; i + 1U < encoded_bytes; ++i) {
        expect_status(
            ff_uart_boot_response_reader_push(&reader,
                                              encoded[i],
                                              &response_ready,
                                              &response),
            FF_STATUS_OK,
            "response reader accepts partial response byte");
        expect_true(!response_ready, "response reader waits for full frame");
        expect_true(response.payload == NULL && response.payload_bytes == 0U,
                    "response reader clears partial response view");
    }

    expect_status(
        ff_uart_boot_response_reader_push(&reader,
                                          encoded[encoded_bytes - 1U],
                                          &response_ready,
                                          &response),
        FF_STATUS_OK,
        "response reader accepts closing delimiter");
    expect_true(response_ready, "response reader reports parsed response");
    expect_true(response.command == FF_UART_BOOT_COMMAND_SYNC,
                "response reader stores parsed command");
    expect_true(response.value == 0x12345678U,
                "response reader stores parsed value");
    expect_true(response.payload == &frame[FF_UART_BOOT_RESPONSE_HEADER_BYTES],
                "response reader payload points into frame storage");
    expect_true(response.payload_bytes == FF_UART_BOOT_STATUS_BYTES,
                "response reader stores parsed payload length");
}

static void test_response_reader_errors(void)
{
    const uint8_t bad_response[] = {
        FF_UART_BOOT_FRAME_DIRECTION_COMMAND,
        FF_UART_BOOT_COMMAND_SYNC,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
    };
    uint8_t encoded_bad[16] = {0};
    size_t encoded_bad_bytes = 0U;
    uint8_t frame[16] = {0};
    ff_uart_boot_response_reader_t reader = {0};
    ff_uart_boot_response_t response = {0};
    bool response_ready = false;

    expect_status(
        ff_uart_boot_slip_encode(bad_response,
                                 sizeof(bad_response),
                                 encoded_bad,
                                 sizeof(encoded_bad),
                                 &encoded_bad_bytes),
        FF_STATUS_OK,
        "response reader fixture encodes malformed response");
    expect_status(
        ff_uart_boot_response_reader_init(&reader, frame, sizeof(frame)),
        FF_STATUS_OK,
        "response reader init succeeds for error test");

    for (size_t i = 0U; i + 1U < encoded_bad_bytes; ++i) {
        expect_status(
            ff_uart_boot_response_reader_push(&reader,
                                              encoded_bad[i],
                                              &response_ready,
                                              &response),
            FF_STATUS_OK,
            "response reader accepts malformed response prefix");
        expect_true(!response_ready,
                    "response reader waits before malformed frame closes");
    }

    expect_status(
        ff_uart_boot_response_reader_push(
            &reader,
            encoded_bad[encoded_bad_bytes - 1U],
            &response_ready,
            &response),
        FF_STATUS_CHECK_FAILED,
        "response reader rejects malformed decoded response");
    expect_true(!response_ready,
                "response reader does not report malformed response ready");
    expect_true(!reader.accumulator.in_frame &&
                    reader.accumulator.frame_bytes == 0U,
                "response reader resets after malformed response");

    expect_reader_push(&reader,
                       FF_UART_BOOT_SLIP_END,
                       FF_STATUS_OK,
                       false,
                       "response reader restarts after malformed response");
    expect_reader_push(&reader,
                       FF_UART_BOOT_SLIP_ESC,
                       FF_STATUS_OK,
                       false,
                       "response reader records pending escape");
    expect_reader_push(&reader,
                       0x00U,
                       FF_STATUS_CHECK_FAILED,
                       false,
                       "response reader rejects malformed SLIP escape");
    expect_true(!reader.accumulator.in_frame,
                "response reader resets after malformed SLIP escape");

    expect_status(
        ff_uart_boot_response_reader_push(NULL,
                                          FF_UART_BOOT_SLIP_END,
                                          &response_ready,
                                          &response),
        FF_STATUS_INVALID_ARGUMENT,
        "response reader push rejects missing state");
    expect_status(
        ff_uart_boot_response_reader_push(&reader,
                                          FF_UART_BOOT_SLIP_END,
                                          NULL,
                                          &response),
        FF_STATUS_INVALID_ARGUMENT,
        "response reader push rejects missing ready output");
    expect_status(
        ff_uart_boot_response_reader_push(&reader,
                                          FF_UART_BOOT_SLIP_END,
                                          &response_ready,
                                          NULL),
        FF_STATUS_INVALID_ARGUMENT,
        "response reader push rejects missing response output");
}

static void test_build_command(void)
{
    const uint8_t payload[] = {0x01U, 0x02U};
    uint8_t output[FF_UART_BOOT_COMMAND_HEADER_BYTES + sizeof(payload)] = {0};
    size_t output_bytes = 0U;

    expect_status(
        ff_uart_boot_build_command(FF_UART_BOOT_COMMAND_SYNC,
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
    expect_true(
        memcmp(&output[FF_UART_BOOT_COMMAND_HEADER_BYTES],
               payload,
               sizeof(payload)) == 0,
        "command frame appends payload");

    expect_status(
        ff_uart_boot_build_command(FF_UART_BOOT_COMMAND_SYNC,
                                   payload,
                                   sizeof(payload),
                                   output,
                                   sizeof(output) - 1U,
                                   &output_bytes),
        FF_STATUS_NO_MEMORY,
        "command build reports small output");
    expect_status(
        ff_uart_boot_build_command(FF_UART_BOOT_COMMAND_SYNC,
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

    expect_status(
        ff_uart_boot_build_sync_command(output,
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

    expect_status(
        ff_uart_boot_build_sync_command(output,
                                        sizeof(output) - 1U,
                                        &output_bytes),
        FF_STATUS_NO_MEMORY,
        "sync command reports small output");
    expect_status(
        ff_uart_boot_build_sync_command(NULL,
                                        sizeof(output),
                                        &output_bytes),
        FF_STATUS_INVALID_ARGUMENT,
        "sync command rejects missing output");
}

static void test_build_sync_packet(void)
{
    uint8_t command[FF_UART_BOOT_SYNC_COMMAND_BYTES] = {0};
    size_t command_bytes = 0U;
    uint8_t expected[FF_UART_BOOT_SYNC_PACKET_BYTES] = {0};
    size_t expected_bytes = 0U;
    uint8_t output[FF_UART_BOOT_SYNC_PACKET_BYTES] = {0};
    size_t output_bytes = 0U;

    expect_status(
        ff_uart_boot_build_sync_command(command,
                                        sizeof(command),
                                        &command_bytes),
        FF_STATUS_OK,
        "sync packet fixture builds command");
    expect_status(
        ff_uart_boot_slip_encode(command,
                                 command_bytes,
                                 expected,
                                 sizeof(expected),
                                 &expected_bytes),
        FF_STATUS_OK,
        "sync packet fixture encodes command");

    expect_status(
        ff_uart_boot_build_sync_packet(output,
                                       sizeof(output),
                                       &output_bytes),
        FF_STATUS_OK,
        "sync packet build succeeds");
    expect_true(output_bytes == expected_bytes,
                "sync packet reports encoded size");
    expect_true(output_bytes == FF_UART_BOOT_SYNC_PACKET_BYTES,
                "sync packet uses fixed encoded size");
    expect_true(memcmp(output, expected, expected_bytes) == 0,
                "sync packet matches encoded sync command");

    expect_status(
        ff_uart_boot_build_sync_packet(output,
                                       sizeof(output) - 1U,
                                       &output_bytes),
        FF_STATUS_NO_MEMORY,
        "sync packet reports small output");
    expect_status(
        ff_uart_boot_build_sync_packet(NULL,
                                       sizeof(output),
                                       &output_bytes),
        FF_STATUS_INVALID_ARGUMENT,
        "sync packet rejects missing output");
    expect_status(
        ff_uart_boot_build_sync_packet(output, sizeof(output), NULL),
        FF_STATUS_INVALID_ARGUMENT,
        "sync packet rejects missing byte count output");
}

static void test_sync_exchange_response(void)
{
    const uint8_t raw_response[] = {
        FF_UART_BOOT_FRAME_DIRECTION_RESPONSE,
        FF_UART_BOOT_COMMAND_SYNC,
        0x02U,
        0x00U,
        0x78U,
        0x56U,
        0x34U,
        0x12U,
        FF_UART_BOOT_STATUS_SUCCESS,
        FF_UART_BOOT_STATUS_SUCCESS,
    };
    uint8_t encoded[32] = {0};
    size_t encoded_bytes = 0U;
    uint8_t frame[sizeof(raw_response)] = {0};
    ff_uart_boot_response_reader_t reader = {0};
    bool sync_ready = false;
    uint32_t value = 0U;

    expect_status(
        ff_uart_boot_slip_encode(raw_response,
                                 sizeof(raw_response),
                                 encoded,
                                 sizeof(encoded),
                                 &encoded_bytes),
        FF_STATUS_OK,
        "sync exchange fixture encodes response");
    expect_status(
        ff_uart_boot_response_reader_init(&reader, frame, sizeof(frame)),
        FF_STATUS_OK,
        "sync exchange reader init succeeds");

    for (size_t i = 0U; i + 1U < encoded_bytes; ++i) {
        expect_status(
            ff_uart_boot_sync_exchange_push(&reader,
                                            encoded[i],
                                            &sync_ready,
                                            &value),
            FF_STATUS_OK,
            "sync exchange accepts partial response byte");
        expect_true(!sync_ready, "sync exchange waits for full response");
    }

    expect_status(
        ff_uart_boot_sync_exchange_push(&reader,
                                        encoded[encoded_bytes - 1U],
                                        &sync_ready,
                                        &value),
        FF_STATUS_OK,
        "sync exchange accepts completed response");
    expect_true(sync_ready, "sync exchange reports validated response");
    expect_true(value == 0x12345678U,
                "sync exchange returns response value");

    expect_status(
        ff_uart_boot_sync_exchange_push(NULL,
                                        FF_UART_BOOT_SLIP_END,
                                        &sync_ready,
                                        &value),
        FF_STATUS_INVALID_ARGUMENT,
        "sync exchange rejects missing reader");
    expect_status(
        ff_uart_boot_sync_exchange_push(&reader,
                                        FF_UART_BOOT_SLIP_END,
                                        NULL,
                                        &value),
        FF_STATUS_INVALID_ARGUMENT,
        "sync exchange rejects missing ready output");
}

static void test_sync_exchange_errors(void)
{
    const uint8_t wrong_command[] = {
        FF_UART_BOOT_FRAME_DIRECTION_RESPONSE,
        0x09U,
        0x02U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        FF_UART_BOOT_STATUS_SUCCESS,
        FF_UART_BOOT_STATUS_SUCCESS,
    };
    uint8_t encoded[32] = {0};
    size_t encoded_bytes = 0U;
    uint8_t frame[sizeof(wrong_command)] = {0};
    ff_uart_boot_response_reader_t reader = {0};
    bool sync_ready = false;

    expect_status(
        ff_uart_boot_slip_encode(wrong_command,
                                 sizeof(wrong_command),
                                 encoded,
                                 sizeof(encoded),
                                 &encoded_bytes),
        FF_STATUS_OK,
        "sync exchange fixture encodes wrong-command response");
    expect_status(
        ff_uart_boot_response_reader_init(&reader, frame, sizeof(frame)),
        FF_STATUS_OK,
        "sync exchange reader init succeeds for error test");

    for (size_t i = 0U; i + 1U < encoded_bytes; ++i) {
        expect_status(
            ff_uart_boot_sync_exchange_push(&reader,
                                            encoded[i],
                                            &sync_ready,
                                            NULL),
            FF_STATUS_OK,
            "sync exchange accepts wrong-command prefix");
        expect_true(!sync_ready,
                    "sync exchange waits before wrong-command frame closes");
    }

    expect_status(
        ff_uart_boot_sync_exchange_push(&reader,
                                        encoded[encoded_bytes - 1U],
                                        &sync_ready,
                                        NULL),
        FF_STATUS_CHECK_FAILED,
        "sync exchange rejects wrong command response");
    expect_true(!sync_ready,
                "sync exchange does not report invalid response ready");
    expect_true(!reader.accumulator.in_frame &&
                    reader.accumulator.frame_bytes == 0U,
                "sync exchange resets reader after invalid response");
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

    expect_status(
        ff_uart_boot_parse_response(frame,
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
    expect_status(
        ff_uart_boot_response_status(&response,
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
    expect_status(
        ff_uart_boot_parse_response(bad_direction,
                                    sizeof(bad_direction),
                                    &response),
        FF_STATUS_CHECK_FAILED,
        "response parser rejects command-direction frames");

    uint8_t bad_length[sizeof(frame)] = {0};
    memcpy(bad_length, frame, sizeof(frame));
    bad_length[2] = 0x05U;
    expect_status(
        ff_uart_boot_parse_response(bad_length,
                                    sizeof(bad_length),
                                    &response),
        FF_STATUS_CHECK_FAILED,
        "response parser rejects inconsistent lengths");

    expect_status(
        ff_uart_boot_parse_response(
            frame,
            FF_UART_BOOT_RESPONSE_HEADER_BYTES - 1U,
            &response),
        FF_STATUS_CHECK_FAILED,
        "response parser rejects short frames");
    expect_status(
        ff_uart_boot_parse_response(NULL,
                                    sizeof(frame),
                                    &response),
        FF_STATUS_INVALID_ARGUMENT,
        "response parser rejects missing frame");
    expect_status(
        ff_uart_boot_parse_response(frame,
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

    expect_status(
        ff_uart_boot_parse_response(frame,
                                    sizeof(frame),
                                    &response),
        FF_STATUS_OK,
        "response parser accepts status-only frame");
    expect_status(
        ff_uart_boot_response_status(&response,
                                     0U,
                                     &status,
                                     &error),
        FF_STATUS_OK,
        "response status reads status-only payload");
    expect_true(status == 0x05U && error == 0x06U,
                "response status exposes failure bytes");
    expect_status(
        ff_uart_boot_response_status(&response,
                                     1U,
                                     &status,
                                     &error),
        FF_STATUS_CHECK_FAILED,
        "response status rejects missing status bytes");
    expect_status(
        ff_uart_boot_response_status(NULL,
                                     0U,
                                     &status,
                                     &error),
        FF_STATUS_INVALID_ARGUMENT,
        "response status rejects missing response");
    expect_status(
        ff_uart_boot_response_status(&response,
                                     0U,
                                     NULL,
                                     &error),
        FF_STATUS_INVALID_ARGUMENT,
        "response status rejects missing status output");
}

static void test_validate_sync_response(void)
{
    const uint8_t frame[] = {
        FF_UART_BOOT_FRAME_DIRECTION_RESPONSE,
        FF_UART_BOOT_COMMAND_SYNC,
        0x02U,
        0x00U,
        0x78U,
        0x56U,
        0x34U,
        0x12U,
        FF_UART_BOOT_STATUS_SUCCESS,
        FF_UART_BOOT_STATUS_SUCCESS,
    };
    ff_uart_boot_response_t response = {0};
    uint32_t value = 0U;

    expect_status(
        ff_uart_boot_parse_response(frame,
                                    sizeof(frame),
                                    &response),
        FF_STATUS_OK,
        "sync response parser accepts valid frame");
    expect_status(
        ff_uart_boot_validate_sync_response(&response, &value),
        FF_STATUS_OK,
        "sync response validator accepts success status");
    expect_true(value == 0x12345678U,
                "sync response validator returns response value");
    expect_status(
        ff_uart_boot_validate_sync_response(&response, NULL),
        FF_STATUS_OK,
        "sync response validator accepts ignored value");

    ff_uart_boot_response_t wrong_command = response;
    wrong_command.command = (ff_uart_boot_command_t)0x09U;
    expect_status(
        ff_uart_boot_validate_sync_response(&wrong_command, &value),
        FF_STATUS_CHECK_FAILED,
        "sync response validator rejects wrong command");

    uint8_t failure_frame[sizeof(frame)] = {0};
    memcpy(failure_frame, frame, sizeof(frame));
    failure_frame[FF_UART_BOOT_RESPONSE_HEADER_BYTES] = 0x01U;
    expect_status(
        ff_uart_boot_parse_response(failure_frame,
                                    sizeof(failure_frame),
                                    &response),
        FF_STATUS_OK,
        "sync response parser accepts failure status frame");
    expect_status(
        ff_uart_boot_validate_sync_response(&response, &value),
        FF_STATUS_CHECK_FAILED,
        "sync response validator rejects failure status");

    uint8_t short_payload[sizeof(frame) - 1U] = {0};
    memcpy(short_payload, frame, sizeof(short_payload));
    short_payload[2] = 0x01U;
    expect_status(
        ff_uart_boot_parse_response(short_payload,
                                    sizeof(short_payload),
                                    &response),
        FF_STATUS_OK,
        "sync response parser accepts short payload frame");
    expect_status(
        ff_uart_boot_validate_sync_response(&response, &value),
        FF_STATUS_CHECK_FAILED,
        "sync response validator rejects missing status");
    expect_status(
        ff_uart_boot_validate_sync_response(NULL, &value),
        FF_STATUS_INVALID_ARGUMENT,
        "sync response validator rejects missing response");
}

int main(void)
{
    test_checksum();
    test_slip_encode();
    test_slip_decode();
    test_frame_accumulator_init_reset();
    test_frame_accumulator_packet();
    test_frame_accumulator_errors();
    test_response_reader_init_reset();
    test_response_reader_packet();
    test_response_reader_errors();
    test_build_command();
    test_build_sync_command();
    test_build_sync_packet();
    test_sync_exchange_response();
    test_sync_exchange_errors();
    test_parse_response();
    test_response_status();
    test_validate_sync_response();

    if (s_failures != 0) {
        fprintf(stderr,
                "%d protocol_uart_boot host test failure(s)\n",
                s_failures);
        return 1;
    }

    puts("protocol_uart_boot host tests passed");
    return 0;
}
