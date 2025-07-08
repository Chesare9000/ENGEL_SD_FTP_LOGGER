#pragma once 
//These are useful tools at init or runtime invoked by the loop on main for debugging or testing

// Init all I/O pins
void gpios_init(); 

//Replace Delay statement with this little tool to not block the processor
void wait(int time_in_s);

//Little function (not a task)
void built_in_led_blink_once(int time_in_ms); //time in seconds 


void wait_for_btn_1_release();  
void wait_for_btn_2_release();

void init_all();

void check_movement();

void get_hw_version();

void get_esp_id();

void update_hw_variant_string();

void update_backend_led_color_string();

void global_vars_mutex_init();


void create_task_movement_monitor();
void task_movement_monitor_i2c_declare();
void task_movement_monitor_i2c_release();
void task_movement_monitor(void * parameters);
void movement_monitor_poll(int movement_monitor_log_mode);

enum movement_monitor_log_mode
{
    movement_monitor_log_mode_silent,
    movement_monitor_log_mode_moderate,
    movement_monitor_log_mode_verbose
};

//Quickly update the minutes without reset

enum mwr_log_mode
{
  mwr_log_mode_silent,
  mwr_log_mode_moderate,
  mwr_log_mode_verbose,
};

int get_mwr(int mwr_log_mode);


/*
LEDS in descending order 

1 -> BAT_VCC to BAT- (Bat is connected)
2 -> NRF_EN
3 -> BAT_VCC to GND (Babt entered on System Rail)
4 -> USB_VCC
5 -> 3V3_REG
6 -> 5V_REG
7 -> ESP_LED
8 -> LORA_VCC
9 -> MOV_SWITCH_STATUS
10-> MOV_SWITCH
11-> FF1_CLK




Useful Shell Commmands

Set permissions for the port
sudo chmod 777 /dev/ttyUSB0

*/
