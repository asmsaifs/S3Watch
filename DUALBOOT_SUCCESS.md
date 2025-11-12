# ✅ Dual-Boot System - Successfully Built!

## 🎉 Build Status: COMPLETE

The S3Watch dual-boot system has been successfully built with a custom bootloader that allows switching between two firmwares.

## 📊 Flash Layout (32MB Total)

```
┌─────────────────────────────────────────────────────────────┐
│ 0x0      Bootloader (Custom - GPIO0 Detection)     21KB    │
├─────────────────────────────────────────────────────────────┤
│ 0x8000   Partition Table                           4KB     │
├─────────────────────────────────────────────────────────────┤
│ 0x9000   NVS (Non-Volatile Storage)                24KB    │
├─────────────────────────────────────────────────────────────┤
│ 0xF000   PHY Init Data                             4KB     │
├─────────────────────────────────────────────────────────────┤
│ 0x10000  OTA Data (Boot Selection)                 8KB     │
├─────────────────────────────────────────────────────────────┤
│ 0x20000  OTA_0: S3Watch Smartwatch                 12MB    │
│          (Current size: 2.3MB - 81% free)                  │
├─────────────────────────────────────────────────────────────┤
│ 0xC20000 OTA_1: XiaoZhi AI Assistant                12MB    │
│          (Ready for firmware)                              │
├─────────────────────────────────────────────────────────────┤
│ 0x1820000 Storage (SPIFFS - Shared)                7MB     │
└─────────────────────────────────────────────────────────────┘
```

## 🎮 Boot Selection Logic

**Normal Boot (GPIO 0 HIGH):**
- Boots to **S3Watch** (OTA_0)
- Default behavior on power-up
- Regular smartwatch functionality

**BOOT Button Held (GPIO 0 LOW):**
- Boots to **XiaoZhi AI Assistant** (OTA_1)
- Hold BOOT button during power-up
- Voice assistant mode

## 📁 Build Artifacts

All files ready in `build/`:
- ✅ `bootloader/bootloader.bin` - Custom dual-boot bootloader (21KB)
- ✅ `partition_table/partition-table.bin` - 32MB partition layout
- ✅ `ota_data_initial.bin` - Initial OTA boot data
- ✅ `S3Watch.bin` - S3Watch firmware (2.3MB)
- ✅ `storage.bin` - SPIFFS filesystem image

## 🔧 Custom Bootloader Features

Located in: `components/bootloader_override/subproject/main/bootloader_start.c`

**Functionality:**
1. Reads GPIO 0 state at boot time
2. Selects OTA partition based on button state
3. Logs boot decision to serial
4. Loads selected firmware

**Boot Messages:**
```
⌚ Normal boot - Loading S3Watch (OTA_0)
🎤 BOOT button pressed - Loading XiaoZhi-ESP32 (OTA_1)
```

## 📦 Next Steps

### 1. Flash S3Watch (Current Build)
```bash
idf.py -p /dev/cu.usbmodem* flash monitor
```

### 2. Build XiaoZhi-ESP32
```bash
# Clone repository
cd /Users/asmsaifs/Workshop
git clone https://github.com/78/xiaozhi-esp32.git
cd xiaozhi-esp32

# Configure for Waveshare board
idf.py set-target esp32s3
idf.py menuconfig
# Select: Waveshare ESP32-S3-Touch-AMOLED-2.06

# Build
idf.py build
```

### 3. Flash XiaoZhi to OTA_1 Partition
```bash
cd /Users/asmsaifs/Workshop/S3Watch

# Flash XiaoZhi firmware to OTA_1 partition at 0xC20000
esptool.py -p /dev/cu.usbmodem* write_flash 0xC20000 \
  ../xiaozhi-esp32/build/xiaozhi-esp32.bin
```

### 4. Test Boot Selection
```bash
# Normal boot → S3Watch
# Press and hold BOOT button during power-up → XiaoZhi
```

## 🔍 Verification Commands

**Check partition table:**
```bash
python /Users/asmsaifs/esp/v5.4.3/esp-idf/components/partition_table/gen_esp32part.py \
  build/partition_table/partition-table.bin
```

**Monitor serial output:**
```bash
idf.py -p /dev/cu.usbmodem* monitor
```

**Check firmware sizes:**
```bash
ls -lh build/*.bin
```

## ⚙️ Configuration Files

- `partitions_dualboot.csv` - Partition layout definition
- `sdkconfig` - Project configuration (32MB flash enabled)
- `components/bootloader_override/` - Custom bootloader source

## 🎯 Hardware Requirements

- **Board:** Waveshare ESP32-S3-Touch-AMOLED-2.06
- **Flash:** 32MB (full capacity utilized)
- **Boot Button:** GPIO 0 (physical BOOT button on board)

## 📝 Notes

- Both firmwares share the same `storage` partition (7MB SPIFFS)
- Settings and data can be accessed by both firmwares
- Custom bootloader is ~21KB (36% of available bootloader space)
- OTA updates can be implemented for both partitions
- Each firmware has 12MB available (currently ~10MB free per partition)

---

**Status:** Ready for XiaoZhi integration and testing! 🚀
