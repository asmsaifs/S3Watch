# 🚀 S3Watch Dual-Boot Quick Start

## Current Status
✅ S3Watch firmware built (2.3MB)  
✅ Custom bootloader with GPIO 0 detection  
✅ Partition table configured for 32MB flash  
⏳ XiaoZhi-ESP32 ready to integrate  

---

## 📋 Quick Commands

### Flash S3Watch Only (Test Current Build)
```bash
idf.py -p /dev/cu.usbmodem* flash monitor
```

### Setup XiaoZhi-ESP32 (Interactive)
```bash
./setup_xiaozhi.sh
```

### Flash Complete Dual-Boot System
```bash
./flash_dualboot.sh
# Or specify port: ./flash_dualboot.sh /dev/cu.usbmodem14201
```

### Monitor Serial Output
```bash
idf.py -p /dev/cu.usbmodem* monitor
```

---

## 🎮 Boot Selection

| Action | Boots To | Partition |
|--------|----------|-----------|
| **Normal power-on** | S3Watch Smartwatch | OTA_0 @ 0x20000 |
| **Hold BOOT button** | XiaoZhi AI Assistant | OTA_1 @ 0xC20000 |

---

## 📊 Flash Memory Map (32MB)

```
0x000000  ┌─────────────────────────────┐
          │ Bootloader (Custom)    21KB │
0x008000  ├─────────────────────────────┤
          │ Partition Table         4KB │
0x009000  ├─────────────────────────────┤
          │ NVS Storage            24KB │
0x00F000  ├─────────────────────────────┤
          │ PHY Init                4KB │
0x010000  ├─────────────────────────────┤
          │ OTA Data                8KB │
0x020000  ├─────────────────────────────┤
          │                             │
          │ OTA_0: S3Watch         12MB │
          │ (2.3MB used, 81% free)      │
          │                             │
0xC20000  ├─────────────────────────────┤
          │                             │
          │ OTA_1: XiaoZhi         12MB │
          │ (Ready for firmware)        │
          │                             │
0x1820000 ├─────────────────────────────┤
          │                             │
          │ Storage (SPIFFS)        7MB │
          │ (Shared by both)            │
          │                             │
0x1F20000 └─────────────────────────────┘
```

---

## 🔧 Manual XiaoZhi Setup

If `setup_xiaozhi.sh` doesn't work:

```bash
# 1. Clone
cd /Users/asmsaifs/Workshop
git clone https://github.com/78/xiaozhi-esp32.git
cd xiaozhi-esp32

# 2. Configure
idf.py set-target esp32s3
idf.py menuconfig
# → Xiaozhi Assistant → Board Type → Waveshare ESP32-S3-Touch-AMOLED-2.06
# → Serial flasher config → Flash size → 32 MB

# 3. Build
idf.py build

# 4. Return and flash
cd ../S3Watch
./flash_dualboot.sh
```

---

## 🐛 Troubleshooting

### Serial port not detected
```bash
# List available ports
ls /dev/cu.usbmodem* /dev/ttyUSB* 2>/dev/null

# Flash with specific port
./flash_dualboot.sh /dev/cu.usbmodem14201
```

### Build errors
```bash
# Full clean rebuild
idf.py fullclean
idf.py build
```

### Check partition table
```bash
python /Users/asmsaifs/esp/v5.4.3/esp-idf/components/partition_table/gen_esp32part.py \
  build/partition_table/partition-table.bin
```

### Bootloader not switching
- Verify GPIO 0 is connected to BOOT button
- Check serial output during boot
- Expected messages:
  - Normal: `⌚ Normal boot - Loading S3Watch (OTA_0)`
  - Button held: `🎤 BOOT button pressed - Loading XiaoZhi-ESP32 (OTA_1)`

---

## 📁 Important Files

| File | Purpose |
|------|---------|
| `components/bootloader_override/subproject/main/bootloader_start.c` | Custom bootloader logic |
| `partitions_dualboot.csv` | Partition layout (32MB) |
| `sdkconfig` | Main config (32MB flash enabled) |
| `build_dualboot.sh` | Build both firmwares |
| `flash_dualboot.sh` | Flash complete system |
| `setup_xiaozhi.sh` | Interactive XiaoZhi setup |

---

## ✅ Verification Steps

After flashing:

1. **Normal boot test:**
   ```bash
   # Reset device → Should boot S3Watch
   idf.py -p /dev/cu.usbmodem* monitor
   # Look for: "⌚ Normal boot - Loading S3Watch (OTA_0)"
   ```

2. **XiaoZhi boot test:**
   ```bash
   # Hold BOOT button, press RESET → Should boot XiaoZhi
   # Look for: "🎤 BOOT button pressed - Loading XiaoZhi-ESP32 (OTA_1)"
   ```

3. **Check partition info:**
   ```bash
   # Both firmwares should show correct partition
   # S3Watch: "Running from partition: ota_0"
   # XiaoZhi: "Running from partition: ota_1"
   ```

---

## 🎯 Next Actions

- [ ] Flash and test S3Watch: `idf.py flash monitor`
- [ ] Setup XiaoZhi: `./setup_xiaozhi.sh`
- [ ] Flash dual-boot: `./flash_dualboot.sh`
- [ ] Test boot switching with BOOT button
- [ ] Verify both firmwares work correctly

---

**Ready to proceed!** Start with `./setup_xiaozhi.sh` 🚀
