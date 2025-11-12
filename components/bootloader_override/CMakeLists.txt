# This component is empty, but it triggers the custom bootloader build
idf_component_register()

# Set custom bootloader subproject path
set(BOOTLOADER_BUILD_DIR "${COMPONENT_DIR}/subproject")
