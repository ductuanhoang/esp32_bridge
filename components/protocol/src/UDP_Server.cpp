/**
 *
 */

#include "UDP_Server.h"
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "device_data.h"

#define TAG "UDP_Server"

#define TASK_PRIORITY_UDP_SEVER 5

#define UDP_SERVER_PORT 3333
#define UDP_SERVER_CONFIG_EXAMPLE_KEEPALIVE_IDLE 5
#define UDP_SERVER_CONFIG_EXAMPLE_KEEPALIVE_INTERVAL 5
#define UDP_SERVER_CONFIG_EXAMPLE_KEEPALIVE_COUNT 3

static TaskHandle_t udp_server_task_handle = NULL;

static void UDP_Server_Task(void *ctx);
static int sock;

static struct sockaddr_in dest_addr;
uint8_t udp_socket_client_connected = 0;
static udp_server_messge_call_back_t udp_server_messge_call_back;

void UDP_Server_Init(void)
{
    ESP_LOGD(TAG, "UDP_Server called");
    udp_socket_client_connected = 1;
    if (udp_server_task_handle == NULL)
        xTaskCreate(UDP_Server_Task, "UDP_Server_Task", 4096 * 2, NULL, TASK_PRIORITY_UDP_SEVER, &udp_server_task_handle);
}

void UDP_Server_Destroy(void)
{
    udp_socket_client_connected = 0;
    // if (sock > 2)
    // {
    //     shutdown(sock, 0);
    //     close(sock);
    //     // shutdown(listen_sock, 0);
    //     close(listen_sock);
    // }
    // vTaskDelete(udp_server_task_handle);
    // ESP_LOGI(TAG, "UDP_Server_Destroy called done");
    // udp_server_task_handle = NULL;
}
/**
 * @brief Registers a callback function for handling UDP server messages.
 *
 * This function allows you to register a callback function that will be called
 * whenever a message is received by the UDP server.
 *
 * @param callback The callback function to be registered.
 */
void UDP_ServerRegisterCallback(udp_server_messge_call_back_t callback)
{
    if (callback != NULL)
        udp_server_messge_call_back = callback;
}

/**
 * @brief Sends a UDP server message.
 *
 * This function sends a UDP server message.
 *
 * @param message The pointer to the message to be sent.
 * @param len The length of the message.
 */
void UDP_Server_Send(uint8_t *message, size_t len)
{
    int err = sendto(sock, message, len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0)
    {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
    }
}

/**
 * @brief Task function for the UDP server.
 *
 * This function is responsible for handling the UDP server functionality.
 * It runs in a separate task and is executed by the FreeRTOS scheduler.
 *
 * @param ctx The context pointer for the task.
 */
static void UDP_Server_Task(void *ctx)
{

    char tx_buffer[256] = "Message from ESP32 ";
    char addr_str[128];
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;

    dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(ESP32_BRIDGE_UDP_TEST_PORT);
    inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

    sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Socket created");

    int broadcast = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0)
    {
        ESP_LOGE(TAG, "Failed to set broadcast: errno %d", errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    while (1)
    {
        // ESP_LOGI(TAG, "sending message");
        ESP_LOGI(TAG, "Sending message to %s:%d", addr_str, ESP32_BRIDGE_UDP_TEST_PORT);
        int err = sendto(sock, tx_buffer, strlen(tx_buffer), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0)
        {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            break;
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    if (sock != -1)
    {
        ESP_LOGE(TAG, "Shutting down socket and restarting...");
        shutdown(sock, 0);
        close(sock);
    }
    vTaskDelete(NULL);
}