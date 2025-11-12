#!/bin/bash
# Quick helper script to clone and configure XiaoZhi-ESP32

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
XIAOZHI_DIR="$SCRIPT_DIR/xiaozhi-esp32"

echo "========================================"
echo "  XiaoZhi-ESP32 Setup Helper"
echo "========================================"

# Step 1: Clone if needed
if [ ! -d "$XIAOZHI_DIR" ]; then
    echo ""
    echo "[1/4] Cloning xiaozhi-esp32..."
    cd "$(dirname "$SCRIPT_DIR")"
    git clone https://github.com/78/xiaozhi-esp32.git
    echo "✓ Repository cloned"
else
    echo ""
    echo "[1/4] Repository already exists at: $XIAOZHI_DIR"
fi

# Step 2: Set target
echo ""
echo "[2/4] Setting ESP32-S3 target..."
cd "$XIAOZHI_DIR"
idf.py set-target esp32s3
echo "✓ Target set to ESP32-S3"

# Step 3: Configure
echo ""
echo "[3/4] Opening configuration menu..."
echo ""
echo "IMPORTANT SETTINGS TO CONFIGURE:"
echo "  1. Xiaozhi Assistant → Board Type"
echo "     → Select: Waveshare ESP32-S3-Touch-AMOLED-2.06"
echo ""
echo "  2. Serial flasher config → Flash size"
echo "     → Select: 32 MB (if available)"
echo ""
echo "  3. Save and exit (S, then Enter, then Q)"
echo ""
read -p "Press Enter to open menuconfig..."

idf.py menuconfig

# Step 4: Build
echo ""
echo "[4/4] Building XiaoZhi firmware..."
idf.py build

echo ""
echo "========================================"
echo "  ✓ XiaoZhi-ESP32 Ready!"
echo "========================================"
echo ""
echo "Firmware: $XIAOZHI_DIR/build/xiaozhi-esp32.bin"
echo "Size: $(du -h "$XIAOZHI_DIR/build/xiaozhi-esp32.bin" 2>/dev/null | cut -f1)"
echo ""
echo "Next: Flash both firmwares with:"
echo "  cd $SCRIPT_DIR"
echo "  ./flash_dualboot.sh"
echo ""
