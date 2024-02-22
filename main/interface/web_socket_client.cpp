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

// this file is tcp transport web_socket_client
#include "interface/web_socket_client.h"
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

#include <stdio.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"

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
#define NO_DATA_TIMEOUT_SEC 5

static const char *TAG = "WEB_SOCKET_CLIENT";

// static data_board_type0_t socket_data_board_type0;
static uint8_t ping_to_server = 0;
static TimerHandle_t shutdown_signal_timer;
static SemaphoreHandle_t shutdown_sema;
static esp_websocket_client_handle_t web_socket_client;
static stream_stats_handle_t stream_stats = NULL;
static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void shutdown_signaler(TimerHandle_t xTimer)
{
    //ESP_LOGI(TAG, "No data received for %d seconds, signaling shutdown", NO_DATA_TIMEOUT_SEC);
    xSemaphoreGive(shutdown_sema);
}

static void web_socket_client_task(void *ctx);

static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

#if CONFIG_WEBSOCKET_URI_FROM_STDIN
static void get_string(char *line, size_t size)
{
    int count = 0;
    while (count < size)
    {
        int c = fgetc(stdin);
        if (c == '\n')
        {
            line[count] = '\0';
            break;
        }
        else if (c > 0 && c < 127)
        {
            line[count] = c;
            ++count;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

#endif /* CONFIG_WEBSOCKET_URI_FROM_STDIN */

/**
 * @brief
 *
 */
TaskHandle_t xWebClientHandle;
void web_socket_client_init(void)
{
    xTaskCreate(web_socket_client_task, "web_socket_client_task", 4096 * 2, NULL, TASK_PRIORITY_INTERFACE, NULL);
}


/**
 * @brief 
 * 
 * @param message_send 
 */
void web_socket_client_send_message(std::string message_send)
{
    if (esp_websocket_client_is_connected(web_socket_client))
    {
        //ESP_LOGI(TAG, "webclient tcp: %s", message_send.c_str());
        // //ESP_LOGI(TAG, "This Task send message: %d bytes", uxTaskGetStackHighWaterMark(NULL));
        //  //ESP_LOGI(TAG, "webclient send");
        ping_to_server = 1;
        esp_websocket_client_send_text(web_socket_client, message_send.c_str(), (message_send.length()), portMAX_DELAY);
    }
}

/**
 * @brief
 *
 */
static void web_socket_client_task(void *ctx)
{
    retry_delay_handle_t delay_handle = retry_init(true, 5, 2000, 0);
    char *host;
    uint16_t port = config_get_u16(CONF_ITEM(KEY_CONFIG_SOCKET_CLIENT_PORT));
    esp_websocket_client_config_t websocket_cfg = {
        .reconnect_timeout_ms = 10000, // 10 seconds
        .network_timeout_ms = 10000    // 10 seconds
    };
    const char *ws_template = "ws://%s:%d";
    char buffer_uri[50];
    const TickType_t delay_time = 1000 / portTICK_PERIOD_MS;

    while (1)
    {
        retry_delay(delay_handle);
        wait_for_ip();
        break;
    }

    config_get_str_blob_alloc(CONF_ITEM(KEY_CONFIG_SOCKET_CLIENT_HOST), (void **)&host);
    shutdown_signal_timer = xTimerCreate("Websocket shutdown timer", NO_DATA_TIMEOUT_SEC * 1000 / portTICK_PERIOD_MS,
                                         pdFALSE, NULL, shutdown_signaler);
    shutdown_sema = xSemaphoreCreateBinary();

#if CONFIG_WEBSOCKET_URI_FROM_STDIN
    char line[128];
    //ESP_LOGI(TAG, "Please enter uri of websocket endpoint");
    get_string(line, sizeof(line));
    websocket_cfg.uri = line;
    //ESP_LOGI(TAG, "Endpoint uri: %s\n", line);
#else
    ESP_LOGE(TAG, "---- check host = %s  -- port = %d", host, port);
    sprintf(buffer_uri, ws_template, ESP32_Bridge_TCP_IP, ESP32_BRIDGE_TCP_PORT);
    websocket_cfg.uri = buffer_uri;
    websocket_cfg.transport = WEBSOCKET_TRANSPORT_OVER_TCP;
    websocket_cfg.disable_auto_reconnect = false;
#endif

    //ESP_LOGI(TAG, "Connecting to %s...", websocket_cfg.uri);

    web_socket_client = esp_websocket_client_init(&websocket_cfg);
    esp_websocket_register_events(web_socket_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)web_socket_client);

    esp_websocket_client_start(web_socket_client);
    xTimerStart(shutdown_signal_timer, portMAX_DELAY);
    std::string response_ping = "{\"cmd\":1,\"data\":true}";
    //ESP_LOGI(TAG, "-------This Task uart_handler 03: %d bytes", uxTaskGetStackHighWaterMark(NULL));
    while (1)
    {
        while (esp_websocket_client_is_connected(web_socket_client))
        {
            if (ping_to_server == 1)
            {
                ping_to_server = 0;
            }
            else
            {
                esp_websocket_client_send_text(web_socket_client, response_ping.c_str(), response_ping.length(), portMAX_DELAY);
            }
            vTaskDelay(delay_time);
        }
    }

    xSemaphoreTake(shutdown_sema, portMAX_DELAY);
    esp_websocket_client_close(web_socket_client, portMAX_DELAY);
    //ESP_LOGI(TAG, "Websocket Stopped");
    esp_websocket_client_destroy(web_socket_client);
}

/**
 * @brief
 *
 * @param handler_args
 * @param base
 * @param event_id
 * @param event_data
 */
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id)
    {
    case WEBSOCKET_EVENT_CONNECTED:
        //ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
        // send data {\"cmd\":401,\"con_id\":12131415,\"data\":{\"Game Clock\":{\"val\":\"4:58\",\"vis\":1},\"Shot Timer\":{\"val\":\"27\",\"vis\":1}}}

        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        //ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        log_error_if_nonzero("HTTP status code", data->error_handle.esp_ws_handshake_status_code);
        if (data->error_handle.error_type == WEBSOCKET_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", data->error_handle.esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", data->error_handle.esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", data->error_handle.esp_transport_sock_errno);
        }
        break;
    case WEBSOCKET_EVENT_DATA:
        // //ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
        // //ESP_LOGI(TAG, "Received opcode=%d", data->op_code);
        // if (data->op_code == 0x08 && data->data_len == 2)
        // {
        //     ESP_LOGW(TAG, "Received closed message with code=%d", 256 * data->data_ptr[0] + data->data_ptr[1]);
        // }
        // else
        // {
        //     ESP_LOGW(TAG, "Received=%.*s", data->data_len, (char *)data->data_ptr);
        // }
        // //If received data contains json structure it succeed to parse

        xTimerReset(shutdown_signal_timer, portMAX_DELAY);
        break;
    case WEBSOCKET_EVENT_ERROR:
        //ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
        log_error_if_nonzero("HTTP status code", data->error_handle.esp_ws_handshake_status_code);
        if (data->error_handle.error_type == WEBSOCKET_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", data->error_handle.esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", data->error_handle.esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", data->error_handle.esp_transport_sock_errno);
        }
        break;
    }
}