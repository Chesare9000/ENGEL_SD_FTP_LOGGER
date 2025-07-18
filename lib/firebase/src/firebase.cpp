

//log_firmware version


//check if needed to make a smaller payload for mubea can


//SIGNAL QUALITY FOR GPS AND LTE



//make order to reset the device on demand


//reset the ota order beore entering


//SPACE USED from 00 to 100%
//SPACE LEFT ON SD 
//SIZE OF THE SD CARD

//LAST COMMUNICATION WITH SERVER


//LOGGING SENSITIVITY AND GPS SENSITIVITY


//For bike if moving after 1 seconds then adjust again the imu if values are weid on the main axis
//meaning it was moving whille tilted , parked


//MUST MATCH WITH lte.cpp
// Define the modem type before including TinyGsmClient.h
#define TINY_GSM_MODEM_SIM7080
#define TINY_GSM_RX_BUFFER 4096
#define TINY_GSM_TX_BUFFER 4096
// Define the serial console for debug prints, if needed
//#define TINY_GSM_DEBUG Serial

#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#define TINY_GSM_USE_GPRS true   //check later
#define TINY_GSM_USE_WIFI false //todo check later for compatibility issues with wifi

//#include <ExampleFunctions.h> // Provides the functions used in the examples.

#include <Arduino.h>

#include "esp_task_wdt.h"

#include <TinyGsmClient.h>
#include <FirebaseJson.h>
#include <FirebaseClient.h>
#include <client/SSLClient/ESP_SSLClient.h>

#include <ArduinoJson.h>
#include <firebase.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
//#include <firestore.h>
#include <vars.h>
#include <tools.h>
#include <interrupts.h>
#include <wifi.h>
#include <oled.h>
#include <tasks.h>
#include <nvs.h>
#include <rgb.h>
#include <imu.h>
#include <buzzer.h>
#include <tasks.h>
#include <can.h>
#include <gps.h>
#include <lux.h>
#include <sd.h>
#include <lte.h>
#include <ota.h>
#include <gpio_exp.h>


bool movement_monitor_enabled = false;

constexpr unsigned int firebase_buffer = 1024;
// Paths for Firebase
String base_path_no_id = "/devices/taillight/";
String base_path_with_id = base_path_no_id; //will be updated
              
int firebase_update_input_interval_seconds_default = 60;
int firebase_update_input_interval_seconds = firebase_update_input_interval_seconds_default;
int firebase_update_output_interval_seconds_default = 30;
int firebase_update_output_interval_seconds = firebase_update_output_interval_seconds_default;

int black_box_offline_trigger = 3;
int black_box_offline_iterations = 0;


int firebase_log_mode = firebase_log_mode_moderate;

bool inputs_are_missing = false;

//TODO These Values must be encapsulated on NVS data
//char firebase_api_key[50] = "";
//char firebase_database_url[100] = "";

int max_brightness = 255;
int min_brightness = 0;

int firebase_delay = 1;

bool display_heap = true;

bool firebase_initialized = false;
bool firebase_connected   = false;

int firebase_retry_count = 0;
unsigned long firebase_retry_timer = 0;

bool firebase_something_changed = false;

bool firebase_input_override_waiting_acknowledge = false ;
int  firebase_input_override_waiting_acknowledge_counter = 0;


String gral_status_default = "on_and_running";
String gral_status = gral_status_default;
//Forcing to upload on state upon connection
String last_known_gral_status = "off_due_to_sleep_on_mwm";

bool last_info_before_shutdown_acknowledged = false;
bool last_info_before_restart_acknowledged = false;
bool last_info_before_ota_acknowledged = false;


//FOR THE VARIABLES THAT NEED TO BE CHECKED JUST ONCE
//WE WILL MAKE SURE THEY SYNC ON BOOT AND THEN NEVER CHECK THEM ANYMORE

//Start as not synced until confirming
bool hw_variant_synced = false;
bool hw_version_synced = false;


//To keep track
unsigned long firebase_heartbeat_last_update = 0;

int firebase_heartbeat_update_interval_seconds_default = 10;
int firebase_heartbeat_update_interval_seconds = firebase_heartbeat_update_interval_seconds_default;

int firebase_heartbeat_count = 0;

bool firebase_was_connected_via_wifi = false;
bool firebase_was_connected_via_lte = false;

//Defaults (TODO : later get them from NVS)
int firebase_preferred_internet_connection_default = connection_via_lte;
int firebase_preferred_internet_connection = firebase_preferred_internet_connection_default;

int firebase_connection_method = -1;    

bool output_json_sent_to_rtdb = false;

bool input_json_requested = false;

bool firebase_test_payload_ack = false; // Reset ACK flag before sending

bool firebase_reset_order = false;  

//OLD
//FirebaseData fbdo;
//FirebaseAuth auth;
//FirebaseConfig config;

#define firebase_user "cesar.gc@outlook.com"
#define firebase_pass "firebase"   
#define firebase_api_key "AIzaSyDPxQ-3--VDEGU37vpG7FdTiweSYohak68"
#define firebase_url "https://engel-dev-61ef3-default-rtdb.europe-west1.firebasedatabase.app/"
#define firebase_url_for_lte "engel-dev-61ef3-default-rtdb.europe-west1.firebasedatabase.app"


///defined first on lte.cpp
//HardwareSerial simcom_serial(2);
//TinyGsm modem(simcom_serial);
//TinyGsmClient lte_client(modem,0); //For Firebase over LTE


//FOR LTE
ESP_SSLClient ssl_client;    


// Authentication
UserAuth user_auth(firebase_api_key, firebase_user, firebase_pass);

// Firebase components
FirebaseApp app;



using AsyncClient = AsyncClientClass;
//WiFiClientSecure ssl_client;
//AsyncClient aClient(ssl_client); //FOR WIFI

//AsyncClient aClient(ssl_client,getNetwork(gsm_network)); //FOR LTE
AsyncClient aClient(ssl_client); //FOR LTE

//TODO , later Switch
RealtimeDatabase Database;

//TODO later declare globals


// User function
void process_data(AsyncResult &aResult);


bool task_firebase_active = false;


char firebase_project_id[50] = "engel-dev-61ef3" ;

int firebase_cycle_nr =0;
int firebase_cycle_last_known =0;



bool backend_led_status = false;
char backend_led_color = 'r';
int  backend_led_brightness = 100;

int led_brightness_min = 0;
int led_brightness_max = 255;

//TODO change this ones to a backend repo together with firebase and firestore

bool backend_parking_alarm_state = false;
// 0-> silent , 1 -> loud 
bool backend_parking_alarm_mode = false;
bool backend_parking_alarm_triggered = false;
bool backend_parking_alarm_movement_detected = false;

//To check if the outputs have changed from the last cycle

//For Sensors
int last_known_temp = board_temp; 
int last_known_lux =  lux_val;
int last_known_soc =  bat_percent;

//Thresholds to avoid refreshing all the time

int firebase_temp_threshold = 10;
int firebase_lux_threshold = 1000;
int firebase_soc_threshold = 1;

//For Status

bool last_known_charging = charging;
bool last_known_moving = moving;
bool last_known_usb_connected = usb_connected;
bool last_known_low_bat = low_bat;
bool last_known_imu_running = imu_running;
//0 parked , 1 riding 
bool main_status = false;
bool last_known_main_status = main_status; 

//LTE
int last_known_lte_signal_quality = 0; //So it uploads at least once
int lte_signal_quality_threshold = 5; //If the signal quality changes more than this, we will update it on firebase


//For Accident
bool accident_detected = false;
bool last_known_accident_detected = accident_detected;
bool accident_confirmed = false;
bool last_known_accident_confirmed = accident_confirmed;
bool accident_dismissed = false; //This is an input managed by both ends , careful
bool backend_accident_dismissed =  false;
//Check logic between the accident dismissed and the backend_accident dismissed on firebase
bool last_know_backend_accident_dismissed = backend_accident_dismissed;

bool last_known_backend_parking_alarm_state = false;

bool backend_parking_alarm_dismissed = false;

//For Parking Alarm (TODO later set this one on NVS) so we dont dismiss upon reboot

bool backend_parking_alarm_snooze = false;
int backend_parking_alarm_snooze_time_ms = 1000;

bool input_json_received = false;
int input_json_received_retry_counter = 0;//TODO later change to an error detectetor to retry right away

bool output_json_ack_received = false;
int output_json_ack_received_retry_counter = 0;


bool last_known_parking_alarm_movement_detected = backend_parking_alarm_movement_detected;
bool last_known_parking_alarm_triggered = backend_parking_alarm_triggered;
bool last_known_parking_alarm_snooze = backend_parking_alarm_snooze;

//HW_INFO and variant will never change during execution


//GPS

float last_known_gps_lat =  gps_latitude;
float last_known_gps_lon =  gps_longitude;
float last_known_gps_kph =  gps_speed_kmh;
float last_known_gps_mph =  gps_speed_mph;
float last_known_gps_hea =  gps_heading;
float last_known_gps_alt =  gps_altitude;
int last_known_gps_sat = gps_sat_count;

bool last_known_gps_enabled = gps_enabled;
bool last_known_gps_upload = gps_upload;
int last_known_gps_refresh_seconds = gps_refresh_seconds;

bool last_known_gps_locked =false;



//Thresholds to avoid spamming the firebase (later modified if moving)
float firebase_gps_lat_threshold = 0.0001;
float firebase_gps_lon_threshold = 0.0001;
float firebase_gps_kph_threshold = 1;
float firebase_gps_mph_threshold = 1;
float firebase_gps_hea_threshold = 1;
float firebase_gps_alt_threshold = 1;
float firebase_gps_hdop_threshold = 10;
float firebase_gps_sat_threshold = 2;

//CAN

int last_known_can_vel = can_vel;
int last_known_can_rpm = can_rpm;
int last_known_can_odo = can_odo;
int last_known_can_soc = can_soc;

bool last_known_can_enabled = can_enabled;
bool last_known_can_upload =  can_upload; //TODO implement this one


int last_known_mwr = mwr;

//every X minutes we will update the mwr on firebase
int firebase_mwr_update_gap_default = 10; 
int firebase_mwr_update_gap = firebase_mwr_update_gap_default; 


//COUNTERS
int last_known_minutes_without_moving = 0;
int firebase_mwm_update_gap_default = 10;
int firebase_mwm_update_gap = firebase_mwm_update_gap_default; //TODO TOMORROW add to inputs


// Retry logic for SSL connection
int lte_retries_before_esp_reset_default = 5;
int lte_retries_before_esp_reset = lte_retries_before_esp_reset_default; // Maximum number of retries before restarting


int firebase_retries_before_key_turn_and_esp_reset_default = 3;
int firebase_retries_before_key_turn_and_esp_reset = firebase_retries_before_key_turn_and_esp_reset_default;




int last_info_before_shutdown_acknowledge_retry_nr = 0;
int last_info_before_shutdown_acknowledge_max_retries = 5 ;

int last_info_before_restart_acknowledge_retry_nr = 0;
int last_info_before_restart_acknowledge_max_retries = 5 ;

int last_info_before_ota_acknowledge_retry_nr = 0;
int last_info_before_ota_acknowledge_max_retries = 5 ;



//Thresholds to avoid spamming the firebase
int firebase_mubea_can_motor_power_threshold = 100;
int firebase_mubea_can_motor_rpm_threshold = 300;
int firebase_mubea_can_motor_temp_threshold = 20;
int firebase_mubea_can_gen_power_threshold = 200;
int firebase_mubea_can_assist_level_threshold = 1;

int firebase_mubea_can_soc_threshold = 5;
int firebase_mubea_can_soh_threshold = 0;
int firebase_mubea_can_power_threshold = 100;
int firebase_mubea_can_voltage_threshold = 100;
int firebase_mubea_can_temperature_threshold = 10;
int firebase_mubea_can_speed_threshold = 10;
int firebase_mubea_can_direction_threshold = 10;
int firebase_mubea_can_gear_threshold = 0;
int firebase_mubea_can_mileage_threshold = 100;
int firebase_mubea_can_error_code_threshold = 0;
int firebase_mubea_can_recuperation_threshold = 1;

int  last_known_can_refresh_seconds = can_refresh_seconds;

// Periodically check for updates
static unsigned long firebase_last_input_update = 0;
static unsigned long firebase_last_output_update = 0;

//In case we need to force a refresh on inputs

//INPUT OVERRIDES
bool firebase_leds_status_needs_override = false; 
bool firebase_leds_brightness_needs_override = false;
bool firebase_leds_color_needs_override = false;
bool firebase_park_alarm_on_needs_override = false;
bool firebase_park_alarm_mode_needs_override = false;
bool firebase_park_alarm_snooze_needs_override = false;
bool firebase_accident_dismissed_needs_override = false;
bool firebase_gps_enabled_needs_override = false;
bool firebase_gps_upload_needs_override = false;
bool firebase_gps_refresh_seconds_needs_override  = false;
bool firebase_can_enabled_needs_override  = false;
bool firebase_can_upload_needs_override = false;
bool firebase_can_refresh_seconds_needs_override = false;
bool firebase_black_box_enabled_needs_override = false;
bool firebase_black_box_refresh_milliseconds_needs_override = false;
bool firebase_oled_dev_screen_nr_needs_override = false;
bool firebase_hw_version_needs_override = false;
bool firebase_hw_variant_needs_override = false;
bool firebase_power_mode_needs_override = false;    
bool firebase_ota_enabled_needs_override = false;
bool firebase_black_box_serial_iterations_gap_needs_override = false;
bool firebase_black_box_firebase_iterations_gap_needs_override = false;


bool firebase_allow_lte_connection = true;
bool firebase_allow_wifi_connection = true;

//New to add
bool firebase_lte_retries_before_esp_reset_needs_override =  false;
bool firebase_lte_retries_before_key_turn_needs_override = false;

bool firebase_update_input_interval_seconds_needs_override = false;
bool firebase_update_output_interval_seconds_needs_override = false;
bool firebase_heartbeat_update_interval_seconds_needs_override = false;
bool firebase_firebase_preferred_internet_connection_needs_override = false;
bool firebase_allow_lte_connection_needs_override = false;
bool firebase_allow_wifi_connection_needs_override = false;

bool firebase_movement_monitor_enabled_needs_override = false;
bool firebase_mwm_before_sleep_needs_override = false;

bool firebase_minutes_without_movement_before_sleep_needs_override = false;
bool firebase_retries_before_key_turn_and_esp_reset_needs_override = false;
bool firebase_preferred_internet_connection_needs_override = false;
bool firebase_mwr_update_gap_needs_override = false;
bool firebase_mwm_update_gap_needs_override = false;

bool firebase_reset_order_needs_override = false;   

int check_signal_quality_interval_mins = 5;  
unsigned long last_signal_quality_check = 0; // Timestamp of the last signal quality check

//SD CARD    
//different than zero so we can always log first time
int last_known_sd_space_total_gb = 1000;
int last_known_sd_space_used_percent_int = 110; 

/*
NEW INPUTS

config/

    lte/ 
        lte_retries_before_esp_reset
        lte_retries_before_key_turn


    firebase /
        update_input_interval_seconds
        update_output_interval_seconds
        int firebase_heartbeat_update_interval_seconds
        firebase_preferred_internet_connection
        allow_lte_connection
        allow_wifi_connection
        firebase_mwr_update_gap
        bool firebase_retries_before_key_turn_and_esp_reset = false;

    power /
        
        movement_monitor_enabled
        mwm_before_sleep

        
NEW OUTPUTS

lte/carrier,data_used,status:(use the defined ones),

counters/ last update (time and date) , mwr(move this one), mwm

status : last_shutdown_reason




*/




bool firebase_first_loop =  false;



//mubea can data
int16_t  last_known_mubea_can_motor_power  = 127;  
int16_t  last_known_mubea_can_motor_rpm    = 127;  
int8_t   last_known_mubea_can_motor_temp   = 127; 
uint16_t last_known_mubea_can_gen_power    = 127;  
uint8_t  last_known_mubea_can_assist_level = 127;

//Frame 0x778 -> batteryData
#define mubea_can_hex_battery_data 0x778
uint8_t  last_known_mubea_can_soc         = 127;   
uint8_t  last_known_mubea_can_soh         = 127;
int16_t  last_known_mubea_can_power       = 127;   
uint16_t last_known_mubea_can_voltage     = 127;
uint16_t last_known_mubea_can_temperature = 127;

//Frame 0x779 -> VEHICLE_PART1
#define mubea_can_hex_vehicle_part_1 0x779 
uint8_t last_known_mubea_can_speed        = 127; 
int8_t last_known_mubea_can_direction     = 127; 
uint8_t last_known_mubea_can_gear         = 127;
uint32_t last_known_mubea_can_mileage     = 127;
        
//Frame 0x77A -> VEHICLE_PART2
#define mubea_can_hex_vehicle_part_2 0x77A
uint16_t last_known_mubea_can_error_code   = 127;
uint8_t last_known_mubea_can_recuperation = 127; 

String received_input_json_string = "" ;
bool input_json_needs_processing = false;


int power_mode_default = power_mode_continous; //default power mode
int power_mode = power_mode_default; //default power mode


