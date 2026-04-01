/* DO NOT MODIFY */
/**
 * @file malloc_wrapper.c
 * @brief Malloc call counter for testing - tracks dynamic allocation attempts
 *
 * This wrapper intercepts malloc calls and increments a counter at a fixed
 * address. The test framework reads this counter to verify no dynamic
 * allocation occurred during firmware execution.
 */

#include <stddef.h>
#include <stdint.h>

/* Counter at fixed address (must match linker script) */
volatile uint32_t __attribute__((section(".malloc_counter"))) __malloc_call_count = 0;

/* Weak reference to real malloc (will be NULL with heap_none.c) */
extern void *__real_malloc(size_t size) __attribute__((weak));

/**
 * @brief Wrapper for malloc - increments counter and calls real malloc
 * @param size Requested allocation size
 * @return NULL (heap_none.c doesn't implement malloc)
 */
void *__wrap_malloc(size_t size) {
    __malloc_call_count++;

    /* With heap_none.c, __real_malloc doesn't exist or returns NULL */
    if (__real_malloc) {
        return __real_malloc(size);
    }

    return NULL;  /* Always fail allocation */
}
