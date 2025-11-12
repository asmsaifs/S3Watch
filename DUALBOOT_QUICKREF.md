# 🎯 S3Watch Dual-Boot System - Ready to Flash!

## 🚀 Quick Start (3 Commands)

```bash
# 1. Build S3Watch + Custom Bootloader
./build_dualboot.sh

# 2. Build XiaoZhi-ESP32
cd xiaozhi-esp32
idf.py menuconfig  # Select: Xiaozhi Assistant → Board → Waveshare ESP32-S3-Touch-AMOLED-2.06
idf.py build
cd ..

# 3. Flash Everything
./flash_dualboot.sh
```

## 🎮 How to Switch Firmwares

### Boot S3Watch (Default)
```
Power On → S3Watch Smartwatch
```

### Boot XiaoZhi AI Assistant
```
Hold BOOT Button → Press RESET → Release BOOT → XiaoZhi AI
```

## 📁 Project Structure

```
S3Watch/
├── components/bootloader/          # Custom dual-boot bootloader
├── xiaozhi-esp32/                 # XiaoZhi AI (cloned)
├── partitions_dualboot.csv        # Dual-boot partition table
├── build_dualboot.sh              # Build script
├── flash_dualboot.sh              # Flash script
├── DUALBOOT_README.md             # Complete guide
├── QUICKSTART_DUALBOOT.md         # Quick reference
└── IMPLEMENTATION_SUMMARY.md      # Technical details
```

## 📖 Documentation

- **[QUICKSTART_DUALBOOT.md](QUICKSTART_DUALBOOT.md)** - Get started in 5 minutes
- **[DUALBOOT_README.md](DUALBOOT_README.md)** - Complete technical guide
- **[IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md)** - How it works

## ✨ What You Get

✅ **S3Watch** - BLE smartwatch (OTA_0, 8MB)  
✅ **XiaoZhi-ESP32** - WiFi AI voice assistant (OTA_1, 8MB)  
✅ **Boot button selection** - Physical hardware switch  
✅ **Independent firmwares** - No shared state  
✅ **Easy updates** - Flash either firmware separately  

## 🔧 System Requirements

- **Hardware:** Waveshare ESP32-S3-Touch-AMOLED-2.06
- **Flash:** 16MB minimum (your board has this ✅)
- **Software:** ESP-IDF 5.4+
- **Port:** Auto-detected (or specify manually)

## 📊 Flash Layout

| Address | Size | Content |
|---------|------|---------|
| 0x0 | 128KB | Custom Bootloader |
| 0x20000 | 8MB | S3Watch Firmware |
| 0x820000 | 8MB | XiaoZhi Firmware |
| 0x1020000 | 4MB | Storage (SPIFFS) |

## 🎯 Next Steps

1. Read [QUICKSTART_DUALBOOT.md](QUICKSTART_DUALBOOT.md)
2. Run `./build_dualboot.sh`
3. Configure XiaoZhi board
4. Flash with `./flash_dualboot.sh`
5. Enjoy dual-boot! 🎉

---

**Based on:** [multi-firmware-esp](https://github.com/SurajSonawane2415/multi-firmware-esp) + [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32)
