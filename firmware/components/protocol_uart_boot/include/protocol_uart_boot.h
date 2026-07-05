/**
 * @file protocol_uart_boot.h
 * @brief ESP UART bootloader framing helpers.
 */

#ifndef FF_PROTOCOL_UART_BOOT_H
#define FF_PROTOCOL_UART_BOOT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "common.h"

/** SLIP frame delimiter byte used by the ROM bootloader transport. */
#define FF_UART_BOOT_SLIP_END 0xC0U

/** SLIP escape marker byte used by the ROM bootloader transport. */
#define FF_UART_BOOT_SLIP_ESC 0xDBU

/** Escaped value that represents a frame delimiter byte. */
#define FF_UART_BOOT_SLIP_ESC_END 0xDCU

/** Escaped value that represents an escape marker byte. */
#define FF_UART_BOOT_SLIP_ESC_ESC 0xDDU

/** Direction byte for command frames sent to the target bootloader. */
#define FF_UART_BOOT_FRAME_DIRECTION_COMMAND 0x00U

/** Direction byte for response frames returned by the target bootloader. */
#define FF_UART_BOOT_FRAME_DIRECTION_RESPONSE 0x01U

/** Bytes in a command frame before the payload. */
#define FF_UART_BOOT_COMMAND_HEADER_BYTES 8U

/** Bytes in a response frame before the payload. */
#define FF_UART_BOOT_RESPONSE_HEADER_BYTES 8U

/** Bytes in a ROM command status result. */
#define FF_UART_BOOT_STATUS_BYTES 2U

/** Bytes in the ROM SYNC command payload. */
#define FF_UART_BOOT_SYNC_PAYLOAD_BYTES 36U

/** ROM status byte that indicates command success. */
#define FF_UART_BOOT_STATUS_SUCCESS 0x00U

/** ROM bootloader command identifiers used by the protocol contract. */
typedef enum {
    /** Synchronizes with the ROM bootloader. */
    FF_UART_BOOT_COMMAND_SYNC = 0x08,
} ff_uart_boot_command_t;

/**
 * @brief Decoded ESP UART bootloader response frame.
 */
typedef struct {
    ff_uart_boot_command_t command; /**< Command opcode echoed by the target. */
    uint32_t value;                 /**< Command-specific response value. */
    /** Response payload inside the input frame. */
    const uint8_t *payload;
    size_t payload_bytes; /**< Number of response payload bytes. */
} ff_uart_boot_response_t;

/**
 * @brief Streaming SLIP packet accumulator for transport receive paths.
 */
typedef struct {
    uint8_t *frame;      /**< Caller-owned decoded frame storage. */
    size_t capacity;     /**< Decoded frame storage capacity in bytes. */
    size_t frame_bytes;  /**< Number of decoded bytes currently stored. */
    bool in_frame;       /**< True while bytes are inside SLIP delimiters. */
    bool escaping;       /**< True after an escape marker inside a frame. */
} ff_uart_boot_frame_accumulator_t;

/**
 * @brief Streaming ESP UART bootloader response reader.
 */
typedef struct {
    ff_uart_boot_frame_accumulator_t accumulator; /**< Decoded frame state. */
} ff_uart_boot_response_reader_t;

/**
 * @brief Calculates the bootloader checksum for command payload bytes.
 *
 * @param[in] payload Payload bytes to checksum, or NULL for an empty payload.
 * @param[in] payload_bytes Number of payload bytes.
 *
 * @return Command checksum byte widened to 32 bits for frame storage.
 */
uint32_t ff_uart_boot_checksum(const uint8_t *payload, size_t payload_bytes);

/**
 * @brief Encodes raw frame bytes as one complete SLIP packet.
 *
 * @param[in] input Raw frame bytes to encode, or NULL when input_bytes is 0.
 * @param[in] input_bytes Number of raw frame bytes.
 * @param[out] output Destination buffer for the complete SLIP packet.
 * @param[in] output_capacity Destination buffer capacity in bytes.
 * @param[out] output_bytes Receives the number of bytes written.
 *
 * @return FF_STATUS_OK when encoding succeeds; FF_STATUS_NO_MEMORY when the
 *         destination is too small; otherwise an argument validation status.
 */
