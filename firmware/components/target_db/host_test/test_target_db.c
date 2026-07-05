/**
 * @file test_target_db.c
 * @brief Host-side tests for target descriptor storage and validation.
 */

#include "target_db.h"

#include <stdio.h>
#include <string.h>

static int s_failures;

static void expect_true(bool condition, const char *message)
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

static void test_descriptor_count(void)
{
    size_t count = 0U;

    expect_status(ff_target_descriptor_count(&count),
                  FF_STATUS_OK,
                  "descriptor count succeeds");
    expect_true(count == 1U, "descriptor table contains one entry");
    expect_status(ff_target_descriptor_count(NULL),
                  FF_STATUS_INVALID_ARGUMENT,
                  "descriptor count rejects NULL output");
}

static void test_index_lookup(void)
{
    const ff_target_descriptor_t *descriptor = NULL;

    expect_status(ff_target_descriptor_at(0U, &descriptor),
                  FF_STATUS_OK,
                  "index lookup finds first descriptor");
    expect_true(descriptor != NULL, "index lookup returns descriptor");

    if (descriptor != NULL) {
        expect_true(strcmp(descriptor->id, "esp32-uart-boot") == 0,
                    "descriptor ID matches ESP32 UART bootloader");
        expect_true(descriptor->family == FF_TARGET_FAMILY_ESP32,
                    "descriptor family is ESP32");
        expect_true(descriptor->protocol == FF_TARGET_PROTOCOL_ESP_UART_BOOT,
                    "descriptor protocol is ESP UART bootloader");
        expect_true(descriptor->match.kind == FF_TARGET_MATCH_ESP_CHIP_ID,
                    "descriptor match kind is ESP chip ID");
        expect_true(descriptor->match.value != 0U,
                    "descriptor chip ID is present");
        expect_true(descriptor->flash.size_bytes == 16U * 1024U * 1024U,
                    "descriptor flash size is 16 MB");
        expect_true(descriptor->rates.connect_baud == 115200U,
                    "descriptor connect baud is conservative");
    }

    descriptor = (const ff_target_descriptor_t *)0x1;
    expect_status(ff_target_descriptor_at(1U, &descriptor),
                  FF_STATUS_NOT_FOUND,
                  "index lookup rejects out-of-range index");
    expect_true(descriptor == NULL, "missing index clears descriptor output");
    expect_status(ff_target_descriptor_at(0U, NULL),
                  FF_STATUS_INVALID_ARGUMENT,
                  "index lookup rejects NULL output");
}

static void test_id_lookup(void)
{
    const ff_target_descriptor_t *descriptor = NULL;

    expect_status(ff_target_descriptor_find_by_id("esp32-uart-boot",
                                                  &descriptor),
                  FF_STATUS_OK,
                  "ID lookup finds ESP32 UART bootloader descriptor");
    expect_true(descriptor != NULL, "ID lookup returns descriptor");

    descriptor = (const ff_target_descriptor_t *)0x1;
    expect_status(ff_target_descriptor_find_by_id("missing-target",
                                                  &descriptor),
                  FF_STATUS_NOT_FOUND,
                  "ID lookup reports missing descriptors");
    expect_true(descriptor == NULL, "missing ID clears descriptor output");
    expect_status(ff_target_descriptor_find_by_id(NULL, &descriptor),
                  FF_STATUS_INVALID_ARGUMENT,
                  "ID lookup rejects NULL ID");
    expect_status(ff_target_descriptor_find_by_id("esp32-uart-boot", NULL),
                  FF_STATUS_INVALID_ARGUMENT,
                  "ID lookup rejects NULL output");
}

static void test_descriptor_validation(void)
{
    const ff_target_descriptor_t *descriptor = NULL;

    expect_status(ff_target_descriptor_at(0U, &descriptor),
                  FF_STATUS_OK,
                  "baseline descriptor is available");
    if (descriptor == NULL) {
        return;
    }

    ff_target_descriptor_t candidate = *descriptor;
    expect_status(ff_target_descriptor_validate(&candidate),
                  FF_STATUS_OK,
                  "baseline descriptor validates");

    candidate = *descriptor;
    candidate.id = "";
    expect_status(ff_target_descriptor_validate(&candidate),
                  FF_STATUS_INVALID_ARGUMENT,
                  "validation rejects empty ID");

    candidate = *descriptor;
    candidate.match.value = 0U;
    expect_status(ff_target_descriptor_validate(&candidate),
                  FF_STATUS_CHECK_FAILED,
                  "validation rejects missing chip ID");

    candidate = *descriptor;
    candidate.flash.write_block_bytes = 0U;
    expect_status(ff_target_descriptor_validate(&candidate),
                  FF_STATUS_CHECK_FAILED,
                  "validation rejects zero write block");

    candidate = *descriptor;
    candidate.rates.connect_baud = candidate.rates.max_baud + 1U;
    expect_status(ff_target_descriptor_validate(&candidate),
                  FF_STATUS_CHECK_FAILED,
                  "validation rejects inverted baud limits");
}

int main(void)
{
    test_descriptor_count();
    test_index_lookup();
    test_id_lookup();
    test_descriptor_validation();

    if (s_failures != 0) {
        fprintf(stderr, "%d target_db host test failure(s)\n", s_failures);
        return 1;
    }

    puts("target_db host tests passed");
    return 0;
}
