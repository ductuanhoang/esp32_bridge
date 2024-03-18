/**
 * @file web_socket_client.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-08-02
 *
 * @copyright Copyright (c) 2023
 *
 */

// this file test
#include "interface/user_test_data_com.h"
/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* ESP HTTP Client Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "common_protocol.h"
#include <stdio.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include <driver/uart.h>
#include <driver/gpio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_websocket_client.h"
#include "esp_event.h"
#include <cJSON.h>

#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "freertos/queue.h"

#include <sys/param.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <uart.h>
#include <util.h>
#include <wifi.h>
#include <esp_log.h>
#include <sys/socket.h>

#include <config.h>
#include <retry.h>
#include <stream_stats.h>
#include <tasks.h>
#include <config.h>

#include "esp_transport.h"
#include "esp_transport_tcp.h"
#include "esp_transport_socks_proxy.h"

#include "ArduinoJson.h"
#include "device_data.h"

#include "interface/web_socket_client.h"
#include "Bluetooth.h"
#include "TCP.h"
#include "UDP.h"
#include "simulation.h"
#define TAG "COMMON_PROTOCOL"

/***********************************************************************************************************************
 * Macro definitions
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Typedef definitions
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Private global variables and functions
 ***********************************************************************************************************************/

// <return_type> (*<name_of_pointer>)( <data_type_of_parameters> );

/**
 * @brief struct common interface
 *
 */
typedef struct
{
    void (*send_u8)(uint8_t *, size_t);
    void (*send_str)(std::string);
    void (*init)(void);
} protocol_connection_t;
typedef struct
{
    uint8_t buffer[UART_MAX_BUFFER_SIZE];
    uint16_t len;
    /* data */
} user_common_queue_t;

/***********************************************************************************************************************
 * Exported global variables and functions (to be accessed by other files)
 ***********************************************************************************************************************/
static void common_send_task(void *ctx);

static void free_send_message_report(void);
static void common_uart_handler(void *handler_args, esp_event_base_t base, int32_t length, void *buffer);
static void protocol_udp_tcp_handler(uint8_t *buffer, int32_t length);
static void common_uart_send(uint8_t *message, size_t len);

static protocol_connection_t protocol_connection;
static user_common_queue_t user_buffer_common_queue;

static QueueHandle_t common_queue;
static TaskHandle_t xCommonSendHandle;

static stream_stats_handle_t common_stream_stats = NULL;
/***********************************************************************************************************************
 * Imported global variables and functions (from other files)
 ***********************************************************************************************************************/

/**
 * @brief initialize protocol
 * socket client connection
 * websocket connection
 * uart output connection
 *
 */
void udp_send(std::string message);

/**
 * @brief
 *
 */
