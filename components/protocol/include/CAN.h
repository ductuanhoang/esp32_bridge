/**
 * @file CAN.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2024-02-19
 * 
 * @copyright Copyright (c) 2024
 * 
 */
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



typedef void (*can_messge_call_back_t)(uint8_t *, int32_t);


void CAN_Init(void);


void CANRegisterCallback(can_messge_call_back_t callback);

/**
 * Sends a CAN message.
 *
 * @param message The pointer to the message to be sent.
 * @param len The length of the message.
 */
void CAN_Send(uint8_t *message, size_t len);