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

void vShellTask(void *arg) {
    (void)arg;
    uint8_t c;
    static char buf[32];
    static uint8_t idx = 0;

    shell_print("Shell Online\n");

    for (;;) {
        // Bloqueia esperando Notificação (Botão) OU Dado na Fila (UART)
        if (xQueueReceive(shell_rx_queue, &c, pdMS_TO_TICKS(50)) == pdPASS) {
            if (c == '\n' || c == '\r') {
                buf[idx] = '\0';
                if (idx > 0) process_command(buf);
                idx = 0;
            } else if (idx < 31) {
                buf[idx++] = c;
            }
        }
        
        // Verifica se houve notificação do botão (sem bloquear, pois a fila já bloqueia)
        if (ulTaskNotifyTake(pdTRUE, 0) > 0) {
            process_command("DUMP");
        }
    }
}