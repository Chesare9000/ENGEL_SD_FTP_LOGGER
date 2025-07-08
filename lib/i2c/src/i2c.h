#pragma once 

//Generic Useful tool for initializing and utilizing i2c functionalities

void i2c_init(void);

void i2c_scanner_raw(void); //log_enabled must be active to use this tool

void i2c_scanner_with_names(void); //log_enabled must be active to use this tool

//For stopping the tasks from other fns.
extern TaskHandle_t i2c_manager_handle;

void create_task_i2c_manager();

void i2c_manager(void * parameters);





//TODO MAKE A FULL LIST OF RESOURCES THAT NEED THE I2C RAIL

//AS THERE ARE MUTEX PROBLEMS FOR ACCESSING THE INTERFACE AT 

//THE SAME TIME