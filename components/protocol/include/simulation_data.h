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
    "$GNGGA,,,,,,,,,,M,,M,,*48 \
    $GNVTG,000.0,T,,M,000.00,N,000.00,K*7E\
    $GNZDA,,,,,00,00*56\
    $PTMSX,0,111,4,1,,25,,454325000,0*6C\
    0\
    $GNGSA,A,1,,,,,,,,,,,,,99.99,99.99,99.99,3*31\
    $GNGSA,A,1,,,,,,,,,,,,,99.99,99.99,99.99,4*36\
    $GNGSA,A,1,,,,,,,,,,,,,99.99,99.99,99.99,5*37\
    $GPGSV,1,1,00,0*65\
    *74\
    $GBGSV,1,1,00,0*77\
    $GQGSV,1,1,00,0*64\
    !AIVDM,1,1,,A,142OI84Oh?R5u:@GGdR5kQLp2@@r,0*2E"};