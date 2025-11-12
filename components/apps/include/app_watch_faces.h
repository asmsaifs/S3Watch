#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/**
 * @brief Create and show the watch face gallery app
 * 
 * @param parent Parent object to create the app on
 */
void app_watch_faces_create(lv_obj_t* parent);

/**
 * @brief Destroy the watch face gallery app and clean up resources
 */
void app_watch_faces_destroy(void);

#ifdef __cplusplus
}
#endif
