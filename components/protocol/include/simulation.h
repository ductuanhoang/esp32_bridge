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

#ifndef SIMULATION_PRO_H
#define SIMULATION_PRO_H

#ifdef __cplusplus
extern "C"
{
#endif
#include "esp_system.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

    void simulation_stop(void);

    void simulation_start(void);

#ifdef __cplusplus
}
#endif

#endif // SIMULATION_PRO_H