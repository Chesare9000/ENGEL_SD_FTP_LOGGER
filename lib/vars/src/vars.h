#pragma once 

#include <Arduino.h>
#include <Wire.h>

//To share firebase <> Firestore
//#include <Firebase_ESP_Client.h>

#include <map>
#include <string>
#include <cassert>
#include <memory>
#include <mutex>

// Define the modem type before including TinyGsmClient.h
#define TINY_GSM_MODEM_SIM7080
#include <TinyGsmClient.h>//Firebase over LTE
//#include <FirebaseClient.h>
#include <HardwareSerial.h>

//TODO,implement hardware versioning here

//Defining Pin for AnalogRead
#define hw_version_pin 4

extern int hw_version;

//Defining HW VARIANT through HW_ID

extern bool hw_id_0;
extern bool hw_id_1;
extern bool hw_id_2;
extern bool hw_id_3;

enum hw_variants
{
	hw_variant_lite,   
	hw_variant_lora,   
	hw_variant_lte_gps,
	hw_variant_pro,    
	hw_variant_devel,   
};
extern int hw_variant;

//Global Variables to be used by everyone

//TODO later make nice sections


//ESP32-Related GPIOs--------------------------------

//This ones remain constant for all HW_Versions

#define esp_sda_pin 21
#define esp_scl_pin 22

#define esp_built_in_led_pin 2

#define esp_imu_int_pin 27

#define esp_rgb_data_pin 13

#define esp_btn_1_pin 25
#define esp_btn_2_pin 26

#define esp_lora_DIO_0_pin 34

#define esp_mov_switch_pin 35

#define esp_v_mosi 37
#define esp_v_miso 31
#define esp_v_clk 30
#define esp_v_cs 29


//HW_Version-Related ESP32_GPIOs 
//handled on gpio_init() via get_hw_version()

//NEW
extern int esp_int_gpio_exp_pin;//Added on V4
extern int esp_reg_5v_en_pin;   // Moved from EXP_P04 on >V4
//OLD
extern int esp_buzzer_pin;      //<- Moved to 33 on V4
extern int esp_lora_vcc_en_pin; //<- Moved to FF2 on V4
extern int esp_nrf_reset_pin;   //<- Moved to EXP_P13 on V4
extern int esp_lora_reset_pin;  //<- Moved to EXP_P11 on V4
extern int esp_int_charger_pin; //<- Moved to EXP_P05 on V4


//GPIO_EXP Expected Inputs States-------------

extern bool int_usb;
extern bool int_charger;
extern bool int_rtc;
extern bool int_low_bat;
extern bool int_temp;
extern bool int_lux;


//GPIO EXP Derived Flags----------------------

extern bool debug_mode;
extern bool usb_connected;
extern bool charging;
extern bool rtc_alarm;
extern bool low_bat;
extern bool overheat;
extern bool dark;

extern bool gpio_exp_changed;

//I2C Adresses--------------------------------

static byte lux_sens_addr_hex = 0x29;
static byte temp_sens_addr_hex = 0x49;
static byte rtc_addr_hex = 0x6F;
static byte gpio_exp_addr_hex = 0x74;
static byte imu_addr_hex = 0x68;
static byte oled_addr_hex = 0x3C;
static byte fuel_gauge_addr_hex = 0x36;
static byte gps_q_lc29h_addr_hex = 0x50;//For the Quectel LC29H


//Flags --------------------------------------

extern bool log_enabled;
extern bool oled_enabled; 
extern bool oled_initialized;
extern bool i2c_initialized;
extern bool serial_initialized;
extern bool gpios_initialized;
extern bool lux_initialized;
extern bool reg_5v_enabled;
extern bool lvl_shftr_enabled;
extern bool ff2_q_status;
extern bool imu_initialized;
extern bool rtc_initialized;
extern bool rtc_calibrated;
extern bool ble_initialized;
extern bool interrupts_initialized;


extern bool i2c_manager_running ;

extern int  wifi_status;
extern bool wifi_connected;
extern bool wifi_has_credentials;


//When Creating and Destroying Tasks we have to specify
//Which I2C peripherals we want to call 
//Under the Task Creation a list of needed I2C_devs will always be found

//I2c Bus Vars
extern int imu_needed;
extern int rgb_needed;
extern int temp_needed;
extern int lux_needed;
extern int rtc_needed;
extern int fuel_gauge_needed;
extern int oled_needed;

extern bool rgb_bypassed;