void firebase_process_input_json()
{
    //TODO COmment This Out Later 
    //Serial.print("\n---received_input_json_string:");
    //Serial.println(received_input_json_string);

    //Serial.print("\n---Payload length: ");
    //Serial.println(received_input_json_string.length());
    //Serial.println();

    if (received_input_json_string.length() > 2) //Its not empty
    {
        bool some_input_changed = false;
        
        // Parse the JSON string into a FirebaseJson object
        FirebaseJson json;
        json.setJsonData(received_input_json_string);
        // Parse the JSON data using FirebaseJson
        FirebaseJsonData data;

        // Example: Extract all input fields from the JSON

        if (json.get(data,"leds/on"))
        {
            //Check if its different and if yes then update
            if(backend_led_status != data.to<bool>() )
            {
                backend_led_status = data.to<bool>();
                Serial.printf("\nbackend_led_status changed to: %s\n", backend_led_status ? "true" : "false");
                
                if(backend_led_status)
                {
                    Serial.println("\n---Turning ON LEDs via Firebase!");
                    rgb_leds_on(backend_led_color,backend_led_brightness);
                }  
                else
                {
                    Serial.println("\n---Turning OFF LEDs via Firebase!");
                    rgb_leds_off();
                }

                some_input_changed = true;    
            }     
        }
        else 
        {
            Serial.printf("----backend_led_status not found : Updating Node with default values----\n");
            firebase_leds_status_needs_override = true;
            inputs_are_missing = true;
        }

        if (json.get(data, "leds/brightness"))
        {
            if (backend_led_brightness != data.to<int>())
            {
                backend_led_brightness = data.to<int>();

                if      (backend_led_brightness > max_brightness) backend_led_brightness = max_brightness;
                else if (backend_led_brightness < min_brightness) backend_led_brightness = min_brightness;

                Serial.printf("\nbackend_led_brightness changed to: %d\n", backend_led_brightness);

                if(backend_led_status)rgb_leds_on(backend_led_color,backend_led_brightness);

                else { if(log_enabled)Serial.printf("\n ERROR ---LEDS are OFF (backend_leds_status:%d) \n",backend_led_status);}


                some_input_changed = true;    
            }
        }
        else
        {
            Serial.printf("---backend_led_brightness not found: Updating Node with default values----\n");
            firebase_leds_brightness_needs_override = true;
            inputs_are_missing = true;
        }

    
        if (json.get(data, "leds/color"))
        {
            if (backend_led_color_string != data.to<String>())
            {
                backend_led_color_string = data.to<String>();
                

                if     (backend_led_color_string == "red")   backend_led_color   = 'r';
                else if(backend_led_color_string == "green") backend_led_color = 'g';
                else if(backend_led_color_string == "blue")  backend_led_color  = 'b';
                else if(backend_led_color_string == "white") backend_led_color = 'w';
                else
                {
                    Serial.print("\nColor Not Recognized,defaulting to red"); backend_led_color = 'r';

                    backend_led_color_string = "red";

                    firebase_leds_color_needs_override = true;
                    inputs_are_missing = true;
                } 

                Serial.printf("\nbackend_led_color changed to: %s\n", backend_led_color_string);

                if(backend_led_status)rgb_leds_on(backend_led_color,backend_led_brightness);
                else { if(log_enabled)Serial.printf("\n ERROR ---LEDS are OFF (backend_leds_status:%d) \n",backend_led_status);}


                some_input_changed = true;    

                //TODO Make implementation here
            }
        }
        else
        {
            Serial.printf("----backend_led_color not found: Updating Node with default values----\n");
            firebase_leds_color_needs_override = true;
            inputs_are_missing = true;
        }

        if (json.get(data,"park_alarm/on"))
        {
            if (backend_parking_alarm_state != data.to<bool>())
            {
                backend_parking_alarm_state = data.to<bool>();
                Serial.printf("\nbackend_parking_alarm_state changed to: %s\n", backend_parking_alarm_state ? "true" : "false");


                if(backend_parking_alarm_state)
                {
                    //We can activate the alarm JUST if we are in parking mode
                    
                    if (main_status)
                    {
                        if(moving)
                        {
                            Serial.print("\n---ERROR : Cannot Turn on the parking_alarm_while riding! ");
                            Serial.print("\n---Waiting to see if we stop moving soon , otherwise throw an error");

                            wait(5000);
                            
                            if(!moving )//TODO add the imu_checks here as well
                            {
                            main_status = false; 
                            }
                        }

                        else //Forcing change from riding to parked upon request and without having to wait still
                        {
                            Serial.print("\n---Main Status changed to parked upon request");
                            main_status = false; 

                            oled_token = oled_taken;
                            //if(log_enabled)Serial.println("\n---Sending to OLED---> "); 
                            wait(100); 
                            oled_main_status_changed_to_parking();
                            wait(1000);
                            oled_token = oled_free;

                            //TODO later turn off this flag also after waiting for some time
                            task_imu_status = task_imu_not_needed;//turn off the imu and reset all flags
                            wait(500);
                        }               
                    }

                    if(!main_status)
                    {
                        Serial.println("\n---Turning ON ALARM via Firebase");

                        buzzer_one_tone(1000,500,1,1);
                        //Here Start the alarm with the given mode
                        create_task_parking_alarm();
                        //Give chance for the task to be created and run before continuing
                        wait(2000);
                    }
                } 

                else
                {
                    Serial.println("\n---Turning OFF ALARM_ON flag on Firebase");
                    wait(100);
                    //The Alarm will handle all the turning off by itself
                }

                some_input_changed = true;    
            }
        }
        else
        {
            Serial.printf("----backend_parking_alarm_state not found: Updating Node with default values----\n");
            firebase_park_alarm_on_needs_override = true;
            inputs_are_missing = true;
        }

        if (json.get(data,"park_alarm/mode"))
        {
            if (backend_parking_alarm_mode != data.to<bool>())
            {
                backend_parking_alarm_mode = data.to<bool>();
                Serial.printf("\nbackend_parking_alarm_mode changed to: %s\n", backend_parking_alarm_mode ? "true" : "false");
                
                //Displaying Change as there is no command to execute here
                if(backend_parking_alarm_mode)
                {
                    if(log_enabled)Serial.println("\n---Setting Parking Alarm to LOUD Mode via Firebase!");
                }
                else
                {
                if(log_enabled) Serial.println("\n---Setting Parking Alarm to SILENT Mode via Firebase!"); 
                } 

                some_input_changed = true;    
            }
        }
        else
        {
            Serial.printf("----backend_parking_alarm_mode not found: Updating Node with default values----\n");
            firebase_park_alarm_mode_needs_override = true;
            inputs_are_missing = true;
        }

        if (json.get(data,"park_alarm/snooze"))
        {
            if (backend_parking_alarm_snooze != data.to<bool>())
            {
                backend_parking_alarm_snooze = data.to<bool>();
                Serial.printf("\nbackend_parking_alarm_snooze changed to: %s\n", backend_parking_alarm_snooze ? "true" : "false");
                
                if(backend_parking_alarm_snooze)
                {
                    if(log_enabled)Serial.printf("\n---Park_Alarm Snoozed for %d  s ", backend_parking_alarm_snooze_time_ms/1000);
                }
                else
                {
                if(log_enabled) Serial.println("\n---park_alarm_snooze resetted!"); 
                } 



                some_input_changed = true;    
            
            
            }
        }
        else
        {
            Serial.printf("----backend_parking_alarm_snooze not found: Updating Node with default values----\n");
            firebase_park_alarm_snooze_needs_override = true;
            inputs_are_missing = true;
        }

        if (json.get(data,"accident/dismissed"))
        {
            if(accident_dismissed != data.to<bool>())
            {
                //gps_enabled = data.to<bool>();
                //Serial.printf("gps_enabled changed to: %s\n", gps_enabled ? "true" : "false");

                //just dismissing if the flag is inactive , and the accident is detected or confirmed

                if( !data.to<bool>() && ( accident_detected || accident_confirmed ))
                {
                    accident_dismissed = false;
                
                    if(firebase_log_mode > firebase_log_mode_silent)
                    {
                        Serial.printf("\naccident_dismissed changed to: %s\n", accident_dismissed ? "true" : "false");
                    }      

                    //Executing the change
                    //TOD , check if they indeed are propagated to RTDB
                    accident_detected  = false;
                    accident_confirmed = false;

                    //Resetting the flag
                    accident_dismissed = false;
                } 

                //If is dismissed without an accident_mode detected then is forced to reset
                else if (data.to<bool>())
                {
                    if(log_enabled)
                    {
                        Serial.print("\nError on Accident_Dismiss!:");                
                    }

                    if(!accident_detected && !accident_confirmed )
                    {
                        if(log_enabled)
                        {
                            Serial.print("\n 'accident mode' not triggered!. Resetting flag!");                
                        }

                        //Resetting Flag    
                        accident_dismissed = false;
                        firebase_accident_dismissed_needs_override = true;
                        inputs_are_missing = true;

                        if(firebase_log_mode > firebase_log_mode_silent)
                        {
                            Serial.printf("\n---accident_dismissed forcing an input override on firebase ..");
                        } 
                    }            
                }                          
                some_input_changed = true; 
            }
        }
        else
        {
            Serial.printf("----accident_dismissed not found: Updating Node with default values----\n");
            firebase_accident_dismissed_needs_override = true;
            inputs_are_missing = true;
        }

        if (json.get(data,"config/gps/enabled"))
        {
            if (gps_enabled != data.to<bool>())
            {
                gps_enabled = data.to<bool>();
                Serial.printf("\ngps_enabled changed to: %s\n", gps_enabled ? "true" : "false");

                //HERE Execute any needed change

                if(gps_enabled)
                {
                    if(!task_gps_active)
                    {
                        //will be handled by firebase loop
                        //create_task_gps();
                    }
                } 
                else
                {
                    if(task_gps_active)
                    {
                        task_gps_active = false;
                    }
                }

                some_input_changed = true;    
            }
        }
        else
        {
            Serial.printf("\n----gps_enabled not found: Updating Node with default values----\n");
            firebase_gps_enabled_needs_override = true;
            inputs_are_missing = true;
        }

        if (json.get(data,"\nconfig/gps/upload"))
        {
            if (gps_upload != data.to<bool>())
            {
                gps_upload = data.to<bool>();
                Serial.printf("\ngps_upload changed to: %s\n", gps_upload ? "true" : "false");

                if(firebase_log_mode > firebase_log_mode_silent)
                {
                    if(gps_upload) Serial.print("\n---GPS Data Will be uploaded to Firebase ");
                    else Serial.print("\n---GPS Data Will NOT be uploaded to Firebase ");
                }  

                some_input_changed = true;    
            }
        }
        else
        {
            Serial.printf("----gps_upload not found: Updating Node with default values----\n");
            firebase_gps_upload_needs_override = true;
            inputs_are_missing = true;
        }

        if (json.get(data,"config/gps/refresh_seconds"))
        {
            if (gps_refresh_seconds != data.to<int>())
            {
                if(data.to<int>() < 0 )
                {
                    Serial.printf("\n---ERROR on config/gps/refresh_seconds : Unacceptable Value , defaulting to : %d\n", gps_refresh_seconds_default);
                    gps_refresh_seconds = gps_refresh_seconds_default ;//default on error
                    firebase_gps_refresh_seconds_needs_override = true;
                    inputs_are_missing = true;
                }
                else
                {
                    gps_refresh_seconds = data.to<int>();
                    Serial.printf("\ngps_refresh_seconds changed to: %d\n", gps_refresh_seconds);
                }                    
                
                some_input_changed = true;           
            }
        }
        else
        {
            Serial.printf("\n----gps_refresh_seconds not found: Updating Node with default values----\n");
            firebase_gps_refresh_seconds_needs_override = true;
            inputs_are_missing = true;
        }

        //CAN

        if (json.get(data,"config/can/enabled"))
        {
            if (can_enabled != data.to<bool>())
            {
                can_enabled = data.to<bool>();
                Serial.printf("\ncan_enabled changed to: %s\n", can_enabled ? "true" : "false");

                if(can_enabled)
                {
                    if(!task_can_active)
                    {
                        //will be handled by firebase loop
                        //create_task_can();
                    }
                } 
                else
                {
                    if(task_can_active)
                    {
                        task_can_active = false;
                    }
                }

                some_input_changed = true;    
            }
        }
        else
        {
            Serial.printf("\n----can_enabled not found: Updating Node with default values----\n");
            firebase_can_enabled_needs_override = true;
            inputs_are_missing = true;
        }

        if (json.get(data,"config/can/upload"))
        {
            if (can_upload != data.to<bool>())
            {
                can_upload = data.to<bool>();
                Serial.printf("\ncan_upload changed to: %s\n", can_upload ? "true" : "false");

                if(firebase_log_mode > firebase_log_mode_silent)
                {
                    if(can_upload) Serial.print("\n---CAN Data Will be uploaded to Firebase ");
                    else Serial.print("\n---CAN Data Will NOT be uploaded to Firebase ");
                } 

                some_input_changed = true;    
            }
        }
        else
        {
            Serial.printf("\n----can_upload not found: Updating Node with default values----\n");
            firebase_can_upload_needs_override = true;
            inputs_are_missing = true;
        }

        if (json.get(data,"\nconfig/can/refresh_seconds"))
        {
            if (can_refresh_seconds != data.to<int>())
            {
                if(data.to<int>() < 0 )
                {
                    Serial.printf("\n---ERROR on config/can/refresh_seconds : Unacceptable Value , defaulting to : %d\n", can_refresh_seconds_default);
                    can_refresh_seconds = can_refresh_seconds_default ;//default on error
                    firebase_can_refresh_seconds_needs_override = true;
                    inputs_are_missing = true;
                }
                else
                {
                    can_refresh_seconds = data.to<int>();
                    Serial.printf("\ncan_refresh_seconds changed to: %d\n", can_refresh_seconds);
                }                    

                some_input_changed = true;    
            }
        }
        else
        {
            Serial.printf("\n----can_refresh_seconds not found: Updating Node with default values----\n");
            firebase_can_refresh_seconds_needs_override = true;
            inputs_are_missing = true;
        }

        //BLACK_BOX

        if (json.get(data,"config/black_box/enabled"))
        {
            if (black_box_enabled != data.to<bool>())
            {
                black_box_enabled = data.to<bool>();
                Serial.printf("\nblack_box_enabled changed to: %s\n", black_box_enabled ? "true" : "false");

                if(black_box_enabled)
                {
                    if(!task_sd_active)
                    {
                        //will be handled by firebase loop
                        //create_task_sd();
                    }
                } 
                else
                {
                    if(task_sd_active)
                    {
                        task_sd_active = false;
                    }
                }

                some_input_changed = true;    

            }
        }
        else
        {
            Serial.printf("\n----black_box_enabled not found: Updating Node with default values----\n");
            firebase_black_box_enabled_needs_override = true;
            inputs_are_missing = true;
        }

        if (json.get(data,"config/black_box/refresh_ms"))
        {
            if (black_box_logging_interval_milliseconds != data.to<int>())
            {
                if(data.to<int>() < 0 )
                {
                    Serial.printf("\n---ERROR on config/black_box/refresh_milliseconds : Unacceptable Value , defaulting to : %d\n", black_box_logging_interval_milliseconds_default);
                    black_box_logging_interval_milliseconds = black_box_logging_interval_milliseconds_default ;//default on error
                    firebase_black_box_refresh_milliseconds_needs_override = true;
                    inputs_are_missing = true;
                }
                else
                {
                    black_box_logging_interval_milliseconds = data.to<int>();
                    Serial.printf("\nblack_box_logging_interval_milliseconds changed to: %d\n", black_box_logging_interval_milliseconds);
                }                    
                some_input_changed = true;    
            }
        }
        else
        {
            Serial.printf("\n----black_box_logging_interval_milliseconds not found: Updating Node with default values----\n");
            firebase_black_box_refresh_milliseconds_needs_override = true;
            inputs_are_missing = true;
        }


        if (json.get(data,"config/black_box/serial_gap"))
        {
            if (black_box_serial_iterations_gap != data.to<int>())
            {
                black_box_serial_iterations_gap = data.to<int>();
                
                if(black_box_serial_iterations_gap > 0)
                {
                    Serial.printf("\n ---black_box_serial_iterations_gap set to %d", black_box_serial_iterations_gap);                    
                } 
                
                else 
                {
                    Serial.printf("\n---ERROR on black_box_serial_iterations_gap : Unacceptable Value , defaulting to : %d\n", black_box_serial_iterations_gap_default);
                    black_box_serial_iterations_gap = black_box_serial_iterations_gap_default ;//default on error
                    firebase_black_box_serial_iterations_gap_needs_override = true;
                    inputs_are_missing = true;
                }
                some_input_changed = true;   
            }
            //TODO later add here as well the still minutes before power off
        }
        else
        {
            Serial.printf("\n----black_box_serial_iterations_gap not found: Updating Node with default values----\n");
            firebase_black_box_serial_iterations_gap_needs_override = true;
            inputs_are_missing = true;
        }

        if (json.get(data,"config/black_box/firebase_gap"))
        {
            if (black_box_firebase_iterations_gap != data.to<int>())
            {
                black_box_firebase_iterations_gap = data.to<int>();
                
                if(black_box_firebase_iterations_gap > 0)
                {
                    Serial.printf("\n---black_box_firebase_iterations_gap set to %d", black_box_firebase_iterations_gap);                    
                } 
                
                else 
                {
                    Serial.printf("\n---ERROR on black_box_firebase_iterations_gap : Unacceptable Value , defaulting to : %d\n", black_box_firebase_iterations_gap_default);
                    black_box_firebase_iterations_gap = black_box_firebase_iterations_gap_default ;//default on error
                    firebase_black_box_firebase_iterations_gap_needs_override = true;
                    inputs_are_missing = true;
                }
                some_input_changed = true;   
            }
            //TODO later add here as well the still minutes before power off
        }
        else
        {
            Serial.printf("\n----firebase_black_box_firebase_iterations_gap not found: Updating Node with default values----\n");
            firebase_black_box_firebase_iterations_gap_needs_override = true;
            inputs_are_missing = true;
        }



        //firebase_reset_order
        if (json.get(data,"config/commands/reset"))
        {

            if (firebase_reset_order != data.to<bool>())
            {
                firebase_reset_order = data.to<bool>();
                Serial.printf("\nfirebase_reset_order changed to: %s\n", firebase_reset_order ? "true" : "false");

                if(firebase_log_mode > firebase_log_mode_silent)
                {
                    if(firebase_reset_order) Serial.print("\n---Reset Order Received from Firebase ");
                } 

                some_input_changed = true;    
            }
        }
        else
        {
            Serial.printf("\n----firebase_reset_order not found: Updating Node with default values----\n");
            firebase_reset_order_needs_override = true;
            inputs_are_missing = true;
        }






        if (json.get(data,"config/oled/oled_dev_screen_nr"))
        {
            if (oled_dev_screen_nr != data.to<int>())
            {
                oled_dev_screen_nr = data.to<int>();
                
                if(oled_dev_screen_nr == 0)
                {
                    Serial.printf("\n---oled_dev_screen deactivated");
                    oled_needs_clear = true;
                } 

                else if ( oled_dev_screen_nr > 0 && oled_dev_screen_nr <= oled_dev_screen_nr_max )
                {
                    Serial.printf("\n---oled_dev_screen_nr changed to: %d\n", oled_dev_screen_nr);
                }

                else 
                {
                    Serial.printf("\n---ERROR on oled_dev_screen_nr : Unacceptable Value , defaulting to : %d\n", oled_dev_screen_nr_default);
                    oled_dev_screen_nr = oled_dev_screen_nr_default ;//default on error
                    firebase_oled_dev_screen_nr_needs_override = true;
                    inputs_are_missing = true;
                }
                some_input_changed = true;    
            }
        }
        else
        {
            Serial.printf("\n----oled_dev_screen_nr not found: Updating Node with default values----\n");
            firebase_oled_dev_screen_nr_needs_override = true;
            inputs_are_missing = true;
        }

        //Dangerous part , it will start the ota updater task and kill this task if enabled 
        if (json.get(data,"config/commands/ota"))
        {
            if (ota_enabled != data.to<bool>())
            {
                ota_enabled = data.to<bool>();
                
                Serial.printf("\nota_enabled changed to: %s\n", ota_enabled ? "true" : "false");

                if(ota_enabled) 
                {
                    buzzer_warning();

                    Serial.print("\n---OTA Order Received ... ");

                    //Using Generic Vars so we can reuse the code
                    FirebaseJson json; 

                    //Setting the ota_enabled to false to avoid an ota loop after connecting to firebase

                    ota_enabled = false;

                    json.add("ota",ota_enabled);

                    //TODO ADD here other things later relevant for last info before ota

                    String string;
                    json.toString(string, false); // false = minified JSON  , true = pretiffy

                    Serial.print("\n---last info before ota :");
                    Serial.println(string);
                    //Serial.print("\n---Payload length: ");
                    //Serial.println(string.length());
                    //Serial.println();

                    if (string.length() > 2)
                    {
                        Serial.print("\n---resetting flag before ota to RTDB ---\n");

                        String path = base_path_with_id;
                        //TODO change this if info will be saved on other subnodes as well
                        path += String("config/commands/");

                        //Resetting Flag before Sending

                        last_info_before_restart_acknowledged = false;

                        //SENDING INFO 

                        Database.update<object_t>
                        (
                            aClient,
                            path,
                            object_t(string),
                            process_data,
                            "last_info_before_ota"
                        );

                        int waiting_time = 10000;

                        unsigned long timer = millis();

                        Serial.print("\n---WAITING FOR last_info_before_ota_acknowledged----\n");
                        
                        while(1)
                        {
                            if( last_info_before_ota_acknowledged )
                            {
                                //Will be ACk and printed on callback
                                Serial.print("\n---Continuing with the OTA!\n");
                                buzzer_notification();
                                wait(1000);
                                break;
                            }

                            else if(millis() > timer + waiting_time)
                            {
                                if(last_info_before_ota_acknowledge_retry_nr < last_info_before_ota_acknowledge_max_retries )
                                {
                                    Serial.print("\n---last_info_before_ota_acknowledged waiting time exhausted!---");
                                    Serial.print("\n---Exiting and Retrying !\n");

                                    last_info_before_ota_acknowledge_max_retries++;
                                    
                                    return;
                                }
                                else
                                {
                                    Serial.print("\n---last_info_before_ota_acknowledged max retries reached ---!\n");
                                    Serial.print("\n---Continuing with the OTA without resetting flag on RTDB---\n");
                                    break;
                                }
                            } 
                            else
                            {
                                app.loop(); // Maintain authentication and async tasks
                                Database.loop();
                                wait(100);
                            } 
                        }
                    }
                    else
                    {
                        Serial.println("--- json ERROR : PAYLOAD EMPTY");
                        //continuing
                        //return;
                    }

                    if(firebase_log_mode > firebase_log_mode_silent)
                    {
                        //Serial.print("\n---ota_enabled !");

                        Serial.print("\n---Disabling LTE and Starting OTA via WIFI !");
                    } 

                    oled_dev_screen_nr = 7;

                    //simcom_turn_off();

                    //wait(2000);

                    //lte_status = lte_off_due_to_hard_reset;
                   
                    //wait(1000);

                    if(task_sd_active)
                    {
                        task_sd_active = false;
                    }
                    if(task_can_active)
                    {
                        task_can_active = false;
                    }
                    if(task_gps_active)
                    {
                        task_gps_active = false;
                    }

                    if(i2c_manager_running)
                    {
                        i2c_manager_running = false;
                    }

                    wait(2000);
                    
                    //TODO here start the new task and then kill this one 
                    Serial.print("\n---Creating ota_task");

                    create_task_ota();

                    //Killing this task as well now
                    task_firebase_active = false;
                    vTaskDelete(NULL);

                }
                else Serial.print("\n---ota_disabled");                        

                some_input_changed = true;    
            }
        }
        else
        {
            Serial.printf("\n----ota_enabled not found: Updating Node with default values----\n");
            firebase_ota_enabled_needs_override = true;
            inputs_are_missing = true;
        }


        //adding also the cases for hw_version and hw_variant
        // HW Version
        if (json.get(data,"hw_info/hw_version"))
        {
            //If the fied is there then we do nothing
        }
        else
        {
            Serial.printf("\n----hw_version not found: Updating Node with default values----\n");
            firebase_hw_version_needs_override = true;
            hw_version_synced = false;
            inputs_are_missing = true;
        }

        // HW Variant (Not properly an INPUT but wil never be updated as OUTPUT so we push it from here

        if (json.get(data,"hw_info/hw_variant"))
        {
            //If the fied is there then we do nothing
        }
        else
        {
            Serial.printf("\n----hw_variant_string not found: Updating Node with default values----\n");
            firebase_hw_variant_needs_override = true;
            hw_variant_synced = false;
            inputs_are_missing = true;
        }  

        
        // Power Mode
        if (json.get(data,"config/power/mode"))
        {
            if (power_mode != data.to<int>())
            {
                power_mode = data.to<int>();
                
                if(power_mode == power_mode_continous)
                {
                    Serial.printf("\n---power_mode set to continous");
                } 

                else if(power_mode == power_mode_saving)
                {
                    Serial.printf("\n---power_mode set to saving");
                }

                else 
                {
                    Serial.printf("\n---ERROR on power_mode : Unacceptable Value , defaulting to : %d\n", power_mode_default);
                    power_mode = power_mode_default ;//default on error
                    firebase_power_mode_needs_override = true;
                    inputs_are_missing = true;
                }
                some_input_changed = true;    
            }
            //TODO later add here as well the still minutes before power off
        }
        else
        {
            Serial.printf("\n----power_mode not found: Updating Node with default values----\n");
            firebase_power_mode_needs_override = true;
            inputs_are_missing = true;
        }

        //movement_monitor_enabled
        if (json.get(data,"config/power/movement_monitor_enabled"))
        {
            if (movement_monitor_enabled != data.to<bool>())
            {
                movement_monitor_enabled = data.to<bool>();
                Serial.printf("\n---movement_monitor_enabled changed to: %s\n", movement_monitor_enabled ? "true" : "false");

                //HERE Execute any needed change
                some_input_changed = true;    
            }
        }
        else
        {
            Serial.printf("---movement_monitor_enabled not found: Updating Node with default values---\n");
            firebase_movement_monitor_enabled_needs_override = true;
            inputs_are_missing = true;
        }

        //mwm_before_sleep
        if (json.get(data,"config/power/mwm_before_sleep"))
        {
            if (mwm_before_sleep != data.to<int>())
            {
                if(data.to<int>() <= 0 )
                {
                    Serial.printf("\n---ERROR on config/power/mwm_before_sleep : Unacceptable Value , defaulting to : %d\n", mwm_before_sleep_default);
                    mwm_before_sleep = mwm_before_sleep_default ;//default on error
                    firebase_mwm_before_sleep_needs_override = true;
                    inputs_are_missing = true;
                }
                else
                {
                    mwm_before_sleep = data.to<int>();
                    Serial.printf("\n---mwm_before_sleep changed to: %d\n", mwm_before_sleep);
                }                    

                some_input_changed = true;     
            }
            //TODO later add here as well the still minutes before power off
        }
        else
        {
            Serial.printf("----mwm_before_sleep not found: Updating Node with default values----\n");
            firebase_mwm_before_sleep_needs_override = true;
            inputs_are_missing = true;
        }



        //do thuis , rule out all 
       /*
        NEW INPUTS

        config/

            
            firebase /
                update_input_interval_seconds
                update_output_interval_seconds
                int firebase_heartbeat_update_interval_seconds
                firebase_preferred_internet_connection
                allow_lte_connection
                allow_wifi_connection
                firebase_mwr_update_gap
                bool firebase_retries_before_key_turn_and_esp_reset = false;

            power /
                
                movement_monitor_enabled
                mwm_before_sleep

        NEW OUTPUTS

        lte/carrier,data_used,status:(use the defined ones),

        counters/ last update (time and date) , mwr(move this one), mwm

        TODO TOMOTTOW ---status : last_shutdown_reason

        */

        // LTE Related


        //TODO TOMORROW  
        
        //power /
                
             //   movement_monitor_enabled
             //   mwm_before_sleep




        if (json.get(data,"config/lte/retries_before_key_turn"))
        {
            if (lte_retries_before_key_turn != data.to<int>())
            {
                lte_retries_before_key_turn = data.to<int>();
                
                if(lte_retries_before_key_turn > 0)
                {
                    Serial.printf("\n ---lte_retries_before_key_turn set to %d", lte_retries_before_key_turn);                    
                } 
                
                else 
                {
                    Serial.printf("\n---ERROR on lte_retries_before_key_turn : Unacceptable Value , defaulting to : %d\n", lte_retries_before_key_turn_default);
                    lte_retries_before_key_turn = lte_retries_before_key_turn_default ;//default on error
                    firebase_lte_retries_before_key_turn_needs_override = true;
                    inputs_are_missing = true;
                }
                some_input_changed = true;   
                
                //TODO after train save it to nvs here
                //set_to_nvs_lte_retries_before_esp_reset();
            }
            //TODO later add here as well the still minutes before power off
        }
        else
        {
            Serial.printf("----lte_retries_before_key_turn not found: Updating Node with default values----\n");
            firebase_lte_retries_before_key_turn_needs_override = true;
            inputs_are_missing = true;
        }





        if (json.get(data,"config/lte/retries_before_esp_reset"))
        {
            if (lte_retries_before_esp_reset != data.to<int>())
            {
                lte_retries_before_esp_reset = data.to<int>();
                
                if(lte_retries_before_esp_reset > 0)
                {
                    Serial.printf("\n ---lte_retries_before_esp_reset set to %d", lte_retries_before_esp_reset);                    
                } 
                
                else 
                {
                    Serial.printf("\n---ERROR on lte_retries_before_esp_reset : Unacceptable Value , defaulting to : %d\n", lte_retries_before_esp_reset_default);
                    lte_retries_before_esp_reset = lte_retries_before_esp_reset_default ;//default on error
                    firebase_lte_retries_before_esp_reset_needs_override = true;
                    inputs_are_missing = true;
                }
                some_input_changed = true;   
                
                //TODO after train save it to nvs here
                //set_to_nvs_lte_retries_before_esp_reset();
            }
            //TODO later add here as well the still minutes before power off
        }
        else
        {
            Serial.printf("----lte_retries_before_esp_reset not found: Updating Node with default values----\n");
            firebase_lte_retries_before_esp_reset_needs_override = true;
            inputs_are_missing = true;
        }



        //../config/firebase

        if (json.get(data,"config/firebase/retries_before_key_turn_and_esp_reset"))
        {
            if (firebase_retries_before_key_turn_and_esp_reset != data.to<int>())
            {
                firebase_retries_before_key_turn_and_esp_reset = data.to<int>();
                
                if(firebase_retries_before_key_turn_and_esp_reset > 0)
                {
                    Serial.printf("\n ---firebase_retries_before_key_turn_and_esp_reset set to %d", firebase_retries_before_key_turn_and_esp_reset);                    
                } 
                
                else 
                {
                    Serial.printf("\n---ERROR on firebase_retries_before_key_turn_and_esp_reset : Unacceptable Value , defaulting to : %d\n", firebase_retries_before_key_turn_and_esp_reset_default);
                    firebase_retries_before_key_turn_and_esp_reset = firebase_retries_before_key_turn_and_esp_reset_default ;//default on error
                    firebase_retries_before_key_turn_and_esp_reset_needs_override = true;
                    inputs_are_missing = true;
                }
                some_input_changed = true;   
                
                //TODO after train save it to nvs here
                //set_to_nvs_lte_retries_before_esp_reset();
            }
            //TODO later add here as well the still minutes before power off
        }
        else
        {
            Serial.printf("----firebase_retries_before_key_turn_and_esp_reset not found: Updating Node with default values----\n");
            firebase_retries_before_key_turn_and_esp_reset_needs_override = true;
            inputs_are_missing = true;
        }

        /*
                
        firebase_preferred_internet_connection
        allow_lte_connection
        allow_wifi_connection
        firebase_mwr_update_gap
        bool firebase_retries_before_key_turn_and_esp_reset = false;
        */



        if (json.get(data,"config/firebase/update_input_interval_seconds"))
        {
            if (firebase_update_input_interval_seconds != data.to<int>())
            {
                firebase_update_input_interval_seconds = data.to<int>();
                
                if(firebase_update_input_interval_seconds > 0)
                {
                    Serial.printf("\n ---firebase_update_input_interval_seconds set to %d", firebase_update_input_interval_seconds);                    
                } 
                
                else 
                {
                    Serial.printf("\n---ERROR on firebase_update_input_interval_seconds : Unacceptable Value , defaulting to : %d\n", firebase_update_input_interval_seconds_default);
                    firebase_update_input_interval_seconds = firebase_update_input_interval_seconds_default ;//default on error
                    firebase_update_input_interval_seconds_needs_override = true;
                    inputs_are_missing = true;
                }
                some_input_changed = true;   
                
                //TODO after train save it to nvs here
                //set_to_nvs_lte_retries_before_esp_reset();
            }
            //TODO later add here as well the still minutes before power off
        }
        else
        {
            Serial.printf("----firebase_update_input_interval_seconds not found: Updating Node with default values----\n");
            firebase_update_input_interval_seconds_needs_override = true;
            inputs_are_missing = true;
        }



        if (json.get(data,"config/firebase/update_output_interval_seconds"))
        {
            if (firebase_update_output_interval_seconds != data.to<int>())
            {
                firebase_update_output_interval_seconds = data.to<int>();
                
                if(firebase_update_output_interval_seconds > 0)
                {
                    Serial.printf("\n ---firebase_update_output_interval_seconds set to %d", firebase_update_output_interval_seconds);                    
                } 
                
                else 
                {
                    Serial.printf("\n---ERROR on firebase_update_output_interval_seconds : Unacceptable Value , defaulting to : %d\n", firebase_update_output_interval_seconds_default);
                    firebase_update_output_interval_seconds = firebase_update_output_interval_seconds_default ;//default on error
                    firebase_update_output_interval_seconds_needs_override = true;
                    inputs_are_missing = true;
                }
                some_input_changed = true;   
                
                //TODO after train save it to nvs here
                //set_to_nvs_lte_retries_before_esp_reset();
            }
            //TODO later add here as well the still minutes before power off
        }
        else
        {
            Serial.printf("----firebase_update_output_interval_seconds not found: Updating Node with default values----\n");
            firebase_update_output_interval_seconds_needs_override = true;
            inputs_are_missing = true;
        }
     

        if (json.get(data,"config/firebase/heartbeat_update_interval_seconds"))
        {
            if (firebase_heartbeat_update_interval_seconds != data.to<int>())
            {
                firebase_heartbeat_update_interval_seconds = data.to<int>();
                
                if(firebase_heartbeat_update_interval_seconds > 0)
                {
                    Serial.printf("\n ---firebase_heartbeat_update_interval_seconds set to %d", firebase_heartbeat_update_interval_seconds);                    
                } 
                
                else 
                {
                    Serial.printf("\n---ERROR on firebase_heartbeat_update_interval_seconds : Unacceptable Value , defaulting to : %d\n", firebase_heartbeat_update_interval_seconds_default);
                    firebase_heartbeat_update_interval_seconds = firebase_heartbeat_update_interval_seconds_default ;//default on error
                    firebase_heartbeat_update_interval_seconds_needs_override = true;
                    inputs_are_missing = true;
                }
                some_input_changed = true;   
                
                //TODO after train save it to nvs here
                //set_to_nvs_lte_retries_before_esp_reset();
            }
            //TODO later add here as well the still minutes before power off
        }
        else
        {
            Serial.printf("----firebase_heartbeat_update_interval_seconds not found: Updating Node with default values----\n");
            firebase_heartbeat_update_interval_seconds_needs_override = true;
            inputs_are_missing = true;
        }
        
        //TODO , maybe move it to string for better visualization
        if (json.get(data,"config/firebase/preferred_internet_connection"))
        {
            if (firebase_preferred_internet_connection != data.to<int>())
            {
                firebase_preferred_internet_connection = data.to<int>();
                
                if( firebase_preferred_internet_connection == connection_via_wifi)
                {
                    Serial.print("\n ---firebase_preferred_internet_connection set to wifi");                    
                } 

                else if( firebase_preferred_internet_connection == connection_via_lte)
                {
                    Serial.print("\n ---firebase_preferred_internet_connection set to lte");                    
                } 
                
                else 
                {
                    Serial.printf("\n---ERROR on firebase_preferred_internet_connection : Unacceptable Value , defaulting to : %d\n", firebase_preferred_internet_connection_default);
                    firebase_preferred_internet_connection = firebase_preferred_internet_connection_default ;//default on error
                    firebase_preferred_internet_connection_needs_override = true;
                    inputs_are_missing = true;
                }
                some_input_changed = true;   
                
                //TODO after train save it to nvs here
                //set_to_nvs_lte_retries_before_esp_reset();
            }
            //TODO later add here as well the still minutes before power off
        }
        else
        {
            Serial.printf("----firebase_preferred_internet_connection not found: Updating Node with default values----\n");
            firebase_preferred_internet_connection_needs_override = true;
            inputs_are_missing = true;
        }

        if (json.get(data,"config/firebase/allow_lte_connection"))
        {
            if (firebase_allow_lte_connection != data.to<bool>())
            {
                firebase_allow_lte_connection = data.to<bool>();
                Serial.printf("\nfirebase_allow_lte_connection changed to: %s\n", firebase_allow_lte_connection ? "true" : "false");

                some_input_changed = true;    

            }
        }
        else
        {
            Serial.printf("----firebase_allow_lte_connection not found: Updating Node with default values----\n");
            firebase_allow_lte_connection_needs_override = true;
            inputs_are_missing = true;
        }

        
        if (json.get(data,"config/firebase/allow_wifi_connection"))
        {
            if (firebase_allow_wifi_connection != data.to<bool>())
            {
                firebase_allow_wifi_connection = data.to<bool>();
                Serial.printf("\nfirebase_allow_wifi_connection changed to: %s\n", firebase_allow_wifi_connection ? "true" : "false");

                some_input_changed = true;    
            }
        }
        else
        {
            Serial.printf("----firebase_allow_wifi_connection not found: Updating Node with default values----\n");
            firebase_allow_wifi_connection_needs_override = true;
            inputs_are_missing = true;
        }


        
        
        //firebase_mwr_update_gap
        
        
        if (json.get(data,"config/firebase/mwr_update_gap"))
        {
            if (firebase_mwr_update_gap != data.to<int>())
            {
                firebase_mwr_update_gap = data.to<int>();
                
                if(firebase_mwr_update_gap > 0)
                {
                    Serial.printf("\n ---firebase_mwr_update_gap set to %d", firebase_mwr_update_gap);                    
                } 
                
                else 
                {
                    Serial.printf("\n---ERROR on firebase_mwr_update_gap : Unacceptable Value , defaulting to : %d\n", firebase_mwr_update_gap_default);
                    firebase_mwr_update_gap = firebase_mwr_update_gap_default ;//default on error
                    firebase_mwr_update_gap_needs_override = true;
                    inputs_are_missing = true;
                }
                some_input_changed = true;   
            }
            //TODO later add here as well the still minutes before power off
        }
        else
        {
            Serial.printf("----firebase_mwr_update_gap not found: Updating Node with default values----\n");
            firebase_mwr_update_gap_needs_override = true;
            inputs_are_missing = true;
        }

        //firebase_mwm_update_gap

        if (json.get(data,"config/firebase/mwm_update_gap"))
        {
            if (firebase_mwm_update_gap != data.to<int>())
            {
                firebase_mwm_update_gap = data.to<int>();
                
                if(firebase_mwm_update_gap > 0)
                {
                    Serial.printf("\n ---firebase_mwm_update_gap set to %d", firebase_mwm_update_gap);                    
                } 
                
                else 
                {
                    Serial.printf("\n---ERROR on firebase_mwm_update_gap : Unacceptable Value , defaulting to : %d\n", firebase_mwm_update_gap_default);
                    firebase_mwm_update_gap = firebase_mwm_update_gap_default ;//default on error
                    firebase_mwm_update_gap_needs_override = true;
                    inputs_are_missing = true;
                }
                some_input_changed = true;   
            }
            //TODO later add here as well the still minutes before power off
        }
        else
        {
            Serial.printf("----firebase_mwm_update_gap not found: Updating Node with default values----\n");
            firebase_mwm_update_gap_needs_override = true;
            inputs_are_missing = true;
        }

       




        

        //black-box serialiterations gap
        //bb firebase iteration gap

        

 






















        

        //NEXT STEP----------------------------------------------------------------------------------------------

        //The inputs wil be overriden from within the main loop
        if(inputs_are_missing)
        {
            if(log_enabled) Serial.println("---\nInput data is missing on RTDB ! -> starting overriding function");
            //Generate the JSON for the current flags
        }

        else //Structure is complete
        {
            //If no input changed from the values on RTDB
            if(some_input_changed)
            {
                if(log_enabled) Serial.println("\n---Changes on input values between device and RTDB detected----");
                some_input_changed = false; //Resetting flag as we dont use it anymore at the moment as the inputs are changed within their if-else block                
            }
            else
            {               
                if(log_enabled) Serial.println("\n---No changes detected on input values between device and RTDB----");
            }
        }            
    }
    else
    {
        // Handle errors
        Serial.printf("\n---Failed to fetch JSON : JSON EMPTY \n");
    }

    input_json_needs_processing = false;  
}

