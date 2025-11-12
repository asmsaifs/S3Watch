#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/**
 * @brief Create and show the step counter app
 * 
 * @param parent Parent object to create the app on
 */
void app_step_counter_create(lv_obj_t* parent);

/**
 * @brief Destroy the step counter app and clean up resources
 */
void app_step_counter_destroy(void);

#ifdef __cplusplus
}
#endif
