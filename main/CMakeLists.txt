idf_component_register(
    SRCS
        boot_selector.c
        lwmalloc.c
        main.cpp
    INCLUDE_DIRS "."
    REQUIRES ble_sync gui sensors settings bsp_extra esp_event audio_alert
    PRIV_REQUIRES driver esp_partition app_update
)

# Create SPIFFS image for S3Watch data partition (settings.json, etc)
spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)