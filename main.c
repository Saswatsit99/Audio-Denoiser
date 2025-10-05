#include <stdio.h>
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_intr_alloc.h"
#include "soc/uart_struct.h"



#define UART_PORT       UART_NUM_2
#define UART_RX_PIN     16
#define UART_TX_PIN     17
#define BUF_SIZE        2048
#define BAUD_RATE       9600
#define CHUNK_SIZE      512
#define INPUT_BUF_SIZE  128
#define OUTPUT_BUF_SIZE 128 
static const char *TAG = "UART_EVT";
uint8_t full_data[BUF_SIZE];
size_t previous_idx=0;
uint8_t start_flag=0;
uint8_t inp_buf[CHUNK_SIZE * INPUT_BUF_SIZE];
size_t inp_head=0;
size_t inp_tail=0;
uint8_t out_buf[CHUNK_SIZE * OUTPUT_BUF_SIZE];
size_t out_head=0;
size_t out_tail=0;

SemaphoreHandle_t inp_mux;
SemaphoreHandle_t out_mux;
void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t data[512];

    ESP_LOGI(TAG, "UART ready on RX=%d TX=%d", UART_RX_PIN, UART_TX_PIN);
    QueueHandle_t uart_queue = * (QueueHandle_t *) pvParameters; 
    while (1) {
        // Wait for UART events
        if (xQueueReceive(uart_queue, &event, portMAX_DELAY)) {
            if (event.type == UART_DATA) {
                int len = uart_read_bytes(UART_PORT, data, event.size, portMAX_DELAY);
                if(len != CHUNK_SIZE){
                ESP_LOGE(TAG,"DATA OF UNWANTED SIZE\n");
                
                }
            if((((inp_head + len)&((1<<16) -1 )) - inp_tail)==0){
                ESP_LOGE(TAG,"INPUT BUFFER OVERFLOW\n");
                }
            else{
            memcpy(inp_buf + inp_head, data, len);
            inp_head += len;
            inp_head &=((1<<16)-1);
            ESP_LOGI(TAG, "Read %d bytes from UART", len);}
            }
            else if (event.type == UART_FIFO_OVF) {
                ESP_LOGW(TAG, "hw fifo overflow");
                uart_flush_input(UART_PORT);
                xQueueReset(uart_queue);
            } else if (event.type == UART_BUFFER_FULL) {
                ESP_LOGW(TAG, "ring buffer full");
                uart_flush_input(UART_PORT);
                xQueueReset(uart_queue);
            } else if (event.type == UART_PARITY_ERR) {
                ESP_LOGE(TAG, "uart parity error");
            } else if (event.type == UART_FRAME_ERR) {
                ESP_LOGE(TAG, "uart frame error");
            } else {
                ESP_LOGW(TAG, "uart event type: %d", event.type);
            }
        }
    }
}
void app_main(void)
{
    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    QueueHandle_t uart_queue;
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, BUF_SIZE, BUF_SIZE, 20, &uart_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    inp_mux = xSemaphoreCreateCounting(200,0);
    out_mux = xSemaphoreCreateCounting(200,0);
    xTaskCreatePinnedToCore(uart_event_task,"task_core0",2048,&uart_queue,12,NULL,0);


}