extern bool imu_running;
extern bool rgb_running;
extern bool temp_running;
extern bool lux_running;
extern bool rtc_running;
//extern bool fuelgauge_running;
extern bool oled_running;

//Extra vars for I2C manager
extern bool oled_needs_refresh;

enum 
{
	oled_free,
	oled_taken
};

extern int oled_token;

//RGB Related------------------------------ 

#define rgb_leds_total 5

extern bool rgb_leds_enabled;
extern bool rgb_leds_initialized;
extern int rgb_leds_brightness; //0-255 brightness scale 

//Misc. Variables-----------------------------

extern int board_temp;
extern float board_temp_max_threshold;
extern float board_temp_min_threshold;

extern int lux_val;
extern int lux_low_threshold;
extern int lux_int_persistance_counts;

//TODO maybe led_current_brightness?

extern int demos_total;


//FOR RTC
extern int second;
extern int minute;
extern int hour;
extern int day;
extern int month;
extern int year;


//extern int imu_mode;

extern bool lora_enabled;
extern int engel_lora_id;
extern int engel_lora_status;
extern int engel_bat_soc;
extern int engel_last_message_since_seconds;

extern double sd_space_total_bytes;
extern double sd_space_used_bytes;
extern double sd_space_left_bytes;

extern double sd_space_total_mb;
extern double sd_space_used_mb;
extern double sd_space_available_mb;

extern float sd_space_used_percent; 

extern int sd_space_used_percent_int; //Percentage of space used on the SD card
extern int sd_space_total_gb_int; //Total space on the SD card in GB


//TODO maybe move all vars into nice generic structures like this
struct button 
{
	const uint8_t pin;
	uint8_t number_of_push;
	bool is_pressed;
};

extern button btn_1;
extern button btn_2;

extern bool demo_is_ongoing;

extern bool fuel_gauge_initialized;

extern bool charger_initialized;
extern bool charging; //Based on Charging status Pin  
extern bool charger_pgood; //Pin to indicate usb connected with 5V

extern int bat_percent;
extern float bat_voltage;
extern float bat_c_rate;

extern float low_bat_threshold;

extern bool movement_detected;
extern bool moving;

extern int non_critical_task_refresh_ms; 

extern bool black_box_running;


//boot mode 

//Based on that our tasks will change
//0 -> Dev Menu
//1 -> Demo 
//2 -> First Time Config
//3 -> Parking
//4 -> Alarm
//etc....

extern int boot_mode;
extern int current_mode;


//IMU
/*
AVAILABLE OUTPUTS ON IMU IF Vectors are needed
// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
VectorInt16 aa;         // [x, y, z]            accel sensor measurements
VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity;    // [x, y, z]            gravity vector
float euler[3];         // [psi, theta, phi]    Euler angle container
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector
*/

extern int imu_run_mode;

extern int imu_recalibrate_log_handler;

extern TaskHandle_t task_imu_run_handle;

// MPU control/status vars
extern bool dmpReady;  // set true if DMP init was successful
extern float euler[3];         // [psi, theta, phi]    Euler angle container
extern float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector
extern volatile bool mpuInterrupt;     // indicates whether MPU interrupt pin has gone high

extern float imu_yaw ;
extern float imu_pitch ;
extern float imu_roll ;

extern int imu_acc_x ;
extern int imu_acc_y ;
extern int imu_acc_z ;

extern int imu_acc_comp_grav_x ;
extern int imu_acc_comp_grav_y ;
extern int imu_acc_comp_grav_z ;

extern bool imu_new_data;


//BLE 
extern int ble_rgb_pwm;
extern uint32_t ble_beat;
extern int ble_oled_status;
extern bool ble_oled_refresh;

//std::map<std::string, std::shared_ptr<int>> map = {{"temperature", std::make_shared<int>(board_temp)}};

//WIFI

extern int wifi_rgb_led;
extern int wifi_rgb_pwm;
extern int wifi_alarm_status;
extern int wifi_clients;

extern String wifi_ssid;
extern String wifi_pass;

extern String wifi_ssid_default ;
extern String wifi_pass_default ;


extern String wifi_ssid_motionlab;
extern String wifi_pass_motionlab;

extern String wifi_ssid_hq;
extern String wifi_pass_hq;




extern bool new_wifi_info;


extern int wifi_status;




extern bool mqtt_initialized;
extern bool mqtt_connected;

//Related with the mqtt loop running inside wifi()
extern int mqtt_cycle_nr;


// used to prepare the data
// BLE::BLEngelService *engelService->var_mutex;