ff_status_t ff_uart_boot_slip_encode(const uint8_t *input,
                                     size_t input_bytes,
                                     uint8_t *output,
                                     size_t output_capacity,
                                     size_t *output_bytes);

/**
 * @brief Decodes one complete SLIP packet into raw frame bytes.
 *
 * @param[in] input Complete SLIP packet bytes.
 * @param[in] input_bytes Number of SLIP packet bytes.
 * @param[out] output Destination buffer for decoded raw frame bytes.
 * @param[in] output_capacity Destination buffer capacity in bytes.
 * @param[out] output_bytes Receives the number of decoded bytes.
 *
 * @return FF_STATUS_OK when decoding succeeds; FF_STATUS_NO_MEMORY when the
 *         destination is too small; FF_STATUS_CHECK_FAILED when the packet is
 *         malformed; otherwise an argument validation status.
 */
ff_status_t ff_uart_boot_slip_decode(const uint8_t *input,
                                     size_t input_bytes,
                                     uint8_t *output,
                                     size_t output_capacity,
                                     size_t *output_bytes);

/**
 * @brief Initializes a streaming SLIP frame accumulator.
 *
 * @param[out] accumulator Accumulator state to initialize.
 * @param[out] frame Caller-owned storage for decoded frame bytes.
 * @param[in] frame_capacity Decoded frame storage capacity in bytes.
 *
 * @return FF_STATUS_OK when initialization succeeds; otherwise an argument
 *         validation status.
 */
ff_status_t ff_uart_boot_frame_accumulator_init(
    ff_uart_boot_frame_accumulator_t *accumulator,
    uint8_t *frame,
    size_t frame_capacity);

/**
 * @brief Clears buffered bytes and receive state from an accumulator.
 *
 * @param[in,out] accumulator Accumulator state to clear.
 *
 * @return FF_STATUS_OK when the accumulator is valid; otherwise an argument
 *         validation status.
 */
ff_status_t ff_uart_boot_frame_accumulator_reset(
    ff_uart_boot_frame_accumulator_t *accumulator);

/**
 * @brief Adds one transport byte to a streaming SLIP frame accumulator.
 *
 * @param[in,out] accumulator Accumulator receiving decoded frame bytes.
 * @param[in] byte Transport byte read from the serial stream.
 * @param[out] frame_ready Receives true when a decoded frame is complete.
 * @param[out] frame_bytes Receives the decoded frame byte count.
 *
 * @return FF_STATUS_OK when the byte is accepted; FF_STATUS_NO_MEMORY when the
 *         decoded frame storage is full; FF_STATUS_CHECK_FAILED when the SLIP
 *         stream is malformed; otherwise an argument validation status.
 */
ff_status_t ff_uart_boot_frame_accumulator_push(
    ff_uart_boot_frame_accumulator_t *accumulator,
    uint8_t byte,
    bool *frame_ready,
    size_t *frame_bytes);

/**
 * @brief Initializes a streaming bootloader response reader.
 *
 * @param[out] reader Response reader state to initialize.
 * @param[out] frame Caller-owned storage for decoded frame bytes.
 * @param[in] frame_capacity Decoded frame storage capacity in bytes.
 *
 * @return FF_STATUS_OK when initialization succeeds; otherwise an argument
 *         validation status.
 */
ff_status_t ff_uart_boot_response_reader_init(
    ff_uart_boot_response_reader_t *reader,
    uint8_t *frame,
    size_t frame_capacity);

/**
 * @brief Clears buffered bytes and receive state from a response reader.
 *
 * @param[in,out] reader Response reader state to clear.
 *
 * @return FF_STATUS_OK when the reader is valid; otherwise an argument
 *         validation status.
 */
ff_status_t ff_uart_boot_response_reader_reset(
    ff_uart_boot_response_reader_t *reader);

