#pragma once

/*
enum firestore_log_mode
{
  firestore_log_mode_silent,
  firestore_log_mode_moderate,
  firestore_log_mode_verbose, 
};

void create_task_firestore();
void task_firestore_i2c_declare();
void task_firestore_i2c_release();
void task_firestore(void * parameters);

void check_firestore_request(int firestore_log_mode);




//To update all 
void firestore_update_json_all(firestore_log_mode log_mode);

void format_firestore_path(char* path_buffer, const char* format, int id);
// Function to update Firestore (patch or create document)
void update_firestore(const char* path, FirebaseJson& content, const char* fields,firestore_log_mode log_mode);
// Function to log the Firestore path
void log_firestore_path(const char* path, const char* section,firestore_log_mode log_mode);

//INDIVIDUAL 

//SENSORS---------------------------------------------------------------------------------------------------
//TEMP
bool firestore_update_json_sensors_temp(firestore_log_mode log_mode);
//LUX
bool firestore_update_json_sensors_lux(firestore_log_mode log_mode);
//SOC
bool firestore_update_json_sensors_soc(firestore_log_mode log_mode);

//HW_INFO----------------------------------------------------------------------------------
//VARIANT
bool firestore_update_json_hw_info_variant(firestore_log_mode log_mode);
//VERSION
bool firestore_update_json_hw_info_version(firestore_log_mode log_mode);

//For the LEDs ---------------------------------------------------------------------------------------
//LEDS_ON
bool firestore_update_json_leds_on(firestore_log_mode log_mode);
//LEDS_COLOR
bool firestore_update_json_leds_color(firestore_log_mode log_mode);
//LEDS_BRIGHTNESS
bool firestore_update_json_leds_brightness(firestore_log_mode log_mode);

//For the Park_alarm---------------------------------------------------------------------------------------
//ON
bool firestore_update_json_park_alarm_on(firestore_log_mode log_mode);
//MODE
bool firestore_update_json_park_alarm_mode(firestore_log_mode log_mode);
//MOVEMENT
bool firestore_update_json_park_alarm_movement(firestore_log_mode log_mode);
//TRIGGERED
bool firestore_update_json_park_alarm_triggered(firestore_log_mode log_mode);
//SNOOZE
bool firestore_update_json_park_alarm_snooze(firestore_log_mode log_mode);



//For the Status ---------------------------------------------------------------------------------------
//CHARGING
bool firestore_update_json_status_charging(firestore_log_mode log_mode);
//LOW_BAT
bool firestore_update_json_status_low_bat(firestore_log_mode log_mode);
//MOVING
bool firestore_update_json_status_moving(firestore_log_mode log_mode);
//USB_CONNECTED
bool firestore_update_json_status_usb_connected(firestore_log_mode log_mode);

//For the Accident ---------------------------------------------------------------------------------------
//DETECTED
bool firestore_update_json_accident_detected(firestore_log_mode log_mode);
//CONFIRMED
bool firestore_update_json_accident_confirmed(firestore_log_mode log_mode);
//DISMISSED
bool firestore_update_json_accident_dismissed(firestore_log_mode log_mode);
*/