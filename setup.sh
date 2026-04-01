#!/bin/bash
# Setup script for firmware challenge - downloads FreeRTOS

set -e

FREERTOS_VERSION="V10.6.2"
FREERTOS_URL="https://github.com/FreeRTOS/FreeRTOS-Kernel/archive/refs/tags/${FREERTOS_VERSION}.tar.gz"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FREERTOS_DIR="${SCRIPT_DIR}/firmware/freertos"

echo "================================================"
echo "Firmware Challenge Setup"
echo "================================================"
echo ""

# Check if FreeRTOS already exists
if [ -d "${FREERTOS_DIR}/include" ]; then
    echo "✓ FreeRTOS already installed in ${FREERTOS_DIR}"
    exit 0
fi

echo "Downloading FreeRTOS ${FREERTOS_VERSION}..."
mkdir -p "${FREERTOS_DIR}"
cd "${FREERTOS_DIR}"

# Download and extract
wget -q "${FREERTOS_URL}" -O freertos.tar.gz
tar -xzf freertos.tar.gz --strip-components=1
rm freertos.tar.gz

echo "✓ FreeRTOS downloaded successfully"

# Create heap_none.c (not included in standard FreeRTOS)
echo "Creating heap_none.c..."
cat > "${FREERTOS_DIR}/portable/MemMang/heap_none.c" << 'HEAP_NONE_EOF'
/* FreeRTOS heap_none.c - No dynamic allocation */
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"

void *pvPortMalloc( size_t xWantedSize ) {
    ( void ) xWantedSize;
    configASSERT( 0 );
    return NULL;
}

void vPortFree( void *pv ) {
    ( void ) pv;
}

void vPortInitialiseBlocks( void ) {
}

size_t xPortGetFreeHeapSize( void ) {
    return 0;
}

size_t xPortGetMinimumEverFreeHeapSize( void ) {
    return 0;
}

void vPortGetHeapStats( HeapStats_t * pxHeapStats ) {
    if( pxHeapStats != NULL ) {
        pxHeapStats->xAvailableHeapSpaceInBytes = 0;
        pxHeapStats->xSizeOfLargestFreeBlockInBytes = 0;
        pxHeapStats->xSizeOfSmallestFreeBlockInBytes = 0;
        pxHeapStats->xNumberOfFreeBlocks = 0;
        pxHeapStats->xMinimumEverFreeBytesRemaining = 0;
        pxHeapStats->xNumberOfSuccessfulAllocations = 0;
        pxHeapStats->xNumberOfSuccessfulFrees = 0;
    }
}
HEAP_NONE_EOF

echo "✓ heap_none.c created"
echo ""
echo "Setup complete! You can now build the firmware:"
echo "  cd firmware"
echo "  mkdir build"
echo "  cd build"
echo "  cmake .."
echo "  make"
echo ""
