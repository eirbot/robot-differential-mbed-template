/*
 * Copyright (c) 2025, EIRBOT
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
#include "robot_eirbot.h"

static DigitalIn button(BUTTON1);

// static DigitalOut led1(LED1);

int main() {
    terminal_printf("[MAIN] Init ...\n");

    // disable deep sleep (can cause issues on STM32)
    terminal_printf("Disabling deep sleep\n");
    sleep_manager_lock_deep_sleep();

    // Setup Eirbot RBDC thread
    terminal_printf("Setup EIRBOT robot base control thread for a differential robot\n");
    start_eirbot_rbdc_control_thread();

    terminal_printf("[MAIN] Init Done.\n");

    // Main loop (This is a thread from Mbed point of view)
    while (true) {

        // Do the square indefinitely
        robot_goto(1.0, 0.0);
        robot_goto(1.0, 1.0);
        robot_goto(0.0, 1.0);
        robot_goto(0.0, 0.0);

        // Wait 5 sec and do another square
        ThisThread::sleep_for(5s);
    }
}
