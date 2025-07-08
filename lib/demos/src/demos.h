#pragma once


/*

There will be five big demos.

BTN1 is to select the number nd interact inside the loop
BT2 will start/stop and comeback to the main menu

Demo 1 , different illumination patterns

Demo 2 , Depending on the inclination the leds will illuminate certain area and change in green , yellow or red

Demo 3 , smart lock 

Demo 4 alarm mode test with vibration

Demo 5 ,, to think still 

*/


void create_task_demo_menu();
void demo_menu(void * parameters);
void task_demo_menu_i2c_declare();
void task_demo_menu_i2c_release();


void create_task_demo_0_rgb_patterns();
void demo_0_rgb_patterns(void * parameters);
void task_demo_0_rgb_patterns_i2c_declare();
void task_demo_0_rgb_patterns_i2c_release();

void create_task_demo_1_lux();
void demo_1_lux(void * parameters);
void task_demo_1_lux_i2c_declare();
void task_demo_1_lux_i2c_release();

void create_task_demo_2_imu_pos();
void demo_2_imu_pos(void * parameters);
void task_demo_2_imu_pos_i2c_declare();
void task_demo_2_imu_pos_i2c_release();


void create_task_demo_3_imu_acc();
void demo_3_imu_acc(void * parameters);
void task_demo_3_imu_acc_i2c_declare();
void task_demo_3_imu_acc_i2c_release();


void create_task_demo_4_lock();
void demo_4_lock(void * parameters);
void task_demo_4_lock_i2c_declare();
void task_demo_4_lock_i2c_release();

void create_task_demo_5_buzzer();
void demo_5_buzzer(void * parameters);
void task_demo_5_buzzer_i2c_declare();
void task_demo_5_buzzer_i2c_release();

void create_task_demo_6_status();
void demo_6_status(void * parameters);
void task_demo_6_status_i2c_declare();
void task_demo_6_status_i2c_release();
