#include "app_watch_faces.h"
#include "ui_fonts.h"
#include "esp_log.h"
#include <time.h>
#include <sys/time.h>
#include <math.h>

static const char* TAG = "APP_WATCH_FACES";

static lv_obj_t* app_container = NULL;
static lv_obj_t* face_container = NULL;
static lv_obj_t* time_label = NULL;
static lv_obj_t* date_label = NULL;
static lv_obj_t* analog_hour = NULL;
static lv_obj_t* analog_min = NULL;
static lv_obj_t* analog_sec = NULL;
static lv_timer_t* update_timer = NULL;
static int current_face = 0;

typedef enum {
    FACE_DIGITAL = 0,
    FACE_ANALOG,
    FACE_MINIMAL,
    FACE_COUNT
} watch_face_type_t;

static const char* face_names[] = {
    "Digital",
    "Analog",
    "Minimal"
};

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
        face_container = NULL;
        time_label = NULL;
        date_label = NULL;
        analog_hour = NULL;
        analog_min = NULL;
        analog_sec = NULL;
        app_container = NULL;
    }
}

static void get_current_time(int* hour, int* minute, int* second)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm timeinfo;
    localtime_r(&tv.tv_sec, &timeinfo);
    
    *hour = timeinfo.tm_hour;
    *minute = timeinfo.tm_min;
    *second = timeinfo.tm_sec;
}

static void create_digital_face(lv_obj_t* parent)
{
    // Time label (large)
    time_label = lv_label_create(parent);
    lv_label_set_text(time_label, "00:00");
    lv_obj_set_style_text_font(time_label, &font_bold_42, 0);
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -20);

    // Seconds (smaller)
    lv_obj_t* sec_label = lv_label_create(parent);
    lv_label_set_text(sec_label, "00");
    lv_obj_set_style_text_font(sec_label, &font_normal_26, 0);
    lv_obj_set_style_text_color(sec_label, lv_color_hex(0x808080), 0);
    lv_obj_align_to(sec_label, time_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    // Date
    date_label = lv_label_create(parent);
    lv_label_set_text(date_label, "Mon, Jan 1");
    lv_obj_set_style_text_font(date_label, &font_normal_26, 0);
    lv_obj_set_style_text_color(date_label, lv_color_hex(0x00D9FF), 0);
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 40);
}

