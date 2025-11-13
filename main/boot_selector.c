#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "rom/uart.h"

#define BOOT_BUTTON_GPIO GPIO_NUM_0
#define BOOT_MENU_TIMEOUT_MS 3000  // 3 seconds
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

// Show boot menu and wait for selection
static void show_boot_menu(const esp_partition_t *current_partition) {
    early_print("\n\n");
    early_print("╔════════════════════════════════════════╗");
    early_print("║      DUAL-BOOT SELECTION MENU          ║");
    early_print("╚════════════════════════════════════════╝");
    early_print("");
    
    // Determine which partition we're running from
    bool is_ota0 = (current_partition->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_0);
    
    early_print("  1. S3Watch (Smartwatch)");
    if (is_ota0) {
        early_print("     └─> [CURRENT - DEFAULT]");
    }
    early_print("");
    early_print("  2. XiaoZhi (AI Assistant)");
    if (!is_ota0) {
        early_print("     └─> [CURRENT - DEFAULT]");
    }
    early_print("");
    early_print("═══════════════════════════════════════════");
    if (is_ota0) {
        early_print("Hold BOOT button (3s) to switch to XiaoZhi");
    } else {
        early_print("Hold BOOT button (3s) to switch to S3Watch");
    }
    early_print("Release button now to stay on current");
    early_print("═══════════════════════════════════════════");
    early_print("");
    
    // Wait for selection with timeout
    int last_state = gpio_get_level(BOOT_BUTTON_GPIO); // Start with current state
    int selection = is_ota0 ? 1 : 2; // Default to current partition
    bool button_released = false;
    
    // Count iterations: 10ms per iteration, need 3000ms total = 300 iterations
    const int max_iterations = 300; // 3 seconds at 10ms per iteration
    
    for (int i = 0; i < max_iterations; i++) {
        // Check button state
        int button_state = gpio_get_level(BOOT_BUTTON_GPIO);
        
        // Detect button release (stay with current partition)
        if (last_state == 0 && button_state == 1) {
            button_released = true;
            early_print("Button released - staying with current partition");
            goto menu_selection_done;
        }
        
        last_state = button_state;
        ets_delay_us(10000); // 10ms delay
    }
    
    // Timeout reached
    early_print("Timeout - checking final button state...");
    
    // If button still held after timeout, SWITCH to the OTHER partition
    if (gpio_get_level(BOOT_BUTTON_GPIO) == 0) {
        selection = is_ota0 ? 2 : 1; // Switch to OTHER partition
        early_print(is_ota0 ? 
            "Button held through timeout - switching to XiaoZhi" :
            "Button held through timeout - switching to S3Watch");
    } else {
        early_print("Button released during timeout - staying with current");
    }
    
menu_selection_done:
    early_print("");
    
    // Execute selection
    const esp_partition_t *target = NULL;
    
    if (selection == 1) {
        // Boot S3Watch (OTA_0)
        target = esp_partition_find_first(
            ESP_PARTITION_TYPE_APP, 
            ESP_PARTITION_SUBTYPE_APP_OTA_0, 
            NULL
        );
        early_print("Booting S3Watch...");
        ESP_LOGI(TAG, "Boot menu: Selected S3Watch (OTA_0)");
    } else {
        // Boot XiaoZhi (OTA_1)
        target = esp_partition_find_first(
            ESP_PARTITION_TYPE_APP, 
            ESP_PARTITION_SUBTYPE_APP_OTA_1, 
            NULL
        );
        early_print("Booting XiaoZhi...");
        ESP_LOGI(TAG, "Boot menu: Selected XiaoZhi (OTA_1)");
    }
    
    if (target != NULL && target != current_partition) {
        esp_err_t err = esp_ota_set_boot_partition(target);
        if (err == ESP_OK) {
            early_print("Switching partition...");
            ESP_LOGI(TAG, "Boot partition changed to %s, restarting...", target->label);
            ets_delay_us(500000);  // 0.5 second delay
            esp_restart();
        } else {
            early_print("ERROR: Failed to set partition");
            ESP_LOGE(TAG, "Failed to set boot partition: %s", esp_err_to_name(err));
        }
    } else if (target == NULL) {
        early_print("ERROR: Target partition not found!");
        ESP_LOGE(TAG, "Target partition not found!");
    }
    // If same partition, just continue booting
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
    
    // If button is pressed (LOW), show boot menu
    if (button_state == 0) {
        early_print("Boot button detected - showing boot menu...");
        ESP_LOGI(TAG, "Boot button held - showing boot menu");
        show_boot_menu(running);
        // If we get here, menu timed out with same partition selected
        ESP_LOGI(TAG, "Continuing with current partition");
    } else {
        ESP_LOGI(TAG, "Button not pressed, normal boot");
    }
    
    early_print("=== BOOT SELECTOR END ===");
}

// Dummy function to ensure this file is linked
void boot_selector_init(void) {
    // Constructor already ran, nothing to do
}
