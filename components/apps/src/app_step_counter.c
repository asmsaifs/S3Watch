#include "app_step_counter.h"
#include "sensors.h"
#include "ui_fonts.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char* TAG = "APP_STEP_COUNTER";

static lv_obj_t* app_container = NULL;
static lv_obj_t* step_label = NULL;
static lv_obj_t* activity_label = NULL;
static lv_obj_t* progress_arc = NULL;
static lv_timer_t* update_timer = NULL;

#define DAILY_STEP_GOAL 10000

static const char* activity_to_string(sensors_activity_t activity)
{
    switch (activity) {
        case SENSORS_ACTIVITY_IDLE: return "Idle";
        case SENSORS_ACTIVITY_WALK: return "Walking";
        case SENSORS_ACTIVITY_RUN: return "Running";
        case SENSORS_ACTIVITY_OTHER: return "Active";
        default: return "Unknown";
    }
}

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
        step_label = NULL;
        activity_label = NULL;
        progress_arc = NULL;
        app_container = NULL;
    }
}

static void update_step_display(lv_timer_t* timer)
{
    (void)timer;
    
    if (!app_container || !step_label || !activity_label || !progress_arc) {
        return;
    }

    uint32_t steps = sensors_get_step_count();
    sensors_activity_t activity = sensors_get_activity();

    // Update step count
    char step_buf[32];
    snprintf(step_buf, sizeof(step_buf), "%lu", (unsigned long)steps);
    lv_label_set_text(step_label, step_buf);

    // Update activity
    lv_label_set_text(activity_label, activity_to_string(activity));

    // Update progress arc (0-100%)
    uint16_t progress = (steps * 100) / DAILY_STEP_GOAL;
    if (progress > 100) progress = 100;
    lv_arc_set_value(progress_arc, progress);

    ESP_LOGD(TAG, "Steps: %lu, Activity: %s, Progress: %d%%", 
             (unsigned long)steps, activity_to_string(activity), progress);
}

void app_step_counter_create(lv_obj_t* parent)
{
    ESP_LOGI(TAG, "Creating step counter app");

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
    lv_label_set_text(title, "Steps");
    lv_obj_set_style_text_font(title, &font_bold_32, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // Progress arc (circular progress)
    progress_arc = lv_arc_create(app_container);
    lv_obj_set_size(progress_arc, 240, 240);
    lv_obj_center(progress_arc);
    lv_arc_set_rotation(progress_arc, 135);
    lv_arc_set_bg_angles(progress_arc, 0, 270);
    lv_arc_set_value(progress_arc, 0);
    lv_obj_remove_style(progress_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(progress_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(progress_arc, 12, LV_PART_MAIN);
    lv_obj_set_style_arc_color(progress_arc, lv_color_hex(0x2A2A2A), LV_PART_MAIN);
    lv_obj_set_style_arc_width(progress_arc, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(progress_arc, lv_color_hex(0x00D9FF), LV_PART_INDICATOR);

    // Step count (large number in center)
    step_label = lv_label_create(app_container);
    lv_label_set_text(step_label, "0");
    lv_obj_set_style_text_font(step_label, &font_numbers_80, 0);
    lv_obj_set_style_text_color(step_label, lv_color_white(), 0);
    lv_obj_align(step_label, LV_ALIGN_CENTER, 0, -10);

    // "steps" label
    lv_obj_t* steps_text = lv_label_create(app_container);
    lv_label_set_text(steps_text, "steps");
    lv_obj_set_style_text_font(steps_text, &font_normal_26, 0);
    lv_obj_set_style_text_color(steps_text, lv_color_hex(0x808080), 0);
    lv_obj_align(steps_text, LV_ALIGN_CENTER, 0, 35);

    // Activity status
    activity_label = lv_label_create(app_container);
    lv_label_set_text(activity_label, "Idle");
    lv_obj_set_style_text_font(activity_label, &font_normal_26, 0);
    lv_obj_set_style_text_color(activity_label, lv_color_hex(0x00D9FF), 0);
    lv_obj_align(activity_label, LV_ALIGN_BOTTOM_MID, 0, -80);

    // Goal label
    lv_obj_t* goal_label = lv_label_create(app_container);
    char goal_buf[32];
    snprintf(goal_buf, sizeof(goal_buf), "Goal: %d", DAILY_STEP_GOAL);
    lv_label_set_text(goal_label, goal_buf);
    lv_obj_set_style_text_font(goal_label, &font_normal_26, 0);
    lv_obj_set_style_text_color(goal_label, lv_color_hex(0x606060), 0);
    lv_obj_align(goal_label, LV_ALIGN_BOTTOM_MID, 0, -40);

    // Create LVGL timer to update display every second
    update_timer = lv_timer_create(update_step_display, 1000, NULL);
    
    // Initial update
    update_step_display(NULL);

    ESP_LOGI(TAG, "Step counter app created");
}

void app_step_counter_destroy(void)
{
    ESP_LOGI(TAG, "Destroying step counter app");
    
    // Pause and delete timer first to prevent any further callbacks
    if (update_timer) {
        lv_timer_pause(update_timer);
        lv_timer_del(update_timer);
        update_timer = NULL;
    }
    
    // Clear references before deleting container
    step_label = NULL;
    activity_label = NULL;
    progress_arc = NULL;
    
    if (app_container) {
        lv_obj_del(app_container);
        app_container = NULL;
    }
}
