#include "apps_screen.h"
#include "app_2048.h"
#include "app_step_counter.h"
#include "app_watch_faces.h"
#include "app_stopwatch.h"
#include "ui.h"
#include "ui_fonts.h"
#include "esp_log.h"
#include "bsp/esp32_s3_touch_amoled_2_06.h"

static const char* TAG = "APPS_SCREEN";

static lv_obj_t* apps_screen = NULL;

// App icons - using placeholder icons for now (you can add custom icons later)
LV_IMAGE_DECLARE(image_settings_icon); // Reusing existing icon as placeholder
LV_IMAGE_DECLARE(image_apps_icon);

static const lv_image_dsc_t* app_icons[] = {
    &image_settings_icon, // 2048 icon
    &image_apps_icon,     // Step Counter icon
    &image_settings_icon, // Watch Faces icon
    &image_apps_icon,     // Stopwatch icon
};

static const char* app_labels[] = {
    "2048",
    "Steps",
    "Faces",
    "Timer",
};

enum app_id {
    APP_2048 = 0,
    APP_STEP_COUNTER,
    APP_WATCH_FACES,
    APP_STOPWATCH,
};

static void app_click_event_cb(lv_event_t* e)
{
    int app = (int)(uintptr_t)lv_event_get_user_data(e);
    
    ESP_LOGI(TAG, "App clicked: %d", app);
    
    lv_obj_t* tile = ui_dynamic_subtile_acquire();
    if (!tile) {
        ESP_LOGW(TAG, "Failed to acquire dynamic subtile");
        return;
    }
    
    switch (app) {
    case APP_2048:
        ESP_LOGI(TAG, "Launching 2048 game");
        app_2048_create(tile);
        break;
    case APP_STEP_COUNTER:
        ESP_LOGI(TAG, "Launching Step Counter");
        app_step_counter_create(tile);
        break;
    case APP_WATCH_FACES:
        ESP_LOGI(TAG, "Launching Watch Faces");
        app_watch_faces_create(tile);
        break;
    case APP_STOPWATCH:
        ESP_LOGI(TAG, "Launching Stopwatch");
        app_stopwatch_create(tile);
        break;
    default:
        ESP_LOGW(TAG, "Unknown app ID: %d", app);
        return;
    }
    
    ui_dynamic_subtile_show();
}

void apps_screen_create(lv_obj_t* parent)
{
    ESP_LOGI(TAG, "Creating apps screen");

    static lv_style_t main_style;
    lv_style_init(&main_style);
    lv_style_set_text_color(&main_style, lv_color_white());
    lv_style_set_bg_color(&main_style, lv_color_hex(0x000000));
    lv_style_set_bg_opa(&main_style, LV_OPA_100);

    apps_screen = lv_obj_create(parent);
    lv_obj_remove_style_all(apps_screen);
    lv_obj_add_style(apps_screen, &main_style, 0);
    lv_obj_set_size(apps_screen, lv_pct(100), lv_pct(100));
    lv_obj_center(apps_screen);
    lv_obj_clear_flag(apps_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(apps_screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(apps_screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(apps_screen, 12, 0);

    // Header
    lv_obj_t* header = lv_obj_create(apps_screen);
    lv_obj_remove_style_all(header);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(header, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* title_label = lv_label_create(header);
    lv_obj_set_style_text_font(title_label, &font_bold_32, 0);
    lv_obj_set_style_text_color(title_label, lv_color_white(), 0);
    lv_label_set_text(title_label, "Apps");

    // Grid container for apps
    lv_obj_t* grid = lv_obj_create(apps_screen);
    lv_obj_remove_style_all(grid);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(grid, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_top(grid, 10, 0);
    lv_obj_set_style_pad_left(grid, 12, 0);
    lv_obj_set_style_pad_right(grid, 12, 0);
    lv_obj_set_style_pad_row(grid, 14, 0);
    lv_obj_set_style_pad_column(grid, 14, 0);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Create app items
    const int num_apps = sizeof(app_icons) / sizeof(app_icons[0]);
    for (int i = 0; i < num_apps; i++) {
        lv_obj_t* item = lv_obj_create(grid);
        lv_obj_remove_style_all(item);
        lv_obj_set_width(item, lv_pct(46));
        lv_obj_set_height(item, 110);
        lv_obj_set_style_bg_color(item, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_bg_opa(item, 38, 0);
        lv_obj_set_style_radius(item, 16, 0);
        lv_obj_set_style_pad_all(item, 8, 0);
        lv_obj_set_style_text_align(item, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_flex_flow(item, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(item, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(item, app_click_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)i);

        lv_obj_t* image = lv_image_create(item);
        lv_image_set_src(image, app_icons[i]);
        lv_obj_set_align(image, LV_ALIGN_TOP_MID);
        lv_obj_remove_flag(image, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t* label = lv_label_create(item);
        lv_label_set_text(label, app_labels[i]);
        lv_obj_set_style_text_color(label, lv_color_hex(0xD0D0D0), 0);
        lv_obj_set_style_text_font(label, &font_normal_26, 0);
    }

    ESP_LOGI(TAG, "Apps screen created with %d apps", num_apps);
}

lv_obj_t* apps_screen_get(void)
{
    return apps_screen;
}
