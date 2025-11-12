idf_component_register(
    SRCS "src/apps_screen.c"
         "src/app_2048.c"
         "src/app_step_counter.c"
         "src/app_watch_faces.c"
         "src/app_stopwatch.c"
    INCLUDE_DIRS "include"
    REQUIRES lvgl gui sensors
)
