


#pragma once

#include "ArduinoJson.h"
#include "esp_system.h"
#include "esp_log.h"
#include <map>
#include "esp_random.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstring>
#include <string.h>
#include <string>
#include <vector>
#include "device_data.h"


typedef void (*tcp_server_messge_call_back_t)(uint8_t *, int32_t);

/**
 * @brief Initializes the TCP server.
 * 
 * This function initializes the TCP server for communication.
 */
void TCP_Server_Init(void);


void TCP_Server_Destroy(void);
/**
 * @brief Registers a callback function for TCP server messages.
 * 
 * This function registers a callback function to be called when a TCP server message is received.
 * 
 * @param callback The callback function to be registered.
 */
void TCP_ServerRegisterCallback(tcp_server_messge_call_back_t callback);

/**
 * @brief Sends a TCP server message.
 * 
 * This function sends a TCP server message.
 * 
 * @param message The pointer to the message to be sent.
 * @param len The length of the message.
 */
void TCP_Server_Send(uint8_t *message, size_t len);