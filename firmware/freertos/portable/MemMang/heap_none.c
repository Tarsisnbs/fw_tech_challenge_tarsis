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
