# 🚀 S3Watch Dual-Boot System

Welcome to the S3Watch dual-boot system! This setup allows you to run **two completely different firmwares** on your Waveshare ESP32-S3-Touch-AMOLED-2.06:

1. **S3Watch** - Your BLE smartwatch firmware (OTA_0)
2. **XiaoZhi-ESP32** - WiFi-based AI voice assistant (OTA_1)

## 🎯 How It Works

The system uses a **custom bootloader** that checks the BOOT button (GPIO 0) at startup:

- **Normal boot** → Loads S3Watch (OTA_0)
- **Hold BOOT button during power-on** → Loads XiaoZhi-ESP32 (OTA_1)

Based on: [multi-firmware-esp](https://github.com/SurajSonawane2415/multi-firmware-esp)

## 📋 Partition Layout

```
0x0      - Custom Bootloader
0x8000   - Partition Table
0x10000  - OTA Data
0x20000  - OTA_0: S3Watch (8MB)
0x820000 - OTA_1: XiaoZhi-ESP32 (8MB)
0x1020000 - Storage (4MB)
```

## 🛠️ Quick Start

### 1. Build Everything

```bash
./build_dualboot.sh
```

This will:
- Build the custom bootloader
- Build S3Watch firmware
- Clone and prepare XiaoZhi-ESP32

### 2. Configure XiaoZhi Board

```bash
cd xiaozhi-esp32
idf.py menuconfig
```

Navigate to:
```
Xiaozhi Assistant → Board Type → Waveshare ESP32-S3-Touch-AMOLED-2.06
```

Save and build:
```bash
idf.py build
cd ..
```

### 3. Flash Everything

```bash
./flash_dualboot.sh
```

Or specify port manually:
```bash
./flash_dualboot.sh /dev/cu.usbmodem21201
```

## 🎮 Usage

### Switching Between Firmwares

1. **Boot S3Watch (default)**
   - Just power on or reset normally
   - Your smartwatch interface loads

2. **Boot XiaoZhi AI Assistant**
   - Power off device
   - **Hold the BOOT button**
   - While holding, power on or press RESET
   - Release BOOT button after ~1 second
   - XiaoZhi voice assistant loads

### Returning to S3Watch

Simply reset/reboot without holding the BOOT button.

## 📦 Directory Structure

```
S3Watch/
├── bootloader_custom/          # Custom dual-boot bootloader
│   ├── bootloader_components/
│   │   └── main/
│   │       └── bootloader_start.c  # Boot selection logic
│   └── CMakeLists.txt
├── xiaozhi-esp32/             # XiaoZhi AI assistant (cloned)
├── partitions_dualboot.csv    # Dual-boot partition table
├── sdkconfig.dualboot         # Dual-boot configuration
├── build_dualboot.sh          # Build script
└── flash_dualboot.sh          # Flash script
```

## 🔧 Manual Build Steps

If you prefer manual control:

### Build Custom Bootloader
```bash
cd bootloader_custom
idf.py build
cd ..
```

### Build S3Watch
```bash
idf.py -D SDKCONFIG_DEFAULTS=sdkconfig.dualboot reconfigure
idf.py build
```

### Build XiaoZhi
```bash
cd xiaozhi-esp32
idf.py set-target esp32s3
idf.py menuconfig  # Select board: Waveshare ESP32-S3-Touch-AMOLED-2.06
idf.py build
cd ..
```

### Manual Flash
```bash
PORT=/dev/cu.usbmodem21201

# Flash bootloader and partition table
python -m esptool --chip esp32s3 --port $PORT --baud 921600 \
    write_flash --flash_mode dio --flash_freq 80m --flash_size 16MB \
    0x0     bootloader_custom/build/bootloader/bootloader.bin \
    0x8000  bootloader_custom/build/partition_table/partition-table.bin \
    0x10000 bootloader_custom/build/ota_data_initial.bin

# Flash S3Watch (OTA_0)
python -m esptool --chip esp32s3 --port $PORT --baud 921600 \
    write_flash --flash_mode dio --flash_freq 80m \
    0x20000 build/S3Watch.bin

# Flash XiaoZhi (OTA_1)
python -m esptool --chip esp32s3 --port $PORT --baud 921600 \
    write_flash --flash_mode dio --flash_freq 80m \
    0x820000 xiaozhi-esp32/build/xiaozhi-esp32.bin

# Flash storage
python -m esptool --chip esp32s3 --port $PORT --baud 921600 \
    write_flash --flash_mode dio --flash_freq 80m \
    0x1020000 build/storage.bin
```

## 🐛 Troubleshooting

### "Failed to load partition table"
- Ensure custom bootloader built successfully
- Check partition table exists: `bootloader_custom/build/partition_table/`

### "Boot button not detected"
- GPIO 0 (BOOT button) should be LOW when pressed
- Check button physically works
- Try holding button longer during power-on

### XiaoZhi won't boot
- Verify XiaoZhi firmware is built: `xiaozhi-esp32/build/xiaozhi-esp32.bin`
- Check it was flashed to OTA_1 (0x820000)
- Ensure correct board selected in menuconfig

### S3Watch settings lost
- Storage partition is separate - should persist
- Check storage.bin flashed to correct offset

## 🎨 Customization

### Change Boot Button GPIO

Edit `bootloader_custom/bootloader_components/main/bootloader_start.c`:

```c
#define BOOT_BUTTON_GPIO    21  // Change from 0 to 21
#define GPIO_PIN_REG_0      IO_MUX_GPIO21_REG  // Update register
```

### Adjust Partition Sizes

Edit `partitions_dualboot.csv`:

```csv
ota_0,  app, ota_0, 0x20000,  10M,   # Increase S3Watch to 10MB
ota_1,  app, ota_1, ,         6M,    # Reduce XiaoZhi to 6MB
```

### Add Third Firmware (Advanced)

Create OTA_2 partition and modify bootloader logic to check multiple GPIOs.

## 📚 References

- [ESP-IDF Bootloader](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/bootloader.html)
- [OTA Updates](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/ota.html)
- [Partition Tables](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html)
- [multi-firmware-esp](https://github.com/SurajSonawane2415/multi-firmware-esp)
- [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32)

## 📝 Notes

- Both firmwares are **completely independent**
- No shared state between S3Watch and XiaoZhi
- Storage partition belongs to S3Watch only
- Each firmware has its own NVS, WiFi credentials, etc.
- Flash size must be ≥20MB for dual-boot

## 🤝 Contributing

Feel free to improve the bootloader logic, add more boot options, or create triple-boot configurations!

---

**Enjoy switching between your smartwatch and AI assistant! 🎉**