void common_protocol_init(void)
{
    uint8_t protocol_uart_enabled = 0;
    uint8_t protocol_bluetooth_enabled = 0;
    uint8_t protocol_udp_enabled = 0;
    uint8_t protocol_tcp_enabled = 0;
    uint8_t protocol_canbus_enabled = 0;

    // check input config uart file
    if (board_data.input == E_SERIAL_UART)
    {
        protocol_uart_enabled = 1;
        ESP_LOGI(TAG, "config input with E_SERIAL_UART");
        uart_register_read_handler(common_uart_handler);
    }
    else if (board_data.input == E_BLUETOOTH)
    {
        protocol_bluetooth_enabled = 1;
        ESP_LOGI(TAG, "config input with E_BLUETOOTH");
        Bluetooth_init();
    }
    else if (board_data.input == E_UDP)
    {
        protocol_udp_enabled = 1;
        ESP_LOGI(TAG, "config input with E_UDP");
        wait_for_ip();
        UDPRegisterCallback(protocol_udp_tcp_handler);
        UDP_Init();
    }
    else if (board_data.input == E_TCP)
    {
        protocol_tcp_enabled = 1;
        ESP_LOGI(TAG, "config input with E_TCP");
        wait_for_ip();
        TCPRegisterCallback(protocol_udp_tcp_handler);
        TCP_Init();
    }
    else if (board_data.input == E_CAN_BUS)
    {
        protocol_canbus_enabled = 1;
        ESP_LOGI(TAG, "config input with input");
    }

    // check output
    if (board_data.output == E_SERIAL_UART)
    {
        ESP_LOGI(TAG, "config output with E_SERIAL_UART");
        protocol_connection.send_u8 = common_uart_send;
    }
    if (board_data.output == E_BLUETOOTH)
    {
        ESP_LOGI(TAG, "config output with E_BLUETOOTH");
        if (protocol_bluetooth_enabled != 1)
            Bluetooth_init();
        protocol_connection.send_u8 = BluetoothSendMessage;
        // send via BLUETOOTH
    }
    if (board_data.output == E_CAN_BUS)
    {
        ESP_LOGI(TAG, "config output with E_CAN_BUS");
        // send via can bus
    }
    else if (board_data.output == E_UDP)
    {
        ESP_LOGI(TAG, "config output with E_UDP");
        wait_for_ip();
        if (protocol_udp_enabled != 1)
            UDP_Init();
        // protocol_connection.send_str = udp_send;
    }
    else if (board_data.output == E_TCP)
    {
        ESP_LOGI(TAG, "config output with E_TCP");
        wait_for_ip();
        if (protocol_tcp_enabled != 1)
            TCP_Init();
        // protocol_connection.send_str = tcp_send;
    }

    common_queue = xQueueCreate(12, sizeof(user_buffer_common_queue));
    if (common_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create queue\n");
    }
    common_stream_stats = stream_stats_new("common_send_task");

    xTaskCreate(common_send_task, "common_send_task", 4096 * 2, NULL, TASK_PRIORITY_INTERFACE, &xCommonSendHandle);
}
/***********************************************************************************************************************
 * static functions
 ***********************************************************************************************************************/

static void common_uart_handler(void *handler_args, esp_event_base_t base, int32_t length, void *buffer)
{
    // ESP_LOGI(TAG, "board_data.input.protocol_name = %d", board_data.input.protocol_name);
    stream_stats_increment(common_stream_stats, 0, length);
    if (board_data.input == E_SERIAL_UART)
    {
        ESP_LOGI(TAG, "recieved with len = %ld", length);
        // int ret = device_manager_check_package((const char *)buffer, length);
        // if (ret == 0)
        {
            // ESP_LOG_BUFFER_HEXDUMP(TAG, (const char *)buffer, length, ESP_LOG_INFO);
            user_buffer_common_queue.len = length;
            for (size_t i = 0; i < length; i++)
            {
                user_buffer_common_queue.buffer[i] = ((uint8_t *)buffer)[i];
            }
            xQueueSend(common_queue, (void *)(&user_buffer_common_queue), (TickType_t)0);
            vTaskResume(xCommonSendHandle);
        }
    }
    else
    {
        ESP_LOGI(TAG, "need to config protocol name");
    }
}

/**
 * @brief
 *
 * @param buffer
 * @param length
 */
