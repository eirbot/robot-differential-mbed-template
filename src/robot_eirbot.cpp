/*
 * Copyright (c) 2025, EIRBOT
 * SPDX-License-Identifier: Apache-2.0
 */

#include "robot_eirbot.h"

// CONTROL LOOP (Mbed THREAD in real time)
#define CONTROL_THREAD_RATE 50ms
#define CONTROL_THREAD_FLAG 0x02
Ticker controlThreadTicker;
EventFlags controlThreadFlag;
Thread controlThread(osPriorityRealtime, OS_STACK_SIZE * 4);

// RBDC class instantiation
sixtron::MobileBaseEirbot *mobile_base;
sixtron::OdometryEirbot *odom;
sixtron::RBDC *rbdc;

const string rbdc_status[RBDC_MAX_STATUS] = {
    "RBDC_standby",
    "RBDC_working",
    "RBDC_done",
    "RBDC_correct_f_angle",
    "RBDC_moving",
    "RBDC_moving_&_angle",
    "RBDC_correct_i_angle",
};

volatile int rbdc_result = sixtron::RBDC_status::RBDC_standby;

/* ######################  GOTO FUNCTIONS   ############################################ */

// Be aware this is a blocking function by default
void robot_goto(float x, float y, float theta, bool blocking, sixtron::RBDC_reference reference) {
    rbdc->setTarget(x, y, theta, reference);
    ThisThread::sleep_for(100ms); // let time for the control loop to update
    if (blocking) {
        while (rbdc_result != sixtron::RBDC_status::RBDC_done) {
            ThisThread::sleep_for(10ms);
        }
    }
}

// Just go to X/Y, no final theta
void robot_goto(float x, float y, bool blocking, sixtron::RBDC_reference reference) {
    rbdc->setTarget(x, y, reference);
    ThisThread::sleep_for(100ms);
    if (blocking) {
        while (rbdc_result != sixtron::RBDC_status::RBDC_done) {
            ThisThread::sleep_for(10ms);
        }
    }
}

// void robot_vector(float x, float y)
// {
//     rbdc->setVector(x, y);
//     ThisThread::sleep_for(100ms);
// }

/* ######################  RBDC CONTROL LOOP   ############################################ */

void controlThreadUpdate() {
    controlThreadFlag.set(CONTROL_THREAD_FLAG);
}

void control() {

    terminal_printf("[RBDC] Init ...\n");

    // Convert current rate of the loop in seconds (float)
    // (todo copper: lossy convertion, I need to update that)
    auto f_secs = std::chrono::duration_cast<std::chrono::duration<float>>(CONTROL_THREAD_RATE);
    float dt_pid = f_secs.count(); // Very important for all PIDs
    float hz_pid = 1.0f / dt_pid;

    // Setup and init odometry (this will init steppers communication)
    // In Eirbot case, for stepper in direct drive, the motor resolution is the same as sensor.
    odom = new sixtron::OdometryEirbot(
            hz_pid, SENSOR_RESOLUTION, (float)SENSOR_RESOLUTION, WHEEL_RADIUS, WHEELS_DISTANCE);
    odom->init();

    // Setup and init the motor base
    mobile_base = new sixtron::MobileBaseEirbot(WHEELS_DISTANCE);
    mobile_base->init();

    // Setup RBDC
    // TODO: update the values !!
    sixtron::RBDC_params rbdc_params;
    rbdc_params.rbdc_format = sixtron::RBDC_format::two_wheels_robot;
    rbdc_params.max_output_dv = 1.0f; // in [m/s]
    rbdc_params.max_output_dtheta = 8.0f; // in [rad/s]
    rbdc_params.can_go_backward = true;
    rbdc_params.dt_seconds = dt_pid;
    rbdc_params.final_theta_precision = DEG_TO_RAD(3);
    rbdc_params.moving_theta_precision = 10 * ONE_DEGREE_IN_RAD;
    rbdc_params.target_precision = 3 * PID_DV_PRECISION;
    rbdc_params.dv_precision = PID_DV_PRECISION;

    // Theta, or angular speed, PID parameters
    // TODO: update the values !!
    rbdc_params.pid_param_dteta.Kp = 1.0f;
    rbdc_params.pid_param_dteta.Ki = 0.0f;
    rbdc_params.pid_param_dteta.Kd = 0.0f;
    rbdc_params.pid_param_dteta.ramp = 1.75f;

    // Dv, or linear speed, PID parameters
    // TODO: update the values !!
    rbdc_params.pid_param_dv.Kp = 2.0f;
    rbdc_params.pid_param_dv.Ki = 0.1f;
    rbdc_params.pid_param_dv.Kd = 0.25f;
    // rbdc_params.pid_param_dv.ramp = 0.5f;

    rbdc = new sixtron::RBDC(odom, mobile_base, rbdc_params);

    // start control loop ticker
    controlThreadTicker.attach(&controlThreadUpdate, CONTROL_THREAD_RATE);
    terminal_printf("[RBDC] Init done.\n");
    float time_passed = 0.0f;

    while (true) {

        // Wait for asserv tick
        controlThreadFlag.wait_any(CONTROL_THREAD_FLAG);

        // Update RBDC (will automatically update odometry, motor base, QEI, motors, PIDs...)
        rbdc_result = rbdc->update();

        // update time counter for next round (for debug purposes)
        time_passed += dt_pid;
    }
}

// MBED STARTING NEW THREAD
void start_eirbot_rbdc_control_thread() {
    controlThread.start(control);
    ThisThread::sleep_for(1s); // wait a bit before continue
}
