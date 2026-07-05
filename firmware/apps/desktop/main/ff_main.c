/**
 * @file ff_main.c
 * @brief FlexiFlash desktop application entry point.
 *
 * Logs the compiled board profile and shared contracts for the desktop
 * scaffold.
 */

#include "board.h"
#include "config.h"
#include "esp_log.h"
#include "target_db.h"

/** Log tag for top-level application messages. */
static const char *TAG = "flexiflash";

/**
 * @brief ESP-IDF application entry point for the desktop variant.
 *
 * Emits the compiled board profile and shared contracts so the scaffold proves
 * that app-level configuration selects the expected shared components.
 */
void app_main(void)
{
    size_t target_count = 0U;
    ff_status_t target_db_status = ff_target_descriptor_count(&target_count);

    ESP_LOGI(TAG, "FlexiFlash starting: %s", board_get()->name);
    ESP_LOGI(TAG, "Firmware image chunk limit: %u bytes",
             ff_config_limits()->firmware_image_chunk_bytes);
    ESP_LOGI(TAG, "Target protocol contract: %s",
             ff_target_protocol_name(FF_TARGET_PROTOCOL_ESP_UART_BOOT));
    ESP_LOGI(TAG, "Target descriptor table: %u entries (%s)",
             (unsigned int)target_count,
             ff_status_name(target_db_status));
}