/**
 * @brief Adds one transport byte to a streaming response reader.
 *
 * @param[in,out] reader Reader receiving encoded response bytes.
 * @param[in] byte Transport byte read from the serial stream.
 * @param[out] response_ready Receives true when a response is complete.
 * @param[out] response Receives the parsed response when ready.
 *
 * @return FF_STATUS_OK when the byte is accepted; FF_STATUS_NO_MEMORY when the
 *         decoded frame storage is full; FF_STATUS_CHECK_FAILED when the stream
 *         or decoded response is malformed; otherwise an argument validation
 *         status.
 */
ff_status_t ff_uart_boot_response_reader_push(
    ff_uart_boot_response_reader_t *reader,
    uint8_t byte,
    bool *response_ready,
    ff_uart_boot_response_t *response);

/**
 * @brief Builds an unescaped command frame.
 *
 * @param[in] command ROM bootloader command identifier.
 * @param[in] payload Command payload bytes, or NULL when payload_bytes is 0.
 * @param[in] payload_bytes Number of command payload bytes.
 * @param[out] output Destination buffer for the unescaped command frame.
 * @param[in] output_capacity Destination buffer capacity in bytes.
 * @param[out] output_bytes Receives the number of frame bytes written.
 *
 * @return FF_STATUS_OK when frame construction succeeds; FF_STATUS_NO_MEMORY
 *         when the destination is too small; otherwise an argument validation
 *         status.
 */
ff_status_t ff_uart_boot_build_command(ff_uart_boot_command_t command,
                                       const uint8_t *payload,
                                       size_t payload_bytes,
                                       uint8_t *output,
                                       size_t output_capacity,
                                       size_t *output_bytes);

/**
 * @brief Builds an unescaped ESP UART bootloader SYNC command frame.
 *
 * @param[out] output Destination buffer for the unescaped command frame.
 * @param[in] output_capacity Destination buffer capacity in bytes.
 * @param[out] output_bytes Receives the number of frame bytes written.
 *
 * @return FF_STATUS_OK when frame construction succeeds; FF_STATUS_NO_MEMORY
 *         when the destination is too small; otherwise an argument validation
 *         status.
 */
ff_status_t ff_uart_boot_build_sync_command(uint8_t *output,
                                            size_t output_capacity,
                                            size_t *output_bytes);

/**
 * @brief Parses one decoded ESP UART bootloader response frame.
 *
 * @param[in] frame Unescaped response frame bytes.
 * @param[in] frame_bytes Number of unescaped response frame bytes.
 * @param[out] response Receives the decoded response view.
 *
 * @return FF_STATUS_OK when the response is well formed; otherwise a validation
 *         status describing the failed check.
 */
ff_status_t ff_uart_boot_parse_response(const uint8_t *frame,
                                        size_t frame_bytes,
                                        ff_uart_boot_response_t *response);

/**
 * @brief Reads command status bytes from a parsed response payload.
 *
 * @param[in] response Parsed response frame.
 * @param[in] data_bytes Number of command-specific data bytes before status.
 * @param[out] status Receives the ROM status byte.
 * @param[out] error Receives the ROM error byte.
 *
 * @return FF_STATUS_OK when status bytes are present; otherwise a validation
 *         status describing the failed check.
 */
ff_status_t ff_uart_boot_response_status(
    const ff_uart_boot_response_t *response,
    size_t data_bytes,
    uint8_t *status,
    uint8_t *error);

/**
 * @brief Validates a decoded ESP UART bootloader SYNC response.
 *
 * @param[in] response Parsed response frame.
 * @param[out] value Receives the response value, or NULL to ignore it.
 *
 * @return FF_STATUS_OK when the response is a successful SYNC response;
 *         otherwise a validation status describing the failed check.
 */
ff_status_t ff_uart_boot_validate_sync_response(
    const ff_uart_boot_response_t *response,
    uint32_t *value);

#endif /* FF_PROTOCOL_UART_BOOT_H */
