


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


typedef void (*udp_server_messge_call_back_t)(uint8_t *, int32_t);

/**
 * @brief Initializes the UDP server.
 * 
 * This function initializes the UDP server for communication.
 */
void UDP_Server_Init(void);


void UDP_Server_Destroy(void);
/**
 * @brief Registers a callback function for UDP server messages.
 * 
 * This function registers a callback function to be called when a UDP server message is received.
 * 
 * @param callback The callback function to be registered.
 */
void UDP_ServerRegisterCallback(udp_server_messge_call_back_t callback);

/**
 * @brief Sends a UDP server message.
 * 
 * This function sends a UDP server message.
 * 
 * @param message The pointer to the message to be sent.
 * @param len The length of the message.
 */
void UDP_Server_Send(uint8_t *message, size_t len);