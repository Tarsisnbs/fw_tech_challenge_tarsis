/* DO NOT MODIFY */
#include "fw_main.h"

int main(void) {
    /* Initialize firmware */
    fw_init();

    /* Start firmware main loop (never returns) */
    fw_run();

    /* Should never reach here */
    while (1);
}
