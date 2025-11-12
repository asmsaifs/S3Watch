/*
 * Custom Bootloader for S3Watch Dual-Boot System
 * 
 * Boot Selection Logic:
 * - Hold BOOT button (GPIO 0) during power-on: Boot XiaoZhi-ESP32 (OTA_1)
 * - Normal boot: Boot S3Watch (OTA_0)
 */

#include <stdbool.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "bootloader_init.h"
#include "bootloader_utility.h"
#include "bootloader_common.h"
#include "soc/gpio_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"

static const char* TAG = "dual_boot";

void __attribute__((noreturn)) call_start_cpu0(void)
{
    // 1. Hardware initialization
    bootloader_init();

    // 2. Configure GPIO 0 (BOOT button) as input with pull-up
    // For ESP32-S3, GPIO 0 is available in bootloader
    REG_SET_BIT(RTC_IO_TOUCH_PAD0_REG, RTC_IO_TOUCH_PAD0_MUX_SEL);
    REG_CLR_BIT(RTC_IO_TOUCH_PAD0_REG, RTC_IO_TOUCH_PAD0_FUN_IE);
    REG_SET_BIT(RTC_IO_TOUCH_PAD0_REG, RTC_IO_TOUCH_PAD0_RUE);
    
    // Small delay for GPIO to stabilize
    for (volatile int i = 0; i < 100000; i++);

    // 3. Load partition table
    bootloader_state_t bs = {0};
    if (!bootloader_utility_load_partition_table(&bs)) {
        ESP_LOGE(TAG, "Failed to load partition table");
        bootloader_reset();
    }

    // 4. Determine boot partition based on GPIO 0
    int boot_index = 0; // Default: OTA_0 (S3Watch)
    
    // Read GPIO 0 state
    uint32_t gpio_val = REG_GET_FIELD(RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT);
    
    if ((gpio_val & BIT(0)) == 0) {
        // GPIO 0 is LOW = BOOT button pressed
        boot_index = 1; // OTA_1 (XiaoZhi)
        ESP_LOGI(TAG, "🎤 BOOT button pressed - Loading XiaoZhi-ESP32 (OTA_1)");
    } else {
        // GPIO 0 is HIGH = normal boot
        ESP_LOGI(TAG, "⌚ Normal boot - Loading S3Watch (OTA_0)");
    }

    // 5. Load and boot the selected partition
    ESP_LOGI(TAG, "Booting partition index %d...", boot_index);
    bootloader_utility_load_boot_image(&bs, boot_index);
    
    // Should never reach here
    ESP_LOGE(TAG, "Boot failed!");
    bootloader_reset();
}
