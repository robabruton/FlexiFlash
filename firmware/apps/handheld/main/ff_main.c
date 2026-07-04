/**
 * @file ff_main.c
 * @brief FlexiFlash handheld application entry point.
 *
 * Logs the compiled board profile for the handheld product scaffold.
 */

#include "esp_log.h"
#include "board.h"

/** Log tag for top-level application messages. */
static const char *TAG = "flexiflash";

/**
 * @brief ESP-IDF application entry point for the handheld variant.
 *
 * Emits the compiled board profile so the scaffold proves that app-level
 * configuration selects the expected shared board component.
 */
void app_main(void)
{
    ESP_LOGI(TAG, "FlexiFlash starting: %s", board_get()->name);
}
