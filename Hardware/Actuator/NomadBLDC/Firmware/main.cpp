/*
 * main.cpp
 *
 *  Created on: August 21, 2019
 *      Original Work Written By: Ben Katz, with much inspiration from Bayley Wang, 
 *      Nick Kirkby, Shane Colton, David Otten, and others.
 *      Original Documentation at: http://build-its.blogspot.com/
 *      Forked From: https://os.mbed.com/users/benkatz/code/Hobbyking_Cheetah_Compact/
 *      Original License: Unspecified/Unknown
 * 
 *      Modifications By: Quincy Jones
 *
 * Copyright (c) <2019> <Quincy Jones - quincy@implementedrobotics.com/>
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 */

#define VERSION_MAJOR 1
#define VERSION_MINOR 0

#define LED_PIN PC_5

#include "mbed.h"
#include "rtos.h"
#include "Core/MotorController.h"
#include "Core/LEDService.h"
#include "Core/UserMenu.h"
#include "Core/FlashInterface.h"

extern "C"
{
    #include "Core/motor_controller_interface.h"
}

// #include "CAN_com.h"

// ObserverStruct observer;

// CAN can(PB_8, PB_9, 1000000); // CAN Rx pin name, CAN Tx pin name
// CANMessage rxMsg;
// CANMessage txMsg;

volatile int count = 0;
//volatile int state = REST_MODE;
volatile int state_change;

// Serial Handler
Serial serial(PA_2, PA_3);

// Control Thread
Thread control_task(osPriorityRealtime, 2048);

// void onMsgReceived()
// {
//     //msgAvailable = true;
//     printf("%df\n\r", rxMsg.id);
//     can.read(rxMsg);
//     if ((rxMsg.id == CAN_ID))
//     {
//         controller.timeout = 0;
//         if (((rxMsg.data[0] == 0xFF) & (rxMsg.data[1] == 0xFF) & (rxMsg.data[2] == 0xFF) & (rxMsg.data[3] == 0xFF) & (rxMsg.data[4] == 0xFF) & (rxMsg.data[5] == 0xFF) & (rxMsg.data[6] == 0xFF) & (rxMsg.data[7] == 0xFC)))
//         {
//             state = MOTOR_MODE;
//             state_change = 1;
//         }
//         else if (((rxMsg.data[0] == 0xFF) & (rxMsg.data[1] == 0xFF) & (rxMsg.data[2] == 0xFF) & (rxMsg.data[3] == 0xFF) * (rxMsg.data[4] == 0xFF) & (rxMsg.data[5] == 0xFF) & (rxMsg.data[6] == 0xFF) & (rxMsg.data[7] == 0xFD)))
//         {
//             state = REST_MODE;
//             state_change = 1;
//             gpio.led->write(0);
//             ;
//         }
//         else if (((rxMsg.data[0] == 0xFF) & (rxMsg.data[1] == 0xFF) & (rxMsg.data[2] == 0xFF) & (rxMsg.data[3] == 0xFF) * (rxMsg.data[4] == 0xFF) & (rxMsg.data[5] == 0xFF) & (rxMsg.data[6] == 0xFF) & (rxMsg.data[7] == 0xFE)))
//         {
//             spi.ZeroPosition();
//         }
//         else if (state == MOTOR_MODE)
//         {
//             unpack_cmd(rxMsg, &controller);
//         }
//         pack_reply(&txMsg, controller.theta_mech, controller.dtheta_mech, controller.i_q_filt * KT_OUT);
//         can.write(txMsg);
//     }
// }