extern int time_critical_tasks_running;

//NVS Related

extern uint32_t esp_id; //Unique ID for this ESP32 Chip


//extern char firebase_api_key[50];
//extern char firebase_database_url[100];

extern bool firebase_initialized;
extern bool firebase_connected;

extern bool firebase_something_changed;

extern int firebase_cycle_nr;

extern unsigned int esp_boot_count ;

enum backend_config
{
	backend_config_websockets,
	backend_config_mqtt,
	backend_config_firebase,
	backend_config_firestore,
};
extern int backend_config;

//Backend Related vars

//to trigger a refresh on wifi loop
extern bool backend_updated;

extern bool backend_parking_alarm_state;
// 0-> silent , 1 -> loud 
extern bool backend_parking_alarm_mode;

extern bool backend_parking_alarm_movement_detected;
extern bool backend_parking_alarm_triggered;

extern bool backend_parking_alarm_snooze;
extern int  backend_parking_alarm_snooze_time_ms;

extern char backend_led_color;

extern int  backend_led_brightness;

extern bool backend_led_status;

//extern bool firebase_inputs_need_manual_refresh;

extern bool task_firebase_active;

//Callback Related
extern bool last_known_led_status;
extern int  last_known_led_brightness;




//INPUTS
extern bool firebase_leds_status_needs_override; 
extern bool firebase_leds_brightness_needs_override; 
extern bool firebase_leds_color_needs_override; 
extern bool firebase_park_alarm_on_needs_override; 
extern bool firebase_park_alarm_mode_needs_override; 
extern bool firebase_park_alarm_snooze_needs_override;
extern bool firebase_accident_dismissed_needs_override;
extern bool firebase_gps_enabled_needs_override;
extern bool firebase_gps_upload_needs_override;
extern bool firebase_gps_refresh_seconds_needs_override;
extern bool firebase_can_enabled_needs_override;
extern bool firebase_can_upload_needs_override;
extern bool firebase_can_refresh_seconds_needs_override;
extern bool firebase_black_box_enabled_needs_override;
extern bool firebase_black_box_refresh_milliseconds_needs_override;
extern bool firebase_oled_dev_screen_nr_needs_override;


extern int firebase_heartbeat_count;



extern int firebase_oled_last_update;
extern int firebase_oled_update_interval;

extern int firebase_connection_method; 




//Firestore

extern bool firebase_first_loop;

extern bool firestore_initialized;

extern bool firestore_needs_update;
extern bool firestore_update_in_progress;

//extern FirebaseData fbdo;
//extern FirebaseAuth auth;
//extern FirebaseConfig config;

extern char firebase_project_id[50];

extern String hw_variant_string;
extern String backend_led_color_string;

extern TaskHandle_t task_firebase_handle;


//0 parked , 1 riding
extern bool main_status ;

extern int acc_status;
extern int gyro_status;

extern int imu_status;
extern int task_imu_status;


extern bool accident_detected;
extern bool accident_confirmed;
extern bool accident_dismissed;


//Firestore
//Flags
extern bool firestore_update_all;
//OUTPUTS
extern bool firestore_temp_needs_update; 
extern bool firestore_lux_needs_update; 
extern bool firestore_soc_needs_update; 
extern bool firestore_charging_needs_update; 
extern bool firestore_moving_needs_update; 
extern bool firestore_usb_connected_needs_update; 
extern bool firestore_low_bat_needs_update; 
extern bool firestore_main_status_needs_update; 
extern bool firestore_imu_running_needs_update; 
extern bool firestore_accident_detected_needs_update; 
extern bool firestore_accident_confirmed_needs_update; 
extern bool firestore_park_alarm_movement_needs_update; 
extern bool firestore_park_alarm_triggered_needs_update;
extern bool firestore_hw_info_version_needs_update; 
extern bool firestore_hw_info_variant_needs_update; 

extern bool firestore_gps_lat_needs_update;
extern bool firestore_gps_lon_needs_update;
extern bool firestore_gps_kph_needs_update;
extern bool firestore_gps_mph_needs_update;
extern bool firestore_gps_hea_needs_update;
extern bool firestore_gps_alt_needs_update;

extern bool firestore_can_vel_needs_update;
extern bool firestore_can_rpm_needs_update;
extern bool firestore_can_odo_needs_update;
extern bool firestore_can_soc_needs_update;




