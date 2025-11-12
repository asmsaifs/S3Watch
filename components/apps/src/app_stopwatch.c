#include "app_stopwatch.h"
#include "ui_fonts.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char* TAG = "APP_STOPWATCH";

static lv_obj_t* app_container = NULL;
static lv_obj_t* time_label = NULL;
static lv_obj_t* ms_label = NULL;
static lv_obj_t* start_btn = NULL;
static lv_obj_t* reset_btn = NULL;
static lv_obj_t* lap_list = NULL;

static lv_timer_t* update_timer = NULL;
static int64_t start_time_us = 0;
static int64_t elapsed_us = 0;
static bool is_running = false;
static int lap_count = 0;

#define MAX_LAPS 10

static void container_delete_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_DELETE) {
        ESP_LOGI(TAG, "Container being deleted, cleaning up timer");
        
        // Pause and delete timer when container is being deleted
        if (update_timer) {
            lv_timer_pause(update_timer);
            lv_timer_del(update_timer);
            update_timer = NULL;
        }
        
        // Clear all references
        time_label = NULL;
        ms_label = NULL;
        start_btn = NULL;
        reset_btn = NULL;
        lap_list = NULL;
        app_container = NULL;
        
        // Reset state
        start_time_us = 0;
        elapsed_us = 0;
        is_running = false;
        lap_count = 0;
    }
}

static void update_display(void)
{
    if (!app_container || !time_label || !ms_label) return;

    int64_t current_us = is_running ? 
        (esp_timer_get_time() - start_time_us + elapsed_us) : elapsed_us;

    int hours = current_us / 3600000000LL;
    int minutes = (current_us % 3600000000LL) / 60000000LL;
    int seconds = (current_us % 60000000LL) / 1000000LL;
    int milliseconds = (current_us % 1000000LL) / 10000LL;

    char time_buf[16];
    char ms_buf[8];

    if (hours > 0) {
        snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d", hours, minutes, seconds);
    } else {
        snprintf(time_buf, sizeof(time_buf), "%02d:%02d", minutes, seconds);
    }
    snprintf(ms_buf, sizeof(ms_buf), ".%02d", milliseconds);

    lv_label_set_text(time_label, time_buf);
    lv_label_set_text(ms_label, ms_buf);
}

static void update_timer_cb(lv_timer_t* timer)
{
    (void)timer;
    if (app_container && is_running) {
        update_display();
    }
}

static void start_stop_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        if (!is_running) {
            // Start
            is_running = true;
            start_time_us = esp_timer_get_time();
            lv_label_set_text(lv_obj_get_child(start_btn, 0), "Stop");
            lv_obj_set_style_bg_color(start_btn, lv_color_hex(0xFF4444), 0);
            ESP_LOGI(TAG, "Stopwatch started");
        } else {
            // Stop
            is_running = false;
            elapsed_us += esp_timer_get_time() - start_time_us;
            lv_label_set_text(lv_obj_get_child(start_btn, 0), "Start");
            lv_obj_set_style_bg_color(start_btn, lv_color_hex(0x00D9FF), 0);
            ESP_LOGI(TAG, "Stopwatch stopped at %lld us", elapsed_us);
        }
        update_display();
    }
}

static void reset_lap_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        if (!is_running && elapsed_us > 0) {
            // Reset
            elapsed_us = 0;
            lap_count = 0;
            lv_obj_clean(lap_list);
            update_display();
            ESP_LOGI(TAG, "Stopwatch reset");
        } else if (is_running) {
            // Lap
            if (lap_count < MAX_LAPS) {
                int64_t lap_time_us = esp_timer_get_time() - start_time_us + elapsed_us;
                
                int minutes = (lap_time_us % 3600000000LL) / 60000000LL;
                int seconds = (lap_time_us % 60000000LL) / 1000000LL;
                int milliseconds = (lap_time_us % 1000000LL) / 10000LL;
                
                char lap_buf[64];
                snprintf(lap_buf, sizeof(lap_buf), "Lap %d: %02d:%02d.%02d", 
                         ++lap_count, minutes, seconds, milliseconds);
                
                lv_obj_t* lap_label = lv_label_create(lap_list);
                lv_label_set_text(lap_label, lap_buf);
                lv_obj_set_style_text_font(lap_label, &font_normal_26, 0);
                lv_obj_set_style_text_color(lap_label, lv_color_hex(0xA0A0A0), 0);
                
                // Scroll to bottom
                lv_obj_scroll_to_y(lap_list, LV_COORD_MAX, LV_ANIM_ON);
                
                ESP_LOGI(TAG, "Lap %d recorded: %02d:%02d.%02d", 
                         lap_count, minutes, seconds, milliseconds);
            }
        }
    }
}

