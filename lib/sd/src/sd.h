#pragma once

#include <Arduino.h>

//FOR SD 
#include <FS.h>
#include <SPI.h>
#include <SD.h> 


void create_task_sd();

void task_sd_i2c_declare();

void task_sd_i2c_release();

void task_sd(void * parameters);




bool sd_init();
bool log_timestamp();

void run_black_box();

bool log_timestamp();
// Write to the SD card
void writeFile(fs::FS &fs, const char * path, const char * message);
// Append data to the SD card
void appendFile(fs::FS &fs, const char * path, const char * message);


bool sd_get_log_nr();
bool sd_get_log_temp();

bool sd_get_log_lux();
bool sd_get_log_soc();

enum 
{
    blak_box_no_serial,
    black_box_serial_raw,
    black_box_serial_formatted
};

bool sd_get_log_timestamp_date();
bool sd_get_log_timestamp_time();

void sd_log_jump_line();

void sd_log_add_parameter();

bool sd_get_log_imu_pitch();
bool sd_get_log_imu_roll();
bool sd_get_log_imu_yaw();

bool sd_get_log_imu_acc_x();
bool sd_get_log_imu_acc_y();
bool sd_get_log_imu_acc_z();

bool sd_get_log_mubea_can_motor_power();
bool sd_get_log_mubea_can_motor_rpm();
bool sd_get_log_mubea_can_motor_temp();
bool sd_get_log_mubea_can_gen_power();
bool sd_get_log_mubea_can_assist_level();
bool sd_get_log_mubea_can_soc();
bool sd_get_log_mubea_can_soh();
bool sd_get_log_mubea_can_power();
bool sd_get_log_mubea_can_voltage();
bool sd_get_log_mubea_can_temperature();
bool sd_get_log_mubea_can_speed();
bool sd_get_log_mubea_can_direction();
bool sd_get_log_mubea_can_gear();
bool sd_get_log_mubea_can_mileage();
bool sd_get_log_mubea_can_error_code();
bool sd_get_log_mubea_can_recuperation();

bool sd_get_black_box_update_gap_ms();

int print_sd_log_folder_content() ;

bool ensure_sd_space(int minFreeBytes, bool log_to_serial);// 1 MB = 1024 * 1024 bytes

//based on the mode the function will react
void black_box_serial_print(int given_serial_mode);


void splitFileIntoChunks(const char *originalFilePath, size_t chunkSizeBytes = 3 * 1024 * 1024);
void splitFileIntoChunksIfNeeded(const String &filePath, std::vector<String> &partFiles, size_t maxPartSizeBytes = 3 * 1024 * 1024);