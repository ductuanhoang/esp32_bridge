
/**
 * @file simulation.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2024-02-19
 *
 * @copyright Copyright (c) 2024
 *
 */
/***********************************************************************************************************************
 * Pragma directive
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes <System Includes>
 ***********************************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "device_data.h"

#include "simulation.h"
#include "simulation_data.h"
#include "Bluetooth.h"
#include "TCP.h"
#include "UDP.h"
#include "TCP_Server.h"
/***********************************************************************************************************************
 * Macro definitions
 ***********************************************************************************************************************/
#define TAG "SIMULATION"
/***********************************************************************************************************************
 * Typedef definitions
 ***********************************************************************************************************************/
/**
 * @brief struct common interface
 *
 */
typedef struct
{
    void (*send_u8)(uint8_t *, size_t);
    void (*send_str)(std::string);
    void (*init)(void);
} simulation_connection_t;
/***********************************************************************************************************************
 * Private global variables and functions
 ***********************************************************************************************************************/
static TaskHandle_t xSimulationSendHandle;
static simulation_connection_t simulation_connection;
static void simulation_send_task(void *ctx);
static void simulation_uart_send(uint8_t *message, size_t len);

/***********************************************************************************************************************
 * Exported global variables and functions (to be accessed by other files)
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Imported global variables and functions (from other files)
 ***********************************************************************************************************************/

/**
 * @brief
 *
 * @param p
 * @return uint8_t
 */
void simulation_start(void)
{
    // check output
    if (board_data.simulation_info.protocol == E_SERIAL_UART)
    {
        ESP_LOGI(TAG, "config output with E_SERIAL_UART");
        simulation_connection.send_u8 = simulation_uart_send;
    }
    if (board_data.simulation_info.protocol == E_BLUETOOTH)
    {
        ESP_LOGI(TAG, "config output with E_BLUETOOTH");
        if (board_data.input != E_BLUETOOTH && board_data.output != E_BLUETOOTH)
            Bluetooth_init();
        simulation_connection.send_u8 = BluetoothSendMessage;
        // send via BLUETOOTH
    }
    if (board_data.simulation_info.protocol == E_CAN_BUS)
    {
        ESP_LOGI(TAG, "config output with E_CAN_BUS");
        // send via can bus
    }
    else if (board_data.simulation_info.protocol == E_UDP)
    {
        ESP_LOGI(TAG, "config output with E_UDP");
        // wait_for_ip();
        if (board_data.input != E_UDP && board_data.output != E_UDP)
            UDP_Init();
        simulation_connection.send_u8 = UDP_Send;
    }
    else if (board_data.simulation_info.protocol == E_TCP)
    {
        ESP_LOGI(TAG, "config output with E_TCP");
        // wait_for_ip();
        if (board_data.input != E_TCP && board_data.output != E_TCP)
            // TCP_Init();
            TCP_Server_Init();
        simulation_connection.send_u8 = TCP_Server_Send;
    }
    xTaskCreate(simulation_send_task, "simulation_send_task", 4096 * 2, NULL, 5, &xSimulationSendHandle);
}

void simulation_stop(void)
{
    ESP_LOGI(TAG, "simulation_stop called");
    if (xSimulationSendHandle != NULL)
    {
        ESP_LOGI(TAG, "simulation_stop called 1");
        vTaskDelete(xSimulationSendHandle);
        xSimulationSendHandle = NULL;
        if (board_data.simulation_info.protocol == E_TCP)
        {
            ESP_LOGI(TAG, "simulation_stop called 2");
            /* code */
            TCP_Server_Destroy();
        }

    }
}

typedef struct
{
    char *buffer;
    uint16_t len;
} simulation_buffer_t;
/***********************************************************************************************************************
 * static functions
 ***********************************************************************************************************************/
static void simulation_send_task(void *ctx)
{
    while (1)
    {
        simulation_buffer_t simulation_buffer;
        // snprintf(simulation_buffer.buffer, "%s", "tuan123");
        simulation_buffer.buffer = (char *)simulation_data;
        simulation_buffer.len = sizeof(simulation_data) / sizeof(uint8_t);
        // update data to view log contents
        sprintf((char *)board_data.message, "%s\r\n", (char *)simulation_buffer.buffer);
        board_data.new_event = 1;
        ESP_LOGI(TAG, "simulation_send_task called sends: %s", simulation_buffer.buffer);
        // send simulation data use protocol simulation input
        if (board_data.simulation_info.protocol == E_BLUETOOTH)
        {
            if (simulation_connection.send_u8 != NULL)
                simulation_connection.send_u8((uint8_t *)simulation_buffer.buffer, (uint16_t)simulation_buffer.len);
        }
        else if (board_data.simulation_info.protocol == E_CAN_BUS)
        {
            if (simulation_connection.send_u8 != NULL)
                simulation_connection.send_u8((uint8_t *)simulation_buffer.buffer, (uint16_t)simulation_buffer.len);
        }
        else if (board_data.simulation_info.protocol == E_SERIAL_UART)
        {
            if (simulation_connection.send_u8 != NULL)
                simulation_connection.send_u8((uint8_t *)simulation_buffer.buffer, (uint16_t)simulation_buffer.len);
        }
        else if (board_data.simulation_info.protocol == E_UDP)
        {
            if (simulation_connection.send_u8 != NULL)
                simulation_connection.send_u8((uint8_t *)simulation_buffer.buffer, (uint16_t)simulation_buffer.len);
        }
        else if (board_data.simulation_info.protocol == E_TCP)
        {
            if (simulation_connection.send_u8 != NULL)
                simulation_connection.send_u8((uint8_t *)simulation_buffer.buffer, (uint16_t)simulation_buffer.len);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

/**
 * @brief
 *
 * @param message
 * @param len
 */
static void simulation_uart_send(uint8_t *message, size_t len)
{
    // ESP_LOG_BUFFER_HEXDUMP(TAG, (const char *)message, len, ESP_LOG_INFO);
    uart_write_bytes(UART_NUM_1, message, len);
}
/***********************************************************************************************************************
 * End of file
 ***********************************************************************************************************************/