//INPUTS
extern bool firestore_leds_on_needs_update; 
extern bool firestore_leds_brightness_needs_update; 
extern bool firestore_leds_color_needs_update; 
extern bool firestore_park_alarm_on_needs_update; 
extern bool firestore_park_alarm_mode_needs_update; 
extern bool firestore_park_alarm_snooze_needs_update;
extern bool firestore_accident_dismissed_needs_update;
extern bool firestore_gps_enabled_needs_update;
extern bool firestore_gps_upload_needs_update;
extern bool firestore_gps_refresh_seconds_needs_update;
extern bool firestore_can_enabled_needs_update;
extern bool firestore_can_refresh_seconds_needs_update;


//Model defined on the gps.cpp
extern double gps_latitude, gps_longitude;
extern double gps_speed_kmh, gps_speed_mph; 
extern double gps_heading, gps_altitude;
extern int gps_sat_count, gps_hdop;

extern TaskHandle_t task_gps_handle;

extern bool gps_initialized;
extern bool gps_locked;

extern int gps_retry_nr;
extern int gps_poll_nr;

extern bool gps_is_on;

extern bool gps_data_logging_needs_refresh;


//From Backend
extern int  gps_refresh_seconds_default;
extern int  gps_refresh_seconds;
extern bool gps_enabled;
extern bool gps_upload;
extern bool task_gps_active;



//FOR THE SIMCOM GPS+LTE

extern bool simcom_initialized;
extern bool simcom_gps_initialized;
extern bool simcom_lte_initialized;

extern double simcom_gps_latitude, simcom_gps_longitude;
extern double simcom_gps_speed_kmh, simcom_gps_speed_mph; 
extern double simcom_gps_heading, simcom_gps_altitude;
extern int simcom_gps_sat_count, simcom_gps_hdop;

extern TaskHandle_t task_simcom_gps_handle;

extern bool simcom_gps_locked;

extern int simcom_gps_retry_nr;
extern int simcom_gps_poll_nr;

extern bool simcom_gps_is_on;

extern bool simcom_gps_data_logging_needs_refresh;

//From Backend
extern int simcom_gps_refresh_seconds_default;
extern int  simcom_gps_refresh_seconds;
extern bool simcom_gps_enabled;
extern bool simcom_gps_upload;
extern bool simcom_task_gps_active;



extern int8_t can_vel;
extern int8_t can_rpm;
extern int8_t can_odo;
extern int8_t can_soc;
extern bool can_initialized;
extern bool can_enabled;
extern bool can_upload;
extern int can_refresh_seconds_default;
extern int can_refresh_seconds;
extern bool task_can_active;
extern TaskHandle_t task_can_handle;




//TODO check if this mode can be deprecated
extern int esp_pin_gps_tx;
extern int esp_pin_gps_rx;

extern int esp_pin_sd_v_miso;
extern int esp_pin_sd_v_mosi;
extern int esp_pin_sd_v_clk;
extern int esp_pin_sd_v_cs;

//----------------------------------

extern bool sd_initialized;
extern bool task_sd_active;

extern int black_box_logging_interval_milliseconds_default;
extern int black_box_logging_interval_milliseconds;


//SD black box

//Name of the file
extern String sd_file_name;

//Data Content 
extern String sd_data_header ;
extern String sd_data_payload;

extern uint64_t black_box_log_nr;

//VALUES TO APPEND

extern bool black_box_enabled;
//extern int black_box_refresh_seconds;

extern bool black_box_log_nr_enabled;

extern bool black_box_timestamp_log_enabled;

extern bool black_box_imu_pitch_log_enabled;
extern bool black_box_imu_roll_log_enabled;
extern bool black_box_imu_yaw_log_enabled;

extern bool black_box_imu_acc_x_log_enabled;
extern bool black_box_imu_acc_y_log_enabled;
extern bool black_box_imu_acc_z_log_enabled;

extern bool black_box_soc_log_enabled;
extern bool black_box_lux_log_enabled;
extern bool black_box_temp_log_enabled;

extern bool black_box_mubea_can_motor_power_log_enabled ; 
extern bool black_box_mubea_can_motor_rpm_log_enabled   ; 
extern bool black_box_mubea_can_motor_temp_log_enabled  ;
extern bool black_box_mubea_can_gen_power_log_enabled   ; 
extern bool black_box_mubea_can_assist_level_log_enabled;

extern bool black_box_mubea_can_soc_log_enabled        ;  
extern bool black_box_mubea_can_soh_log_enabled        ;
extern bool black_box_mubea_can_power_log_enabled      ;  
extern bool black_box_mubea_can_voltage_log_enabled    ;
extern bool black_box_mubea_can_temperature_log_enabled;

