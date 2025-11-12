# Testing Boot Button Switch

## Updated Bootloader
The bootloader has been fixed to properly read GPIO 0 using the correct registers for ESP32-S3.

## Changes Made
- Changed from RTC_IO registers to standard GPIO registers
- Using `GPIO_IN_REG` to read GPIO 0 state
- Added debug logging to show GPIO state and boot index
- Increased stabilization delay to 1,000,000 cycles

## Testing Procedure

### 1. Normal Boot Test
```bash
# Reset device WITHOUT holding BOOT button
idf.py -p /dev/cu.usbmodem* monitor
```

**Expected output:**
```
dual_boot: ⌚ Normal boot - Loading S3Watch (OTA_0)
dual_boot: GPIO0=1, boot_index=0
```

### 2. Boot Button Test
```bash
# Steps:
# 1. Hold down BOOT button (GPIO 0)
# 2. Press RESET button (or power cycle)
# 3. Keep holding BOOT button for 1-2 seconds
# 4. Release BOOT button
# 5. Watch serial output
```

**Expected output:**
```
dual_boot: 🎤 BOOT button pressed - Loading XiaoZhi-ESP32 (OTA_1)
dual_boot: GPIO0=0, boot_index=1
```

### 3. Verify GPIO State
The bootloader now logs:
- `GPIO0=1` means button NOT pressed (pull-up active)
- `GPIO0=0` means button IS pressed (pulled to ground)

## Troubleshooting

### If boot doesn't switch:
1. Check GPIO 0 is actually connected to BOOT button on your board
2. Verify button timing - hold BOOT before pressing RESET
3. Check serial output for GPIO0 value

### Check bootloader was flashed:
```bash
ls -lh build/bootloader/bootloader.bin
# Should show recent timestamp
```

### Manual flash bootloader only:
```bash
esptool.py -p /dev/cu.usbmodem* write_flash 0x0 build/bootloader/bootloader.bin
```

## Hardware Verification

For Waveshare ESP32-S3-Touch-AMOLED-2.06:
- BOOT button should be connected to GPIO 0
- When pressed, GPIO 0 is pulled to GND (LOW)
- When released, GPIO 0 is pulled HIGH by internal pull-up

You can verify with a multimeter:
- Measure GPIO 0 voltage
- Normal: ~3.3V (HIGH)
- Button pressed: ~0V (LOW)

## Next Steps

Once boot switching works:
1. Build XiaoZhi firmware: `./setup_xiaozhi.sh`
2. Flash to OTA_1: `esptool.py write_flash 0xC20000 ../xiaozhi-esp32/build/xiaozhi-esp32.bin`
3. Test switching between both firmwares