// void enter_setup_state(void)
// {
//     printf("\n\r\n\r Configuration Options \n\r\n\n");
//     wait_us(10);
//     printf(" %-4s %-31s %-5s %-6s %-5s\n\r\n\r", "prefix", "parameter", "min", "max", "current value");
//     wait_us(10);
//     printf(" %-4s %-31s %-5s %-6s %.1f\n\r", "b", "Current Bandwidth (Hz)", "100", "2000", I_BW);
//     wait_us(10);
//     printf(" %-4s %-31s %-5s %-6s %-5i\n\r", "i", "CAN ID", "0", "127", CAN_ID);
//     wait_us(10);
//     printf(" %-4s %-31s %-5s %-6s %-5i\n\r", "m", "CAN Master ID", "0", "127", CAN_MASTER);
//     wait_us(10);
//     printf(" %-4s %-31s %-5s %-6s %.1f\n\r", "l", "Torque Limit (N-m)", "0.0", "18.0", TORQUE_LIMIT);
//     wait_us(10);
//     printf(" %-4s %-31s %-5s %-6s %d\n\r", "t", "CAN Timeout (cycles)(0 = none)", "0", "100000", CAN_TIMEOUT);
//     wait_us(10);
//     printf("\n\r To change a value, type 'prefix''value''ENTER'\n\r i.e. 'b1000''ENTER'\n\r\n\r");
//     wait_us(10);
//     state_change = 0;
// }

/// Manage state machine with commands from serial terminal or configurator gui ///
/// Called when data received over serial ///
//void serial_interrupt(void)
//{
    // while (pc.readable())
    // {
    //     char c = pc.getc();
    //     if (c == 27)
    //     {
    //         state = REST_MODE;
    //         state_change = 1;
    //         char_count = 0;
    //         cmd_id = 0;
    //         gpio.led->write(0);
    //         ;
    //         for (int i = 0; i < 8; i++)
    //         {
    //             cmd_val[i] = 0;
    //         }
    //     }
    //     if (state == REST_MODE)
    //     {
    //     }
    //     // else if (state == SETUP_MODE)
    //     // {
    //     //     if (c == 13)
    //     //     {
    //     //         switch (cmd_id)
    //     //         {
    //     //         case 'b':
    //     //             I_BW = fmaxf(fminf(atof(cmd_val), 2000.0f), 100.0f);
    //     //             break;
    //     //         case 'i':
    //     //             CAN_ID = atoi(cmd_val);
    //     //             break;
    //     //         case 'm':
    //     //             CAN_MASTER = atoi(cmd_val);
    //     //             break;
    //     //         case 'l':
    //     //             TORQUE_LIMIT = fmaxf(fminf(atof(cmd_val), 18.0f), 0.0f);
    //     //             break;
    //     //         case 't':
    //     //             CAN_TIMEOUT = atoi(cmd_val);
    //     //             break;
    //     //         default:
    //     //             printf("\n\r '%c' Not a valid command prefix\n\r\n\r", cmd_id);
    //     //             break;
    //     //         }

    //     //         if (!prefs.ready())
    //     //             prefs.open();
    //     //         prefs.flush(); // Write new prefs to flash
    //     //         prefs.close();
    //     //         prefs.load();
    //     //         state_change = 1;
    //     //         char_count = 0;
    //     //         cmd_id = 0;
    //     //         for (int i = 0; i < 8; i++)
    //     //         {
    //     //             cmd_val[i] = 0;
    //     //         }
    //     //     }
    //     //     else
    //     //     {
    //     //         if (char_count == 0)
    //     //         {
    //     //             cmd_id = c;
    //     //         }
    //     //         else
    //     //         {
    //     //             cmd_val[char_count - 1] = c;
    //     //         }
    //     //         pc.putc(c);
    //     //         char_count++;
    //     //     }
    //     // }
    //     else if (state == ENCODER_MODE)
    //     {
    //         switch (c)
    //         {
    //         case 27:
    //             state = REST_MODE;
    //             state_change = 1;
    //             break;
    //         }
    //     }
    //     // else if (state == MOTOR_MODE)
    //     // {
    //     //     switch (c)
    //     //     {
    //     //     case 'd':
    //     //         controller.i_q_ref = 0;
    //     //         controller.i_d_ref = 0;
    //     //     }
    //     // }
    // }
//}

