#include "FreeRTOS.h"    
#include "task.h"        
#include "queue.h"       
#include "semphr.h"
#include "shell.h"
#include "storage.h"
#include "hal_uart.h"
#include <string.h>
#include <stdio.h>

extern TaskHandle_t xSHELLTaskHandle;
extern QueueHandle_t shell_rx_queue;

// Função auxiliar interna para enviar via UART1 com segurança (Mutex ou Direto)
// Se você moveu o Mutex para cá, use-o. Se não, use hal_uart1_send_str direto.
static void shell_print(const char *str) {
    hal_uart1_send_str(str);
}

static void process_command(char *cmd) {
    char out[64];
    if (strcmp(cmd, "STATUS") == 0) {
        int n = 0;
        for (int i = 0; i < MAX_SENSORS; i++) {
            if (storage_get_sensor(i)->registered) n++;
        }
        snprintf(out, sizeof(out), "OK sensors=%d fault=%d\n", n, sensors_check_for_timeout(2000));
        shell_print(out);
    } 
    else if (strcmp(cmd, "DUMP") == 0) {
        if (storage_lock(pdMS_TO_TICKS(100))) {
            for (int i = 0; i < MAX_SENSORS; i++) {
                sensor_data_t *s = storage_get_sensor(i);
                if (s && s->registered) {
                    uint16_t hex = (s->last_payload[0] << 8) | s->last_payload[1];
                    snprintf(out, sizeof(out), "%d last=%04X count=%lu\n", i, hex, s->sample_count);
                    shell_print(out);
                }
            }
            shell_print("END\n");
            storage_unlock();
        }
    }
    else if (strcmp(cmd, "RESET") == 0) {
        storage_init();
        shell_print("OK\n");
    }
    else if (strcmp(cmd, "?") == 0) {
        shell_print("Commands: STATUS, DUMP, RESET, ?\nEND\n");
    }
}

/**
 * @brief   Main Shell Processing Loop.
 * @details Dual-Input handling:
 * 1. Queue-based: Receives bytes from UART ISR and assembles lines.
 * 2. Notification-based: Responds immediately to hardware button events.
 */
void vShellTask(void *arg) {
    (void)arg;
    uint8_t c;
    static char buf[32];
    static uint8_t idx = 0;

    shell_print("Shell Online\n");

    for (;;) {
        /**
         * Wait for characters from the RX queue.
         * Timeout is set to 50ms to allow periodic polling of Task Notifications.
         */
        if (xQueueReceive(shell_rx_queue, &c, pdMS_TO_TICKS(50)) == pdPASS) {
            /* Line End Detection (CR or LF) */
            if (c == '\n' || c == '\r') {
                buf[idx] = '\0'; /* Reset buffer for next command */
                if (idx > 0) process_command(buf);
                idx = 0;
            }
            /* Buffer overflow protection */ 
            else if (idx < 31) {
                buf[idx++] = c;
            }
        }
        /**
         * Direct-to-Task Notification Check.
         * Triggered by pb0_falling_callback (Physical Button).
         * This allows an 'Out-of-Band' command trigger without UART input.
         */
        if (ulTaskNotifyTake(pdTRUE, 0) > 0) {
            process_command("DUMP");
        }
    }
}