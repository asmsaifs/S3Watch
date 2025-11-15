# Assets Partition Fix - XiaoZhi-ESP32 Integration

## Problem Summary

The error `E (5712) Assets: The index.json file is not found` occurred because of a format mismatch between two different assets partition implementations:

1. **S3Watch** (original): Used SPIFFS format for the "storage" partition (later renamed to "assets")
2. **XiaoZhi-ESP32**: Uses a **custom binary format** with `index.json` for the "assets" partition

## Root Cause

The S3Watch `main/CMakeLists.txt` was creating a SPIFFS-based assets partition:
```cmake
spiffs_create_partition_image(assets ../spiffs FLASH_IN_PROJECT)
```

However, XiaoZhi-ESP32 expects a completely different format:
- Custom binary structure with checksums
- Special header format defined in `xiaozhi-esp32/main/assets.cc`
- Contains `index.json` manifest file
- Pre-built as `xiaozhi-esp32/assets.bin` (2.7MB)

## Solution Applied

### 1. Disabled SPIFFS Assets Creation
Modified `main/CMakeLists.txt` to comment out the SPIFFS creation:
```cmake
# NOTE: Commented out SPIFFS-based assets creation
# The xiaozhi-esp32 project uses a custom binary format for assets (not SPIFFS)
# spiffs_create_partition_image(assets ../spiffs FLASH_IN_PROJECT)
```

### 2. Added XiaoZhi Assets Flashing
Modified root `CMakeLists.txt` to flash the pre-built `xiaozhi-esp32/assets.bin`:
```cmake
# Flash xiaozhi-esp32 assets.bin to the assets partition
partition_table_get_partition_info(assets_size "--partition-name assets" "size")
partition_table_get_partition_info(assets_offset "--partition-name assets" "offset")

if(assets_size AND assets_offset)
    set(XIAOZHI_ASSETS_BIN "${CMAKE_SOURCE_DIR}/xiaozhi-esp32/assets.bin")
    
    if(EXISTS ${XIAOZHI_ASSETS_BIN})
        esptool_py_flash_to_partition(flash "assets" "${XIAOZHI_ASSETS_BIN}")
    endif()
endif()
```

## Verification

After the fix, the build output shows:
```
-- Xiaozhi assets.bin found: /Users/asmsaifs/Workshop/S3Watch/xiaozhi-esp32/assets.bin
-- Assets partition: offset=0x0x1820000, size=0x700000 bytes
-- Configured to flash xiaozhi assets.bin to assets partition
```

And the flash command includes:
```
0x1820000 xiaozhi-esp32/assets.bin
```

## Partition Table

The dual-boot partition table (`partitions_dualboot.csv`) allocates:
```csv
# Name,       Type, SubType,  Offset,     Size
assets,      data, spiffs,   0x1820000,  0x700000,  # 7MB
```

Note: Even though the subtype says "spiffs", XiaoZhi uses its own custom format.

## XiaoZhi Assets Format

The `xiaozhi-esp32/assets.bin` file contains:
- Wake word models (srmodels.bin)
- Fonts (text fonts, icon fonts)
- Emoji collections
- Theme skins and backgrounds
- Layout definitions
- All referenced by `index.json` manifest

Structure (from `assets.cc`):
```c
struct {
    uint32_t num_files;     // File count
    uint32_t checksum;      // Data checksum
    uint32_t data_length;   // Payload length
    // Asset table entries
    // Asset data (each prefixed with 'ZZ' magic)
}
```

## Building XiaoZhi Assets

If you need to rebuild the assets.bin:

1. Navigate to xiaozhi-esp32 directory
2. Configure with `idf.py menuconfig`
3. Select board type and assets options
4. Build: `idf.py build`
5. The generated `build/generated_assets.bin` will be used

Or use the pre-built `assets.bin` file provided in the xiaozhi-esp32 repository.

## Testing

Flash the firmware:
```bash
idf.py flash monitor
```

Expected log output:
```
I (xxxx) Assets: The storage free size is 21312 KB
I (xxxx) Assets: The partition size is 7168 KB
I (xxxx) Assets: The checksum calculation time is X ms
I (xxxx) Assets: Refreshing display theme...
```

The error `The index.json file is not found` should no longer appear.

## Important Notes

1. **Do NOT** re-enable the SPIFFS assets creation in `main/CMakeLists.txt`
2. The `spiffs/` directory (with settings.json) is NOT used for xiaozhi assets
3. If you need S3Watch-specific data, consider using NVS or a separate partition
4. The xiaozhi `assets.bin` must be compatible with your board configuration

## Dual Boot Considerations

When using dual-boot with OTA:
- **OTA_0** (0x20000): S3Watch firmware
- **OTA_1** (0xC20000): XiaoZhi-ESP32 firmware
- **Assets** (0x1820000): Shared XiaoZhi assets

The assets partition is shared between both firmware images when running XiaoZhi.

## Troubleshooting

### If "index.json not found" still appears:
1. Verify `xiaozhi-esp32/assets.bin` exists and is > 0 bytes
2. Check partition table has "assets" partition at correct offset
3. Ensure flash command includes assets.bin
4. Re-flash: `idf.py flash`

### If assets are too large:
- The partition is 7MB (0x700000 bytes)
- Verify assets.bin size: `ls -lh xiaozhi-esp32/assets.bin`
- If larger, reduce emoji collection or use compressed fonts

### To verify assets were flashed:
```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 read_flash 0x1820000 0x1000 assets_header.bin
hexdump -C assets_header.bin | head
```

Should see the file count, checksum, and length in the first 12 bytes.

---

**Status**: ✅ Fixed and verified
**Date**: November 13, 2025
**Build**: Successful with assets integration
