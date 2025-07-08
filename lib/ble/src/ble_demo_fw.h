#pragma once

/*
THIS IS A SPECIAL MODE IN WHERE THE DEVICE WILL BE FOLLOWING ORDERS FROM BLE
DEVICE WILl UPDATE THE VARS JUST WITH THE RELEVANT INFO 

//THIS WILL INTERRUPT ALL OTHER TASKS 


IMU VARS INIT IMMEDIATELY AND ALWAYS UPDATING

LEDS REACTIVE TO ORDERS AND ALWAYS ACTIVE

NON-CRITICAL loop will run once a second

updated vars on Non-Critical loop
and will be displayed on the dev_oled after refresh

bat_voltage
bat_percent
bat_c_rate
board_temp
bool usb_connected
bool charging
bool low_bat
bool overheat
bool dark   <-Lux Threshold
bool moving <-Device is shaking 


ON the callback we will need to refresh the following variables

VARS CREATED FOR THE APP TO MODIFY 

----FOR LEDS------------

char ble_led_color
r Red
g Green
b Blue
y Yellow
p Purple
w White
o Orange


ble_led_mode
0 solid
1 blinking 1 second
2 fading
3 pong

int ble_led_brightness
from 0 to 255

----FOR BUZZER----------

int ble_buzzer_mode
0 Off
1 Single Beep - Lock On on Loud Mode
2 Double Beep - Lock Off off on Loud Mode
3 Alarm


--TO DO 

---FOR ALARM------

int ble_alarm_mode
0 disable alarm
1 silent alarm enabled
2 loud alarm enabled

Talk to see if we implement a locked demo mode in where the device will
beep and notify the app if moved (loud mode) 
or will just send notification (silent mode)

*/


//For stopping the tasks from other fns.
extern TaskHandle_t task_ble_app_demo_handle;
void create_task_ble_app_demo();
void ble_app_demo(void * parameters);


//For Handling the LEDS
extern TaskHandle_t task_ble_led_mode_handle ;
void create_task_ble_led_mode();
void ble_led_mode_task(void * parameters);
extern bool running_task_ble_led_mode;

//For Handling the buzzer
extern TaskHandle_t task_ble_buzzer_handle ;
void create_task_ble_buzzer();
void ble_buzzer_task(void * parameters);
extern bool running_task_ble_buzzer;


//For Handling the ALARM
extern TaskHandle_t task_ble_alarm_handle ;
void create_task_ble_alarm();
void ble_alarm_task(void * parameters);
extern bool running_task_ble_alarm;


extern char ble_led_color;

extern int ble_led_mode;
extern int ble_led_mode_current;//only one so far used by ext. fn.

extern int ble_led_brightness;

extern int ble_buzzer_mode;

extern int ble_alarm_mode;

extern bool callback_received;

