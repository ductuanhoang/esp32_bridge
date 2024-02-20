/**
 * @file UDP.h
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



typedef void (*udp_messge_call_back_t)(uint8_t *, int32_t);


void UDP_Init(void);


void UDPRegisterCallback(udp_messge_call_back_t callback);