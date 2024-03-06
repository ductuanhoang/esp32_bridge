/**
 * @file TCP.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2024-02-22
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "TCP.h"
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "device_data.h"

#define TAG "TCP"

#define TASK_PRIORITY_TCP 5

static void TCP_Task(void *ctx);

tcp_messge_call_back_t tcp_messge_call_back;
static uint8_t rx_buffer[30000];

void TCP_Init(void)
{
    ESP_LOGI(TAG, "TCP_Init called");
    xTaskCreatePinnedToCore(TCP_Task, "TCP_Task", 4096 * 4, NULL, TASK_PRIORITY_TCP, NULL, 1);
}

void TCPRegisterCallback(tcp_messge_call_back_t callback)
{
    if (callback != NULL)
        tcp_messge_call_back = callback;
}

static void TCP_Task(void *ctx)
{
    ESP_LOGI(TAG, "TCP_Task called");
    ESP_LOGI(TAG, "board_data.input = %d", board_data.input);
    ESP_LOGI(TAG, "board_data.tcp_port = %ld", board_data.tcp_port);
    ESP_LOGI(TAG, "board_data.ip = %s", board_data.ip_addressp);

    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;
    char host_ip[] = ESP32_Bridge_TCP_IP;

    while (1)
    {
        ESP_LOGI(TAG, "connect to ip %s and port %ld", board_data.ip_addressp, board_data.tcp_port);
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(board_data.ip_addressp);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(board_data.tcp_port);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;

        int sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket created, connecting to %s:%ld", board_data.ip_addressp, board_data.tcp_port);
        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0)
        {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Successfully connected");
        while (1)
        {
            struct sockaddr_in source_addr;
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

            if (len < 0) // Check for errors
            {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                if (errno == EINTR) // Interrupted by a signal before any data was available
                {
                    continue; // Optionally handle or retry
                }
                break; // Other errors, break from the loop
            }
            else if (len == 0) // Received an empty packet
            {
                // //ESP_LOGI(TAG, "Received an empty packet, possibly due to a connection issue or sender behavior.");
                continue; // Handle according to your application's needs
            }
            else if (len > UART_MAX_BUFFER_SIZE)
            {
                ESP_LOGE(TAG, "recvfrom failed: overflow len %d", len);
            }
            else // Data received
            {
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                ESP_LOGI(TAG, "Received %d bytes from %s", len, inet_ntoa(source_addr.sin_addr));
                if (tcp_messge_call_back != NULL)
                    tcp_messge_call_back(rx_buffer, len);
                // //ESP_LOGI(TAG, "%s", rx_buffer);
            }

            vTaskDelay(10 / portTICK_PERIOD_MS); // Simple delay between read operations
        }

        if (sock != -1)
        {
            ESP_LOGI(TAG, "Shutting down socket...");
            shutdown(sock, 0);
            close(sock);
        }
    }

    vTaskDelete(NULL); // Delete this task if we exit from the loop above
}

// ... Rest of your code remains unchanged