//this is an emergency function to update the whole node when a value is not available anymore
//it will replace the whole node with the default values
//Generate the JSON for the current flags

//We always have to upload until the last node as if we go before it wil overwrite the other parallel nodes TODO (find a fix for this)
//By some reason while overriding we have to make it all the way till the last node so we will update individually to avoid overwriting the whole node 
void firebase_input_override()
{
    //This Function will be divided in two sections , the config section and the other ones

    //Starting with config/... 

    // CONFIG/GPS
    if (firebase_gps_enabled_needs_override         ||
        firebase_gps_upload_needs_override          ||
        firebase_gps_refresh_seconds_needs_override   )

    {
        //Using Generic Vars so we can reuse the code
        FirebaseJson json; 

        json.add("enabled",gps_enabled);
        firebase_gps_enabled_needs_override = false;

        json.add("upload" ,gps_upload);
        firebase_gps_upload_needs_override = false;

        json.add("refresh_seconds",gps_refresh_seconds);
        firebase_gps_refresh_seconds_needs_override  = false;

        String string;
        json.toString(string, false); // false = minified JSON  , true = pretiffy

        Serial.print("\n---overriden json to upload:");
        Serial.println(string);
        //Serial.print("\n---Payload length: ");
        //Serial.println(string.length());
        //Serial.println();

        if (string.length() > 2)
        {
            Serial.print("\n---uploading overriden input subnode to RTDB ---\n");

            String path = base_path_with_id;
            path += String("config/gps/");
            
            Database.update<object_t>
            (
                aClient,
                path,
                object_t(string),
                process_data,
                "input_json_override"
            );

            inputs_are_missing = false; //TODO maybe later remove this from here 
            firebase_input_override_waiting_acknowledge = true;

            int waiting_time = 5000;

            unsigned long timer = millis();

            Serial.print("\n---WAITING FOR firebase_input_override_ack----\n");
            
            while(1)
            {
                if( !firebase_input_override_waiting_acknowledge )
                {
                    //Will be ACk and printed on callback
                    Serial.print("\n---Continuing with the loop!\n");
                    wait(1000);
                    break;
                }

                else if(millis() > timer + waiting_time)
                {
                    Serial.print("\n---firebase_input_override_ack waiting time exhausted \n---Exiting and Retrying !\n");
                    return;
                } 

                else
                {
                    app.loop(); // Maintain authentication and async tasks
                    Database.loop();
                    wait(100);
                } 
            }
        }
        else
        {
            Serial.println("--- json ERROR : PAYLOAD EMPTY");
            firebase_something_changed = false; //Resetting flag for next case
            //Not continuing
            return;
        }
    } 
    
    // CONFIG/CAN
    if (firebase_can_enabled_needs_override         ||
        firebase_can_upload_needs_override          ||
        firebase_can_refresh_seconds_needs_override   )     
    {
        //Using Generic Vars so we can reuse the code

        FirebaseJson json;        

        json.add("enabled", can_enabled);
        firebase_can_enabled_needs_override  = false;

        json.add("upload", can_upload);
        firebase_can_upload_needs_override = false;

        json.add("refresh_seconds", can_refresh_seconds);
        firebase_can_refresh_seconds_needs_override = false;

        String string;
        json.toString(string, false); // false = minified JSON  , true = pretiffy

        Serial.print("\n---overriden json to upload:");
        Serial.println(string);
        //Serial.print("\n---Payload length: ");
        //Serial.println(string.length());
        //Serial.println();

        if (string.length() > 2)
        {
            Serial.print("\n---uploading overriden input subnode to RTDB ---\n");

            String path = base_path_with_id;
            path += String("config/can/");
            
            Database.update<object_t>
            (
                aClient,
                path,
                object_t(string),
                process_data,
                "input_json_override"
            );

            inputs_are_missing = false; //TODO maybe later remove this from here 
            firebase_input_override_waiting_acknowledge = true;

            int waiting_time = 5000;

            unsigned long timer = millis();

            Serial.print("\n---WAITING FOR firebase_input_override_ack----\n");
            
            while(1)
            {
                if( !firebase_input_override_waiting_acknowledge )
                {
                    //Will be ACk and printed on callback
                    Serial.print("\n---Continuing with the loop!\n");
                    wait(1000);
                    break;
                }

                else if(millis() > timer + waiting_time)
                {
                    Serial.print("\n---firebase_input_override_ack waiting time exhausted \n---Exiting and Retrying !\n");
                    return;
                } 

                else
                {
                    app.loop(); // Maintain authentication and async tasks
                    Database.loop();
                    wait(100);
                } 
            }
        }
        else
        {
            Serial.println("--- json ERROR : PAYLOAD EMPTY");
            firebase_something_changed = false; //Resetting flag for next case
            //Not continuing
            return;
        }
    }

    // CONFIG/BLACK_BOX
    if (firebase_black_box_enabled_needs_override               ||
        firebase_black_box_refresh_milliseconds_needs_override  ||
        firebase_black_box_serial_iterations_gap_needs_override || 
        firebase_black_box_firebase_iterations_gap_needs_override )     
    {
        //Using Generic Vars so we can reuse the code
       
        FirebaseJson json; 

        json.add("enabled",black_box_enabled);
        firebase_black_box_enabled_needs_override = false;

        json.add("refresh_ms",black_box_logging_interval_milliseconds);
        firebase_black_box_refresh_milliseconds_needs_override = false;

        json.add("serial_gap",black_box_serial_iterations_gap);
        firebase_black_box_serial_iterations_gap_needs_override = false;    

        json.add("firebase_gap",black_box_firebase_iterations_gap);
        firebase_black_box_firebase_iterations_gap_needs_override = false;

        String string;
        json.toString(string, false); // false = minified JSON  , true = pretiffy

        Serial.print("\n---overriden json to upload:");
        Serial.println(string);
        //Serial.print("\n---Payload length: ");
        //Serial.println(string.length());
        //Serial.println();

        if (string.length() > 2)
        {
            Serial.print("\n---uploading overriden input subnode to RTDB ---\n");

            String path = base_path_with_id;
            path += String("config/black_box/");
            
            Database.update<object_t>
            (
                aClient,
                path,
                object_t(string),
                process_data,
                "input_json_override"
            );

            inputs_are_missing = false; //TODO maybe later remove this from here 
            firebase_input_override_waiting_acknowledge = true;

            int waiting_time = 5000;

            unsigned long timer = millis();

            Serial.print("\n---WAITING FOR firebase_input_override_ack----\n");
            
            while(1)
            {
                if( !firebase_input_override_waiting_acknowledge )
                {
                    //Will be ACk and printed on callback
                    Serial.print("\n---Continuing with the loop!\n");
                    wait(1000);
                    break;
                }

                else if(millis() > timer + waiting_time)
                {
                    Serial.print("\n---firebase_input_override_ack waiting time exhausted \n---Exiting and Retrying !\n");
                    return;
                } 

                else
                {
                    app.loop(); // Maintain authentication and async tasks
                    Database.loop();
                    wait(100);
                } 
            }
        }
        else
        {
            Serial.println("--- json ERROR : PAYLOAD EMPTY");
            firebase_something_changed = false; //Resetting flag for next case
            //Not continuing
            return;
        }
    }

    // CONFIG/OLED
    if (firebase_oled_dev_screen_nr_needs_override)     
    {
        //Using Generic Vars so we can reuse the code
       
        FirebaseJson json; 

        json.add("oled_dev_screen_nr",oled_dev_screen_nr);
        firebase_oled_dev_screen_nr_needs_override = false;

        String string;
        json.toString(string, false); // false = minified JSON  , true = pretiffy

        Serial.print("\n---overriden json to upload:");
        Serial.println(string);
        //Serial.print("\n---Payload length: ");
        //Serial.println(string.length());
        //Serial.println();

        if (string.length() > 2)
        {
            Serial.print("\n---uploading overriden input subnode to RTDB ---\n");

            String path = base_path_with_id;
            path += String("config/oled/");
            
            Database.update<object_t>
            (
                aClient,
                path,
                object_t(string),
                process_data,
                "input_json_override"
            );

            inputs_are_missing = false; //TODO maybe later remove this from here 
            firebase_input_override_waiting_acknowledge = true;

            int waiting_time = 5000;

            unsigned long timer = millis();

            Serial.print("\n---WAITING FOR firebase_input_override_ack----\n");
            
            while(1)
            {
                if( !firebase_input_override_waiting_acknowledge )
                {
                    //Will be ACk and printed on callback
                    Serial.print("\n---Continuing with the loop!\n");
                    wait(1000);
                    break;
                }

                else if(millis() > timer + waiting_time)
                {
                    Serial.print("\n---firebase_input_override_ack waiting time exhausted \n---Exiting and Retrying !\n");
                    return;
                } 

                else
                {
                    app.loop(); // Maintain authentication and async tasks
                    Database.loop();
                    wait(100);
                } 
            }
        }
        else
        {
            Serial.println("--- json ERROR : PAYLOAD EMPTY");
            firebase_something_changed = false; //Resetting flag for next case
            //Not continuing
            return;
        }
    }

    // CONFIG/POWER
    if (firebase_power_mode_needs_override               ||
        firebase_movement_monitor_enabled_needs_override ||
        firebase_mwm_before_sleep_needs_override           )  
    {
        //Using Generic Vars so we can reuse the code
       
        FirebaseJson json; 

        json.add("mode",power_mode);
        firebase_power_mode_needs_override = false;

        json.set("movement_monitor_enabled", movement_monitor_enabled);
        firebase_movement_monitor_enabled_needs_override = false;

        json.set("mwm_before_sleep", mwm_before_sleep);
        firebase_mwm_before_sleep_needs_override = false;

        String string;
        json.toString(string, false); // false = minified JSON  , true = pretiffy

        Serial.print("\n---overriden json to upload:");
        Serial.println(string);
        //Serial.print("\n---Payload length: ");
        //Serial.println(string.length());
        //Serial.println();

        if (string.length() > 2)
        {
            Serial.print("\n---uploading overriden input subnode to RTDB ---\n");

            String path = base_path_with_id;
            path += String("config/power/");
            
            Database.update<object_t>
            (
                aClient,
                path,
                object_t(string),
                process_data,
                "input_json_override"
            );

            inputs_are_missing = false; //TODO maybe later remove this from here 
            firebase_input_override_waiting_acknowledge = true;

            int waiting_time = 5000;

            unsigned long timer = millis();

            Serial.print("\n---WAITING FOR firebase_input_override_ack----\n");
            
            while(1)
            {
                if( !firebase_input_override_waiting_acknowledge )
                {
                    //Will be ACk and printed on callback
                    Serial.print("\n---Continuing with the loop!\n");
                    wait(1000);
                    break;
                }

                else if(millis() > timer + waiting_time)
                {
                    Serial.print("\n---firebase_input_override_ack waiting time exhausted \n---Exiting and Retrying !\n");
                    return;
                } 

                else
                {
                    app.loop(); // Maintain authentication and async tasks
                    Database.loop();
                    wait(100);
                } 
            }
        }
        else
        {
            Serial.println("--- json ERROR : PAYLOAD EMPTY");
            firebase_something_changed = false; //Resetting flag for next case
            //Not continuing
            return;
        }
    }

    // CONFIG/COMMANDS
    if (firebase_ota_enabled_needs_override ||
        firebase_reset_order_needs_override   )  
    {
        //Using Generic Vars so we can reuse the code
       
        FirebaseJson json; 

        json.add("ota",ota_enabled);
        firebase_ota_enabled_needs_override = false;

        json.add("reset", firebase_reset_order);
        firebase_reset_order_needs_override = false;

        String string;
        json.toString(string, false); // false = minified JSON  , true = pretiffy

        Serial.print("\n---overriden json to upload:");
        Serial.println(string);
        //Serial.print("\n---Payload length: ");
        //Serial.println(string.length());
        //Serial.println();

        if (string.length() > 2)
        {
            Serial.print("\n---uploading overriden input subnode to RTDB ---\n");

            String path = base_path_with_id;
            path += String("config/commands/");
            
            Database.update<object_t>
            (
                aClient,
                path,
                object_t(string),
                process_data,
                "input_json_override"
            );

            inputs_are_missing = false; //TODO maybe later remove this from here 
            firebase_input_override_waiting_acknowledge = true;

            int waiting_time = 5000;

            unsigned long timer = millis();

            Serial.print("\n---WAITING FOR firebase_input_override_ack----\n");
            
            while(1)
            {
                if( !firebase_input_override_waiting_acknowledge )
                {
                    //Will be ACk and printed on callback
                    Serial.print("\n---Continuing with the loop!\n");
                    wait(1000);
                    break;
                }

                else if(millis() > timer + waiting_time)
                {
                    Serial.print("\n---firebase_input_override_ack waiting time exhausted \n---Exiting and Retrying !\n");
                    return;
                } 

                else
                {
                    app.loop(); // Maintain authentication and async tasks
                    Database.loop();
                    wait(100);
                } 
            }
        }
        else
        {
            Serial.println("--- json ERROR : PAYLOAD EMPTY");
            firebase_something_changed = false; //Resetting flag for next case
            //Not continuing
            return;
        }
    }

    // CONFIG/LTE
    if (firebase_lte_retries_before_key_turn_needs_override ||
        firebase_lte_retries_before_esp_reset_needs_override  )     
    {
        //Using Generic Vars so we can reuse the code

        FirebaseJson json; 
       
        json.add("retries_before_key_turn",lte_retries_before_key_turn);
        firebase_lte_retries_before_key_turn_needs_override = false;

        json.add("retries_before_esp_reset",lte_retries_before_esp_reset);
        firebase_lte_retries_before_esp_reset_needs_override = false;

        String string;
        json.toString(string, false); // false = minified JSON  , true = pretiffy

        Serial.print("\n---overriden json to upload:");
        Serial.println(string);
        //Serial.print("\n---Payload length: ");
        //Serial.println(string.length());
        //Serial.println();

        if (string.length() > 2)
        {
            Serial.print("\n---uploading overriden input subnode to RTDB ---\n");

            String path = base_path_with_id;
            path += String("config/lte/");
            
            Database.update<object_t>
            (
                aClient,
                path,
                object_t(string),
                process_data,
                "input_json_override"
            );

            inputs_are_missing = false; //TODO maybe later remove this from here 
            firebase_input_override_waiting_acknowledge = true;

            int waiting_time = 5000;

            unsigned long timer = millis();

            Serial.print("\n---WAITING FOR firebase_input_override_ack----\n");
            
            while(1)
            {
                if( !firebase_input_override_waiting_acknowledge )
                {
                    //Will be ACk and printed on callback
                    Serial.print("\n---Continuing with the loop!\n");
                    wait(1000);
                    break;
                }

                else if(millis() > timer + waiting_time)
                {
                    Serial.print("\n---firebase_input_override_ack waiting time exhausted \n---Exiting and Retrying !\n");
                    return;
                } 

                else
                {
                    app.loop(); // Maintain authentication and async tasks
                    Database.loop();
                    wait(100);
                } 
            }
        }
        else
        {
            Serial.println("--- json ERROR : PAYLOAD EMPTY");
            firebase_something_changed = false; //Resetting flag for next case
            //Not continuing
            return;
        }
    }

    // CONFIG/FIREBASE
    if (firebase_retries_before_key_turn_and_esp_reset_needs_override ||
                
        firebase_update_input_interval_seconds_needs_override ||
        
        firebase_update_output_interval_seconds_needs_override ||
        
        firebase_heartbeat_update_interval_seconds_needs_override ||
            
        firebase_preferred_internet_connection_needs_override ||
        
        firebase_allow_lte_connection_needs_override ||

        firebase_allow_wifi_connection_needs_override ||
        
        firebase_mwr_update_gap_needs_override ||

        firebase_mwm_update_gap_needs_override )     
    {
        //Using Generic Vars so we can reuse the code
       
        FirebaseJson json; 

        json.add("retries_before_key_turn_and_esp_reset",firebase_retries_before_key_turn_and_esp_reset);
        firebase_retries_before_key_turn_and_esp_reset_needs_override = false;
        
        json.add("update_input_interval_seconds",firebase_update_input_interval_seconds);
        firebase_update_input_interval_seconds_needs_override = false;
        
        json.add("update_output_interval_seconds",firebase_update_output_interval_seconds);
        firebase_update_output_interval_seconds_needs_override = false;
        
        json.add("heartbeat_update_interval_seconds",firebase_heartbeat_update_interval_seconds);
        firebase_heartbeat_update_interval_seconds_needs_override =false;
        
        json.add("preferred_internet_connection",firebase_preferred_internet_connection);
        firebase_preferred_internet_connection_needs_override = false;

        json.add("allow_lte_connection",firebase_allow_lte_connection);
        firebase_allow_lte_connection_needs_override =false;

        json.add("allow_wifi_connection",firebase_allow_wifi_connection);
        firebase_allow_wifi_connection_needs_override = false;
        
        json.add("mwr_update_gap",firebase_mwr_update_gap);
        firebase_mwr_update_gap_needs_override = false;   
        
        json.add("mwm_update_gap",firebase_mwm_update_gap);
        firebase_mwm_update_gap_needs_override = false;  

        String string;
        json.toString(string, false); // false = minified JSON  , true = pretiffy

        Serial.print("\n---overriden json to upload:");
        Serial.println(string);
        //Serial.print("\n---Payload length: ");
        //Serial.println(string.length());
        //Serial.println();

        if (string.length() > 2)
        {
            Serial.print("\n---uploading overriden input subnode to RTDB ---\n");

            String path = base_path_with_id;
            path += String("config/firebase/");
            
            Database.update<object_t>
            (
                aClient,
                path,
                object_t(string),
                process_data,
                "input_json_override"
            );

            inputs_are_missing = false; //TODO maybe later remove this from here 
            firebase_input_override_waiting_acknowledge = true;

            int waiting_time = 5000;

            unsigned long timer = millis();

            Serial.print("\n---WAITING FOR firebase_input_override_ack----\n");
            
            while(1)
            {
                if( !firebase_input_override_waiting_acknowledge )
                {
                    //Will be ACk and printed on callback
                    Serial.print("\n---Continuing with the loop!\n");
                    wait(1000);
                    break;
                }

                else if(millis() > timer + waiting_time)
                {
                    Serial.print("\n---firebase_input_override_ack waiting time exhausted \n---Exiting and Retrying !\n");
                    return;
                } 

                else
                {
                    app.loop(); // Maintain authentication and async tasks
                    Database.loop();
                    wait(100);
                } 
            }
        }
        else
        {
            Serial.println("--- json ERROR : PAYLOAD EMPTY");
            firebase_something_changed = false; //Resetting flag for next case
            //Not continuing
            return;
        }
    }


    //ALL THE OTHER ONES


    // LEDS
    if (firebase_leds_status_needs_override     ||
        firebase_leds_brightness_needs_override ||
        firebase_leds_color_needs_override       )     
    {
        //Using Generic Vars so we can reuse the code
       
        FirebaseJson json; 

        json.add("on",backend_led_status);
        firebase_leds_status_needs_override = false;

        json.add("brightness",backend_led_brightness);
        firebase_leds_brightness_needs_override = false;

        json.add("color",backend_led_color_string);
        firebase_leds_color_needs_override = false;      

        String string;
        json.toString(string, false); // false = minified JSON  , true = pretiffy

        Serial.print("\n---overriden json to upload:");
        Serial.println(string);
        //Serial.print("\n---Payload length: ");
        //Serial.println(string.length());
        //Serial.println();

        if (string.length() > 2)
        {
            Serial.print("\n---uploading overriden input subnode to RTDB ---\n");

            String path = base_path_with_id;
            path += String("leds/");
            
            Database.update<object_t>
            (
                aClient,
                path,
                object_t(string),
                process_data,
                "input_json_override"
            );

            inputs_are_missing = false; //TODO maybe later remove this from here 
            firebase_input_override_waiting_acknowledge = true;

            int waiting_time = 5000;

            unsigned long timer = millis();

            Serial.print("\n---WAITING FOR firebase_input_override_ack----\n");
            
            while(1)
            {
                if( !firebase_input_override_waiting_acknowledge )
                {
                    //Will be ACk and printed on callback
                    Serial.print("\n---Continuing with the loop!\n");
                    wait(1000);
                    break;
                }

                else if(millis() > timer + waiting_time)
                {
                    Serial.print("\n---firebase_input_override_ack waiting time exhausted \n---Exiting and Retrying !\n");
                    return;
                } 

                else
                {
                    app.loop(); // Maintain authentication and async tasks
                    Database.loop();
                    wait(100);
                } 
            }
        }
        else
        {
            Serial.println("--- json ERROR : PAYLOAD EMPTY");
            firebase_something_changed = false; //Resetting flag for next case
            //Not continuing
            return;
        }
    }

    // PARKING_ALARM
    if (firebase_park_alarm_on_needs_override ||
        firebase_park_alarm_mode_needs_override ||
        firebase_park_alarm_snooze_needs_override )     
    {
        //Using Generic Vars so we can reuse the code
       
        FirebaseJson json; 

        json.add("on", backend_parking_alarm_state);
        firebase_park_alarm_on_needs_override = false;

        json.add("mode", backend_parking_alarm_mode);
        firebase_park_alarm_mode_needs_override = false;

        json.add("snooze", backend_parking_alarm_snooze); 
        firebase_park_alarm_snooze_needs_override = false;

        String string;
        json.toString(string, false); // false = minified JSON  , true = pretiffy

        Serial.print("\n---overriden json to upload:");
        Serial.println(string);
        //Serial.print("\n---Payload length: ");
        //Serial.println(string.length());
        //Serial.println();

        if (string.length() > 2)
        {
            Serial.print("\n---uploading overriden input subnode to RTDB ---\n");

            String path = base_path_with_id;
            path += String("park_alarm/");
            
            Database.update<object_t>
            (
                aClient,
                path,
                object_t(string),
                process_data,
                "input_json_override"
            );

            inputs_are_missing = false; //TODO maybe later remove this from here 
            firebase_input_override_waiting_acknowledge = true;

            int waiting_time = 5000;

            unsigned long timer = millis();

            Serial.print("\n---WAITING FOR firebase_input_override_ack----\n");
            
            while(1)
            {
                if( !firebase_input_override_waiting_acknowledge )
                {
                    //Will be ACk and printed on callback
                    Serial.print("\n---Continuing with the loop!\n");
                    wait(1000);
                    break;
                }

                else if(millis() > timer + waiting_time)
                {
                    Serial.print("\n---firebase_input_override_ack waiting time exhausted \n---Exiting and Retrying !\n");
                    return;
                } 

                else
                {
                    app.loop(); // Maintain authentication and async tasks
                    Database.loop();
                    wait(100);
                } 
            }
        }
        else
        {
            Serial.println("--- json ERROR : PAYLOAD EMPTY");
            firebase_something_changed = false; //Resetting flag for next case
            //Not continuing
            return;
        }
    }

    // ACCIDENT
    if (firebase_accident_dismissed_needs_override)     
    {
        //Here we need to update the outputs as well 
        //as they are in the same node

        //Using Generic Vars so we can reuse the code
       
        FirebaseJson json; 

        json.add("dismissed",backend_accident_dismissed);
        firebase_accident_dismissed_needs_override = false;

        //Adding as inputs although they are outputs
        json.add("confirmed",accident_confirmed);
        json.add("detected",accident_detected);

        String string;
        json.toString(string, false); // false = minified JSON  , true = pretiffy

        Serial.print("\n---overriden json to upload:");
        Serial.println(string);
        //Serial.print("\n---Payload length: ");
        //Serial.println(string.length());
        //Serial.println();

        if (string.length() > 2)
        {
            Serial.print("\n---uploading overriden input subnode to RTDB ---\n");

            String path = base_path_with_id;
            path += String("accident/");
            
            Database.update<object_t>
            (
                aClient,
                path,
                object_t(string),
                process_data,
                "input_json_override"
            );

            inputs_are_missing = false; //TODO maybe later remove this from here 
            firebase_input_override_waiting_acknowledge = true;

            int waiting_time = 5000;

            unsigned long timer = millis();

            Serial.print("\n---WAITING FOR firebase_input_override_ack----\n");
            
            while(1)
            {
                if( !firebase_input_override_waiting_acknowledge )
                {
                    //Will be ACk and printed on callback
                    Serial.print("\n---Continuing with the loop!\n");
                    wait(1000);
                    break;
                }

                else if(millis() > timer + waiting_time)
                {
                    Serial.print("\n---firebase_input_override_ack waiting time exhausted \n---Exiting and Retrying !\n");
                    return;
                } 

                else
                {
                    app.loop(); // Maintain authentication and async tasks
                    Database.loop();
                    wait(100);
                } 
            }
        }
        else
        {
            Serial.println("--- json ERROR : PAYLOAD EMPTY");
            firebase_something_changed = false; //Resetting flag for next case
            //Not continuing
            return;
        }
    }

    //Updating HW info in case is not on the RTDB
    if (firebase_hw_version_needs_override ||
        firebase_hw_variant_needs_override  )     
    {
        //Here we need to update the outputs as well 
        //as they are in the same node

        //Using Generic Vars so we can reuse the code
       
        FirebaseJson json; 
        
        json.add("hw_version",hw_version);
        firebase_hw_version_needs_override = false;
        hw_version_synced = true;

        json.add("hw_variant",hw_variant_string);
        firebase_hw_variant_needs_override = false;
        hw_variant_synced = true;   
        

        String string;
        json.toString(string, false); // false = minified JSON  , true = pretiffy

        Serial.print("\n---overriden json to upload:");
        Serial.println(string);
        //Serial.print("\n---Payload length: ");
        //Serial.println(string.length());
        //Serial.println();

        if (string.length() > 2)
        {
            Serial.print("\n---uploading overriden input subnode to RTDB ---\n");

            String path = base_path_with_id;
            path += String("hw_info/");
            
            Database.update<object_t>
            (
                aClient,
                path,
                object_t(string),
                process_data,
                "input_json_override"
            );

            inputs_are_missing = false; //TODO maybe later remove this from here 
            firebase_input_override_waiting_acknowledge = true;

            int waiting_time = 5000;

            unsigned long timer = millis();

            Serial.print("\n---WAITING FOR firebase_input_override_ack----\n");
            
            while(1)
            {
                if( !firebase_input_override_waiting_acknowledge )
                {
                    //Will be ACk and printed on callback
                    Serial.print("\n---Continuing with the loop!\n");
                    wait(1000);
                    break;
                }

                else if(millis() > timer + waiting_time)
                {
                    Serial.print("\n---firebase_input_override_ack waiting time exhausted \n---Exiting and Retrying !\n");
                    return;
                } 

                else
                {
                    app.loop(); // Maintain authentication and async tasks
                    Database.loop();
                    wait(100);
                } 
            }
        }
        else
        {
            Serial.println("--- json ERROR : PAYLOAD EMPTY");
            firebase_something_changed = false; //Resetting flag for next case
            //Not continuing
            return;
        }
    }
}

