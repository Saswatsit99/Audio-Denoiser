#include <stdio.h>
#include "esp_log.h"
#include "fft.h"
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
#define CHUNK_SIZE      1U<<9
#define INPUT_BUF_SIZE  1U<<8
// #define OUTPUT_BUF_SIZE 128 
static const char *TAG = "UART_EVT";
static const char *TAG1 = "UART_SRL_DATA";
uint8_t full_data[BUF_SIZE];
size_t previous_idx=0;
uint8_t start_flag=0;
uint8_t inp_buf[CHUNK_SIZE * INPUT_BUF_SIZE];
int16_t packet[CHUNK_SIZE/2], final_packet[CHUNK_SIZE/2];
size_t inp_head=0;
size_t inp_tail=0;
// uint8_t out_buf[CHUNK_SIZE * OUTPUT_BUF_SIZE];
// size_t out_head=0;
// size_t out_tail=0;
uint8_t start_byte=0xAA;
uint8_t end_byte=0xBB;
SemaphoreHandle_t inp_mux;
SemaphoreHandle_t out_mux;
void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t data[512];

    ESP_LOGI(TAG, "UART ready on RX=%d TX=%d", UART_RX_PIN, UART_TX_PIN);
    QueueHandle_t uart_queue = * (QueueHandle_t *) pvParameters; 
    size_t free_space;
    int len = 0;
    while (1) {
        // Wait for UART events
        if (xQueueReceive(uart_queue, &event, portMAX_DELAY)) {
            if (event.type == UART_DATA) {
            len += uart_read_bytes(UART_PORT, data + len, event.size, portMAX_DELAY);
                //ESP_LOGI(TAG, "Read %d bytes from UART", len);
            if(len<512)
                continue;
                //for(int i=0;i<len;i++)
                //{
                //    ESP_LOGI(TAG,"%02X ",data[i]);
                //}
            if(inp_tail<=inp_head)
            free_space = ((1 << 17) - (inp_head - inp_tail));
            else
            free_space = inp_tail - inp_head;
            if (free_space ==0) {
            //ESP_LOGI(TAG,"INPUT BUFFER OVERFLOW\n");
                vTaskDelay(pdMS_TO_TICKS(1)); // to avoid busy wait
            }
            else{
            memcpy(inp_buf + inp_head, data, len);
            inp_head += len;
            inp_head &=(( 1 << 17)-1);
            xSemaphoreGive(inp_mux);
            //ESP_LOGI(TAG, "Read %d bytes from UART", len);
            len=0;
            }
            }
            else if (event.type == UART_BREAK) {
                ESP_LOGW(TAG, "uart rx break");
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
void packet_process_task(void *arg)
{
    while (1)
    {
            // Compute number of bytes available in circular buffer
        if(xSemaphoreTake(inp_mux,portMAX_DELAY)==pdTRUE)
        {
            size_t available;
            if (inp_head >= inp_tail)
                available = inp_head - inp_tail;
            else
                available = ( (1<<17) - (inp_tail - inp_head) );

            // Only process if enough for one CHUNK
            if (available)
            {
                // Copy packet from circular buffer with wrapping
                //ESP_LOGI(TAG1, "copying packet ");
                memcpy((uint8_t *)packet, inp_buf + inp_tail, CHUNK_SIZE);

                // Update tail
                inp_tail = (inp_tail + 512) & ((1<<17)-1);

                // Process packet
                //ESP_LOGI(TAG1, "Processing packet");
                fft_process(packet, final_packet);
                // uint8_t *ptr = (uint8_t *)final_packet; 
                uart_write_bytes(UART_NUM_0, (const char *)&start_byte, 1);
                uart_write_bytes(UART_NUM_0, (const char *)final_packet, CHUNK_SIZE);
                uart_write_bytes(UART_NUM_0, (const char *)&end_byte, 1);
                //ESP_LOGI(TAG1, "Packet processed and sent");
            }
        //xSemaphoreGive(inp_mux);
        // Small delay to avoid tight loop
        vTaskDelay(pdMS_TO_TICKS(1));
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
    inp_mux = xSemaphoreCreateCounting(400,0);
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, BUF_SIZE, BUF_SIZE, 40, &uart_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    xTaskCreatePinnedToCore(uart_event_task,"task_core0",4096,&uart_queue,12,NULL,0);
    xTaskCreatePinnedToCore(packet_process_task,"task_core1",4096,NULL,12,NULL,1);

}



