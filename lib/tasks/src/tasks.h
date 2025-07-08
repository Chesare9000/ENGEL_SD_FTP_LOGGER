#pragma once


/*
Start Demo based on btn2 
Stop Demo based on btn 2
*/




//Tasks can be Invoked and Killed by the handler ID

//Demo tasks -> 10 to 19

void create_task_devel_menu();
void devel_menu(void * parameters);

void task_devel_menu_i2c_declare();
void task_devel_menu_i2c_release();

void create_task_first_time_config();
void first_time_config(void * parameters);


void create_task_ble_first_time_config(); 
void ble_first_time_config(void * parameters);

void ble_init();
void ble_stop_advertising();


void create_task_ble_normal(); 
void ble_normal(void * parameters);
void task_ble_normal_i2c_declare();
void task_ble_normal_i2c_release();

void ble_init();

void non_critical_refresh(int print);
//void create_task_non_critical_task();
//void non_critical_task(void * parameters);


void create_task_parking_alarm();
void task_parking_alarm_i2c_declare();
void task_parking_alarm_i2c_release();
void task_parking_alarm(void * parameters);




void create_task_mubea();
void task_mubea_i2c_declare();
void task_mubea_i2c_release();
void task_mubea(void * parameters);


void create_task_gps();
void task_gps_i2c_declare();
void task_gps_i2c_release();
void task_gps(void * parameters);


void create_task_can();
void task_can_i2c_declare();
void task_can_i2c_release();
void task_can(void * parameters);

