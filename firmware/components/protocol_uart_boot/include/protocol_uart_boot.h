/**
 * @file protocol_uart_boot.h
 * @brief ESP UART bootloader framing helpers.
 */

#ifndef FF_PROTOCOL_UART_BOOT_H
#define FF_PROTOCOL_UART_BOOT_H

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

/** Bytes in a command frame before the payload. */
#define FF_UART_BOOT_COMMAND_HEADER_BYTES 8U

/** ROM bootloader command identifiers used by the protocol contract. */
typedef enum {
    FF_UART_BOOT_COMMAND_SYNC = 0x08,  /**< Synchronizes with the ROM bootloader. */
} ff_uart_boot_command_t;

/**
 * @brief Calculates the bootloader checksum for command payload bytes.
 *
 * @param[in] payload Payload bytes to checksum, or NULL when payload_bytes is 0.
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

#endif /* FF_PROTOCOL_UART_BOOT_H */