void firebase_update_outputs(int firebase_log_mode, bool force_update) 
{
    /*

    Maybe later use template?
    template<typename T>
    void setIfChanged(FirebaseJson &json, const String &path, const T &current, T &lastKnown) 
    {
        if (current != lastKnown) 
        {
            json.set(path, current);
            lastKnown = current;
        }
    }

    */

    
    if (force_update)
    {
        if (firebase_log_mode > firebase_log_mode_silent )
        {
            Serial.println("\n---Forcing an Output Update (first loop or manual override) ----");
            firebase_something_changed = true;
        }
    }
    //else Serial.println();
    
    //WE WILL DIVIDE THIS FUNCTION INTO SEGMENTS TO AVOID HAVING A DROP DUE TO A LARGE PAYLOAD
    //WE WILL UPLOAD ON BATCHES , LATER DECIDE IF IT SHOULD BE INDIVIDUAL

    FirebaseJson json; 
    
    //SENSORS----------------------------------------------------------------------------------------  
    
    //TEMP

    if( force_update || 
        board_temp > last_known_temp + firebase_temp_threshold ||
        board_temp < last_known_temp - firebase_temp_threshold ) 
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---board_temp :%d added to firebase output update---", board_temp);
        
        json.add("temp", board_temp);
                
        last_known_temp = board_temp; 
                
        //From here everyuthing stays the same for all cases on firebase_update_outputs()---------
        
        firebase_something_changed = true;  
    }

    //LUX

    if (force_update || 
        lux_val > last_known_lux + firebase_lux_threshold ||
        lux_val < last_known_lux - firebase_lux_threshold   ) 
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---lux_val :%d added to Firebase batch update---", lux_val);

        json.add("lux", lux_val);

        last_known_lux = lux_val;
                
        //From here everyuthing stays the same for all cases on firebase_update_outputs()---------
        
        firebase_something_changed = true;  
    }

    //SOC

    if (force_update || 
        bat_percent > last_known_soc + firebase_soc_threshold ||
        bat_percent < last_known_soc - firebase_soc_threshold    ) 
    {        
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---bat_percent :%d added to Firebase batch update---", bat_percent);
         
        json.add("soc", bat_percent);
                
        last_known_soc = bat_percent;
                
        //From here everyuthing stays the same for all cases on firebase_update_outputs()---------
        
        firebase_something_changed = true;  
    }

    //IF something changed on the sensors then upload the content

    if(firebase_something_changed)
    {
        String path = base_path_with_id; 
        path += String("sensors/");

        String string;
        json.toString(string, false); // false = minified JSON  , true = pretiffy

        Serial.print("\n---json to upload:");
        Serial.println(string);
        Serial.print("\n---Payload length: ");
        Serial.println(string.length());
        Serial.println();

        if (string.length() > 2)
        {
            Serial.print("\n---uploading json to RTDB ---\n");
            
            Database.update<object_t>
            (
                aClient,
                path,
                object_t(string),
                process_data,
                "output_json_update"
            );

            int waiting_time = 5000;

            unsigned long timer = millis();

            Serial.print("\n---WAITING FOR output_json_ack_received----\n");
            
            while(1)
            {
                if(output_json_ack_received)
                {
                    //Will be ACk and printed on callback

                    //Resetting Flags and payload holders upon success
                    firebase_something_changed = false;
                    json.clear();
                    //Resetting flag for next case
                    output_json_ack_received = false;
                    break;
                }

                else if(millis() > timer + waiting_time)
                {
                    Serial.print("\n---ERROR : output_json_ack_received waiting time exhausted---");
                    return;
                } 

                else
                {
                    app.loop(); // Maintain authentication and async tasks
                    Database.loop();
                    wait(100);
                } 
            }
        }
        else
        {
            Serial.println("--- json ERROR : PAYLOAD EMPTY");
            firebase_something_changed = false; //Resetting flag for next case
            //Not continuing
            return;
        }
    }

    //STATUS----------------------------------------------------------------------------------------


    //Charging 

    if (force_update || charging != last_known_charging) 
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---charging : %s added to Firebase batch update---", charging ? "true" : "false");

        json.add("charging", charging);
                 
        last_known_charging = charging;    
                
        //From here everyuthing stays the same for all cases on firebase_update_outputs()---------
        
        firebase_something_changed = true;  
    }

    //Moving

    if (force_update || moving != last_known_moving) 
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
           Serial.printf("\n---moving : %s added to Firebase batch update---", moving ? "true" : "false" );
         
        json.add("moving", moving);
        
       
        last_known_moving = moving;
                
        //From here everyuthing stays the same for all cases on firebase_update_outputs()---------
        
        firebase_something_changed = true;  
    }    

    //USB Connected

    if (force_update || usb_connected != last_known_usb_connected) 
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---usb_connected : %s added to Firebase batch update---", usb_connected ? "true" : "false");

        json.add("usb_connected", usb_connected);
        
        last_known_usb_connected = usb_connected;               

        //From here everyuthing stays the same for all cases on firebase_update_outputs()---------
        
        firebase_something_changed = true;  
    }

    //LOW_BAT

    if (force_update || low_bat != last_known_low_bat) 
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---low_bat : %s added to Firebase batch update---", low_bat ? "true" : "false");
         
        json.add("low_bat", low_bat );
        
        last_known_low_bat = low_bat;
                
        //From here everyuthing stays the same for all cases on firebase_update_outputs()---------
        
        firebase_something_changed = true;  
    }

    //MAIN STATUS //old

    if( force_update || main_status != last_known_main_status) 
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---main_status : %s added to Firebase batch update---", main_status ? "true" : "false");
 
        json.add("main_status", main_status);
        
        last_known_main_status = main_status;
                

        //From here everyuthing stays the same for all cases on firebase_update_outputs()---------
        
        firebase_something_changed = true;  
    }
   

    if( force_update || gral_status != last_known_gral_status) 
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---gral_status : %s added to Firebase batch update---", gral_status.c_str());
 
        json.add("gral_status", gral_status);
        
        last_known_gral_status = gral_status;             

        //From here everyuthing stays the same for all cases on firebase_update_outputs()---------
        
        firebase_something_changed = true;  
    }
        
    // IMU_RUNNING

    if (force_update || imu_running != last_known_imu_running) 
    {
        
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---imu_running : %s added to Firebase batch update---", imu_running ? "true" : "false");

        json.add("imu_running", imu_running);
                
        last_known_imu_running = imu_running;
                
        //From here everyuthing stays the same for all cases on firebase_update_outputs()---------
        
        firebase_something_changed = true; 
    }

    if( force_update || sd_space_total_gb_int != last_known_sd_space_total_gb) 
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---sd_space_total_gb : %d added to Firebase batch update---", sd_space_total_gb_int);
 
        json.add("sd_total_gb", sd_space_total_gb_int);
        
        last_known_sd_space_total_gb = sd_space_total_gb_int;             

        //From here everyuthing stays the same for all cases on firebase_update_outputs()---------
        
        firebase_something_changed = true;  
    }

    if( force_update || sd_space_used_percent_int != last_known_sd_space_used_percent_int) 
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---sd_space_used_percent_int : %d added to Firebase batch update---", sd_space_used_percent_int);
 
        json.add("sd_used_%", sd_space_used_percent_int);
        
        last_known_sd_space_used_percent_int = sd_space_used_percent_int;             

        //From here everyuthing stays the same for all cases on firebase_update_outputs()---------
        
        firebase_something_changed = true;  
    }  

    if( force_update || black_box_need_firebase_update) 
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---b_b_log_nr : %d added to Firebase batch update---", black_box_log_nr);
 
        json.add("bb_log_nr", black_box_log_nr);
               
        //Will lower the flag on the callback

        //From here everyuthing stays the same for all cases on firebase_update_outputs()---------
        
        firebase_something_changed = true;  
    } 
    
    //lte_signal_quality
    if (force_update || 
        lte_signal_quality > last_known_lte_signal_quality + lte_signal_quality_threshold ||
        lte_signal_quality < last_known_lte_signal_quality - lte_signal_quality_threshold ) 
    {
        //when the lte ids off then we advertise as -
        if(lte_status != lte_on_and_working)
        {
            lte_signal_quality = -1; //Unknown
        }
               
        /*
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---lte_signal_quality : %d added to Firebase batch update---", lte_signal_quality);
        */
        
        switch (lte_signal_quality) 
        {
            case -1: //Unknown
                json.add("lte_csq", "lte_off"); //"(Unknown/Not detectable)";
                break;  
            case 0:
                json.add("lte_csq", "no_signal"); //(Very poor or no signal, ≤ -113 dBm);
                break;
            case 1:
                json.add("lte_csq", "very_poor"); // (Very poor, -111 dBm);
                break;
            case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
                json.add("lte_csq", "poor");//(Poor, -109 to -91 dBm);
                break;
            case 10: case 11: case 12: case 13: case 14:
                json.add("lte_csq", "fair"); //(Fair, -89 to -81 dBm);
                break;
            case 15: case 16: case 17: case 18: case 19:
                json.add("lte_csq", "good");//(Good, -79 to -71 dBm);
                break;
            case 20: case 21: case 22: case 23: case 24:
               json.add("lte_csq", "very_good");//(Very good, -69 to -57 dBm);
                break;
            case 25: case 26: case 27: case 28: case 29: case 30:
                json.add("lte_csq", "excellent");//(Excellent, -55 to -43 dBm);
                break;
            case 31:
                json.add("lte_csq", "maximum");//(Maximum, ≥ -41 dBm)";
                break;
            case 99:
                json.add("lte_csq", "unknown");//(Unknown/Not detectable)";
                break;
            default:
                json.add("lte_csq", "error");//"(Invalid/Out of range)";
                break;
        }
         
        
        last_known_lte_signal_quality = lte_signal_quality;
                
        //From here everyuthing stays the same for all cases on firebase_update_outputs()---------
        
        firebase_something_changed = true;  
    }

    
    if(firebase_something_changed)
    {
        String path = base_path_with_id; 
        path += String("status/");

        String string;
        json.toString(string, false); // false = minified JSON  , true = pretiffy

        Serial.print("\n---json to upload:");
        Serial.println(string);
        Serial.print("\n---Payload length: ");
        Serial.println(string.length());
        Serial.println();

        if (string.length() > 2)
        {
            Serial.print("\n---uploading json to RTDB ---\n");
            
            Database.update<object_t>
            (
                aClient,
                path,
                object_t(string),
                process_data,
                "output_json_update"
            );

            int waiting_time = 5000;

            unsigned long timer = millis();

            Serial.print("\n---WAITING FOR output_json_ack_received----\n");
            
            while(1)
            {
                if(output_json_ack_received)
                {

                    //Unique case for status/bb_log_nr
                    if(black_box_need_firebase_update) black_box_need_firebase_update = false;

                    //Will be ACk and printed on callback

                    //Resetting Flags and payload holders upon success
                    firebase_something_changed = false;
                    json.clear();
                    //Resetting flag for next case
                    output_json_ack_received = false;
                    break;
                }

                else if(millis() > timer + waiting_time)
                {
                    Serial.print("\n---ERROR : output_json_ack_received waiting time exhausted---");
                    return;
                } 

                else
                {
                    app.loop(); // Maintain authentication and async tasks
                    Database.loop();
                    wait(100);
                } 
            }
        }
        else
        {
            Serial.println("--- json ERROR : PAYLOAD EMPTY");
            firebase_something_changed = false; //Resetting flag for next case
            //Not continuing
            return;
        }
    }

    //TODO add here the values on the imu

    //AFTER TRAIN ADD HERE THE OUTPUTS

    //TODO ADD THIS IF INTEREST 
    //lte/carrier,data_used,status:(use the defined ones)

    //status : last_shutdown_reason






        //SPACE USED from 00 to 100%
        //SPACE LEFT ON SD 
        //SIZE OF THE SD CARD

        //LAST COMMUNICATION WITH SERVER 

        //BB ID at tthe moment






















    //COUNTERS-------------------------------------------------------------------------------------

    //counters/ last update (time and date) , mwr(move this one), mwm

    //mwr
    if (force_update || mwr > last_known_mwr + firebase_mwr_update_gap ) 
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---mwr : %d added to Firebase batch update---", mwr);
         
        json.add("mwr", mwr);
        
        last_known_mwr = mwr; 
                
        //From here everyuthing stays the same for all cases on firebase_update_outputs()---------
        
        firebase_something_changed = true;
    }

    //Minutes Without Moving
    
    if (force_update || 
        minutes_without_moving > last_known_minutes_without_moving + firebase_mwm_update_gap ) 
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---mwm : %d added to Firebase batch update---", minutes_without_moving);
         
        json.add("mwm", minutes_without_moving);
        
        last_known_minutes_without_moving = minutes_without_moving; 
                
        //From here everything stays the same for all cases on firebase_update_outputs()---------
        
        firebase_something_changed = true;  
    }

    if(firebase_something_changed)
    {
        String path = base_path_with_id; 
        path += String("counters/");

        String string;
        json.toString(string, false); // false = minified JSON  , true = pretiffy

        Serial.print("\n---json to upload:");
        Serial.println(string);
        Serial.print("\n---Payload length: ");
        Serial.println(string.length());
        Serial.println();

        if (string.length() > 2)
        {
            Serial.print("\n---uploading json to RTDB ---\n");
            
            Database.update<object_t>
            (
                aClient,
                path,
                object_t(string),
                process_data,
                "output_json_update"
            );

            int waiting_time = 5000;

            unsigned long timer = millis();

            Serial.print("\n---WAITING FOR output_json_ack_received----\n");
            
            while(1)
            {
                if(output_json_ack_received)
                {
                    //Will be ACk and printed on callback

                    //Resetting Flags and payload holders upon success
                    firebase_something_changed = false;
                    json.clear();
                    //Resetting flag for next case
                    output_json_ack_received = false;
                    break;
                }

                else if(millis() > timer + waiting_time)
                {
                    Serial.print("\n---ERROR : output_json_ack_received waiting time exhausted---");
                    return;
                } 

                else
                {
                    app.loop(); // Maintain authentication and async tasks
                    Database.loop();
                    wait(100);
                } 
            }
        }
        else
        {
            Serial.println("--- json ERROR : PAYLOAD EMPTY");
            firebase_something_changed = false; //Resetting flag for next case
            //Not continuing
            return;
        }
    }

    //Accident Detected

    if(force_update || accident_detected != last_known_accident_detected) 
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---accident_detected : %s added to Firebase batch update---", accident_detected ? "true" : "false");
         
        json.add("detected", accident_detected);
        
        last_known_accident_detected = accident_detected;
                
        //From here everyuthing stays the same for all cases on firebase_update_outputs()---------
        
        firebase_something_changed = true;  
    }

    //Accident Confirmed

    if(force_update || accident_confirmed != last_known_accident_confirmed) 
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---accident_confirmed : %s added to Firebase batch update---", accident_confirmed ? "true" : "false");
 
       
        json.add("confirmed", accident_confirmed);
        
        
        last_known_accident_confirmed = accident_confirmed;
        
        firebase_something_changed = true;  
    }

    if(firebase_something_changed)
    {
        String path = base_path_with_id; 
        path += String("accident/");

        String string;
        json.toString(string, false); // false = minified JSON  , true = pretiffy

        Serial.print("\n---json to upload:");
        Serial.println(string);
        Serial.print("\n---Payload length: ");
        Serial.println(string.length());
        Serial.println();

        if (string.length() > 2)
        {
            Serial.print("\n---uploading json to RTDB ---\n");
            
            Database.update<object_t>
            (
                aClient,
                path,
                object_t(string),
                process_data,
                "output_json_update"
            );

            int waiting_time = 5000;

            unsigned long timer = millis();

            Serial.print("\n---WAITING FOR output_json_ack_received----\n");
            
            while(1)
            {
                if(output_json_ack_received)
                {
                    //Will be ACk and printed on callback

                    //Resetting Flags and payload holders upon success
                    firebase_something_changed = false;
                    json.clear();
                    //Resetting flag for next case
                    output_json_ack_received = false;
                    break;
                }

                else if(millis() > timer + waiting_time)
                {
                    Serial.print("\n---ERROR : output_json_ack_received waiting time exhausted---");
                    return;
                } 

                else
                {
                    app.loop(); // Maintain authentication and async tasks
                    Database.loop();
                    wait(100);
                } 
            }
        }
        else
        {
            Serial.println("--- json ERROR : PAYLOAD EMPTY");
            firebase_something_changed = false; //Resetting flag for next case
            //Not continuing
            return;
        }
    }



    //backend_parking_alarm_movement_detected

    if (force_update || backend_parking_alarm_movement_detected != last_known_parking_alarm_movement_detected) 
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---backend_parking_alarm_movement_detected : %s added to Firebase batch update---", backend_parking_alarm_movement_detected ? "true" : "false");

       
        json.add("movement", backend_parking_alarm_movement_detected);
        
        
        last_known_parking_alarm_movement_detected = backend_parking_alarm_movement_detected;
                
        
        firebase_something_changed = true;  
    }

    //backend_parking_alarm_triggered

    if (force_update || backend_parking_alarm_triggered != last_known_parking_alarm_triggered) 
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---backend_parking_alarm_triggered : %s added to Firebase batch update---", backend_parking_alarm_triggered ? "true" : "false");

       
        json.add("triggered", backend_parking_alarm_triggered);
        
        
        last_known_parking_alarm_triggered = backend_parking_alarm_triggered;
                
        
        firebase_something_changed = true;  
    }

    if(firebase_something_changed)
    {
        String path = base_path_with_id; 
        path += String("park_alarm/");

        String string;
        json.toString(string, false); // false = minified JSON  , true = pretiffy

        Serial.print("\n---json to upload:");
        Serial.println(string);
        Serial.print("\n---Payload length: ");
        Serial.println(string.length());
        Serial.println();

        if (string.length() > 2)
        {
            Serial.print("\n---uploading json to RTDB ---\n");
            
            Database.update<object_t>
            (
                aClient,
                path,
                object_t(string),
                process_data,
                "output_json_update"
            );

            int waiting_time = 5000;

            unsigned long timer = millis();

            Serial.print("\n---WAITING FOR output_json_ack_received----\n");
            
            while(1)
            {
                if(output_json_ack_received)
                {
                    //Will be ACk and printed on callback

                    //Resetting Flags and payload holders upon success
                    firebase_something_changed = false;
                    json.clear();
                    //Resetting flag for next case
                    output_json_ack_received = false;
                    break;
                }

                else if(millis() > timer + waiting_time)
                {
                    Serial.print("\n---ERROR : output_json_ack_received waiting time exhausted---");
                    return;
                } 

                else
                {
                    app.loop(); // Maintain authentication and async tasks
                    Database.loop();
                    wait(100);
                } 
            }
        }
        else
        {
            Serial.println("--- json ERROR : PAYLOAD EMPTY");
            firebase_something_changed = false; //Resetting flag for next case
            //Not continuing
            return;
        }
    }

    //GPS

    if(gps_enabled && gps_upload)
    {
        //changing the GPS threshold if the vehicle is moving
         if(moving)
        {
            firebase_gps_lat_threshold = 0.0001;
            firebase_gps_lon_threshold = 0.0001;
            firebase_gps_kph_threshold = 1;
            firebase_gps_mph_threshold = 1;
            firebase_gps_hea_threshold = 1;
            firebase_gps_alt_threshold = 1;
            firebase_gps_hdop_threshold = 10;
            firebase_gps_sat_threshold = 2;
        }
        else
        {
            firebase_gps_lat_threshold = 0.5;
            firebase_gps_lon_threshold = 0.5;
            firebase_gps_kph_threshold = 10;
            firebase_gps_mph_threshold = 10;
            //we will not update heading as it changes a lot during cycles >300 per loop
            firebase_gps_hea_threshold = 500;  
            firebase_gps_alt_threshold = 10;
            firebase_gps_hdop_threshold = 100;
            firebase_gps_sat_threshold = 5;
        }

        //gps_locked

        if (force_update || 
            gps_locked != last_known_gps_locked)
        {
            
            //Using Generic Vars so we can reuse the code----------------------------------------------
                
            if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---gps_locked : %s added to Firebase batch update---", gps_locked ? "true" : "false");

       
            json.add("locked", gps_locked);
            
            
            last_known_gps_locked = gps_locked;
                    
            
            firebase_something_changed = true;   

        }


        //gps_latitude
  
        if (force_update || 
            gps_latitude > last_known_gps_lat + firebase_gps_lat_threshold ||
            gps_latitude < last_known_gps_lat - firebase_gps_lat_threshold   )
        {
            
            //Using Generic Vars so we can reuse the code----------------------------------------------
                
            if (firebase_log_mode > firebase_log_mode_silent)
                Serial.printf("\n---gps_latitude : %f added to Firebase batch update---", gps_latitude);

            json.add("lat",gps_latitude);
            
            last_known_gps_lat = gps_latitude;
            
            firebase_something_changed = true;  

        }

        //gps_longitude

        if (force_update || 
            gps_longitude > last_known_gps_lon + firebase_gps_lon_threshold ||
            gps_longitude < last_known_gps_lon - firebase_gps_lon_threshold   ) 
        {
            //Using Generic Vars so we can reuse the code----------------------------------------------
                
            if (firebase_log_mode > firebase_log_mode_silent)
                Serial.printf("\n---gps_longitude : %f added to Firebase batch update---", gps_longitude);
            
        
            json.add("lon",gps_longitude);
            
            last_known_gps_lon = gps_longitude;
            
            firebase_something_changed = true;  

        }

        //gps_speed_kph

        if (force_update || 
            gps_speed_kmh > last_known_gps_kph + firebase_gps_kph_threshold ||
            gps_speed_kmh < last_known_gps_kph - firebase_gps_kph_threshold    )
        {
            //Using Generic Vars so we can reuse the code----------------------------------------------
                
            if (firebase_log_mode > firebase_log_mode_silent)
                Serial.printf("\n---gps_speed_kmh : %f added to Firebase batch update---", gps_speed_kmh);
            
        
            json.add("kph",gps_speed_kmh);
            
            last_known_gps_kph = gps_speed_kmh;
            
            firebase_something_changed = true;  

        }

        //gps_speed_mph 

        if (force_update || 
            gps_speed_mph > last_known_gps_mph + firebase_gps_mph_threshold ||
            gps_speed_mph < last_known_gps_mph - firebase_gps_mph_threshold   )
        {
            //Using Generic Vars so we can reuse the code----------------------------------------------
                
            if (firebase_log_mode > firebase_log_mode_silent)
                Serial.printf("\n---gps_speed_mph : %f added to Firebase batch update---", gps_speed_mph);
            
        
            json.add("mph",gps_speed_mph);
            
            last_known_gps_mph = gps_speed_mph;                
            
            firebase_something_changed = true;  
        }

        //gps_heading

        if (force_update || 
            gps_heading > last_known_gps_hea + firebase_gps_hea_threshold ||
            gps_heading < last_known_gps_hea - firebase_gps_hea_threshold  )
        {
            //Using Generic Vars so we can reuse the code----------------------------------------------
                
            if (firebase_log_mode > firebase_log_mode_silent)
                Serial.printf("\n---gps_heading : %f added to Firebase batch update---", gps_heading);
            
            json.add("hea",gps_heading);
            
            last_known_gps_hea = gps_heading;
            
            firebase_something_changed = true;  
        }

        //gps_altitude

        if (force_update || 
            gps_altitude > last_known_gps_alt + firebase_gps_alt_threshold ||
            gps_altitude < last_known_gps_alt - firebase_gps_alt_threshold  )
        {
            //Using Generic Vars so we can reuse the code----------------------------------------------
                
            if (firebase_log_mode > firebase_log_mode_silent)
                Serial.printf("\n---gps_altitude : %f added to Firebase batch update---", gps_altitude);
            
        
            json.add("alt",gps_altitude);
            
            last_known_gps_alt = gps_altitude;
                    
            firebase_something_changed = true;  

        }

        //gps_altitude

        if (force_update || 
            gps_sat_count > last_known_gps_sat + firebase_gps_sat_threshold ||
            gps_sat_count < last_known_gps_sat - firebase_gps_sat_threshold    )
        {
            //Using Generic Vars so we can reuse the code----------------------------------------------
                
            if (firebase_log_mode > firebase_log_mode_silent)
                Serial.printf("\n---gps_sat : %d added to Firebase batch update---", gps_sat_count);
            
        
            json.add("sat",gps_sat_count);
                    
            last_known_gps_sat = gps_sat_count;
                    
            firebase_something_changed = true;  

        }

        if(firebase_something_changed)
        {
            String path = base_path_with_id; 
            path += String("gps/");

            String string;
            json.toString(string, false); // false = minified JSON  , true = pretiffy

            Serial.print("\n---json to upload:");
            Serial.println(string);
            Serial.print("\n---Payload length: ");
            Serial.println(string.length());
            Serial.println();

            if (string.length() > 2)
            {
                Serial.print("\n---uploading json to RTDB ---\n");
                
                Database.update<object_t>
                (
                    aClient,
                    path,
                    object_t(string),
                    process_data,
                    "output_json_update"
                );

                int waiting_time = 5000;

                unsigned long timer = millis();

                Serial.print("\n---WAITING FOR output_json_ack_received----\n");
                
                while(1)
                {
                    if(output_json_ack_received)
                    {
                        //Will be ACk and printed on callback

                        //Resetting Flags and payload holders upon success
                        firebase_something_changed = false;
                        json.clear();
                        //Resetting flag for next case
                        output_json_ack_received = false;
                        break;
                    }

                    else if(millis() > timer + waiting_time)
                    {
                        Serial.print("\n---ERROR : output_json_ack_received waiting time exhausted---");
                        return;
                    } 

                    else
                    {
                        app.loop(); // Maintain authentication and async tasks
                        Database.loop();
                        wait(100);
                    } 
                }
            }
            else
            {
                Serial.println("--- json ERROR : PAYLOAD EMPTY");
                //Not continuing
                return;
            }
        }
    }
    
    //mubea_can_motor_power

    if (force_update || 
            (   can_enabled && can_upload && 
                ( 
                    mubea_can_motor_power > last_known_mubea_can_motor_power + firebase_mubea_can_motor_power_threshold ||
                    mubea_can_motor_power < last_known_mubea_can_motor_power - firebase_mubea_can_motor_power_threshold
                )
            )
        ) 
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---mubea_can_motor_power : %d added to Firebase batch update---", mubea_can_motor_power);
         
        json.add("motor_power",mubea_can_motor_power);
        
        last_known_mubea_can_motor_power = mubea_can_motor_power;

        firebase_something_changed = true;  

    }

    //mubea_can_motor_rpm   
    
    if (force_update || 
            (   can_enabled && can_upload && 
                ( 
                    mubea_can_motor_rpm > last_known_mubea_can_motor_rpm + firebase_mubea_can_motor_rpm_threshold ||
                    mubea_can_motor_rpm < last_known_mubea_can_motor_rpm - firebase_mubea_can_motor_rpm_threshold
                )
            )
        )
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---mubea_can_motor_rpm : %d added to Firebase batch update---", mubea_can_motor_rpm);
 
       
        json.add("motor_rpm",mubea_can_motor_rpm);
        
        last_known_mubea_can_motor_rpm = mubea_can_motor_rpm;
        
        firebase_something_changed = true;  

    }

    

    //mubea_can_motor_temp

    if (force_update || 
            (   can_enabled && can_upload && 
                ( 
                    mubea_can_motor_temp > last_known_mubea_can_motor_temp + firebase_mubea_can_motor_temp_threshold ||
                    mubea_can_motor_temp < last_known_mubea_can_motor_temp - firebase_mubea_can_motor_temp_threshold
                )
            )
        )
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---mubea_can_motor_temp : %d added to Firebase batch update---", mubea_can_motor_temp);

       
        json.add("motor_temp",mubea_can_motor_temp);
        
        last_known_mubea_can_motor_temp = mubea_can_motor_temp;
        
        firebase_something_changed = true;  

    }

    

    //mubea_can_gen_power

    if (force_update || 
            (   can_enabled && can_upload && 
                ( 
                    mubea_can_gen_power > last_known_mubea_can_gen_power + firebase_mubea_can_gen_power_threshold ||
                    mubea_can_gen_power < last_known_mubea_can_gen_power - firebase_mubea_can_gen_power_threshold
                )
            )
        )
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---mubea_can_gen_power : %d added to Firebase batch update---", mubea_can_gen_power);
 
       
        json.add("gen_power",mubea_can_gen_power);
        
        last_known_mubea_can_gen_power = mubea_can_gen_power;
        
        firebase_something_changed = true;  

    }

    //mubea_can_assist_level

    if (force_update || 
            (   can_enabled && can_upload && 
                ( 
                    mubea_can_assist_level > last_known_mubea_can_assist_level + firebase_mubea_can_assist_level_threshold ||
                    mubea_can_assist_level < last_known_mubea_can_assist_level - firebase_mubea_can_assist_level_threshold
                )
            )
        )
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---mubea_can_assist_level : %d added to Firebase batch update---", mubea_can_assist_level);

       
        json.add("assist_level",mubea_can_assist_level);
        
        last_known_mubea_can_assist_level = mubea_can_assist_level;
                
        //From here everyuthing stays the same for all cases on firebase_update_outputs()---------
        
        firebase_something_changed = true;  

    }

    //mubea_can_soc

    if (force_update || 
            (   can_enabled && can_upload && 
                ( 
                    mubea_can_soc > last_known_mubea_can_soc + firebase_mubea_can_soc_threshold ||
                    mubea_can_soc < last_known_mubea_can_soc - firebase_mubea_can_soc_threshold
                )
            )
        )
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---mubea_can_soc : %d added to Firebase batch update---", mubea_can_soc);

        json.add("soc",mubea_can_soc);
        
        last_known_mubea_can_soc = mubea_can_soc;
        
        firebase_something_changed = true;  

    }

    //mubea_can_soh

    if (force_update || 
            (   can_enabled && can_upload && 
                ( 
                    mubea_can_soh > last_known_mubea_can_soh + firebase_mubea_can_soh_threshold ||
                    mubea_can_soh < last_known_mubea_can_soh - firebase_mubea_can_soh_threshold
                )
            )
        )
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---mubea_can_soh : %d added to Firebase batch update---", mubea_can_soh);

        json.add("soh",mubea_can_soh);
        
        last_known_mubea_can_soh = mubea_can_soh;
        
        firebase_something_changed = true;  
    }

    

    //mubea_can_power

    if (force_update || 
            (   can_enabled && can_upload && 
                ( 
                    mubea_can_power > last_known_mubea_can_power + firebase_mubea_can_power_threshold ||
                    mubea_can_power < last_known_mubea_can_power - firebase_mubea_can_power_threshold
                )
            )
        )
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---mubea_can_power : %d added to Firebase batch update---", mubea_can_power);
 
        json.add("power",mubea_can_power);
        
        last_known_mubea_can_power = mubea_can_power;

        //From here everyuthing stays the same for all cases on firebase_update_outputs()---------
        
        firebase_something_changed = true;  
    }

    

    //mubea_can_voltage

    if (force_update || 
            (   can_enabled && can_upload && 
                ( 
                    mubea_can_voltage > last_known_mubea_can_voltage + firebase_mubea_can_voltage_threshold ||
                    mubea_can_voltage < last_known_mubea_can_voltage - firebase_mubea_can_voltage_threshold
                )
            )
        ) 
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---mubea_can_voltage : %d added to Firebase batch update---", mubea_can_voltage);
 
       
        json.add("voltage",mubea_can_voltage);
        
        last_known_mubea_can_voltage = mubea_can_voltage;
                
        firebase_something_changed = true;  

    }

    //mubea_can_temperature

    if (force_update || 
            (   can_enabled && can_upload && 
                ( 
                    mubea_can_temperature > last_known_mubea_can_temperature + firebase_mubea_can_temperature_threshold ||
                    mubea_can_temperature < last_known_mubea_can_temperature - firebase_mubea_can_temperature_threshold
                )
            )
        )
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---mubea_can_temperature : %d added to Firebase batch update---", mubea_can_temperature);
 
       
        json.add("temperature",mubea_can_temperature);
        
        last_known_mubea_can_temperature = mubea_can_temperature;
                 
        firebase_something_changed = true;  

    }

    //mubea_can_speed

    if (force_update || 
            (   can_enabled && can_upload && 
                ( 
                    mubea_can_speed > last_known_mubea_can_speed + firebase_mubea_can_speed_threshold ||
                    mubea_can_speed < last_known_mubea_can_speed - firebase_mubea_can_speed_threshold
                )
            )
        ) 
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---mubea_can_speed : %d added to Firebase batch update---", mubea_can_speed);
 
       
        json.add("speed",mubea_can_speed);
        
        last_known_mubea_can_speed = mubea_can_speed;
                 
        firebase_something_changed = true;  
    }

    //mubea_can_direction

    if (force_update || 
            (   can_enabled && can_upload && 
                ( 
                    mubea_can_direction > last_known_mubea_can_direction + firebase_mubea_can_direction_threshold ||
                    mubea_can_direction < last_known_mubea_can_direction - firebase_mubea_can_direction_threshold
                )
            )
        )
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---mubea_can_direction : %d added to Firebase batch update---", mubea_can_direction);

        json.add("direction",mubea_can_direction);
        
        last_known_mubea_can_direction = mubea_can_direction;
                
        firebase_something_changed = true;  

    }

    //mubea_can_gear

    if (force_update || 
            (   can_enabled && can_upload && 
                ( 
                    mubea_can_gear > last_known_mubea_can_gear + firebase_mubea_can_gear_threshold ||
                    mubea_can_gear < last_known_mubea_can_gear - firebase_mubea_can_gear_threshold
                )
            )
        )
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---mubea_can_gear : %d added to Firebase batch update---", mubea_can_gear);

       
        json.add("gear",mubea_can_gear);
        
        last_known_mubea_can_gear = mubea_can_gear;
        
        firebase_something_changed = true;  

    }

    //mubea_can_mileage

    if (force_update || 
            (   can_enabled && can_upload && 
                ( 
                    mubea_can_mileage > last_known_mubea_can_mileage + firebase_mubea_can_mileage_threshold ||
                    mubea_can_mileage < last_known_mubea_can_mileage - firebase_mubea_can_mileage_threshold
                )
            )
        )
    {
        firebase_something_changed = true;

        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---mubea_can_mileage : %d added to Firebase batch update---", mubea_can_mileage);
 
       
        json.add("mileage",mubea_can_mileage);
        
        last_known_mubea_can_mileage = mubea_can_mileage;
                 
        firebase_something_changed = true;  

    }

    //mubea_can_error_code

    if (force_update || 
            (   can_enabled && can_upload && 
                ( 
                    mubea_can_error_code > last_known_mubea_can_error_code + firebase_mubea_can_error_code_threshold ||
                    mubea_can_error_code < last_known_mubea_can_error_code - firebase_mubea_can_error_code_threshold
                )
            )
        ) 
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---mubea_can_error_code : %d added to Firebase batch update---", mubea_can_error_code);
 
       
        json.add("error_code",mubea_can_error_code);
        
        last_known_mubea_can_error_code = mubea_can_error_code;
                
        firebase_something_changed = true;  

    }

    

    //mubea_can_recuperation

    if (force_update || 
            (   can_enabled && can_upload && 
                ( 
                    mubea_can_recuperation > last_known_mubea_can_recuperation + firebase_mubea_can_recuperation_threshold ||
                    mubea_can_recuperation < last_known_mubea_can_recuperation - firebase_mubea_can_recuperation_threshold
                )
            )
        )
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Serial.printf("\n---mubea_can_recuperation : %d added to Firebase batch update---", mubea_can_recuperation);
 
       
        json.add("recuperation",mubea_can_recuperation);
         
        last_known_mubea_can_recuperation = mubea_can_recuperation;
        
        firebase_something_changed = true;  

    }

    if(firebase_something_changed)
    {
        String path = base_path_with_id; 
        path += String("can/");

        String string;
        json.toString(string, false); // false = minified JSON  , true = pretiffy

        Serial.print("\n---json to upload:");
        Serial.println(string);
        Serial.print("\n---Payload length: ");
        Serial.println(string.length());
        Serial.println();

        if (string.length() > 2)
        {
            Serial.print("\n---uploading json to RTDB ---\n");
            
            Database.update<object_t>
            (
                aClient,
                path,
                object_t(string),
                process_data,
                "output_json_update"
            );

            int waiting_time = 5000;

            unsigned long timer = millis();

            Serial.print("\n---WAITING FOR output_json_ack_received----\n");
            
            while(1)
            {
                if(output_json_ack_received)
                {
                    //Will be ACk and printed on callback

                    //Resetting Flags and payload holders upon success
                    firebase_something_changed = false;
                    json.clear();
                    //Resetting flag for next case
                    output_json_ack_received = false;
                    break;
                }

                else if(millis() > timer + waiting_time)
                {
                    Serial.print("\n---ERROR : output_json_ack_received waiting time exhausted---");
                    return;
                } 

                else
                {
                    app.loop(); // Maintain authentication and async tasks
                    Database.loop();
                    wait(100);
                } 
            }
        }
        else
        {
            Serial.println("--- json ERROR : PAYLOAD EMPTY");
            firebase_something_changed = false; //Resetting flag for next case
            //Not continuing
            return;
        }
    }
    
    //THE OUTPUTS FOR HW_INFO AND OTHERS THAT NEED TO BE SYNCED JUST ONCE ARE MANAGED AS INPUTS TO CHECK DB FIRST 
    //AND RESYNC OR IGNORE DEPENDING ON THE VALUE , HERE ARE JUST FOR THE FORCE UPDATE CASES
    //hw_version 
    if (force_update) 
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Firebase.printf("\n---hw_version : %d added to Firebase batch update ---", hw_version);

       
        json.add("hw_version",hw_version);
        
        last_known_mubea_can_motor_power = mubea_can_motor_power;
                
        firebase_something_changed = true;  

    }


    //hw_variant_string 
    if (force_update) 
    {
        //Using Generic Vars so we can reuse the code----------------------------------------------
               
        if (firebase_log_mode > firebase_log_mode_silent)
            Firebase.printf("\n---hw_variant_string : %s added to Firebase batch update---", hw_variant_string);
         
       
        json.add("hw_variant",hw_variant_string);
        
        last_known_mubea_can_motor_power = mubea_can_motor_power;
        
        firebase_something_changed = true;  
    }

    if(firebase_something_changed)
    {
        String path = base_path_with_id; 
        path += String("hw_info/");

        String string;
        json.toString(string, false); // false = minified JSON  , true = pretiffy

        Serial.print("\n---json to upload:");
        Serial.println(string);
        Serial.print("\n---Payload length: ");
        Serial.println(string.length());
        Serial.println();

        if (string.length() > 2)
        {
            Serial.print("\n---uploading json to RTDB ---\n");
            
            Database.update<object_t>
            (
                aClient,
                path,
                object_t(string),
                process_data,
                "output_json_update"
            );

            int waiting_time = 5000;

            unsigned long timer = millis();

            Serial.print("\n---WAITING FOR output_json_ack_received----\n");
            
            while(1)
            {
                if(output_json_ack_received)
                {
                    //Will be ACk and printed on callback

                    //Resetting Flags and payload holders upon success
                    firebase_something_changed = false;
                    json.clear();
                    //Resetting flag for next case
                    output_json_ack_received = false;
                    break;
                }

                else if(millis() > timer + waiting_time)
                {
                    Serial.print("\n---ERROR : output_json_ack_received waiting time exhausted---");
                    return;
                } 

                else
                {
                    app.loop(); // Maintain authentication and async tasks
                    Database.loop();
                    wait(100);
                } 
            }
        }
        else
        {
            Serial.println("--- json ERROR : PAYLOAD EMPTY");
            firebase_something_changed = false; //Resetting flag for next case
            //Not continuing
            return;
        }

        
    }

    //END OF CASES -----------------------------------------------------------------------------------------
    if(firebase_something_changed)
    {
        Serial.print("\n---OUTPUT(S) CHANGED : END OF LOOP FOR UPDATE_OUTPUT_JSON");
        
        //Resetting Flag
        firebase_something_changed = false;
        
        //Setting the Sent flag Indicationg succesful process finish
        output_json_sent_to_rtdb = true;
        
        //if we are here is because eveything was acknowledged correctly so we exit with ack_on 
        //this is for not freaking out the main loop that expect ack at the end upon succesful loop
        output_json_ack_received = true;
    }
    else 
    { 
        //if(log_enabled)Serial.print("---No Changes detected on OUTPUTS ---"); 
    }
}

