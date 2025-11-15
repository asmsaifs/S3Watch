#!/bin/bash
# Build script for S3Watch dual-boot system

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "======================================"
echo "  S3Watch Dual-Boot Build Script"
echo "======================================"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Step 1: Build S3Watch firmware with custom bootloader (OTA_0)
echo -e "\n${BLUE}[1/2] Building S3Watch firmware with custom bootloader (OTA_0)...${NC}"
idf.py set-target esp32s3
idf.py -D SDKCONFIG_DEFAULTS=sdkconfig.dualboot fullclean reconfigure build
echo -e "${GREEN}✓ S3Watch firmware built with custom bootloader${NC}"

# Step 2: Clone and build XiaoZhi-ESP32 (OTA_1)
echo -e "\n${BLUE}[2/2] Preparing XiaoZhi-ESP32 firmware (OTA_1)...${NC}"
if [ ! -d "xiaozhi-esp32" ]; then
    echo "Cloning xiaozhi-esp32 repository..."
    git clone --depth 1 https://github.com/78/xiaozhi-esp32.git
fi

cd xiaozhi-esp32
echo "Setting up XiaoZhi for Waveshare ESP32-S3-Touch-AMOLED-2.06..."
idf.py set-target esp32s3

echo -e "${YELLOW}⚠ IMPORTANT: Configure XiaoZhi board manually${NC}"
echo ""
echo "Run these commands:"
echo "  cd xiaozhi-esp32"
echo "  idf.py menuconfig"
echo "    → Xiaozhi Assistant → Board Type → Waveshare ESP32-S3-Touch-AMOLED-2.06"
echo "  idf.py build"
echo "  cd .."
echo ""
cd ..

echo -e "\n${GREEN}======================================"
echo "  Build Complete!"
echo "======================================${NC}"
echo ""
echo "Next steps:"
echo "  1. Configure and build XiaoZhi (see above)"
echo "  2. Flash everything: ./flash_dualboot.sh"
echo ""
