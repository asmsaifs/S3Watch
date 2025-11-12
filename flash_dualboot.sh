#!/bin/bash
# Flash script for S3Watch dual-boot system

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "======================================"
echo "  S3Watch Dual-Boot Flash Script"
echo "======================================"

# Detect serial port
PORT=""
if [ -e /dev/cu.usbmodem* ]; then
    PORT=$(ls /dev/cu.usbmodem* | head -n 1)
elif [ -e /dev/ttyUSB* ]; then
    PORT=$(ls /dev/ttyUSB* | head -n 1)
fi

if [ -z "$PORT" ]; then
    echo -e "${RED}Error: No serial port detected!${NC}"
    echo "Please specify port manually: $0 /dev/ttyUSB0"
    exit 1
fi

# Allow manual port override
if [ ! -z "$1" ]; then
    PORT="$1"
fi

echo -e "${BLUE}Using serial port: ${PORT}${NC}"

# Check if builds exist
if [ ! -f "build/bootloader/bootloader.bin" ]; then
    echo -e "${RED}Error: Custom bootloader not built!${NC}"
    echo "Run ./build_dualboot.sh first"
    exit 1
fi

if [ ! -f "build/S3Watch.bin" ]; then
    echo -e "${RED}Error: S3Watch firmware not built!${NC}"
    echo "Run ./build_dualboot.sh first"
    exit 1
fi

# Flash sequence
echo -e "\n${BLUE}Flashing dual-boot system...${NC}"

python -m esptool \
    --chip esp32s3 \
    --port "$PORT" \
    --baud 921600 \
    --before default_reset \
    --after hard_reset \
    write_flash \
    --flash_mode dio \
    --flash_freq 80m \
    --flash_size 32MB \
    0x0     build/bootloader/bootloader.bin \
    0x8000  build/partition_table/partition-table.bin \
    0x10000 build/ota_data_initial.bin \
    0x20000 build/S3Watch.bin

echo -e "\n${GREEN}✓ S3Watch firmware flashed to OTA_0${NC}"

# Flash XiaoZhi if available
if [ -f "xiaozhi-esp32/build/xiaozhi.bin" ]; then
    echo -e "\n${BLUE}Flashing XiaoZhi-ESP32 to OTA_1...${NC}"
    python -m esptool \
        --chip esp32s3 \
        --port "$PORT" \
        --baud 921600 \
        --before default_reset \
        --after hard_reset \
        write_flash \
        --flash_mode dio \
        --flash_freq 80m \
        --flash_size 32MB \
        0xC20000 xiaozhi-esp32/build/xiaozhi.bin
    
    echo -e "${GREEN}✓ XiaoZhi-ESP32 firmware flashed to OTA_1${NC}"
else
    echo -e "\n${YELLOW}⚠ XiaoZhi firmware not found${NC}"
    echo "Build it with: cd xiaozhi-esp32 && idf.py build"
    echo "Then run this script again to flash OTA_1"
fi

# Flash storage if available
if [ -f "build/storage.bin" ]; then
    STORAGE_OFFSET="0x1820000"  # 32MB flash: after OTA_1 (12MB each)
    echo -e "\n${BLUE}Flashing storage partition...${NC}"
    python -m esptool \
        --chip esp32s3 \
        --port "$PORT" \
        --baud 921600 \
        write_flash \
        --flash_mode dio \
        --flash_freq 80m \
        $STORAGE_OFFSET build/storage.bin
    echo -e "${GREEN}✓ Storage partition flashed${NC}"
fi

echo -e "\n${GREEN}======================================"
echo "  Flash Complete!"
echo "======================================${NC}"
echo ""
echo "Boot Instructions:"
echo "  • Normal boot → S3Watch (smartwatch)"
echo "  • Hold BOOT button during power-on → XiaoZhi-ESP32 (AI assistant)"
echo ""
echo -e "${BLUE}Starting monitor...${NC}"
idf.py -p "$PORT" monitor
