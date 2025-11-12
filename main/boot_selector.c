#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "rom/uart.h"

#define BOOT_BUTTON_GPIO GPIO_NUM_0
#define TAG "BOOT_SEL"

// Print directly to UART for early boot debugging
static void early_print(const char *str) {
    uart_tx_wait_idle(0);
    for (const char *p = str; *p != '\0'; p++) {
        uart_tx_one_char(*p);
    }
    uart_tx_one_char('\n');
    uart_tx_wait_idle(0);
}

// This runs before main()
static void __attribute__((constructor)) check_boot_button(void)
{
    early_print("=== BOOT SELECTOR START ===");
    
    // Get current running partition
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
    
    // Configure GPIO 0 as input FIRST, before anything else claims it
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BOOT_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    esp_err_t gpio_err = gpio_config(&io_conf);
    if (gpio_err != ESP_OK) {
        early_print("ERROR: GPIO config failed!");
        return;
    }
    
    // Wait for GPIO to stabilize (can't use vTaskDelay in constructor!)
    ets_delay_us(100000);  // 100ms
    
    // Read button state multiple times to debounce
    int button_state = 1;
    for (int i = 0; i < 10; i++) {
        button_state = gpio_get_level(BOOT_BUTTON_GPIO);
        if (button_state == 1) break;  // If it goes high, button not pressed
        ets_delay_us(1000);
    }
    
    early_print(button_state == 0 ? "GPIO0: LOW (pressed)" : "GPIO0: HIGH (not pressed)");
    
    ESP_LOGI(TAG, "Running: %s (subtype %d), Boot: %s (subtype %d)", 
             running->label, running->subtype,
             boot_partition->label, boot_partition->subtype);
    ESP_LOGI(TAG, "GPIO0 state: %d", button_state);
    
    // If button is pressed (LOW), switch partition
    if (button_state == 0) {
        const esp_partition_t *target = NULL;
        
        // Switch to the OTHER partition
        if (running->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_0) {
            target = esp_partition_find_first(
                ESP_PARTITION_TYPE_APP, 
                ESP_PARTITION_SUBTYPE_APP_OTA_1, 
                NULL
            );
            early_print("Switching: OTA_0 -> OTA_1");
            ESP_LOGI(TAG, "Switching from S3Watch to XiaoZhi");
        } else if (running->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1) {
            target = esp_partition_find_first(
                ESP_PARTITION_TYPE_APP, 
                ESP_PARTITION_SUBTYPE_APP_OTA_0, 
                NULL
            );
            early_print("Switching: OTA_1 -> OTA_0");
            ESP_LOGI(TAG, "Switching from XiaoZhi to S3Watch");
        }
        
        if (target != NULL) {
            esp_err_t err = esp_ota_set_boot_partition(target);
            if (err == ESP_OK) {
                early_print("Partition set, restarting...");
                ESP_LOGI(TAG, "Boot partition changed to %s, restarting...", target->label);
                ets_delay_us(1000000);  // 1 second delay (can't use vTaskDelay!)
                esp_restart();
            } else {
                early_print("ERROR: Failed to set partition");
                ESP_LOGE(TAG, "Failed to set boot partition: %s", esp_err_to_name(err));
            }
        } else {
            early_print("ERROR: Target partition not found!");
            ESP_LOGE(TAG, "Target partition not found!");
        }
    } else {
        ESP_LOGI(TAG, "Button not pressed, normal boot");
    }
    
    early_print("=== BOOT SELECTOR END ===");
}

// Dummy function to ensure this file is linked
void boot_selector_init(void) {
    // Constructor already ran, nothing to do
}
