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

#define BLE_DEVICE_NAME "ESP32_Bridge"
#define NMEA_SERVICE 0x1000
#define NMEA_RMC_READ 0x1001
#define NMEA_RMC_WRITE 0x1002

// 10.33.3.4
// port 8023
#define ESP32_BRIDGE_TCP_PORT 8023
#define ESP32_BRIDGE_UDP_PORT 8023
#define ESP32_Bridge_TCP_IP "10.33.3.4"
#define ESP32_USER_NAME "admin"
#define ESP32_PASSWORD "admin"
#define ESP32_SERIAL_DEFAULT_BAUDRATE 115200
#define UART_MAX_BUFFER_SIZE 1000

#define ESP_UART_RX_PIN (18)
#define ESP_UART_TX_PIN (17)
    /**
     * @brief
     *
     */
    typedef enum
    {
        E_BLUETOOTH = 1,
        E_UDP,
        E_TCP,
        E_SERIAL_UART,
        E_CAN_BUS
    } e_protocol_type;

    typedef struct
    {
        e_protocol_type input;
        e_protocol_type output;
        char *ip_addressp;
        uint32_t tcp_port;
        uint32_t udp_port;
        uint32_t serial_baudrate;
        char *user_name;
        char *password;
        char message[UART_MAX_BUFFER_SIZE+2];
        uint8_t new_event;
        uint8_t first_time;
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

    extern uint8_t uart_message_handle[UART_MAX_BUFFER_SIZE];
    extern uint16_t conn_handle;
    extern bool notify_state;
    extern uint16_t uart_service_handle;
#ifdef __cplusplus
}
#endif

#endif // DEVICE_DATA_H
