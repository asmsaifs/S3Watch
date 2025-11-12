#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/**
 * @brief Create the apps menu screen
 * 
 * @param parent Parent object to create the screen on
 */
void apps_screen_create(lv_obj_t* parent);

/**
 * @brief Get the apps screen object
 * 
 * @return lv_obj_t* The apps screen object
 */
lv_obj_t* apps_screen_get(void);

#ifdef __cplusplus
}
#endif
