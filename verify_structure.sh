#!/bin/bash
# Verify repository structure is complete

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

echo "================================================"
echo "Verifying Firmware Challenge Repository Structure"
echo "================================================"
echo ""

MISSING=0

check_file() {
    if [ -f "$1" ]; then
        echo "✓ $1"
    else
        echo "✗ MISSING: $1"
        MISSING=$((MISSING + 1))
    fi
}

check_dir() {
    if [ -d "$1" ]; then
        echo "✓ $1/"
    else
        echo "✗ MISSING: $1/"
        MISSING=$((MISSING + 1))
    fi
}

echo "Platform files:"
check_file "platform/stm32f411.repl"
check_file "platform/run.resc"
echo ""

echo "Firmware build system:"
check_file "firmware/CMakeLists.txt"
check_file "firmware/STM32F411_FLASH.ld"
check_file "firmware/startup_stm32f411xe.s"
echo ""

echo "Firmware headers:"
check_file "firmware/include/hal_uart.h"
check_file "firmware/include/hal_gpio.h"
check_file "firmware/include/fw_main.h"
check_file "firmware/include/stm32f411xe.h"
check_file "firmware/include/FreeRTOSConfig.h"
echo ""

echo "Firmware sources:"
check_file "firmware/src/hal_uart.c"
check_file "firmware/src/hal_gpio.c"
check_file "firmware/src/main.c"
check_file "firmware/src/fw_main.c"
check_file "firmware/src/malloc_wrapper.c"
echo ""

echo "Test suites:"
check_file "tests/public/smoke.robot"
echo ""

echo "Scripts:"
check_file "scripts/crc8.py"
check_file "setup.sh"
echo ""

echo "Documentation:"
check_file "README.md"
check_file "SETUP.md"
check_file "EVALUATION_GUIDE.md"
echo ""

echo "CI/CD:"
check_file ".github/workflows/grade.yml"
check_file ".gitignore"
echo ""

echo "FreeRTOS (optional - run setup.sh if missing):"
if [ -d "firmware/freertos/include" ]; then
    echo "✓ firmware/freertos/ (FreeRTOS installed)"
else
    echo "⚠ firmware/freertos/ not found - run ./setup.sh to download FreeRTOS"
fi
echo ""

if [ $MISSING -eq 0 ]; then
    echo "================================================"
    echo "✓ All required files present!"
    echo "================================================"
    echo ""
    echo "Next steps:"
    echo "  1. Run ./setup.sh to download FreeRTOS"
    echo "  2. Read SETUP.md for build instructions"
    echo "  3. Read README.md for challenge requirements"
    echo ""
    exit 0
else
    echo "================================================"
    echo "✗ $MISSING file(s) missing!"
    echo "================================================"
    exit 1
fi
