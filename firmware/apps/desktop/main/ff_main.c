/**
 * @file ff_main.c
 * @brief FlexiFlash desktop application entry point.
 *
 * Logs the compiled board profile and build limits for the desktop scaffold.
 */

#include "board.h"
#include "config.h"
#include "esp_log.h"

/** Log tag for top-level application messages. */
static const char *TAG = "flexiflash";

/**
 * @brief ESP-IDF application entry point for the desktop variant.
 *
 * Emits the compiled board profile and build limits so the scaffold proves
 * that app-level configuration selects the expected shared components.
 */
void app_main(void)
{
    ESP_LOGI(TAG, "FlexiFlash starting: %s", board_get()->name);
    ESP_LOGI(TAG, "Firmware image chunk limit: %u bytes",
             ff_config_limits()->firmware_image_chunk_bytes);
}
