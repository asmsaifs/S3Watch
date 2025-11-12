# ⚡ Quick Start Guide - S3Watch Dual-Boot

## TL;DR - 3 Commands

```bash
./build_dualboot.sh          # Build everything
cd xiaozhi-esp32 && idf.py menuconfig  # Select board, then build
cd .. && ./flash_dualboot.sh # Flash to device
```

## Step-by-Step (5 minutes)

### 1️⃣ Build (First Time Setup)

```bash
chmod +x build_dualboot.sh flash_dualboot.sh
./build_dualboot.sh
```

**What happens:**
- ✅ Builds custom bootloader
- ✅ Builds S3Watch firmware  
- ✅ Clones xiaozhi-esp32 repo
- ⚠️ Prompts you to configure XiaoZhi

### 2️⃣ Configure XiaoZhi Board

```bash
cd xiaozhi-esp32
idf.py menuconfig
```

**In the menu:**
1. Navigate: `Xiaozhi Assistant` → `Board Type`
2. Select: `Waveshare ESP32-S3-Touch-AMOLED-2.06`
3. Press `S` to save
4. Press `Q` to quit

Then build:
```bash
idf.py build
cd ..
```

### 3️⃣ Flash Everything

```bash
./flash_dualboot.sh
```

Auto-detects USB port and flashes:
- Custom bootloader
- S3Watch (OTA_0)
- XiaoZhi (OTA_1)
- Storage partition

## 🎮 Using Dual-Boot

### Boot S3Watch (Default)
```
Power On → S3Watch Loads
```

### Boot XiaoZhi AI
```
1. Hold BOOT button
2. Press RESET (or power on)
3. Release BOOT after 1 second
4. XiaoZhi Loads!
```

### Switch Back
```
Press RESET (without holding BOOT)
```

## 🔄 Updating Firmwares

### Update S3Watch Only
```bash
idf.py build
./flash_dualboot.sh
```

### Update XiaoZhi Only
```bash
cd xiaozhi-esp32
idf.py build
cd ..
./flash_dualboot.sh
```

## ⚠️ Important Notes

- **First flash:** Both firmwares installed (~5 minutes)
- **Subsequent:** Updates only changed firmware
- **BOOT button = GPIO 0** (labeled BOOT on board)
- **Flash size:** Requires 16MB+ (your board has 16MB ✅)

## 🆘 Quick Fixes

**Can't flash?**
```bash
# Specify port manually
./flash_dualboot.sh /dev/cu.usbmodem21201
```

**XiaoZhi won't boot?**
```bash
# Check it was built:
ls -lh xiaozhi-esp32/build/*.bin

# Re-flash OTA_1:
./flash_dualboot.sh
```

**Bootloader issues?**
```bash
# Rebuild bootloader
cd bootloader_custom && idf.py fullclean build && cd ..
./flash_dualboot.sh
```

## 📊 Flash Memory Map

```
┌─────────────────────────────────────┐ 0x0
│  Custom Bootloader (128KB)          │
├─────────────────────────────────────┤ 0x8000
│  Partition Table                    │
├─────────────────────────────────────┤ 0x10000
│  OTA Data                            │
├─────────────────────────────────────┤ 0x20000
│  ╔═════════════════════════════════╗│
│  ║   OTA_0: S3Watch (8MB)          ║│
│  ╚═════════════════════════════════╝│
├─────────────────────────────────────┤ 0x820000
│  ╔═════════════════════════════════╗│
│  ║   OTA_1: XiaoZhi-ESP32 (8MB)   ║│
│  ╚═════════════════════════════════╝│
├─────────────────────────────────────┤ 0x1020000
│  Storage (4MB)                       │
└─────────────────────────────────────┘ 0x1420000
```

## 🎯 What's Next?

1. **Customize S3Watch** - Your regular development workflow
2. **Configure XiaoZhi** - WiFi, voice wake word, etc.
3. **Enjoy switching** between watch and AI assistant!

---

💡 **Pro Tip:** Set up WiFi for XiaoZhi first boot, then it remembers!

For detailed docs, see: [DUALBOOT_README.md](DUALBOOT_README.md)