static void protocol_udp_tcp_handler(uint8_t *buffer, int32_t length)
{
    // ESP_LOGI(TAG, "protocol_udp_handler %ld bytes ", length);
    stream_stats_increment(common_stream_stats, 0, length);
    if (board_data.input == E_UDP)
    {
        // ESP_LOG_BUFFER_HEXDUMP(TAG, (const char *)buffer, length, ESP_LOG_INFO);
        // int ret = daktronics_push_data_to_buffer((const uint8_t *)buffer, length);
        // if (ret == 0)
        {
            user_buffer_common_queue.len = length;
            for (size_t i = 0; i < length; i++)
            {
                user_buffer_common_queue.buffer[i] = ((uint8_t *)buffer)[i];
            }

            xQueueSend(common_queue, (void *)(&user_buffer_common_queue), (TickType_t)0);
            vTaskResume(xCommonSendHandle);
        }
    }
    else if (board_data.input == E_TCP)
    {
        ESP_LOGI(TAG, "recieved with len = %ld", length);
        // int ret = device_manager_check_package((const char *)buffer, length);
        // if (ret == 0)
        {
            // ESP_LOG_BUFFER_HEXDUMP(TAG, (const char *)buffer, length, ESP_LOG_INFO);
            user_buffer_common_queue.len = length;
            for (size_t i = 0; i < length; i++)
            {
                user_buffer_common_queue.buffer[i] = ((uint8_t *)buffer)[i];
            }
            xQueueSend(common_queue, (void *)(&user_buffer_common_queue), (TickType_t)0);
            vTaskResume(xCommonSendHandle);
        }
    }
}
/**
 * @brief
 *
 * @param ctx
 */
static void common_send_task(void *ctx)
{
    // this task will take care about sends message to the LED Player
    std::string message_send;
    uint8_t *u8_message_send;
    uint16_t len = 0;
    uint8_t first_time_clean_queue = 0;
    while (1)
    {
        /* code */
        if (xQueueReceive(common_queue, &(user_buffer_common_queue), (TickType_t)(5 / portTICK_PERIOD_MS)))
        {
            if( board_data.simulation_info.simulation_start == 0)
            {
                ESP_LOGI(TAG, "number of messages stored in a queue = %d", uxQueueMessagesWaiting(common_queue));
                ESP_LOGI(TAG, "total heap space available = %d", xPortGetFreeHeapSize());
                // check buffer
                ESP_LOGI(TAG, "data received: %s", (uint8_t *)user_buffer_common_queue.buffer);
                sprintf((char *)board_data.message, "%s\r\n", (char *)user_buffer_common_queue.buffer);
                board_data.new_event = 1;
                // receive message and parse it
                if (board_data.output == E_BLUETOOTH)
                {
                    if (protocol_connection.send_u8 != NULL)
                        protocol_connection.send_u8((uint8_t *)user_buffer_common_queue.buffer, (uint16_t)user_buffer_common_queue.len);
                }
                else if (board_data.output == E_CAN_BUS)
                {
                    if (protocol_connection.send_u8 != NULL)
                        protocol_connection.send_u8((uint8_t *)user_buffer_common_queue.buffer, (uint16_t)user_buffer_common_queue.len);
                }
                else if (board_data.output == E_SERIAL_UART)
                {
                    if (protocol_connection.send_u8 != NULL)
                        protocol_connection.send_u8((uint8_t *)user_buffer_common_queue.buffer, (uint16_t)user_buffer_common_queue.len);
                }
                else if (board_data.output == E_UDP)
                {
                    if (protocol_connection.send_u8 != NULL)
                        protocol_connection.send_u8((uint8_t *)user_buffer_common_queue.buffer, (uint16_t)user_buffer_common_queue.len);
                }
                else if (board_data.output == E_TCP)
                {
                    if (protocol_connection.send_u8 != NULL)
                        protocol_connection.send_u8((uint8_t *)user_buffer_common_queue.buffer, (uint16_t)user_buffer_common_queue.len);
                }
                free_send_message_report();
            }
        }
        else
        {
            vTaskSuspend(NULL);
        }
    }
}
/**
 * @brief
 *
 */
static void free_send_message_report(void)
{
}

/**
 * @brief
 *
 * @param message
 * @param len
 */
static void common_uart_send(uint8_t *message, size_t len)
{
    // ESP_LOG_BUFFER_HEXDUMP(TAG, (const char *)message, len, ESP_LOG_INFO);
    uart_write_bytes(UART_NUM_1, message, len);
}

/***********************************************************************************************************************
 * End of file
 ***********************************************************************************************************************/