void app_stopwatch_create(lv_obj_t* parent)
{
    ESP_LOGI(TAG, "Creating stopwatch app");

    // Main container
    app_container = lv_obj_create(parent);
    lv_obj_remove_style_all(app_container);
    lv_obj_set_size(app_container, lv_pct(100), lv_pct(100));
    lv_obj_center(app_container);
    lv_obj_set_style_bg_color(app_container, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(app_container, LV_OPA_100, 0);
    lv_obj_clear_flag(app_container, LV_OBJ_FLAG_SCROLLABLE);
    
    // Register cleanup handler for when object is deleted
    lv_obj_add_event_cb(app_container, container_delete_event_cb, LV_EVENT_DELETE, NULL);

    // Title
    lv_obj_t* title = lv_label_create(app_container);
    lv_label_set_text(title, "Stopwatch");
    lv_obj_set_style_text_font(title, &font_bold_32, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Time display container
    lv_obj_t* time_container = lv_obj_create(app_container);
    lv_obj_remove_style_all(time_container);
    lv_obj_set_size(time_container, lv_pct(90), 100);
    lv_obj_align(time_container, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_opa(time_container, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(time_container, LV_OBJ_FLAG_SCROLLABLE);

    // Large time display
    time_label = lv_label_create(time_container);
    lv_label_set_text(time_label, "00:00");
    lv_obj_set_style_text_font(time_label, &font_numbers_80, 0);
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, -20, 0);

    // Milliseconds
    ms_label = lv_label_create(time_container);
    lv_label_set_text(ms_label, ".00");
    lv_obj_set_style_text_font(ms_label, &font_normal_26, 0);
    lv_obj_set_style_text_color(ms_label, lv_color_hex(0x808080), 0);
    lv_obj_align_to(ms_label, time_label, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

    // Buttons container
    lv_obj_t* btn_container = lv_obj_create(app_container);
    lv_obj_remove_style_all(btn_container);
    lv_obj_set_size(btn_container, lv_pct(90), 80);
    lv_obj_align(btn_container, LV_ALIGN_TOP_MID, 0, 180);
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(btn_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Start/Stop button
    start_btn = lv_btn_create(btn_container);
    lv_obj_set_size(start_btn, 130, 70);
    lv_obj_set_style_bg_color(start_btn, lv_color_hex(0x00D9FF), 0);
    lv_obj_set_style_radius(start_btn, 35, 0);
    lv_obj_add_event_cb(start_btn, start_stop_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* start_label = lv_label_create(start_btn);
    lv_label_set_text(start_label, "Start");
    lv_obj_set_style_text_font(start_label, &font_normal_26, 0);
    lv_obj_center(start_label);

    // Reset/Lap button
    reset_btn = lv_btn_create(btn_container);
    lv_obj_set_size(reset_btn, 130, 70);
    lv_obj_set_style_bg_color(reset_btn, lv_color_hex(0x404040), 0);
    lv_obj_set_style_radius(reset_btn, 35, 0);
    lv_obj_add_event_cb(reset_btn, reset_lap_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* reset_label = lv_label_create(reset_btn);
    lv_label_set_text(reset_label, "Reset");
    lv_obj_set_style_text_font(reset_label, &font_normal_26, 0);
    lv_obj_center(reset_label);

    // Lap times list
    lap_list = lv_obj_create(app_container);
    lv_obj_remove_style_all(lap_list);
    lv_obj_set_size(lap_list, lv_pct(90), 200);
    lv_obj_align(lap_list, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_opa(lap_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(lap_list, 0, 0);
    lv_obj_set_style_pad_all(lap_list, 10, 0);
    lv_obj_set_flex_flow(lap_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(lap_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scroll_dir(lap_list, LV_DIR_VER);
    lv_obj_set_style_pad_row(lap_list, 5, 0);

    // Create update timer (10 Hz for smooth millisecond display)
    update_timer = lv_timer_create(update_timer_cb, 100, NULL);

    // Initial display
    update_display();

    ESP_LOGI(TAG, "Stopwatch app created");
}

void app_stopwatch_destroy(void)
{
    ESP_LOGI(TAG, "Destroying stopwatch app");
    
    // Pause and delete timer first to prevent any further callbacks
    if (update_timer) {
        lv_timer_pause(update_timer);
        lv_timer_del(update_timer);
        update_timer = NULL;
    }
    
    // Clear references before deleting container
    time_label = NULL;
    ms_label = NULL;
    start_btn = NULL;
    reset_btn = NULL;
    lap_list = NULL;
    
    // Delete UI objects
    if (app_container) {
        lv_obj_del(app_container);
        app_container = NULL;
    }
    
    // Reset state
    start_time_us = 0;
    elapsed_us = 0;
    is_running = false;
    lap_count = 0;
}