int main()
{
    // Setup LED
    LEDService::Instance().Init(LED_PIN);
    LEDService::Instance().Off();

    serial.baud(921600); // set serial baud rateSerial
    printf("\n\r\n\r Implemented Robotics - Nomad BLDC v%d.%d Beta\n\r", VERSION_MAJOR, VERSION_MINOR);

    // Create Menus
    MainMenu *main_menu = new MainMenu("Main Menu", 0x27, NULL, &enter_idle);

    MainMenu *motor_mode = new MainMenu("Motor Mode", 'm', main_menu, &start_control);
    MainMenu *calibrate_mode = new MainMenu("Calibrate Motor", 'c', main_menu);
    MainMenu *setup_mode = new MainMenu("Controller Setup", 's', main_menu);
    MainMenu *encoder_mode = new MainMenu("Encoder Setup", 'e', main_menu, &enter_idle);
    MainMenu *show_config_mode = new MainMenu("Show Configuration", 'i', main_menu);
    MainMenu *save_mode = new MainMenu("Write Configuration", 'w', main_menu, &save_configuration);
    MainMenu *restart_mode = new MainMenu("Restart System", 'r', main_menu, &reboot_system);

    MainMenu *measure_mode = new MainMenu(" Measure Motor Parameters", 'm', calibrate_mode, &measure_motor_parameters);

    MainMenu *encoder_display_mode = new MainMenu(" Display Encoder Debug", 'd', encoder_mode, &show_encoder_debug);
    MainMenu *encoder_zero_mode = new MainMenu(" Zero Encoder Mechanical Output", 'z', encoder_mode, &zero_encoder_offset);

    MainMenu *motor_config_show = new MainMenu(" Show Motor Configuration", 'm', show_config_mode, &show_motor_config);
    MainMenu *controller_config_show = new MainMenu(" Show Controller Configuration", 'c', show_config_mode, &show_controller_config);
    MainMenu *encoder_config_show = new MainMenu(" Show Encoder Configuration", 'e', show_config_mode, &show_encoder_config);

    NVIC_SetPriority(USART1_IRQn, 3); // Set Interrupt Priorities

    UserMenu *user_menu = new UserMenu(&serial, main_menu);
    user_menu->Show();

    // reset_observer(&observer); // Reset observer

    // NVIC_SetPriority(CAN1_RX0_IRQn, 3);
    //can.filter(CAN_ID << 21, 0xFFE00004, CANFormat::CANStandard, 0);

    // txMsg.id = CAN_MASTER;
    // txMsg.len = 6;
    // rxMsg.len = 8;
    // can.attach(&onMsgReceived); // attach 'CAN receive-complete' interrupt handler

    // // If preferences haven't been user configured yet, set defaults
    // if (isnan(CAN_ID) || CAN_ID == -1)
    // {
    //     CAN_ID = 1;
    // }
    // if (isnan(CAN_MASTER) || CAN_MASTER == -1)
    // {
    //     CAN_MASTER = 0;
    // }
    // if (isnan(CAN_TIMEOUT) || CAN_TIMEOUT == -1)
    // {
    //     CAN_TIMEOUT = 0;
    // }

    // printf(" ADC1 Offset: %d    ADC2 Offset: %d\n\r", controller.adc1_offset, controller.adc2_offset);
    // printf(" Position Sensor Electrical Offset:   %.4f\n\r", E_OFFSET);
    // printf(" Output Zero Position:  %.4f\n\r", M_OFFSET);
    // printf(" CAN ID:  %d\n\r", CAN_ID);

    // while (1)
    // {

    //     if (state == MOTOR_MODE)
    //     {
    //         //printf("%.3f  %.3f  %.3f\n\r", (float)observer.temperature, (float)observer.temperature2, observer.resistance);
    //         printf("%.3f  %.3f  %.3f\n\r", (float)controller.i_q_ref, (float)controller.i_d_ref, controller.t_ff);
    //         printf("%.3f  %.3f  %.3f %.3f %.3f\n\r", controller.v_d, controller.v_q, controller.i_d_filt, controller.i_q_filt, controller.dtheta_elec);
    //         //printf("%.3f\n\r", controller.dtheta_mech);
    //         //printf("%.3f @ %.3f\n\r", controller.v_des - controller.dtheta_mech, controller.i_q_filt);
    //         printf("%.3f @ %.3f\n\r", controller.i_d_filt, controller.i_q_filt);
    //         printf("Mech Theta: %.3f\n\r", controller.theta_mech);
    //         printf("Elec Offset: %.3f\n\r", controller.theta_elec);
    //         printf("U: %.3f  V: %.3f  W: %.3f\n\r", controller.v_u,controller.v_v,controller.v_w);
    //         wait(.002);
    //     }
    // }

    control_task.start(motor_controller_thread_entry);

    // TODO: Idle Task
    
}
