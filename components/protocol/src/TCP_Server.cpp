/**
 * @file TCP_Server.cpp
 * @brief Implementation file for the TCP server component.
 *
 * This file contains the implementation of the TCP server component, which is responsible for handling TCP server functionality.
 * It includes functions for initializing the TCP server, registering a callback function for handling server messages, sending server messages, and the task function for the TCP server.
 *
 * @version 0.1
 * @date 2024-02-22
 *
 * @author your name
 * @email you@domain.com
 *
 * @see TCP_Server.h
 */

#include "TCP_Server.h"
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "device_data.h"

#define TAG "TCP_Server"

#define TASK_PRIORITY_TCP_SEVER 5

#define TCP_SERVER_CONFIG_EXAMPLE_KEEPALIVE_IDLE 5
#define TCP_SERVER_CONFIG_EXAMPLE_KEEPALIVE_INTERVAL 5
#define TCP_SERVER_CONFIG_EXAMPLE_KEEPALIVE_COUNT 3

static TaskHandle_t tcp_server_task_handle = NULL;

static void TCP_Server_Task(void *ctx);
static int sock;
int listen_sock;
static struct sockaddr_in dest_addr;
uint8_t tcp_socket_client_connected = 0;
static tcp_server_messge_call_back_t tcp_server_messge_call_back;
void TCP_Server_Init(void)
{
    ESP_LOGD(TAG, "TCP_Server called");
    tcp_socket_client_connected = 1;
    if (tcp_server_task_handle == NULL)
        xTaskCreate(TCP_Server_Task, "TCP_Server_Task", 4096 * 2, NULL, 5, &tcp_server_task_handle);
}

void TCP_Server_Destroy(void)
{
    ESP_LOGI(TAG, "TCP_Server_Destroy called: sock = %d, listen_sock = %d", sock, listen_sock);
    tcp_socket_client_connected = 0;
    // if (sock > 2)
    // {
    //     shutdown(sock, 0);
    //     close(sock);
    //     // shutdown(listen_sock, 0);
    //     close(listen_sock);
    // }
    // vTaskDelete(tcp_server_task_handle);
    // ESP_LOGI(TAG, "TCP_Server_Destroy called done");
    // tcp_server_task_handle = NULL;
}
/**
 * @brief Registers a callback function for handling TCP server messages.
 *
 * This function allows you to register a callback function that will be called
 * whenever a message is received by the TCP server.
 *
 * @param callback The callback function to be registered.
 */
void TCP_ServerRegisterCallback(tcp_server_messge_call_back_t callback)
{
    if (callback != NULL)
        tcp_server_messge_call_back = callback;
}

/**
 * @brief Sends a TCP server message.
 *
 * This function sends a TCP server message.
 *
 * @param message The pointer to the message to be sent.
 * @param len The length of the message.
 */
void TCP_Server_Send(uint8_t *message, size_t len)
{
    ESP_LOGI(TAG, "TCP Server send called");
    if (sock < 0)
    {
        ESP_LOGE(TAG, "Socket not created");
        return;
    }
    if (tcp_socket_client_connected == 0)
        return;

    uint16_t to_write = len;
    while (to_write > 0)
    {
        int written = send(sock, message + (len - to_write), to_write, 0);
        if (written < 0)
        {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            // Failed to retransmit, giving up
            return;
        }
        to_write -= written;
    }
}

/**
 * @brief Task function for the TCP server.
 *
 * This function is responsible for handling the TCP server functionality.
 * It runs in a separate task and is executed by the FreeRTOS scheduler.
 *
 * @param ctx The context pointer for the task.
 */
static void TCP_Server_Task(void *ctx)
{
    ESP_LOGI(TAG, "TCP_Server_Task called");

    char addr_str[128];

    int ip_protocol = 0;
    int keepAlive = 1;
    int keepIdle = TCP_SERVER_CONFIG_EXAMPLE_KEEPALIVE_IDLE;
    int keepInterval = TCP_SERVER_CONFIG_EXAMPLE_KEEPALIVE_INTERVAL;
    int keepCount = TCP_SERVER_CONFIG_EXAMPLE_KEEPALIVE_COUNT;
    struct sockaddr_storage dest_addr;
    int addr_family = AF_INET;

    while (1)
    {
        // config IP v4
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(ESP32_BRIDGE_TCP_TEST_PORT);
        ip_protocol = IPPROTO_IP;

        listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (listen_sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        }
        int opt = 1;
        setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#if defined(CONFIG_EXAMPLE_IPV4) && defined(CONFIG_EXAMPLE_IPV6)
        // Note that by default IPV6 binds to both protocols, it is must be disabled
        // if both protocols used at the same time (used in CI)
        setsockopt(listen_sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
#endif
        ESP_LOGI(TAG, "Socket created");

        int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0)
        {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
            ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
            goto CLEAN_UP;
        }
        ESP_LOGI(TAG, "Socket bound, port %d", ESP32_BRIDGE_TCP_TEST_PORT);

        err = listen(listen_sock, 1);
        if (err != 0)
        {
            ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
            goto CLEAN_UP;
        }
        while (1)
        {

            ESP_LOGI(TAG, "Socket listening");

            struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
            socklen_t addr_len = sizeof(source_addr);
            sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
            if (sock < 0)
            {
                ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
                break;
            }

            // Set tcp keepalive option
            setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
            setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
            setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
            setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
            // Convert ip address to string
            if (source_addr.ss_family == PF_INET)
            {
                inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
            }

            ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);
            tcp_socket_client_connected = 1;

            // do_retransmit(sock);
        }
    CLEAN_UP:
        ESP_LOGI(TAG, "CLEANUP");
        close(listen_sock);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}