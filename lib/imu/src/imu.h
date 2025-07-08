#pragma once

enum task_imu_status
{
    task_imu_not_needed,
    task_imu_needed,
    task_imu_running,
};

enum imu_log_mode 
{
    imu_log_mode_silent,
    imu_log_mode_moderate,
    imu_log_mode_verbose
};

enum imu_run_mode_enum
{
    imu_run_mode_print_values,
    imu_run_mode_simulation,
    imu_run_mode_silent,
};



void dmpDataReady();

void imu_init();

void imu_demo();

void imu_teapot_demo();
void imu_graph_demo();


void imu_run();


void create_task_imu();
void task_imu_i2c_declare();
void task_imu_i2c_release();
void task_imu(void * parameters);

enum imu_recalibrate_log_handler_enum
{
    imu_recalibrate_log_handler_silent,
    imu_recalibrate_log_handler_moderate,
    imu_recalibrate_log_handler_verbose
};

void recalibrate_imu_via_i2c_manager(int imu_recalibrate_log_handler_mode);

//FOR WEB DEMO check also:
//https://randomnerdtutorials.com/esp32-mpu-6050-web-server/