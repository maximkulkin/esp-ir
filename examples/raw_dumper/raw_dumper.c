#include <stdio.h>
#include <stdlib.h>
#include <esp/uart.h>
#include <FreeRTOS.h>
#include <task.h>

#include <esp_ir/ir_rx.h>
#include <esp_ir/ir_raw.h>


#define IR_RX_GPIO 12


void ir_dump_task(void *arg) {
    ir_rx_init(IR_RX_GPIO, 1024);

    uint16_t buffer_size = sizeof(int16_t) * 1024;
    int16_t *buffer = malloc(buffer_size);
    while (1) {
        uint16_t size = buffer_size;
        if (ir_raw_recv(0, buffer, &size) <= 0)
            continue;

        printf("Decoded packet (size = %d):\n", size);
        for (int i=0; i < size; i++) {
            printf("%5d ", buffer[i]);
            if (i % 16 == 15)
                printf("\n");
        }

        if (size % 16)
            printf("\n");
    }
}


void user_init(void) {
    uart_set_baud(0, 115200);

    xTaskCreate(ir_dump_task, "IR dump", 2048, NULL, tskIDLE_PRIORITY, NULL);
}
