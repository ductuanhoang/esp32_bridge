#ifndef ESP32_XBEE_WEB_SOCKET_CLIENT_H
#define ESP32_XBEE_WEB_SOCKET_CLIENT_H

#include <string>
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>
#include <string.h>
#include <esp_event_base.h>

void web_socket_client_init(void);

#ifdef __cplusplus
}
#endif

void web_socket_client_send_message(std::string message_send);
#endif //ESP32_XBEE_WEB_SOCKET_CLIENT_H