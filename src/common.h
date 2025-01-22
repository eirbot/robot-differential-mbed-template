/*
 * Copyright (c) 2025, EIRBOT
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COMMON_H
#define COMMON_H

#define DEBUG_ENABLE 0
#define MAX_PRINTF_LENGTH 100
#include "mbed.h"
#include "string.h"

// TODO: Be aware, that will override any configuration done by PIO
// Set up printf over FTDI
static UnbufferedSerial terminal(CONSOLE_TX, CONSOLE_RX, 115200);

// custom printf
void terminal_printf(const char *fmt, ...);
void terminal_debug(const char *fmt, ...);

#endif
