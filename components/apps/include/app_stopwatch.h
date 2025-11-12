#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/**
 * @brief Create and show the stopwatch app
 * 
 * @param parent Parent object to create the app on
 */
void app_stopwatch_create(lv_obj_t* parent);

/**
 * @brief Destroy the stopwatch app and clean up resources
 */
void app_stopwatch_destroy(void);

#ifdef __cplusplus
}
#endif
