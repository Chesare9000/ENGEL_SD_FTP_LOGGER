#pragma once 


void rgb_leds_enable();
void rgb_leds_disable();

void rgb_leds_init();

void rgb_leds_demo_shift(int brightness, int delay_ms);

void rgb_set_led_color(int led_number , char color);

void rgb_leds_on(char color , int brightness);
void rgb_leds_off();


void rgb_led_on(int led_number, char color, int brightness);

void rgb_led_blink_once(int led_number, char color, int brightness, int interval_ms);

void rgb_led_off(int led_number);

//later make this a task
void rgb_leds_blink_once(char color , int brightness, int delay_ms);


/*
mode 
0 no interruptions
1 interruptions based on buttons
2 interruptions based on ble_callback_data
*/
void rgb_leds_fade_once(int mode,char color , 
               int min_brightness, 
               int max_brightness,
               int step_delay_ms );

void rgb_leds_demo_shift(int brightness, int delay_ms);

void rgb_leds_demo_simple_change_colors(int delay_ms);

void rgb_leds_demo_fade_multicolor();

void rgb_led_demo_pong(char color ,int brightness , int delay_ms);

void rgb_led_demo_rotate(char color ,int brightness , int delay_ms);

void rgb_leds_demo_lux(char color,int max_brightness,int lux_min,int lux_max); 

void rgb_leds_demo_imu_pos(int imu_sel_mode,int brightness,char axis,int max_val);


//acc_raw is true and false is acc_compensated
void rgb_leds_demo_imu_acc(int imu_sel_mode,bool acc_raw,int brightness,char axis,int max_val);

//New Fns. (not for demo )
bool rgb_leds_follow_imu_pos_y(int brightness,int max_val, int imu_log_mode);