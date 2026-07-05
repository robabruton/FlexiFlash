/**
 * @file ff_main.c
 * @brief FlexiFlash handheld application entry point.
 *
 * Logs the compiled board profile and shared contracts for the handheld
 * scaffold.
 */

#include "board.h"
#include "config.h"
#include "esp_log.h"
#include "target_db.h"

/** Log tag for top-level application messages. */
static const char *TAG = "flexiflash";

/**
 * @brief ESP-IDF application entry point for the handheld variant.
 *
 * Emits the compiled board profile and shared contracts so the scaffold proves
 * that app-level configuration selects the expected shared components.
 */
void app_main(void)
{
    ESP_LOGI(TAG, "FlexiFlash starting: %s", board_get()->name);
    ESP_LOGI(TAG, "Firmware image chunk limit: %u bytes",
             ff_config_limits()->firmware_image_chunk_bytes);
    ESP_LOGI(TAG, "Target protocol contract: %s",
             ff_target_protocol_name(FF_TARGET_PROTOCOL_ESP_UART_BOOT));
}
