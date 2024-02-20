#ifndef ESP32_XBEE_UART_H
#define ESP32_XBEE_UART_H
#ifdef __cplusplus
extern "C"
{
#endif

#include <esp_event.h>

ESP_EVENT_DECLARE_BASE(UART_EVENT_READ);
ESP_EVENT_DECLARE_BASE(UART_EVENT_WRITE);

#define UART_BUFFER_SIZE 4096

void uart_init();

void uart_inject(void *data, size_t len);
int uart_log(char *buffer, size_t len);
int uart_nmea(const char *fmt, ...);
int uart_write(char *buffer, size_t len);

void uart_register_read_handler(esp_event_handler_t event_handler);
void uart_register_write_handler(esp_event_handler_t event_handler);

void uart_detect_init(void);

void user_uart_wait_for_find_baudrate();
#ifdef __cplusplus
}
#endif

#endif //ESP32_XBEE_UART_H
