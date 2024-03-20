/**
 * @file simulation_data.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2024-02-19
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once
#include <string.h>
#include <stdint.h>

const uint8_t simulation_data[] = {
    "$GNGGA,,,,,,,,,,M,,M,,*48\r\n"
    "$GNVTG,000.0,T,,M,000.00,N,000.00,K*7E\r\n"
    "$GNZDA,,,,,00,00*56\r\n"
    "$PTMSX,0,111,4,1,,25,,454325000,0*6C\r\n"
    "0\r\n"
    "$GNGSA,A,1,,,,,,,,,,,,,99.99,99.99,99.99,3*31\r\n"
    "$GNGSA,A,1,,,,,,,,,,,,,99.99,99.99,99.99,4*36\r\n"
    "$GNGSA,A,1,,,,,,,,,,,,,99.99,99.99,99.99,5*37\r\n"
    "$GPGSV,1,1,00,0*65\r\n"
    "*74\r\n"
    "$GBGSV,1,1,00,0*77\r\n"
    "$GQGSV,1,1,00,0*64\r\n"
    "!AIVDM,1,1,,A,142OI84Oh?R5u:@GGdR5kQLp2@@r,0*2E\r\n\""};
