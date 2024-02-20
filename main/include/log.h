#ifndef ESP32_XBEE_LOG_H
#define ESP32_XBEE_LOG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdarg.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>

esp_err_t log_init();
int log_vprintf(const char * format, va_list arg);
void *log_receive(size_t *length, TickType_t ticksToWait);
void log_return(void *item);

#ifdef __cplusplus
}
#endif

#endif //ESP32_XBEE_LOG_H
