#pragma once 




void ota_setup(void);

void ota_loop(void);

void create_task_ota();

void task_ota_i2c_declare();

void task_ota_i2c_release();

void task_ota(void * parameters);