static void create_analog_face(lv_obj_t* parent)
{
    // Clock face circle
    lv_obj_t* face = lv_obj_create(parent);
    lv_obj_set_size(face, 200, 200);
    lv_obj_center(face);
    lv_obj_set_style_radius(face, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(face, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_border_color(face, lv_color_hex(0x00D9FF), 0);
    lv_obj_set_style_border_width(face, 3, 0);
    lv_obj_clear_flag(face, LV_OBJ_FLAG_SCROLLABLE);

    // Hour markers
    for (int i = 0; i < 12; i++) {
        float angle = (i * 30.0f - 90.0f) * 3.14159f / 180.0f;
        int x = (int)(85.0f * cosf(angle));
        int y = (int)(85.0f * sinf(angle));
        
        lv_obj_t* marker = lv_obj_create(face);
        lv_obj_set_size(marker, 4, 4);
        lv_obj_set_style_radius(marker, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(marker, lv_color_white(), 0);
        lv_obj_set_style_border_width(marker, 0, 0);
        lv_obj_align(marker, LV_ALIGN_CENTER, x, y);
    }

    // Hour hand
    analog_hour = lv_line_create(face);
    lv_obj_set_style_line_width(analog_hour, 6, 0);
    lv_obj_set_style_line_color(analog_hour, lv_color_white(), 0);
    lv_obj_set_style_line_rounded(analog_hour, true, 0);

    // Minute hand
    analog_min = lv_line_create(face);
    lv_obj_set_style_line_width(analog_min, 4, 0);
    lv_obj_set_style_line_color(analog_min, lv_color_hex(0x00D9FF), 0);
    lv_obj_set_style_line_rounded(analog_min, true, 0);

    // Second hand
    analog_sec = lv_line_create(face);
    lv_obj_set_style_line_width(analog_sec, 2, 0);
    lv_obj_set_style_line_color(analog_sec, lv_color_hex(0xFF6B6B), 0);
    lv_obj_set_style_line_rounded(analog_sec, true, 0);

    // Center dot
    lv_obj_t* center = lv_obj_create(face);
    lv_obj_set_size(center, 10, 10);
    lv_obj_center(center);
    lv_obj_set_style_radius(center, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(center, lv_color_hex(0xFF6B6B), 0);
    lv_obj_set_style_border_width(center, 0, 0);
}

static void create_minimal_face(lv_obj_t* parent)
{
    // Just large time, very clean
    time_label = lv_label_create(parent);
    lv_label_set_text(time_label, "00:00");
    lv_obj_set_style_text_font(time_label, &font_bold_42, 0);
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    lv_obj_center(time_label);

    // Thin separator line
    lv_obj_t* line_obj = lv_line_create(parent);
    static lv_point_precise_t line_points[] = {{0, 0}, {100, 0}};
    lv_line_set_points(line_obj, line_points, 2);
    lv_obj_set_style_line_width(line_obj, 1, 0);
    lv_obj_set_style_line_color(line_obj, lv_color_hex(0x404040), 0);
    lv_obj_align(line_obj, LV_ALIGN_CENTER, 0, 50);

    // Small date below
    date_label = lv_label_create(parent);
    lv_label_set_text(date_label, "MONDAY");
    lv_obj_set_style_text_font(date_label, &font_normal_26, 0);
    lv_obj_set_style_text_color(date_label, lv_color_hex(0x606060), 0);
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 80);
}

static void update_analog_hands(int hour, int minute, int second)
{
    if (!analog_hour || !analog_min || !analog_sec) return;

    static lv_point_precise_t hour_points[2];
    static lv_point_precise_t min_points[2];
    static lv_point_precise_t sec_points[2];

    // Convert to angles (12 o'clock = -90 degrees)
    float hour_angle = ((hour % 12) * 30.0f + minute * 0.5f - 90.0f) * 3.14159f / 180.0f;
    float min_angle = (minute * 6.0f - 90.0f) * 3.14159f / 180.0f;
    float sec_angle = (second * 6.0f - 90.0f) * 3.14159f / 180.0f;

    // Calculate end points (centered at 100, 100)
    hour_points[0].x = 100;
    hour_points[0].y = 100;
    hour_points[1].x = 100 + (int)(50.0f * cosf(hour_angle));
    hour_points[1].y = 100 + (int)(50.0f * sinf(hour_angle));

    min_points[0].x = 100;
    min_points[0].y = 100;
    min_points[1].x = 100 + (int)(70.0f * cosf(min_angle));
    min_points[1].y = 100 + (int)(70.0f * sinf(min_angle));

    sec_points[0].x = 100;
    sec_points[0].y = 100;
    sec_points[1].x = 100 + (int)(80.0f * cosf(sec_angle));
    sec_points[1].y = 100 + (int)(80.0f * sinf(sec_angle));

    lv_line_set_points(analog_hour, hour_points, 2);
    lv_line_set_points(analog_min, min_points, 2);
    lv_line_set_points(analog_sec, sec_points, 2);
}

static void update_time_display(lv_timer_t* timer)
{
    (void)timer;

    if (!app_container || !face_container) {
        return;
    }

    int hour, minute, second;
    get_current_time(&hour, &minute, &second);

    if (current_face == FACE_DIGITAL) {
        if (time_label) {
            char time_buf[16];
            snprintf(time_buf, sizeof(time_buf), "%02d:%02d", hour, minute);
            lv_label_set_text(time_label, time_buf);
        }
        if (date_label) {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            struct tm timeinfo;
            localtime_r(&tv.tv_sec, &timeinfo);
            char date_buf[32];
            strftime(date_buf, sizeof(date_buf), "%a, %b %d", &timeinfo);
            lv_label_set_text(date_label, date_buf);
        }
    } else if (current_face == FACE_ANALOG) {
        update_analog_hands(hour, minute, second);
    } else if (current_face == FACE_MINIMAL) {
        if (time_label) {
            char time_buf[16];
            snprintf(time_buf, sizeof(time_buf), "%02d:%02d", hour, minute);
            lv_label_set_text(time_label, time_buf);
        }
        if (date_label) {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            struct tm timeinfo;
            localtime_r(&tv.tv_sec, &timeinfo);
            char date_buf[32];
            strftime(date_buf, sizeof(date_buf), "%A", &timeinfo);
            // Convert to uppercase
            for (int i = 0; date_buf[i]; i++) {
                if (date_buf[i] >= 'a' && date_buf[i] <= 'z') {
                    date_buf[i] -= 32;
                }
            }
            lv_label_set_text(date_label, date_buf);
        }
    }
}

static void switch_face_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        int dir = (int)(intptr_t)lv_event_get_user_data(e);
        current_face = (current_face + dir + FACE_COUNT) % FACE_COUNT;
        
        ESP_LOGI(TAG, "Switching to face: %s", face_names[current_face]);
        
        // Clear old face
        if (face_container) {
            lv_obj_clean(face_container);
            time_label = NULL;
            date_label = NULL;
            analog_hour = NULL;
            analog_min = NULL;
            analog_sec = NULL;
        }
        
        // Create new face
        switch (current_face) {
            case FACE_DIGITAL:
                create_digital_face(face_container);
                break;
            case FACE_ANALOG:
                create_analog_face(face_container);
                break;
            case FACE_MINIMAL:
                create_minimal_face(face_container);
                break;
        }
        
        update_time_display(NULL);
    }
}

void app_watch_faces_create(lv_obj_t* parent)
{
    ESP_LOGI(TAG, "Creating watch face gallery app");

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
    lv_label_set_text(title, "Watch Faces");
    lv_obj_set_style_text_font(title, &font_bold_32, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Face container
    face_container = lv_obj_create(app_container);
    lv_obj_remove_style_all(face_container);
    lv_obj_set_size(face_container, lv_pct(90), 300);
    lv_obj_center(face_container);
    lv_obj_set_style_bg_opa(face_container, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(face_container, LV_OBJ_FLAG_SCROLLABLE);

    // Previous button
    lv_obj_t* prev_btn = lv_btn_create(app_container);
    lv_obj_set_size(prev_btn, 60, 60);
    lv_obj_align(prev_btn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_add_event_cb(prev_btn, switch_face_event_cb, LV_EVENT_CLICKED, (void*)-1);
    lv_obj_t* prev_label = lv_label_create(prev_btn);
    lv_label_set_text(prev_label, LV_SYMBOL_LEFT);
    lv_obj_center(prev_label);

    // Next button
    lv_obj_t* next_btn = lv_btn_create(app_container);
    lv_obj_set_size(next_btn, 60, 60);
    lv_obj_align(next_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_add_event_cb(next_btn, switch_face_event_cb, LV_EVENT_CLICKED, (void*)1);
    lv_obj_t* next_label = lv_label_create(next_btn);
    lv_label_set_text(next_label, LV_SYMBOL_RIGHT);
    lv_obj_center(next_label);

    // Face name indicator
    lv_obj_t* face_name = lv_label_create(app_container);
    lv_label_set_text(face_name, face_names[current_face]);
    lv_obj_set_style_text_font(face_name, &font_normal_26, 0);
    lv_obj_set_style_text_color(face_name, lv_color_hex(0x808080), 0);
    lv_obj_align(face_name, LV_ALIGN_BOTTOM_MID, 0, -20);

    // Create initial face
    create_digital_face(face_container);

    // Create timer to update time
    update_timer = lv_timer_create(update_time_display, 1000, NULL);
    
    // Initial update
    update_time_display(NULL);

    ESP_LOGI(TAG, "Watch face gallery app created");
}

void app_watch_faces_destroy(void)
{
    ESP_LOGI(TAG, "Destroying watch face gallery app");
    
    // Pause and delete timer first to prevent any further callbacks
    if (update_timer) {
        lv_timer_pause(update_timer);
        lv_timer_del(update_timer);
        update_timer = NULL;
    }
    
    // Clear references before deleting container
    face_container = NULL;
    time_label = NULL;
    date_label = NULL;
    analog_hour = NULL;
    analog_min = NULL;
    analog_sec = NULL;
    
    // Delete UI objects
    if (app_container) {
        lv_obj_del(app_container);
        app_container = NULL;
    }
}