extern bool black_box_mubea_can_speed_log_enabled      ;
extern bool black_box_mubea_can_direction_log_enabled  ;
extern bool black_box_mubea_can_gear_log_enabled       ;
extern bool black_box_mubea_can_mileage_log_enabled    ;
        
extern bool black_box_mubea_can_error_code_log_enabled  ;
extern bool black_box_mubea_can_recuperation_log_enabled; 

extern bool waiting_for_oled;

//CAN Values from MUBEA (can.cpp)

//Frame 0x777 -> motorGenData
//mubea_can_hex_motor_gen_data;
extern int16_t  mubea_can_motor_power;  
extern int16_t  mubea_can_motor_rpm;  
extern int8_t   mubea_can_motor_temp;  
extern uint16_t mubea_can_gen_power;  
extern uint8_t  mubea_can_assist_level;

//Frame 0x778 -> batteryData
//mubea_can_hex_battery_data;
extern uint8_t  mubea_can_soc;   
extern uint8_t  mubea_can_soh;  
extern int16_t  mubea_can_power; 
extern uint16_t mubea_can_voltage;
extern uint16_t mubea_can_temperature;

//Frame 0x779 -> VEHICLE_PART1
//mubea_can_hex_vehicle_part_1; 
extern uint8_t mubea_can_speed;  
extern int8_t mubea_can_direction;  
extern uint8_t mubea_can_gear;  
extern uint32_t mubea_can_mileage;
        
//Frame 0x77A -> VEHICLE_PART2
//mubea_can_hex_vehicle_part_2;
extern uint16_t mubea_can_error_code;  
extern uint8_t mubea_can_recuperation;  

extern int gps_status; 

extern bool task_mubea_running;

extern SemaphoreHandle_t global_vars_mutex; // Global mutex for shared global vars


extern int oled_dev_screen_nr;
extern int oled_dev_screen_nr_default;
extern int oled_dev_screen_nr_max;
extern bool oled_needs_clear;





extern int task_priority_sd;

//NVS related

extern bool nvs_wifi_credentials_retrieved;


//INTERNET Related
extern int firebase_preferred_internet_connection;
extern int current_internet_connection;

enum firebase_preferred_internet_connection
{
  connection_via_wifi,
  connection_via_lte,
};


//For LTE/FIREBASE

///MUST MATCH WITH THE LTE ONES ON lte.cpp!!!
// Define the Serial2 port for communication with the SIM7080G
// Create HardwareSerial instance for the modem
extern HardwareSerial simcom_serial;
extern TinyGsm modem;
extern TinyGsmClient lte_client; //For Firebase over LTE


extern int power_mode;

enum power_mode
{
  power_mode_continous,
  power_mode_saving,
};

extern int mwr; //minutes without reset

extern int one_minute_in_ms; //A minute in milliseconds

extern bool ota_enabled;

extern bool task_ota_active;

extern int lte_status;

enum lte_status_enum
{
  lte_nvs_info_not_found,
  lte_never_initialized,
  lte_off_due_to_hard_reset,
  lte_off_due_to_deep_sleep,
  lte_on_but_not_working,
  lte_on_and_working
};

extern int minutes_without_moving;
extern int mwm_before_sleep_default;
extern int mwm_before_sleep;

extern int trigger_flip_flop_after_minutes; 

extern bool task_movement_monitor_active ; 
extern bool movement_monitor_enabled;

extern int ms_to_s;

extern int lte_retries_before_esp_reset;
extern int lte_retries_before_esp_reset_default;

extern int lte_retries_before_key_turn;
extern int lte_retries_before_key_turn_default;

extern int lte_signal_quality;

extern int firebase_retries_before_key_turn_and_esp_reset;
extern int firebase_retries_before_key_turn_and_esp_reset_default;

extern int black_box_serial_iterations_gap;
extern int black_box_serial_iterations_gap_default;

extern int black_box_firebase_iterations_gap;
extern int black_box_firebase_iterations_gap_default;




extern bool black_box_need_firebase_update;



extern bool task_logger_sd_ftp_running;

extern char* ota_ssid; 

extern bool oled_dev_mode_enabled;

extern bool inputs_are_missing;

extern bool firebase_file_initialized ;

extern bool task_ftp_wifi_running;

extern char* logger_wifi_ssid;
extern char* logger_wifi_password;

extern bool logger_mode_active;

extern bool task_button_mapper_for_oled_dev_screen_nr_running;