void process_data(AsyncResult &aResult)
{
    /*if (!aResult.isResult())
      Firebase.printf("\n---received invalid result ---");  
      return;
    */
  
    if (aResult.isEvent())
      Firebase.printf("\n---Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());
  
    if (aResult.isDebug())
      Firebase.printf("\n---Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
      //TODO after accumulating 5 of this we will restart the LTE connection
  
    if (aResult.isError())
      Firebase.printf("\n---Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
  
    // here you get the values from the database and save them in variables if you need to use them later
    if (aResult.available())    //JUST one value per loop (else if)
    {
        // Log the task and payload
        //Firebase.printf("\n---received task from RTDB : %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
    
        // Extract the payload as a String
        String payload = aResult.c_str();
    
        //OUTPUTS UPDATED
        if      (aResult.uid() == "output_json_update")
        {
            //For the Outputs
            Serial.print("\n---output_json_update_ack received from RTDB ---\n");
            output_json_sent_to_rtdb = false; //Resetting the flag for next update
            output_json_ack_received = true;
            return;
        }
            
        //Confirmation from Overwriting Inputs
        else if (aResult.uid() == "input_json_override")
        {
            //Overriding INPUTS
            Serial.print("\n---input_json_override_ack received from RTDB ---\n");
            firebase_input_override_waiting_acknowledge = false ;
            return;
        }
            
        //FOR ALL THE INPUTS
        else if (aResult.uid() =="input_json_update")
        {
            Serial.print("\n\n---inputs_json received from RTDB---\n");

            // Fetch the full JSON object as a string
            received_input_json_string = aResult.c_str();

            input_json_needs_processing = true;

            input_json_received = true;

            return;
        }

        else if (aResult.uid() =="last_info_before_shutdown")
        {
            Serial.print("\n---last_info_before_shutdown_acknowledged received from RTDB---\n");

            last_info_before_shutdown_acknowledged = true;

            return;
        }

        else if (aResult.uid() =="last_info_before_restart")
        {
            Serial.print("\n---last_info_before_restart_acknowledged received from RTDB---\n");

            last_info_before_restart_acknowledged = true;

            return;
        }

        else if (aResult.uid() =="last_info_before_ota")
        {
            Serial.print("\n---last_info_before_ota_acknowledged received from RTDB---\n");

            last_info_before_ota_acknowledged = true;

            return;
        }


        else if (aResult.uid() =="test_payload")
        {
            Serial.print("\n---firebase_test_payload_ack received from RTDB---\n");

            firebase_test_payload_ack = true;

            return;
        }


        else
        {
            Firebase.printf("\n Unknown task: %s\n", aResult.uid().c_str());
        }   
    }
    //Firebase.printf("\n---TEST: task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());
}




//TODO make defaults for all and set them on error 

void generate_base_path() 
{
    // Ensure the chip ID is read if it hasn't been updated
    if (!esp_id) 
    {
        get_esp_id();
        wait(100);
    }

    // Generate the unique base path using the chip ID
    base_path_with_id  = "";
    base_path_with_id += base_path_no_id;
    base_path_with_id += String(esp_id);
    base_path_with_id += String("/");

    // Log the generated base path
    if (firebase_log_mode > firebase_log_mode_silent)
        Serial.printf("\n---Generated Firebase Base Path:%s\n", base_path_with_id.c_str());
}

//his will be changed for firebase_init()


//TODO If wifi is preferred but LTE is the only available then check wifi signal every 10 mins or while moving

//Either WIFI or LTE
void firebase_get_connection_mode()
{
    //THIS PART IS FOR THE CONNECTION MODE-----------------------------------------------------------

    if( !firebase_was_connected_via_wifi &&
        !firebase_was_connected_via_lte) 
    {
        Serial.println("\n---First Iteration , Setting up preferred connection config.");

        if(firebase_preferred_internet_connection == connection_via_wifi)
        {
            if(firebase_allow_wifi_connection)
            {
                firebase_connection_method = connection_via_wifi;
                Serial.print("\n---firebase_connection_method = connection_via_wifi---");
            }
            else if(firebase_allow_lte_connection)
            {
                firebase_connection_method = connection_via_lte;
                Serial.print("\n---firebase_connection_method = connection_via_lte as wifi is set to not allowed ---");
            }
        }

        else if (firebase_preferred_internet_connection == connection_via_lte)
        {
            if(firebase_allow_lte_connection)
            {
                firebase_connection_method = connection_via_lte;
                Serial.print("\n---firebase_connection_method = connection_via_lte---");
            }
            else if(firebase_allow_wifi_connection)
            {
                firebase_connection_method = connection_via_wifi;
                Serial.print("\n---firebase_connection_method = connection_via_wifi as lte is set to not allowed ---");
            }
        }

        //Handling Boith denials at the end of the loop

    }

    else if(firebase_preferred_internet_connection == connection_via_wifi && 
            firebase_was_connected_via_wifi && 
            firebase_allow_wifi_connection     )
    {
        //Retrying WIFI
        Serial.println("\n---Retrying WIFI connetion as preferred---");
        firebase_connection_method = connection_via_wifi; 

        //TODO later set a max_tries for switch to Fallback
    }

    else if(firebase_preferred_internet_connection == connection_via_lte && 
            firebase_was_connected_via_lte &&
            firebase_allow_lte_connection      )
    {
        //Retrying LTE
        Serial.println("\n---Retrying LTE connetion as preferred---");
        firebase_connection_method = connection_via_lte;

        //TODO later set a max_tries for switch to Fallback
    }

    else if(firebase_preferred_internet_connection == connection_via_wifi && 
            !firebase_was_connected_via_wifi                     &&
            firebase_was_connected_via_lte &&
            firebase_allow_lte_connection                        )
    {
        //Falling Back to LTE
        Serial.println("\n---Retrying LTE connetion as Fallback---");
        firebase_connection_method = connection_via_lte;
        //TODO set a check for coming back to preferred
    }

    else if(firebase_preferred_internet_connection == connection_via_lte && 
            !firebase_was_connected_via_lte &&   
            firebase_was_connected_via_wifi &&
            firebase_allow_wifi_connection                        )
    {
        //Falling Back to WIFI
        Serial.println("\n---Retrying WIFI connetion as Fallback---");
        firebase_connection_method = connection_via_wifi; 
        //TODO set a check for coming back to preferred
    }

    if (!firebase_allow_wifi_connection && !firebase_allow_lte_connection)
    {
        Serial.print("\n ---ERROR : NO OPTION ALLOWED TO CONNECT TO FIREBASE , EXITING---");
        return;

    }
    //Connecting via WIFI

    //Connecting via LTE(TODO)

}





bool firebase_init() 
{
    //TODO , PAIR ALL WITH NVS
    //if(device_is_registered && device_was_paired_with_a_user) 
    //if(all_nvs_data_refreshed)
    
    //else
    //Updating info from NVS - TEST TODO DELETE ONCE NVS IS DEPLOYED
    //update_firebase_api_key_from_nvs(nvs_log_mode_verbose);
    //update_firebase_database_url_from_nvs(nvs_log_mode_verbose);

    
    //OLD
    //TODO encapsulate this on NVS data
    //config.api_key = firebase_api_key;
    //config.database_url = firebase_database_url;


    //if (!simcom_lte_sync_time_and_set_esp32()) return false;

    //if(!simcom_lte_check_sim_card_status())    return false;
   
    //if(!simcom_lte_activate_pdp())             return false;
    
    //if(!simcom_lte_check_gprs_connected())     return false; 


    //if (log_enabled) Serial.println("\n---Testing Firebase ---");
    //if (!simcom_test_lte_firebase()) return false;

    //modem.sendAT("+CMNB=1");        // Preferred selection: CAT-M only
    //modem.sendAT("+CNMP=38");       // LTE only
    //modem.sendAT("+COPS=0");        // Automatic operator selection
    //modem.waitResponse(10000L);

    //modem.sendAT("+CBANDCFG?");
    //modem.waitResponse(10000L);

    //String netStatus = modem.getOperator();
    //Serial.println("Operator: " + netStatus);

    //int csq = modem.getSignalQuality();
    //Serial.printf("Signal quality: %d\n", csq);

    //LTE

    if(firebase_connection_method == connection_via_lte)
    {
        if (!simcom_lte_initialized)
        {
            if (log_enabled) Serial.println("\n---ERROR ON FIREBASE_INIT , SIMCOM_LTE IS NOT INIT !!---");
            return false;
        }       
        else
        {
            String name = modem.getModemName();
            DBG("Modem Name:", name);

            String modemInfo = modem.getModemInfo();
            DBG("Modem Info:", modemInfo);           

            Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);
           
            ssl_client.setInsecure();
            ssl_client.setDebugLevel(4);
            ssl_client.setBufferSizes(4096*2 /* rx */, 2048*2 /* tx */);
            //ssl_client.setTimeout(5000); // 20 seconds
            //ssl_client.setHandshakeTimeout(10); // 10 seconds
            ssl_client.setClient(&lte_client);
            ssl_client.setTimeout(10000); // Set timeout to 30 seconds
        }
    }

    //THIS PART IS FOR THE CONNECTION WITH FIREBASE -----------------------------------------------------------
    
    if (log_enabled) Serial.println("\n---Establishing connection to Firebase ---");
    //wait(1000);

    // Configure SSL client (FOR WIFI)
    //ssl_client.setInsecure();
    //ssl_client.setTimeout(1000);
    //ssl_client.setHandshakeTimeout(5);

    //lte_client.setTimeout(1000);


    for (int i = 0; i < firebase_retries_before_key_turn_and_esp_reset; i++) 
    {
        // Initialize Firebase

        if (log_enabled) Serial.print("\n---INITIALIZING APP---\n");
        initializeApp(aClient, app, getAuth(user_auth), process_data, "authTask");
        wait(1000);
        
        if (log_enabled) Serial.print("\n---ESTABLISHING APP <> RTDB ---\n");
        app.getApp<RealtimeDatabase>(Database);
        wait(1000);
        
        if(firebase_connection_method == connection_via_wifi) 
        {
            Database.url(firebase_url);
            if (log_enabled) Serial.printf("\n---Attempt : %d to connect to Firebase via WiFi ---\n", i + 1);
        }
        else if(firebase_connection_method == connection_via_lte)
        {
            Database.url(firebase_url_for_lte);
            if (log_enabled) Serial.printf("\n---Attempt : %d to connect to Firebase via LTE CAT M ---\n", i + 1);
        } 
    
        unsigned long waiting_time = millis();
        int max_wait = 10000; // max milliseconds after first app.ready() check for next attempt

        if (log_enabled) Serial.print("\n---Waiting for app.ready()");
       
        while (1)//stuck here until ready or time exhausted
        {
            // Reset the watchdog
            //esp_task_wdt_reset();            

            if (app.ready()) 
            {
                if (log_enabled) Serial.print("\n---app.ready() confirmed!--");
                firebase_connected = true;
                break; // Exit the loop if connection is successful
            }

            else if (millis() > waiting_time + max_wait)
            {
                if (log_enabled) Serial.print("\n---Waiting Time exhausted!--");
                firebase_connected = false;
                break; 
            }

            else if (log_enabled) Serial.print(".");
            wait(100);
            //wait(500);


            //else 
            //{
                /*while (ssl_client.available()) 
                {
                    char c = ssl_client.read();
                    // Optionally print or store c
                    Serial.print(c);
                }
                while (ssl_client.connected() || ssl_client.available()) 
                {
                    if (ssl_client.available()) 
                    {
                        char c = ssl_client.read();
                        Serial.write(c); // or store in a buffer if you need to parse it
                    }
                }*/
            //}
            
        }  
        if (firebase_connected) break;
    }

    if (firebase_connected) 
    {
        if (log_enabled) Serial.println("\n---Firebase Connected!--");
    
        // Generate the unique base path using the ESP32's chip ID
        generate_base_path();
        // Test reading a parameter
        
        //if(log_enabled)Serial.print("\n-----Firebase Initialized -----");

        //firebase_first_loop = true;

        return true;
    } 
    else 
    {
        if (log_enabled) Serial.printf("\n---Failed to connect to Firebase after %d retries ---", firebase_retries_before_key_turn_and_esp_reset);

        if(log_enabled)Serial.print("\n-----Saving MODEM failure and restating ESP -----");
        simcom_turn_off();
        wait(1000);

        lte_status = lte_on_but_not_working;
        nvs_set_lte_status(nvs_log_mode_moderate);

        if(log_enabled)Serial.print("\n-----Restarting ESP -----");

        wait(1000);

        ESP.restart();

        //simcom_init();

        return false;
    }
}

void handle_firebase_issues() //Also initializes 
{
    app.loop(); // Maintain authentication and async tasks
    Database.loop();//Check for Callbacks before requesting new info
    wait(100);

    if(app.ready())// If the problem is different than waiting for ready then we display an error
    {
        if(!firebase_first_loop ) Serial.print("\n---Firebase issue : ");
        else Serial.print("\n---Firebase not fully initialized:  ");
    } 
    
    //Check (TODO : and later decide) the connection Method
    // -1 is undefined
    else if (firebase_connection_method == -1) firebase_get_connection_mode();

    //Troubleshooting for WIFI
    else if ( firebase_connection_method == connection_via_wifi && WiFi.status() != WL_CONNECTED) 
    {
        if(WiFi.status() != WL_CONNECTED)
        {
            
            if (firebase_first_loop) Serial.println("\n---Connecting to WiFi ---");
            else Serial.println("---WiFi not connected ---");
            
            /*
            if(firebase_first_loop) 
            {
                //TO DO , USE THIS IF NEED TO CHANGE WIFI ON THE SPOT
                //POINT A LAMP TO CONNECT TO HQ , OTHERWISE TO MOTIONLAB
                
                if(lux_get() > 1000) 
                {
                    Serial.print("\n Connecting to HQ");
                    wifi_ssid = wifi_ssid_hq;
                    wifi_pass = wifi_pass_hq; 
                }
                else
                {
                    Serial.print("Connecting to MotionLab");
                    wifi_ssid = wifi_ssid_motionlab;
                    wifi_pass = wifi_pass_motionlab;                    
                }
            }
            */
            
            wifi_connect();

            while(!WiFi.status() == WL_CONNECTED)
            {
                Serial.print("|");
                wait(500);
            }

        }
    }

    //LTE Troubleshootting
    else if (firebase_connection_method == connection_via_lte && !simcom_lte_initialized)
    {
        //Checking previous status and deciding based on NVS info
        switch(nvs_get_lte_status(nvs_log_mode_moderate))
        {
            case lte_nvs_info_not_found:
            {
                if (firebase_first_loop) Serial.println("\n---LTE status not found in NVS, initializing LTE ---");
                else Serial.println("---LTE status not found in NVS, reinitializing LTE ---");
                //Proceed underneath
                break;
            }
            case lte_off_due_to_hard_reset:
            {
                if (firebase_first_loop) Serial.println("\n---LTE was turned off due to hard reset, reinitializing LTE ---");
                else Serial.println("---LTE was turned off due to hard reset, reinitializing LTE ---");
                //Proceed underneath
                break;
            }
            case lte_off_due_to_deep_sleep:
            {
                if (firebase_first_loop) Serial.println("\n---LTE was turned off due to deep sleep, reinitializing LTE ---");
                else Serial.println("---LTE was turned off due to deep sleep, reinitializing LTE ---");
                //Proceed underneath
                break;
            }
            case lte_on_but_not_working:
            {
                if (firebase_first_loop) Serial.println("\n---LTE was on but not working, reinitializing LTE ---");
                else Serial.println("---LTE was on but not working, reinitializing LTE ---");
                //Proceed underneath
                break;
            }
            case lte_on_and_working:
            {
                if (firebase_first_loop) Serial.println("\n---LTE was on and working, running initiallization ---");
                else Serial.println("---LTE was on and working, running initialization ----");

                //simcom_initialized = true; // SIMCOM was already initialized but needs to init communication
                //simcom_lte_initialized = true; // LTE was already initialized
                //firebase_connected = true; // Firebase is already connected
                //firebase_initialized = true; // Firebase is already initialized
                break;
            }
        }

        //Testing LTE and if it responds ok then we will keep using it
      
        //GENERAL MODULE INITIALIZATION
        if(!simcom_initialized)
        {
            simcom_initialized = simcom_init();
        }

        //LTE MODULE INITIALIZATION (REMEMBER THERE IS ALSO A GPS MODULE)
        if(simcom_initialized && !simcom_lte_initialized)
        {
            if(lte_status == lte_on_and_working)
            { 
                //TODO continue improving minimal init

                Serial.println("SIMCOM full initialization...");
                
                if (simcom_lte_init())
                {
                    Serial.println("LTE Cat-M connection established successfully!");
                    simcom_lte_initialized = true;
                } 
                else 
                {
                    Serial.println("Failed to initialize simcom lte cat-m connection.");
                    simcom_lte_initialized = false;
                    wait(1000);
                    Serial.println("--Restarting ESP---");
                    ESP.restart();
                }

                /*
                if(simcom_lte_init_minimal())
                {
                    Serial.println("LTE Cat-M connection established successfully via simcom_lte_init_minimal!");
                    simcom_lte_initialized = true;
                }
                else 
                {
                    Serial.println("Failed to initialize simcom lte cat-m connection via simcom_lte_init_minimal.");
                    Serial.println("Trying full initialization...");

                    if (simcom_lte_init())
                    {
                        Serial.println("LTE Cat-M connection established successfully!");
                        simcom_lte_initialized = true;
                    } 
                    else 
                    {
                        Serial.println("Failed to initialize simcom lte cat-m connection.");
                        simcom_lte_initialized = false;
                        wait(1000);
                    }
                }*/
            }
            else //FULL INIT , JUST FOR THE FIRST TIME OR ON A NON-PREVIOUSLY WORKING LTE
            {
                Serial.println("SIMCOM full initialization...");
                
                if (simcom_lte_init())
                {
                    Serial.println("LTE Cat-M connection established successfully!");
                    simcom_lte_initialized = true;
                } 
                else 
                {
                    Serial.println("Failed to initialize simcom lte cat-m connection.");
                    simcom_lte_initialized = false;
                    wait(1000);
                    Serial.println("--Restarting ESP---");
                    ESP.restart();
                }
            }
        }        
    }

    else if (!firebase_initialized) 
    {
        if (firebase_first_loop)
        {
            if (log_enabled) Serial.println("\n---Initializing Firebase ---");
        }
        else 
        {
            if(log_enabled) Serial.println("\n---Firebase not initialized ---");
        }    
        
        
        int retry_count = 0;

        while (retry_count < lte_retries_before_esp_reset) 
        {
            if (firebase_init()) 
            {
                Serial.println("\nFirebase initialized successfully.");
                firebase_initialized = true;
                break;
            } 
            else 
            {
                Serial.printf("Retrying Firebase initialization (%d/%d)...\n", retry_count + 1, lte_retries_before_esp_reset);
                retry_count++;
                wait(5000 * ((retry_count)/2)); // Exponential backoff
                firebase_initialized = false;
            }
            wait(1000);
        }

        if (retry_count == lte_retries_before_esp_reset) 
        {
            Serial.println("Failed to initialize Firebase after maximum retries.");
        }
    }
    
    else if(!app.ready())
    {
        if(millis() > firebase_retry_timer + 10000 )
        {
            Serial.printf("\n Firebase is not ready,  Loop Nr: %d \n", firebase_retry_count);

            if(firebase_retry_count > lte_retries_before_esp_reset)
            {
                Serial.println("\n----Restarting Firebase Connection ----\n");

                lte_status = lte_on_but_not_working; // Set LTE status to not working
                nvs_set_lte_status(nvs_log_mode_moderate); // Save the status
                
                //ssl_client.stop();
                ssl_client.stop();

                firebase_connected = false;
                firebase_initialized = false;
                firebase_retry_count = 0;

                Serial.print("\n----Restarting ESP ----\n");
                wait(100);

                ESP.restart(); // Restart the ESP32 to reset the Firebase connection //TODO check if its the best option
            }
            else
            {
                firebase_retry_count++;  
            }
            firebase_retry_timer = millis();   
        } 
        else wait(1000);   
    }

    else
    {
        if(log_enabled) Serial.print(" Unknown Issue , Relooping \n ");
        wait(1000);
    }

    app.loop(); // Maintain authentication and async tasks
    Database.loop();//Check for Callbacks before requesting new info
    wait(100);

}

void firebase_update_inputs(int firebase_log_mode)
{
    if (firebase_log_mode == firebase_log_mode_moderate) Serial.println("\n---Requesting JSON_INPUT from RTDB");
    // Async call with callback function.
    Database.get(aClient, base_path_with_id, process_data,false,"input_json_update");
    
    input_json_requested = true;
    //Database.get(aClient, base_path_with_id, process_data,"input_json_update");
    int waiting_time = 20000;

    unsigned long timer = millis();

    Serial.print("\n---WAITING FOR input_json_update_ack----\n");
    
    while(1)
    {
        if(input_json_received && input_json_needs_processing)
        {
            //Will be ACk and printed on callback
            //Serial.print("<---input_json_update_ack Received ! \n");
            break;
        }

        else if(millis() > timer + waiting_time)
        {
            Serial.print("\n---input_json_update_ack waiting time exhausted \n---Ignoring and Retrying !\n");
            break;
        } 

        else
        {
            app.loop(); // Maintain authentication and async tasks
            Database.loop();
            wait(100);
        } 
    }
}


//Firebase Task----------------------------------------------------------------------------------------------------
//Main Task to keep Firebase alive

TaskHandle_t task_firebase_handle = NULL;

void create_task_firebase() //once created it will automatically run
{
    if (task_firebase_active) 
    {
        if (log_enabled) Serial.println("\n-- Task already active, skipping creation --");
        return; // Exit if the task is already active
    }

    if(log_enabled) Serial.print("\n--creating task_firebase--");
    wait(100);
    

    task_firebase_i2c_declare();
    wait(100);

    xTaskCreate
    (
        task_firebase,   //Function Name (must be a while(1))
        "task_firebase", //Logging Name
        32768,           //Stack Size
        NULL,            //Passing Parameters
        10,              //Task Priority
        &task_firebase_handle
    );   

    task_firebase_active = true;

    //we will need the oled in developer mode so acctivating here
    oled_dev_mode_enabled = true;

    if(log_enabled && task_firebase_active) Serial.print("Task_firebase_active --\n");
}

void task_firebase_i2c_declare()
{
    if(log_enabled)Serial.print("\ntask_firebase_i2c_declared\n");
    wait(100);
    //This Taks will use the following I2C_Devs
    
    //imu_needed++;
    rgb_needed++;
    temp_needed++;
    lux_needed++;
    rtc_needed++;
    //fuelgauge_needed++;
    oled_needed++;
}

void task_firebase_i2c_release()
{
    if(log_enabled)Serial.print("\ntask_firebase_i2c_released\n");
    wait(100);
    
    //This Taks will release the following I2C_Devs
    
    //imu_needed--;
    rgb_needed--;
    temp_needed--;
    lux_needed--;
    rtc_needed--;
    //fuelgauge_needed--;
    oled_needed--;
}

void task_firebase(void * parameters)
{   
    if(log_enabled)Serial.println("\nRunning firebase_task ");
    
    if(task_mubea_running) task_mubea_running = false; //Killing task_mubea

    firebase_first_loop = true;
        
    //OUT with BTN2 (Disable for the moment on testing)
    //btn_2_interr_enable_on_press();

    // Set up the Firebase stream
    //Used by the Stream Approch / Change if using the poll approach
    //setup_firebase_stream();
    //Serial.print("\n---Firebase Stream Initialized ---");
    //wait(3000);
    
    while(1)
    {        
        //Termination Order
        if(!task_firebase_active) //Getting Out (btn_2.is_pressed Deactivated on Mubea)
        {            
            //btn_2_interr_disable();
            
            if(log_enabled) Serial.print("\n-----Destroying task_firebase! ");   

            bool result = wifi_disconnect();
            wifi_off();

            //Clearing Flags
            if(firebase_connection_method == connection_via_wifi)
            {
                wifi_connected   = false;
                firebase_initialized = false;
                firebase_connected   = false;
            }
            else if ( firebase_connection_method == connection_via_lte)
            {
                //TODO decide if here we should turn off or not
            }

            task_firebase_i2c_release();
            wait(10);

            //Returning to MUBEA
            if(!task_mubea_running)create_task_mubea();
            wait(1000);
                
            //Deleted for Mubea
            //wait_for_btn_2_release(); //will reset the btn.pressed flag also
            //wait(100);
            //oled_clear();
            //wait(100);
            //create_task_devel_menu();
            //wait(500);
            //task_firebase_active = false;
            
            vTaskDelete(NULL);//Delete itself
        }

        //Check for Terminal Orders
        if (Serial.available() > 0) 
        {
            int min_possible_range = 0;
            int max_possible_range = 1;

            String input = Serial.readStringUntil('\n'); // Read input until newline
            int input_number = input.toInt(); // Convert input to integer

            if (input_number >= min_possible_range && input_number <= max_possible_range) // Ensure valid range
            {
                Serial.printf("\n---Order No.: %d received via Serial ---\n", input_number);

                switch (input_number)
                {
                    case 0:
                        Serial.print("\n---Resetting ESP in 5 seconds ... ");

                        //Saving the current lte_status to NVS in preparation for reset
                        nvs_set_lte_status(nvs_log_mode_moderate);

                        wait(5000);
                        ESP.restart();
                    break;
                    case 1:
                        Serial.print("\n---Returning to task_mubea... ");
                        create_task_mubea();  
                        task_firebase_active = false;
                    break;                          
                    default:
                        Serial.printf("\n-- case not implemented yet , try another number");
                    break;
                }
            }
            else
            {
                Serial.printf("\n---Invalid input. Enter a number between %d and %d ---",min_possible_range, max_possible_range);
            }
        }

        if(firebase_reset_order)
        {
            buzzer_warning();

            Serial.print("\n---Reset Order Received ... ");

            //Using Generic Vars so we can reuse the code
            FirebaseJson json; 

            //Setting the reset_order to false to avoid a reset loop after connecting to firebase

            firebase_reset_order = false;

            json.add("reset",firebase_reset_order);

            //TODO ADD here other things later relevant for last info before shutdown

            String string;
            json.toString(string, false); // false = minified JSON  , true = pretiffy

            Serial.print("\n---last info before reset :");
            Serial.println(string);
            //Serial.print("\n---Payload length: ");
            //Serial.println(string.length());
            //Serial.println();

            if (string.length() > 2)
            {
                Serial.print("\n---resetting flag before reset to RTDB ---\n");

                String path = base_path_with_id;
                //TODO change this if info will be saved on other subnodes as well
                path += String("config/commands/");

                //Resetting Flag before Sending

                last_info_before_restart_acknowledged = false;

                //SENDING INFO 

                Database.update<object_t>
                (
                    aClient,
                    path,
                    object_t(string),
                    process_data,
                    "last_info_before_restart"
                );

                int waiting_time = 10000;

                unsigned long timer = millis();

                Serial.print("\n---WAITING FOR last_info_before_reset_acknowledged----\n");
                
                while(1)
                {
                    if( last_info_before_restart_acknowledged )
                    {
                        //Will be ACk and printed on callback
                        Serial.print("\n---Continuing with the reset!\n");
                        wait(1000);
                        break;
                    }

                    else if(millis() > timer + waiting_time)
                    {
                        if(last_info_before_restart_acknowledge_retry_nr < last_info_before_restart_acknowledge_max_retries )
                        {
                            Serial.print("\n---last_info_before_reset_acknowledged waiting time exhausted!---");
                            Serial.print("\n---Exiting and Retrying !\n");

                            last_info_before_restart_acknowledge_max_retries++;
                            
                            return;
                        }
                        else
                        {
                            Serial.print("\n---last_info_before_reset_acknowledged max retries reached ---!\n");
                            Serial.print("\n---Continuing with the reset without resettting flag on RTDB---\n");
                            break;
                        }
                    } 
                    else
                    {
                        app.loop(); // Maintain authentication and async tasks
                        Database.loop();
                        wait(100);
                    } 
                }
            }
            else
            {
                Serial.println("--- json ERROR : PAYLOAD EMPTY");
                //Not continuing
                return;
            }

            Serial.print("\n---Resetting ESP in 5 seconds ... ");

            //Saving the current lte_status to NVS in preparation for reset
            nvs_set_lte_status(nvs_log_mode_moderate);

            wait(5000);
            ESP.restart();
        }

        //Confirming that everything that is enabled on the firebase was actually running

        //CAN
        if(can_enabled && !task_can_active)
        {
            create_task_can();
            //wait(2000);      
        }

        //GPS
        if(gps_enabled && !task_gps_active)
        {
            create_task_gps();
            //wait(5000);       
        }

        //BLACK_BOX STARTING AFTER LTE AS IT NEEDS RTC_SYNC TO LOG CORRECTLY

        //TODO -> FIX THIS VIA RTC SO WE CAN LOG IMMEDIATELY ON BOOT

        //TODO HW CHECK THAT RTC IS WIRED TO BACKUP BAT

        // BLACK_BOX
        if(black_box_enabled && !task_sd_active)
        {
            if(rtc_calibrated)
            {
                black_box_offline_iterations = 0; //Resetting the counter in case we were looping before

                if(firebase_connection_method == connection_via_wifi)
                {
                    Serial.println("\n---Disconnecting WiFi to Enable SD Init---\n");
                    wifi_disconnect();
                    wait(1000);
                }

                //For all cases (Wifi and LTE)
                Serial.println("\n---Turning ON BLACK BOX ---\n");

                create_task_sd();

                while(!black_box_running)
                {
                    if(black_box_running)break;
                    else wait(10);
                }

                if(firebase_connection_method == connection_via_wifi)
                {
                    Serial.println("\n---Restarting WIFI---\n");//Handled by the internal loop handle_firebase_issues();
                    //wifi_connect(); 
                }
            }

            else
            {
                if(black_box_offline_iterations < black_box_offline_trigger)
                { 
                    black_box_offline_iterations++; 
                    Serial.printf("\n---BLACK_BOX NOT started : RTC not calibrated , retry nr: %d ---\n",black_box_offline_iterations);
                    //TODO maybe send a message to RTDB that black box was not enabled due to RTC not calibrated
                }
                else
                {
                    black_box_offline_iterations = 0; //Resetting the counter

                    //For all cases
                    Serial.println("\n--- black_box_offline retries Exhausted : Turning ON BLACK BOX in offline mode (Time/Date inaccurate)---\n");
                    create_task_sd();
                    black_box_offline_iterations=0; //Resetting the counter

                }
            }
        } 
               
        if(movement_monitor_enabled && !task_movement_monitor_active)
        {
            create_task_movement_monitor();
            //wait(1000);
        }

        //Checking timer
        if(movement_monitor_enabled && task_movement_monitor_active && minutes_without_moving >= mwm_before_sleep )
        {
            Serial.printf("\n---Power Saving Mode Enabled after %d minute without moving , sending last info before shutting down!", minutes_without_moving);

            //first thing we do is we immediatey send the reason to power off to the rtdb
            //TODO, we also send the time and date and last known gps before powerign off , all important stuff here 

            gral_status = "off_due_to_sleep_on_mwm";

            //Using Generic Vars so we can reuse the code
            FirebaseJson json; 

            json.add("gral_status",gral_status);

            //TODO ADD here other things later relevant for last info before shutdown

            String string;
            json.toString(string, false); // false = minified JSON  , true = pretiffy

            Serial.print("\n---last info before shutdown :");
            Serial.println(string);
            //Serial.print("\n---Payload length: ");
            //Serial.println(string.length());
            //Serial.println();

            if (string.length() > 2)
            {
                Serial.print("\n---uploading last info before shutdown to RTDB ---\n");

                String path = base_path_with_id;
                //TODO change this if info will be saved on other subnodes as well
                path += String("status/");

                //Resetting Flag before Sending

                last_info_before_shutdown_acknowledged = false;

                //SENDING INFO 

                Database.update<object_t>
                (
                    aClient,
                    path,
                    object_t(string),
                    process_data,
                    "last_info_before_shutdown"
                );               
                
               

                unsigned long timer = millis();
                int waiting_time = 10000;

                Serial.print("\n---WAITING FOR last_info_before_shutdown_acknowledged----\n");
                
                while(1)
                {
                    if( last_info_before_shutdown_acknowledged )
                    {
                        //Will be ACk and printed on callback
                        Serial.print("\n---Continuing with the shutdown!\n");
                        wait(1000);
                        break;
                    }

                    else if(millis() > timer + waiting_time)
                    {
                        if(last_info_before_shutdown_acknowledge_retry_nr < last_info_before_shutdown_acknowledge_max_retries )
                        {
                            Serial.print("\n---last_info_before_shutdown_acknowledge_retry_nr waiting time exhausted!---");
                            Serial.print("\n---Exiting and Retrying !\n");

                            last_info_before_shutdown_acknowledge_max_retries++;
                            
                            return;
                        }
                        else
                        {
                            Serial.print("\n---last_info_before_shutdown_acknowledge_retry_nr max retries reached ---!\n");
                            Serial.print("\n---Continuing with the Shutdown without last data available on RTDB---\n");
                            break;
                        }
                    } 
                    else
                    {
                        app.loop(); // Maintain authentication and async tasks
                        Database.loop();
                        wait(100);
                    } 
                }
            }
            else
            {
                Serial.println("--- json ERROR : PAYLOAD EMPTY");
                //Not continuing
                return;
            }

            //Preparing to sleep here 

            Serial.printf("\n--- Executing Shutdown Procedure ---", minutes_without_moving);

            while (1)
            {
                if(simcom_turn_off()) break;

                else wait(5000);
            }

            //Testing if the MODEM was indeed disabled

            



            wait(3000);

            if (lvl_shftr_enabled) lvl_shftr_disable();

            wait(500);

            if(reg_5v_enabled) reg_5v_disable();

            wait(500);

            rgb_leds_off(); 

            Serial.printf("\n---Power Saving Mode Enabled after %d minute without moving , shutting down now!", minutes_without_moving);
            
            wait(500);

            //TURNING OFF ALL
            ff1_q_low();

            wait(1000);

            //Should never enter here
            if(log_enabled)Serial.print("\n-- YOU SHOULD NEVER SEE THIS TEXT IF FF WORKED -- \n");
        }

        //Checking LTE signal quality and connection status
        if(firebase_connection_method == connection_via_lte && 
           lte_status == lte_on_and_working                 &&
           simcom_lte_initialized                              ) 
        {
            //Checking LTE Signal Quality
            if(millis() > last_signal_quality_check + (check_signal_quality_interval_mins*60*1000))
            {
                simcom_lte_get_signal_quality();

                last_signal_quality_check = millis();
            }
        }
               
        //All successfull , Normal Loop
        if (firebase_connected            && 
            app.ready()                   && 
            firebase_initialized          &&
            ( (firebase_connection_method == connection_via_wifi && WiFi.status() == WL_CONNECTED) ||
              (firebase_connection_method == connection_via_lte  && simcom_lte_initialized       )  ) )
        {
            //Resetting the Retry Flag        
            if( firebase_retry_count > 0) firebase_retry_count = 0;

            //Wait a bit in case the black_bosz is still booting
            if(black_box_enabled && task_sd_active && !black_box_running )
            {
                unsigned long timer = millis();
                while(1)
                {
                    if (black_box_running) break;

                    else wait(10);

                    if ( millis() > timer + 5000 )
                    {
                        Serial.print("BB_RUN timing exceeded , continuing");
                        break;
                    } 
                }
            }

            if(log_enabled && firebase_first_loop)
            {
                Serial.print("\n---Firebase First Loop now Running!");
                //force update all inputs and outputs

                //Delete this after test

                //firebase_update_inputs(firebase_log_mode);
                //wait(20000);
                //firebase_update_outputs(firebase_log_mode,true);
                //wait(20000);

                //TEST resutls shown 320 char on payload as limit before breaking the ack
                //firebase_test_payload(250,10);

            }

            //FOR THE INPUTS
            if ((millis() - firebase_last_input_update > firebase_update_input_interval_seconds * ms_to_s) || input_json_needs_processing) 
            { 
                if (xSemaphoreTake(global_vars_mutex, portMAX_DELAY) == pdTRUE)
                {
                    //Keeping the Timer Accurate
                    firebase_last_input_update = millis();

                    //Update every firebase_update_interval seconds 
                    //and just if the ouptut changed
                    //TODO Remove after test

                    //2 steps function , one time it sends and in the other it process 

                    if(!input_json_requested)
                    {
                        firebase_update_inputs(firebase_log_mode_moderate);//this will raise the input_json_requested flag
                    }                   

                    else if(input_json_received)
                    {
                        //Here we can consider a succesful two-way communication was established
                        //Saving lte_status to NVS to avoid having to restart upon reboot and lose time    
                        if(firebase_connection_method == connection_via_lte && lte_status != lte_on_and_working) 
                        {
                            lte_status = lte_on_and_working;
                            nvs_set_lte_status(nvs_log_mode_moderate);
                        } 


                        //TODO later move this to nvs
                        if (power_mode != power_mode_saving) //triggering the movement_monitor
                        {
                            power_mode = power_mode_saving;
                            movement_monitor_enabled = true;
                        }                        
                        

                        if(input_json_needs_processing) 
                        {
                            //Serial.println("\n---Processing INPUTS...");
                            firebase_process_input_json();           
                        } 

            
                        //If Device and RTDB Input Structure do not match...
                        if(inputs_are_missing)
                        {
                            if(firebase_input_override_waiting_acknowledge)
                            {
                                if(log_enabled) Serial.printf("---ERROR : firebase_input_override is still waiting for ACK from RTDB!,loop nr: %d ---\n", firebase_input_override_waiting_acknowledge_counter);
                                
                                firebase_input_override_waiting_acknowledge_counter++;

                                if(firebase_input_override_waiting_acknowledge_counter > 3)
                                {
                                    if(log_enabled) Serial.println("---firebase_input_override_waiting_acknowledge Timeout : Resetting Flag and resending firebase_input_override_json ---");
                                    
                                    firebase_input_override_waiting_acknowledge = false;
                                    firebase_input_override_waiting_acknowledge_counter = 0;                                    
                                }
                            }
                            
                            if(!firebase_input_override_waiting_acknowledge)
                            {
                                if(log_enabled) Serial.println("\n ---Overriding the inputs on RTDB ");
                                //Generate the JSON for the current flags
                                firebase_input_override();

                                //resetting counter as ack was received
                                firebase_input_override_waiting_acknowledge_counter = 0;

                            }  
                        }                                              

                        //Resetting flag to confirm reception and prepare for next request
                        input_json_requested = false;    
                        input_json_received = false;
                        input_json_received_retry_counter = 0;  
                        //firebase_update_inputs(firebase_log_mode);
                    }

                    else if (input_json_requested && !input_json_received)
                    {
                        if(input_json_received_retry_counter == 0) Serial.print("\n---Waiting for input_json_received flag---\n"); 

                        input_json_received_retry_counter++;

                        if(input_json_received_retry_counter > 3)
                        {
                            Serial.print("\n---Lowering Flag and retrying firebase_update_inputs---\n");
                            
                            input_json_requested = false;    
                            input_json_received = false;
                            input_json_received_retry_counter = 0;
                        }
                    }                 
                   
                    xSemaphoreGive(global_vars_mutex); // Release the mutex

                    wait(1000);
                }              
            }

            //FOR THE OUTPUTS

            if (millis() - firebase_last_output_update > firebase_update_output_interval_seconds * ms_to_s) 
            { 

                //Immediately upon enter to keep the timer accurate
                firebase_last_output_update = millis();

                //If the Answer is still pending then check it here
                if(output_json_sent_to_rtdb && !output_json_ack_received)
                {
                    Serial.printf("\n----ERROR : Still waiting for output_json_ack_received flag , loop nr: %d",output_json_ack_received_retry_counter);

                    output_json_ack_received_retry_counter++;

                    if(output_json_ack_received_retry_counter > 2) //TODO add to firebase this as variable input
                    {
                        Serial.print("\n---Lowering Flag and retrying firebase_update_output---\n");
                        output_json_ack_received = true;
                        output_json_ack_received_retry_counter = 0;
                    }      
                } 

                //Ack received or first enter and next iteration ready to send
                else if ((!output_json_sent_to_rtdb || output_json_ack_received) && xSemaphoreTake(global_vars_mutex, portMAX_DELAY) == pdTRUE )
                {                  
                    if(output_json_ack_received)
                    {
                        //Here we can consider a succesful two-way communication was established
                        //Saving lte_status to NVS to avoid having to restart upon reboot and lose time    
                        if(firebase_connection_method == connection_via_lte && lte_status != lte_on_and_working) 
                        {
                            lte_status = lte_on_and_working;
                            nvs_set_lte_status(nvs_log_mode_moderate);
                        } 

                        //later do it for WIFI if needed
                    }
                   
                    //Update every firebase_update_interval seconds 
                    //and just if the ouptut changed
                    //TODO Remove after test
                    if (firebase_log_mode > firebase_log_mode_silent)
                        //Serial.print("\n---Checking OUTPUTS ...");

                    //Resetting flag to confirm reception
                    output_json_ack_received = false;

                    output_json_ack_received_retry_counter = 0;
                    
                    firebase_update_outputs(firebase_log_mode,false);                           

                    //Serial.print("."); //HeartBeat for Serial Monitor     
                   
                    xSemaphoreGive(global_vars_mutex); // Release the mutex

                    wait(1000);
                }  
                else
                {
                    wait(100);
                }            
            }

            if(firebase_first_loop)
            {
                if(log_enabled)Serial.print("\n---End of First Full Firebase Loop---\n ");
                firebase_first_loop = false;
            } 

            //OLED WILLL BE HANDLED FROM WITHIN THE I2C_Manager
            
            //Serial_HeartBeat
            if(millis() - firebase_heartbeat_last_update > firebase_heartbeat_update_interval_seconds * ms_to_s)
            {
                //Keeping the Timer Accurate
                firebase_heartbeat_last_update = millis();

                firebase_heartbeat_count++;

                if (firebase_heartbeat_count > 999) firebase_heartbeat_count = 0;

                if(log_enabled)Serial.print("-");
            }
            wait(100);
        }
        else handle_firebase_issues();

        app.loop(); // Maintain authentication and async tasks
        Database.loop();
        wait(1);
    }
}

//TODO make an ESPrestart for firebase connection issues after max atttempts

/*
With the old implementation
void loop() {
  if (Firebase.RTDB.get(&fbdo, "/taillight/15840448")) {
    FirebaseJson &json = fbdo.jsonObject();
    FirebaseJsonData result;

    // Read assist_level
    json.get(result, "can/assist_level");
    int assist_level = result.to<int>();
    Serial.println("Assist Level: " + String(assist_level));

    // Read speed
    json.get(result, "can/speed");
    float speed = result.to<float>();
    Serial.println("Speed: " + String(speed));

    // Add more keys as needed...

  } else {
    Serial.println("Error getting data: " + fbdo.errorReason());
  }

  delay(10000); // Poll every 10 seconds
}

*/

//Will keep testing until ack fail to arrive
int firebase_test_payload(int char_num, int increment)
{
    FirebaseJson testJson;

    String value = "";

    Serial.printf("\n---Firebase Payload Test---");
    Serial.printf("\n---Testing Now %d chars---", char_num);

    for (int i = 0; i < char_num; ++i) value += 'x';  // n = character count

    testJson.set("payload", value);

    String payload;
    testJson.toString(payload);

    String payload_path = base_path_with_id;    
    payload_path += "/test";

    Serial.printf("Payload Length: %d , sent to %s ", payload.length(), payload_path.c_str());

    Database.update<object_t>
    (
        aClient,
        payload_path,
        object_t(payload),
        process_data,
        "test_payload"
    );

    int waiting_time = 20000;
    unsigned long timer = millis();

    Serial.print("\n---WAITING FOR firebase_test_payload_ack----\n");

    while (1)
    {
        if (firebase_test_payload_ack)
        {
            Serial.print(" Next Test! \n");
            
            //Resetting firebase_test_payload_ack before test
            firebase_test_payload_ack = false;

            // Recursive call with increased payload
            firebase_test_payload(char_num + increment, increment);
            return char_num;
        }
        else if (millis() > timer + waiting_time)
        {
            Serial.print("\n---firebase_test_payload_ack waiting time exhausted ---");
            Serial.printf("\n\n--TEST Finished with max payload of %d before fail ", payload.length());
            return char_num;
        }
        else
        {
            app.loop(); // Maintain authentication and async tasks
            Database.loop();
            wait(100);
        }
    }
}