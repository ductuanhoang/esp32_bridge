#ifndef DEVICE_DATA_H
#define DEVICE_DATA_H
#ifdef __cplusplus
extern "C"
{
#endif
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ESP32_BRIDGE_TCP_PORT 26001
#define ESP32_BRIDGE_UDP_PORT 26002
#define UART_MAX_BUFFER_SIZE 512

#define ESP_UART_RX_PIN (17)
#define ESP_UART_TX_PIN (10)
    /**
     * @brief
     *
     */
    typedef enum
    {
        E_BLUETOOTH = 1,
        E_TCP,
        E_UDP,
        E_SERIAL_UART,
        E_CAN_BUS
    } e_protocol_type;

    typedef struct
    {
        e_protocol_type input;
        e_protocol_type output;
        char ip_addressp[32];
        uint16_t port;
    } board_info_t;

    typedef struct
    {
        char firmware_version[10];
        char esp32_firmware_version[15];
        char esp32_time_built[50];
        uint8_t status_connection;
        uint8_t status_update_firmware;
        uint8_t update_percent;
    } board_status_t;

    extern board_info_t board_data;
    extern board_status_t board_status;
#ifdef __cplusplus
}
#endif

#endif // DEVICE_DATA_H
