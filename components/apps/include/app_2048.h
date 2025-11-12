#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/**
 * @brief Create and show the 2048 game
 * 
 * @param parent Parent object to create the game on
 */
void app_2048_create(lv_obj_t* parent);

#ifdef __cplusplus
}
#endif
