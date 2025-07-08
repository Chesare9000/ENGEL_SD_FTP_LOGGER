#pragma once

enum
{
  firebase_log_mode_silent,
  firebase_log_mode_moderate,
  firebase_log_mode_verbose, 
};

bool firebase_init();
void generate_base_path();



//This is the structue fo the engel_main_devices
void firebase_update_engel_main_output(int firebase_topic_to_update, int firebase_log_mode);

void firebase_update_engel_main_output_all_sensors(int firebase_log_mode);


//Checck structure and initialize topic if not found
void firebase_update_engel_main_input_all(int firebase_log_mode);

//Callback-based reaction for inputs  
void firebase_check_for_input_change(int firebase_log_mode);

//Firebase Task 
//Main Task to keep Firebase alive

void create_task_firebase();
void task_firebase_i2c_declare();
void task_firebase_i2c_release();

void task_firebase(void * parameters);

//Callback Refactorized
void firebase_declare_input_callbacks(int firebase_log_mode);
// Helper function to construct paths
void construct_firebase_path(char *buffer, 
                             const char *category, 
                             const char *field);
// Function to initialize LED streams
void init_led_streams(int log_mode);
// Function to initialize Park Alarm streams
void init_park_alarm_streams(int log_mode);
// Function to initialize Accident streams
void init_accident_streams(int log_mode);

int firebase_test_payload(int char_num, int increment);

/*
THIS IS THE OLD CPP IMPLEMENTATION


#include <Arduino.h>

#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

#include <firebase.h>
#include <firestore.h>

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

unsigned int firebase_buffer = 128 ;

//firebase is always there , firestore is optional






//TODO REACTIVATE LATER - DEACTIVATED FOR MUBEA
bool firestore_enabled = false;
//This will be the trigger for the firestore wrapper
bool firestore_needs_update = false;
bool firestore_update_in_progress = false;

//These Values must be encapsulated on NVS data
char firebase_api_key[50] = "";
char firebase_database_url[100] = "";

int max_brightness = 255;
int min_brightness = 0;

int firebase_delay = 1;

bool display_heap = false;

bool firebase_initialized = false;
bool firebase_connected   = false;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool task_firebase_active = false;


char firebase_project_id[50] = "engel-dev-61ef3" ;

//callback vars for refreshing the changes in DB to the ESP

FirebaseData fbdo_in_leds_on,
             fbdo_in_leds_brightness,
             fbdo_in_leds_color,
             //fbdo_in_leds_pattern,
             fbdo_in_park_alarm_on,
             fbdo_in_park_alarm_mode,
             fbdo_in_park_alarm_snooze,
             fbdo_in_accident_dismissed,
             fbdo_in_gps_enabled,
             fbdo_in_gps_upload,
             fbdo_in_gps_refresh_seconds,
             fbdo_in_can_enabled,
             fbdo_in_can_refresh_seconds;

unsigned long time_millis = 0;
int interval_millis = 5000;

int firebase_cycle_nr =0;
int firebase_cycle_last_known =0;


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

//For Accident
bool accident_detected = false;
bool last_known_accident_detected = accident_detected;
bool accident_confirmed = false;
bool last_known_accident_confirmed = accident_confirmed;
bool accident_dismissed = false;
bool last_know_accident_dismissed = accident_dismissed;


//For Parking Alarm (TODO later set this one on NVS) so we dont dismiss upon reboot

bool parking_alarm_snooze = false;

bool last_known_parking_alarm_movement_detected = backend_parking_alarm_movement_detected;
bool last_known_parking_alarm_triggered = backend_parking_alarm_triggered;
bool last_known_parking_alarm_snooze = parking_alarm_snooze;

//HW_INFO and variant will never change during execution


//GPS

float last_known_gps_lat =  gps_latitude;
float last_known_gps_lon =  gps_longitude;
float last_known_gps_kph =  gps_speed_kmh;
float last_known_gps_mph =  gps_speed_mph;
float last_known_gps_hea =  gps_heading;
float last_known_gps_alt =  gps_altitude;

bool last_known_gps_enabled = gps_enabled;
bool last_known_gps_upload = gps_upload;
bool last_known_gps_refresh_seconds = gps_refresh_seconds;

//CAN

int last_known_can_vel = can_vel;
int last_known_can_rpm = can_rpm;
int last_known_can_odo = can_odo;
int last_known_can_soc = can_soc;

bool last_known_can_enabled = can_enabled;
bool last_known_can_refresh_seconds = can_refresh_seconds;


//In case we need to force a refresh on inputs
bool firebase_inputs_need_manual_refresh = false;

bool firebase_first_loop =  true;

void firebase_connect()
{
    //if(device_is_registered && device_was_paired_with_a_user) 
    //if(all_nvs_data_refreshed)
    
    //else
    //Updating info from NVS - TEST TODO DELETE ONCE NVS IS DEPLOYED
    update_firebase_api_key_from_nvs(nvs_log_mode_verbose);
    update_firebase_database_url_from_nvs(nvs_log_mode_verbose);

    
    if(log_enabled)Serial.println("--- Connecting ---");
    
    //TODO encapsulate this on NVS data
    config.api_key = firebase_api_key;
    config.database_url = firebase_database_url;

    //Entering as Anonymous , TODO later implement security 
    if(Firebase.signUp(&config,&auth,"",""))
    {
        if(log_enabled)Serial.println("--- Firebase Connected!--");
        firebase_connected = true;
    }
    else //Couldn't Connect
    {
        if(log_enabled)
        {
            Serial.printf("%s\n",config.signer.signupError.message.c_str());
            firebase_connected= false;
        }
    }
    //Token Generation Task Assigned
    config.token_status_callback = tokenStatusCallback; 
    config.max_token_generation_retry = 1;
    //config.signer.tokens.legacy_token = ""; // Avoid using extra authentication buffers
  
    Firebase.reconnectWiFi(true);
    wait(100);
    Firebase.begin(&config,&auth);

    //Getting the user UID might take a few seconds
    if(log_enabled) Serial.print("\nGetting User UID");
    while((auth.token.uid) == "")
    {
        Serial.print(" . ");
        wait(1000);
    }
    if(log_enabled) Serial.print("-- \n User UID Received ---");
}


// Helper function to initialize Firebase stream
bool init_firebase_stream(FirebaseData &fbdo, const char *path, int log_mode)
{
    if(log_mode > firebase_log_mode_silent) 
        Serial.printf("\nTopic: %s", path);

    if (!Firebase.RTDB.beginStream(&fbdo,path))
    {
        if (log_mode > firebase_log_mode_silent)
        {
            Serial.printf("\nError on %s: %s", path, fbdo.errorReason().c_str());
        }
        return false;
    }

    if (log_mode > firebase_log_mode_silent)
        Serial.print(" ---> OK ");

    return true;
}

//To Check Heap Size (If needed for debugging)
/*
if(display_heap && log_enabled)
{
    Serial.print("\nFree Heap Size: ");
    Serial.print(esp_get_free_heap_size());
}
*/

//Callbacks Refactor----------------------------------

// Helper function to construct paths
/*
void construct_firebase_path(char *buffer, const char *category, const char *field)
{
    snprintf(buffer, firebase_buffer, "/devices/taillight/%d/%s/%s", esp_id, category, field);
}


// Function to initialize LED streams
void init_led_streams(int log_mode)
{
    char firebase_path[firebase_buffer] = "";

    // LED On
    construct_firebase_path(firebase_path, "leds", "on");
    init_firebase_stream(fbdo_in_leds_on, firebase_path, log_mode);
    
    // LED Brightness
    construct_firebase_path(firebase_path, "leds", "brightness");
    init_firebase_stream(fbdo_in_leds_brightness, firebase_path, log_mode);
   
    // LED Color
    construct_firebase_path(firebase_path, "leds", "color");
    init_firebase_stream(fbdo_in_leds_color, firebase_path, log_mode);
}

// Function to initialize Park Alarm streams
void init_park_alarm_streams(int log_mode)
{
    char firebase_path[firebase_buffer] = "";

    // Park Alarm On
    construct_firebase_path(firebase_path, "park_alarm", "on");
    init_firebase_stream(fbdo_in_park_alarm_on, firebase_path, log_mode);

    // Park Alarm Mode
    construct_firebase_path(firebase_path, "park_alarm", "mode");
    init_firebase_stream(fbdo_in_park_alarm_mode, firebase_path, log_mode);
  
    // Park Alarm Snooze
    construct_firebase_path(firebase_path, "park_alarm", "snooze");
    init_firebase_stream(fbdo_in_park_alarm_snooze, firebase_path, log_mode);
}

// Function to initialize Accident streams
void init_accident_streams(int log_mode)
{
    char firebase_path[firebase_buffer] = "";

    // Accident Dismissed
    construct_firebase_path(firebase_path, "accident", "dismissed");
    init_firebase_stream(fbdo_in_accident_dismissed, firebase_path, log_mode);
}

// Function to initialize GPS streams
void init_gps_streams(int log_mode)
{
    char firebase_path[firebase_buffer] = "";

    // GPS_ENABLED
    construct_firebase_path(firebase_path, "config", "gps/enabled");
    init_firebase_stream(fbdo_in_gps_enabled, firebase_path, log_mode);

    // GPS_UPLOAD
    construct_firebase_path(firebase_path, "config", "gps/upload");
    init_firebase_stream(fbdo_in_gps_upload, firebase_path, log_mode);

    // GPS_REFRESH_TIME_SECONDS
    construct_firebase_path(firebase_path, "config", "gps/refresh_seconds");
    init_firebase_stream(fbdo_in_gps_refresh_seconds, firebase_path, log_mode);
}

// Function to initialize CAN streams
void init_can_streams(int log_mode)
{
    char firebase_path[firebase_buffer] = "";

    // CAN_ENABLED
    construct_firebase_path(firebase_path, "config", "can/enabled");
    init_firebase_stream(fbdo_in_can_enabled, firebase_path, log_mode);

    // CAN_REFRESH_TIME_SECONDS
    //TODO ENABLE THIS
    //construct_firebase_path(firebase_path, "config", "can/refresh_seconds");
   //init_firebase_stream(fbdo_in_can_refresh_seconds, firebase_path, log_mode);
}




// Main function to declare input callbacks
void firebase_declare_input_callbacks(int firebase_log_mode)
{

    if(firebase_log_mode > firebase_log_mode_silent) 
    {
        Serial.print("\n\n -- Updating All Inputs:\n\n");
    }

    // Populate paths first
    firebase_update_engel_main_input_all(firebase_log_mode_moderate);

    if(!esp_id) get_esp_id(); 
    
    if(firebase_log_mode > firebase_log_mode_silent) 
    
    {Serial.print("\n\n -- Creating Callback Streams:\n\n");}

    // Initialize LED, Park Alarm, and Accident streams
    //init_led_streams(firebase_log_mode);

    init_gps_streams(firebase_log_mode);

    init_can_streams(firebase_log_mode);
        
    //TODO DEBUG HEAP SIZE TO ALLOW ALL CALLBACKS

    //init_park_alarm_streams(firebase_log_mode);

    //init_accident_streams(firebase_log_mode);
}

//This is the structure fo the engel_main_devices
void firebase_update_engel_main_output(int firebase_topic_to_update, int firebase_log_mode)
{
    wait(100);
    //Read the chip_id if was not updated before
    if(!esp_id)get_esp_id(); 
    wait(100);

    //any change will cascade a refresh on Firestore as well at the end of the main loop
    //so will update just once at the end of the main task loop even if there is several changes

    //Generic Placeholder
    char firebase_path[firebase_buffer] = "";
    int char_size = 0;

    switch(firebase_topic_to_update)
    {

        //HARDWARE RELATED-------------------------------------------------------------------------

        //HW_VERSION-------------------------------------------------------------------------------
        case firebase_out_hw_info_version:
        {
                       
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/hw_info/version",esp_id);

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending hw_version: %d to Firebase ",hw_version);
                Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
            }

            
            if(Firebase.RTDB.setInt(&fbdo,firebase_path,hw_version))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---hw_version updated to : ");
                    Serial.print(hw_version);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_hw_info_version_needs_update = true;
                firestore_needs_update = true;

            }
            else 
            {
                wait(100);
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;


        //HW_VARIANT-------------------------------------------------------------------------------------
        case firebase_out_hw_info_variant:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/hw_info/variant",esp_id);

            //To get the variant
            update_hw_variant_string();

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending hw_variant_string: %d to Firebase ",hw_variant_string);
                Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
            }

            
            if(Firebase.RTDB.setString(&fbdo,firebase_path,hw_variant_string))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---hw_variant_string updated to : ");
                    Serial.print(hw_variant_string);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_hw_info_variant_needs_update = true;
                firestore_needs_update = true;
            }
            else 
            {
                wait(100);
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;


        //SENSORS RELATED -------------------------------------------------------------------------    

        //LUX -------------------------------------------------------------------------------------
        case firebase_out_sensors_lux:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/sensors/lux",esp_id);

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending lux_val: %d to Firebase ",lux_val);
                Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
            }

            
            if(Firebase.RTDB.setInt(&fbdo,firebase_path,lux_val))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---lux_val updated to : ");
                    Serial.print(lux_val);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_lux_needs_update = true;
                firestore_needs_update = true;
            }
            else 
            {
                wait(100);
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;

        //TEMP-------------------------------------------------------------------------------------
        case firebase_out_sensors_temp:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/sensors/temp",esp_id);

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending board_temp: %d to Firebase ",board_temp);
                Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
            }         
          
            
            if(Firebase.RTDB.setInt(&fbdo,firebase_path,board_temp))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---board_temp updated to : ");
                    Serial.print(board_temp);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_temp_needs_update = true;
                firestore_needs_update = true;
            }
            else 
            {
                wait(100);
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;

        //SOC -------------------------------------------------------------------------------------
        case firebase_out_sensors_soc:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/sensors/soc",esp_id);

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending bat_percent: %d to the Firebase ",bat_percent);
                Serial.printf("topic:%s , Path_size: %d chars", firebase_path,char_size);    
            }

            
            if(Firebase.RTDB.setInt(&fbdo,firebase_path,bat_percent))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---bat_percent updated to : ");
                    Serial.print(bat_percent);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_soc_needs_update = true;
                firestore_needs_update = true;

            }
            else 
            {
                wait(100);
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;

        //STATUS RELATED -----------------------------------------------------------------------------
        
        //CHARGING -----------------------------------------------------------------------------------
        case firebase_out_status_charging:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/status/charging",esp_id);

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending charging: %d to Firebase ",charging);
                Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
            }

            
            if(Firebase.RTDB.setBool(&fbdo,firebase_path,charging))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---charging updated to : ");
                    Serial.print(charging);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_charging_needs_update = true;
                firestore_needs_update = true;
            }
            else 
            {
                wait(100);
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;


        //MOVING -------------------------------------------------------------------------------------
        case firebase_out_status_moving:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/status/moving",esp_id);

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending moving: %d to Firebase ",moving);
                Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
            }

            
            if(Firebase.RTDB.setBool(&fbdo,firebase_path,moving))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---moving updated to : ");
                    Serial.print(moving);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_moving_needs_update = true;
                firestore_needs_update = true;
            }
            else 
            {
                wait(100);
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;

        //USB-CONNECTED ---------------------------------------------------------------------------------
        case firebase_out_status_usb_connected:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/status/usb_connected",esp_id);

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending usb_onnected: %d to Firebase ",usb_connected);
                Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
            }

            
            if(Firebase.RTDB.setBool(&fbdo,firebase_path,usb_connected))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---usb_connected updated to : ");
                    Serial.print(usb_connected);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_usb_connected_needs_update = true;
                firestore_needs_update = true;
            }
            else 
            {
                wait(100);
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;

        //LOW-BAT ---------------------------------------------------------------------------------
        case firebase_out_status_low_bat:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/status/low_bat",esp_id);

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending low_bat: %d to Firebase ",low_bat);
                Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
            }

            
            if(Firebase.RTDB.setBool(&fbdo,firebase_path,low_bat))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---low_bat updated to : ");
                    Serial.print(low_bat);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_low_bat_needs_update = true;
                firestore_needs_update = true;
            }
            else 
            {
                wait(100);
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;

        //MAIN STATUS ---------------------------------------------------------------------------------
        case firebase_out_status_main_status:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/status/main_status",esp_id);

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending main_status: %d to Firebase ",main_status);
                Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
            }

            
            if(Firebase.RTDB.setBool(&fbdo,firebase_path,main_status))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---main_status updated to : ");
                    Serial.print(main_status);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_main_status_needs_update = true;
                firestore_needs_update = true;
            }
            else 
            {
                wait(100);
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;

        //IMU RUNNING ---------------------------------------------------------------------------------
        case firebase_out_status_imu_running:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/status/imu_running",esp_id);

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending imu_running: %d to Firebase ",imu_running);
                Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
            }

            
            if(Firebase.RTDB.setBool(&fbdo,firebase_path,imu_running))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---imu_running updated to : ");
                    Serial.print(imu_running);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_imu_running_needs_update = true;
                firestore_needs_update = true;
            }
            else 
            {
                wait(100);
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;

        //ACCIDENT-----------------------------------------------------------------------------------------------

        //DETECTED------------------------------------------------------------------------

        case firebase_out_accident_detected:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/accident/detected",esp_id);

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending accident_detected: %d to Firebase ",accident_detected);
                Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
            }

            
            if(Firebase.RTDB.setBool(&fbdo,firebase_path,accident_detected))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---accident_detected updated to : ");
                    Serial.print(accident_detected);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_accident_detected_needs_update = true;
                firestore_needs_update = true;
            }
            else 
            {
                wait(100);
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;

        
        //CONFIRMED------------------------------------------------------------------------

        case firebase_out_accident_confirmed:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/accident/confirmed",esp_id);
            
            //int char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/accident/conf_test",esp_id);


            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending accident_confirmed: %d to Firebase ",accident_confirmed);
                Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
            }

            
            if(Firebase.RTDB.setBool(&fbdo,firebase_path,accident_confirmed))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---accident_confirmed updated to : ");
                    Serial.print(accident_confirmed);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_accident_confirmed_needs_update = true;
                firestore_needs_update = true;
            }
            else 
            {
                wait(100);
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;

        //PAK_ALARM --------------------------------------------------------------------------------------

        //PARK_ALARM_MOVEMENT_DETECTED-------------------------------------------------------------------------------
        case firebase_out_park_alarm_movement_detected:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/park_alarm/movement",esp_id);

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending parking_alarm_movement_detected: %d to Firebase ",backend_parking_alarm_movement_detected);
                Serial.printf("\nTopic: %s , Path_size: %d chars", firebase_path , char_size);    
            }

            wait(100);
           
            
            if(Firebase.RTDB.setBool(&fbdo,firebase_path,backend_parking_alarm_movement_detected))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---parking_alarm_movement_detected updated to : ");
                    Serial.print(backend_parking_alarm_movement_detected);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_park_alarm_movement_needs_update = true;
                firestore_needs_update = true;
            }
            else 
            {
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;

        //PARK_ALARM_TRIGGERED-------------------------------------------------------------------------------
        case firebase_out_park_alarm_triggered:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/park_alarm/triggered",esp_id);

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending parking_alarm_triggered: %d to Firebase ",backend_parking_alarm_triggered);
                Serial.printf("\nTopic:%s , Path_size: %d chars", firebase_path,char_size);    
            }

            wait(100);
            
            if(Firebase.RTDB.setBool(&fbdo,firebase_path,backend_parking_alarm_triggered))
            {
                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---parking_alarm_triggered updated to : ");
                    Serial.print(backend_parking_alarm_triggered);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_park_alarm_triggered_needs_update = true;
                firestore_needs_update = true;
            }
            else 
            {
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;

        //GPS RELATED-------------------------------------------------------------------------------------
        
        //GPS_LAT ----------------------------------------------------------------------------------------
        case firebase_out_gps_lat:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/gps/lat",esp_id);

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending gps_lat: %d to Firebase ",gps_latitude);
                Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
            }         
          
            
            if(Firebase.RTDB.setFloat(&fbdo,firebase_path,gps_latitude))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---gps_lat updated to : ");
                    Serial.print(gps_latitude);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_gps_lat_needs_update = true;
                firestore_needs_update = true;
            }
            else 
            {
                wait(100);
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH GPS_LAT : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;

        //GPS_LON ----------------------------------------------------------------------------------------
        case firebase_out_gps_lon:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/gps/lon",esp_id);

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending gps_lon: %d to Firebase ",gps_longitude);
                Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
            }         
          
            
            if(Firebase.RTDB.setFloat(&fbdo,firebase_path,gps_longitude))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---gps_lon updated to : ");
                    Serial.print(gps_longitude);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_gps_lon_needs_update = true;
                firestore_needs_update = true;
            }
            else 
            {
                wait(100);
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH GPS_LON : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;

        //GPS_KPH ----------------------------------------------------------------------------------------
        case firebase_out_gps_kph:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/gps/kph",esp_id);

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending gps_kph: %d to Firebase ",gps_speed_kmh);
                Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
            }         
          
            
            if(Firebase.RTDB.setFloat(&fbdo,firebase_path,gps_speed_kmh))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---gps_kph updated to : ");
                    Serial.print(gps_speed_kmh);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_gps_kph_needs_update = true;
                firestore_needs_update = true;
            }
            else 
            {
                wait(100);
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH GPS_KPH : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;

        //GPS_MPH ----------------------------------------------------------------------------------------
        case firebase_out_gps_mph:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/gps/mph",esp_id);

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending gps_mph: %d to Firebase ",gps_speed_mph);
                Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
            }         
          
            
            if(Firebase.RTDB.setFloat(&fbdo,firebase_path,gps_speed_mph))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---gps_mph updated to : ");
                    Serial.print(gps_speed_mph);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_gps_mph_needs_update = true;
                firestore_needs_update = true;
            }
            else 
            {
                wait(100);
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH GPS_MPH : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;

        //GPS_HEA ----------------------------------------------------------------------------------------
        case firebase_out_gps_hea:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/gps/hea",esp_id);

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending gps_hea: %d to Firebase ",gps_heading);
                Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
            }         
          
            
            if(Firebase.RTDB.setFloat(&fbdo,firebase_path,gps_heading))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---gps_hea updated to : ");
                    Serial.print(gps_heading);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_gps_hea_needs_update = true;
                firestore_needs_update = true;
            }
            else 
            {
                wait(100);
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH GPS_HEA : ");
                    Serial.print(fbdo.errorReason());
                }
            }
        }break;

        //GPS_ALT ----------------------------------------------------------------------------------------
        case firebase_out_gps_alt:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/gps/alt",esp_id);

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending gps_alt: %d to Firebase ",gps_altitude);
                Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
            }         
          
            
            if(Firebase.RTDB.setFloat(&fbdo,firebase_path,gps_altitude))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---gps_alt updated to : ");
                    Serial.print(gps_altitude);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_gps_alt_needs_update = true;
                firestore_needs_update = true;
            }
            else 
            {
                wait(100);
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH GPS_ALT : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;



        //CAN --------------------------------------------------------------------------------------

        //CAN_VEL ----------------------------------------------------------------------------------------
        case firebase_out_can_vel:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/can/vel",esp_id);

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending can_vel: %d to Firebase ", can_vel);
                Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
            }         
          
            
            if(Firebase.RTDB.setInt(&fbdo,firebase_path,can_vel))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---can_vel updated to : ");
                    Serial.print(can_vel);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_can_vel_needs_update = true;
                firestore_needs_update = true;
            }
            else 
            {
                wait(100);
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH CAN_VEL : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;

        //CAN_RPM ----------------------------------------------------------------------------------------
        case firebase_out_can_rpm:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/can/rpm",esp_id);

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending can_rpm: %d to Firebase ", can_rpm);
                Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
            }         
          
            
            if(Firebase.RTDB.setInt(&fbdo,firebase_path,can_rpm))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---can_rpm updated to : ");
                    Serial.print(can_rpm);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_can_rpm_needs_update = true;
                firestore_needs_update = true;
            }
            else 
            {
                wait(100);
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH CAN_RPM : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;


        //CAN_ODO ----------------------------------------------------------------------------------------
        case firebase_out_can_odo:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/can/odo",esp_id);

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending can_odo: %d to Firebase ", can_odo);
                Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
            }         
          
            
            if(Firebase.RTDB.setInt(&fbdo,firebase_path,can_odo))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---can_odo updated to : ");
                    Serial.print(can_odo);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_can_odo_needs_update = true;
                firestore_needs_update = true;
            }
            else 
            {
                wait(100);
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH CAN_ODO : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;

        //CAN_SOC ----------------------------------------------------------------------------------------
        case firebase_out_can_soc:
        {
            char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/can/soc",esp_id);

            if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
            {
                Serial.printf("\n\n---Sending can_soc: %d to Firebase ", can_soc);
                Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
            }         
          
            
            if(Firebase.RTDB.setInt(&fbdo,firebase_path,can_soc))
            {
                wait(100);

                if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---can_soc updated to : ");
                    Serial.print(can_soc);
                    Serial.print(" as ");
                    Serial.print(fbdo.dataType());
                    Serial.print(" on "); 
                    Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                }

                firestore_can_soc_needs_update = true;
                firestore_needs_update = true;
            }
            else 
            {
                wait(100);
                //Will print always as is a big error
                if(log_enabled)
                {
                    Serial.print("\nFAILED TO PUBLISH CAN_SOC : ");
                    Serial.print(fbdo.errorReason());
                }
            }

        }break;

    }
        
}


void firebase_check_for_input_change(int firebase_log_mode)
{

    if(log_enabled && firebase_first_loop && firebase_log_mode > firebase_log_mode_silent) 
    {
        Serial.print("\n ---- Checking Inputs on the DB and setting new value if changed ");
    }
    
    //Checking on the DB and setting new value if changed 

    //FOR LEDS ------------------------------------------------------------------------------------------- 

    //ON
    if(!Firebase.RTDB.readStream(&fbdo_in_leds_on))
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("ERROR on leds_on CallBack: %s " ,fbdo_in_leds_on.errorReason().c_str());
        }
    }

    if(fbdo_in_leds_on.streamAvailable())
    {
        //Refreshing the value
        if(fbdo_in_leds_on.dataType() == "boolean")
        {
            backend_led_status = fbdo_in_leds_on.boolData();
            
            if(firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.printf("\n---leds_on changed to : %d (Type : %s) via firebase", fbdo_in_leds_on.boolData(),
                                                                                   fbdo_in_leds_on.dataType());
            }      

            //Executing the change

            if(backend_led_status)
            {
              Serial.println("\n--- Turning ON LEDs via Firebase!");
              rgb_leds_on(backend_led_color,backend_led_brightness);
            }  
            else
            {
              Serial.println("\n--- Turning OFF LEDs via Firebase!");
              rgb_leds_off();
            }

            //TODO later patch individually
            
            //Raise the flag to update Firestore as well    
            firestore_leds_on_needs_update = true;  
            firestore_needs_update = true;
        }
    }
    //If the stream is unavailable we do nothing because that means that the value has not changed


    //BRIGHTNESS
    
    if(!Firebase.RTDB.readStream(&fbdo_in_leds_brightness))
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("ERROR on leds_brightness CallBack: %s " ,fbdo_in_leds_brightness.errorReason().c_str());
        }
    }
    if(fbdo_in_leds_brightness.streamAvailable())
    {
        //Refreshing the value
        if(fbdo_in_leds_brightness.dataType() == "int")
        {
            if      (fbdo_in_leds_brightness.intData() > max_brightness) backend_led_brightness = max_brightness;
            else if (fbdo_in_leds_brightness.intData() < min_brightness) backend_led_brightness = min_brightness;
            else    backend_led_brightness = fbdo_in_leds_brightness.intData();
            
            if(firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.printf("\n---leds_brightness changed to : %d (Type : %s) via firebase",backend_led_brightness,
                                                                 fbdo_in_leds_brightness.dataType());
            }


            //Executing the Order if leds are enabled

            if(backend_led_status)rgb_leds_on(backend_led_color,backend_led_brightness);

            else { if(log_enabled)Serial.printf("\n ERROR --- LEDS are OFF (backend_leds_status:%d) \n",backend_led_status);}

            //Raise the flag to update Firestore as well    
            firestore_leds_brightness_needs_update = true;  
            firestore_needs_update = true;
        }
    }

    //COLOR
     if(!Firebase.RTDB.readStream(&fbdo_in_leds_color))
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("ERROR on leds_color CallBack: %s " ,fbdo_in_leds_color.errorReason().c_str());
        }
    }
    if(fbdo_in_leds_color.streamAvailable())
    {
        //Refreshing the value
        if(fbdo_in_leds_color.dataType() == "string")
        {
            if     (fbdo_in_leds_color.stringData() == "red")   backend_led_color   = 'r';
            else if(fbdo_in_leds_color.stringData() == "green") backend_led_color = 'g';
            else if(fbdo_in_leds_color.stringData() == "blue")  backend_led_color  = 'b';
            else if(fbdo_in_leds_color.stringData() == "white") backend_led_color = 'w';
            else {Serial.print("\nColor Not Recognized,defaulting to red"); backend_led_color = 'r';}                                   
            
            if(firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.printf("\n---leds_color changed to : %c (Type : %s) via firebase",backend_led_color,
                                                                                         fbdo_in_leds_color.dataType());
            } 

            //Executing Order if LEDS are enable

            if(backend_led_status)rgb_leds_on(backend_led_color,backend_led_brightness);
            else { if(log_enabled)Serial.printf("\n ERROR --- LEDS are OFF (backend_leds_status:%d) \n",backend_led_status);}

            //Raise the flag to update Firestore as well    
            firestore_leds_color_needs_update = true;  
            firestore_needs_update = true;
        }
    }

    //PATTERN , not implemented

    //PARK_ALARM-------------------------------------------------------------------------------------

    //ON
    if(!Firebase.RTDB.readStream(&fbdo_in_park_alarm_on))
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("ERROR on park_alarm_on CallBack: %s " ,fbdo_in_park_alarm_on.errorReason().c_str());
        }
    }
    if(fbdo_in_park_alarm_on.streamAvailable())
    {
        //Refreshing the value
        if(fbdo_in_park_alarm_on.dataType() == "boolean")
        {
            backend_parking_alarm_state = fbdo_in_park_alarm_on.boolData();
            
            if(firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.printf("\n---park_alarm_on changed to : %d (Type : %s) via firebase",fbdo_in_park_alarm_on.boolData(),
                                                                 fbdo_in_park_alarm_on.dataType());
            }         
        }

        if(backend_parking_alarm_state)
        {
            //We can activate the alarm JUST if we are in parking mode
            
            if (main_status)
            {
                if(moving)
                {
                    Serial.print("\n--- ERROR : Cannot Turn on the parking_alarm_while riding! ");
                    Serial.print("\n--- Waiting to see if we stop moving soon , otherwise throw an error");

                    wait(5000);
                    
                    if(!moving )//TODO add the imu_checks here as well
                    {
                      main_status = false; 
                    }
                }

                else //Forcing change from riding to parked upon request and without having to wait still
                {
                    Serial.print("\n--- Main Status changed to parked upon request");
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
                Serial.println("\n--- Turning ON ALARM via Firebase");

                buzzer_one_tone(1000,500,1,1);
                //Here Start the alarm with the given mode
                create_task_parking_alarm();
                //Give chance for the task to be created and run before continuing
                wait(2000);
            }
        } 

        else
        {
            Serial.println("\n--- Turning OFF ALARM_ON flag on Firebase");
            wait(100);
            //The Alarm will handle all the turning off by itself
        }

        //Raise the flag to update Firestore as well    
        firestore_park_alarm_on_needs_update = true;  
        firestore_needs_update = true;

    }

    //MODE
    if(!Firebase.RTDB.readStream(&fbdo_in_park_alarm_mode))
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("ERROR on park_alarm_mode CallBack: %s " ,fbdo_in_park_alarm_mode.errorReason().c_str());
        }
    }
    if(fbdo_in_park_alarm_mode.streamAvailable())
    {
        //Refreshing the value
        if(fbdo_in_park_alarm_mode.dataType() == "boolean")
        {
            backend_parking_alarm_mode = fbdo_in_park_alarm_mode.boolData();
            
            if(firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.printf("\n---park_alarm_mode changed to : %d (Type : %s) via firebase",
                                                                 fbdo_in_park_alarm_mode.boolData(),
                                                                 fbdo_in_park_alarm_mode.dataType());
            }       

            //Displaying Change as there is no command to execute here
            if(backend_parking_alarm_mode)
            {
                if(log_enabled)Serial.println("\n--- Setting Parking Alarm to LOUD Mode via Firebase!");
            }
            else
            {
               if(log_enabled) Serial.println("\n--- Setting Parking Alarm to SILENT Mode via Firebase!"); 
            } 

            //Raise the flag to update Firestore as well    
            firestore_park_alarm_mode_needs_update = true;  
            firestore_needs_update = true;    
        }
    }

    //SNOOZE
    if(!Firebase.RTDB.readStream(&fbdo_in_park_alarm_snooze))
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("ERROR on park_alarm_snooze CallBack: %s " ,fbdo_in_park_alarm_snooze.errorReason().c_str());
        }
    }
    if(fbdo_in_park_alarm_snooze.streamAvailable())
    {
        //Refreshing the value
        if(fbdo_in_park_alarm_snooze.dataType() == "boolean")
        {
            parking_alarm_snooze = fbdo_in_park_alarm_snooze.boolData();
            
            if(firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.printf("\n---park_alarm_snooze changed to : %d (Type : %s) via firebase",
                                                                 fbdo_in_park_alarm_snooze.boolData(),
                                                                 fbdo_in_park_alarm_snooze.dataType());
            }       

            if(parking_alarm_snooze)
            {
                if(log_enabled)Serial.printf("\n--- Park_Alarm Snoozed for %d  s ", parking_alarm_snooze_time_ms/1000);
            }
            else
            {
               if(log_enabled) Serial.println("\n--- park_alarm_snooze resetted!"); 
            } 
        }
    }


    //FOR ACCIDENT ------------------------------------------------------------------------------------------- 
    
    //DISMISSED
    if(!Firebase.RTDB.readStream(&fbdo_in_accident_dismissed))
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("ERROR on accident_dismissed CallBack: %s " ,fbdo_in_accident_dismissed.errorReason().c_str());
        }
    }

    if(fbdo_in_accident_dismissed.streamAvailable())
    {
        //Refreshing the value
        if(fbdo_in_accident_dismissed.dataType() == "boolean")
        {
            //just dismissing if the flag is inactive , and the accident is detected or confirmed

            if(!accident_dismissed && ( accident_detected || accident_confirmed ))
            {
                accident_dismissed = fbdo_in_accident_dismissed.boolData();
            
                if(firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.printf("\n---accident_dismissed changed to : %d (Type : %s) via firebase", fbdo_in_accident_dismissed.boolData(),
                                                                                    fbdo_in_accident_dismissed.dataType());
                }      

                //Executing the change
                accident_detected  = false;
                accident_confirmed = false;

                //Resetting the flag
                accident_dismissed = false;
              
                //need an override to inputs
                firebase_inputs_need_manual_refresh = true;
                
                if(firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.printf("\n---accident_dismissed forcing an input manual refresh on firebase ..");
                }  

                //Raise the flag to update Firestore as well    
                firestore_accident_dismissed_needs_update = true;  
                firestore_needs_update = true;
            } 

            //If is dismissed without an accident_mode detected then is forced to reset
            else if (fbdo_in_accident_dismissed.boolData())
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
                firebase_inputs_need_manual_refresh = true;

                if(firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.printf("\n---accident_dismissed forcing an input manual refresh on firebase ..");
                } 
              }

              else 
              {
                if(log_enabled)
                {
                    Serial.print("\n Unknown!- Check further!");                
                }
              }               

            }

        }
    }
    //If the stream is unavailable we do nothing because that means that the value has not changed


    //FOR GPS ------------------------------------------------------------------------------------------- 

    //GPS_ENABLED

    if(!Firebase.RTDB.readStream(&fbdo_in_gps_enabled))
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("ERROR on gps_enabled CallBack: %s " ,fbdo_in_gps_enabled.errorReason().c_str());
        }
    }

    if(fbdo_in_gps_enabled.streamAvailable())
    {
        //Refreshing the value
        if(fbdo_in_gps_enabled.dataType() == "boolean")
        {
            gps_enabled = fbdo_in_gps_enabled.boolData();
            
            if(firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.printf("\n---gps_enabled changed to : %d (Type : %s) via firebase", fbdo_in_gps_enabled.boolData(),
                                                                                           fbdo_in_gps_enabled.dataType());
            }      

            //HERE Execute any needed change

            if(gps_enabled)
            {
                if(!task_gps_active)
                {
                    create_task_gps();
                }
            } 
            else
            {
                if(task_gps_active)
                {
                    task_gps_active = false;
                }
            }
            
            //Raise the flag to update Firestore as well    
            firestore_gps_enabled_needs_update = true;  
            firestore_needs_update = true;
        }
    }
    //If the stream is unavailable we do nothing because that means that the value has not changed

    //GPS_UPLOAD

    if(!Firebase.RTDB.readStream(&fbdo_in_gps_upload))
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("ERROR on gps_upload CallBack: %s " ,fbdo_in_gps_upload.errorReason().c_str());
        }
    }

    if(fbdo_in_gps_upload.streamAvailable())
    {
        //Refreshing the value
        if(fbdo_in_gps_upload.dataType() == "boolean")
        {
            gps_upload = fbdo_in_gps_upload.boolData();
            
            if(firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.printf("\n---gps_upload changed to : %d (Type : %s) via firebase", fbdo_in_gps_upload.boolData(),
                                                                                           fbdo_in_gps_upload.dataType());
            }      

            //HERE Execute any needed change

            if(firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.print("\n--- GPS Data Will NOT be uploaded to Firesbase ");
            }  


            
            //Raise the flag to update Firestore as well    
            firestore_gps_upload_needs_update = true;  
            firestore_needs_update = true;
        }
    }
    //If the stream is unavailable we do nothing because that means that the value has not changed

    

    //GPS_REFRESH_SECONDS

    if(!Firebase.RTDB.readStream(&fbdo_in_gps_refresh_seconds))
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("ERROR on fbdo_in_gps_refresh_seconds CallBack: %s " ,fbdo_in_gps_refresh_seconds.errorReason().c_str());
        }
    }
    if(fbdo_in_gps_refresh_seconds.streamAvailable())
    {
        //Refreshing the value
        if(fbdo_in_gps_refresh_seconds.dataType() == "int")
        {
            
            gps_refresh_seconds = fbdo_in_gps_refresh_seconds.intData();
            
            if(firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.printf("\n---gps_refresh_seconds changed to : %d (Type : %s) via firebase",fbdo_in_gps_refresh_seconds.intData(),
                                                                                                  fbdo_in_gps_refresh_seconds.dataType());
            }

            //Executing the Order if leds are enabled

            //Raise the flag to update Firestore as well    
            firestore_gps_refresh_seconds_needs_update = true;  
            firestore_needs_update = true;
        }
    }
    //If the stream is unavailable we do nothing because that means that the value has not changed


    //FOR CAN ------------------------------------------------------------------------------------------- 
    
    //CAN_ENABLED

    if(!Firebase.RTDB.readStream(&fbdo_in_can_enabled))
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("ERROR on can_enabled CallBack: %s " ,fbdo_in_can_enabled.errorReason().c_str());
        }
    }

    if(fbdo_in_can_enabled.streamAvailable())
    {
        //Refreshing the value
        if(fbdo_in_can_enabled.dataType() == "boolean")
        {
            can_enabled = fbdo_in_can_enabled.boolData();
            
            if(firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.printf("\n---can_enabled changed to : %d (Type : %s) via firebase", fbdo_in_can_enabled.boolData(),
                                                                                           fbdo_in_can_enabled.dataType());
            }      

            //HERE Execute any needed change

            if(can_enabled)
            {
                if(!task_can_active)
                {
                    create_task_can();
                }
            } 
            else
            {
                if(task_can_active)
                {
                    task_can_active = false;
                }
            }
            
            //Raise the flag to update Firestore as well    
            firestore_can_enabled_needs_update = true;  
            firestore_needs_update = true;
        }
    }
    //If the stream is unavailable we do nothing because that means that the value has not changed


    //CAN_REFRESH_SECONDS

    if(!Firebase.RTDB.readStream(&fbdo_in_can_refresh_seconds))
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("ERROR on fbdo_in_can_refresh_seconds CallBack: %s " ,fbdo_in_can_refresh_seconds.errorReason().c_str());
        }
    }
    if(fbdo_in_can_refresh_seconds.streamAvailable())
    {
        //Refreshing the value
        if(fbdo_in_can_refresh_seconds.dataType() == "int")
        {
            
            can_refresh_seconds = fbdo_in_can_refresh_seconds.intData();
            
            if(firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.printf("\n---fbdo_in_can_refresh_seconds changed to : %d (Type : %s) via firebase",fbdo_in_can_refresh_seconds.intData(),
                                                                                                          fbdo_in_can_refresh_seconds.dataType());
            }

            //Executing the Order if leds are enabled

            //Raise the flag to update Firestore as well    
            firestore_can_refresh_seconds_needs_update = true;  
            firestore_needs_update = true;
        }
    }
    //If the stream is unavailable we do nothing because that means that the value has not changed
}

void firebase_check_for_output_change(int firebase_log_mode)
{
    //Comparing if any output changed and if so updating the relevant topic

    //TODO set thresholds and timers to avoid doing this all the time

    //To check if the outputs have changed from the last cycle 

    if(log_enabled && firebase_first_loop && firebase_log_mode > firebase_log_mode_silent) 
    {
        Serial.print("\n ---- Checking Outputs on the DB and setting new value if changed ");
    }

    //For Sensors

    //Temp
    if (board_temp > last_known_temp + firebase_temp_threshold ||
        board_temp < last_known_temp - firebase_temp_threshold  )
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("\n---Temp changed to : %d -> Publishing to Firebase ",board_temp);
        }

        firebase_update_engel_main_output(firebase_out_sensors_temp,firebase_log_mode);

        last_known_temp = board_temp;
        
        firebase_cycle_nr++;

        //Raise the flag to update Firestore as well    
        firestore_temp_needs_update = true;  
        firestore_needs_update = true;
    }
    //Lux
    if (lux_val > last_known_lux + firebase_lux_threshold ||
        lux_val < last_known_lux - firebase_lux_threshold    )
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("\n---Lux changed to : %d -> Publishing to Firebase ",lux_val);
        }

        //Wont update if the riding mode is active and already checking movement behaviour (Activate if needed)
        //if(!imu_running)firebase_update_engel_main_output(firebase_out_sensors_lux,firebase_log_mode);
        
        firebase_update_engel_main_output(firebase_out_sensors_lux,firebase_log_mode);


        last_known_lux = lux_val;

        firebase_cycle_nr++;

        //Raise the flag to update Firestore as well    
        firestore_lux_needs_update = true; 
        firestore_needs_update = true;
        
    }

    //SOC
    if (bat_percent > last_known_soc + firebase_soc_threshold ||
             bat_percent < last_known_soc - firebase_soc_threshold   )
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("\n---SOC changed to : %d -> Publishing to Firebase ",bat_percent);
        }

        firebase_update_engel_main_output(firebase_out_sensors_soc,firebase_log_mode);

        last_known_soc = bat_percent;

        firebase_cycle_nr++;

        //Raise the flag to update Firestore as well    
        firestore_soc_needs_update = true; 
        firestore_needs_update = true;
    }

    //STATUS

    //CHARGING
    if(charging != last_known_charging)
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("\n---charging changed to : %d -> Publishing to Firebase ",charging);
        }

        firebase_update_engel_main_output(firebase_out_status_charging,firebase_log_mode);

        last_known_charging = charging;

        firebase_cycle_nr++;

        //Raise the flag to update Firestore as well    
        firestore_charging_needs_update = true;
        firestore_needs_update = true; 
    }

    //MOVING
    if(moving != last_known_moving)
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("\n---moving changed to : %d ",moving);
        }

        last_known_moving = moving;
        
        //Moving while on Parking Mode with the alarm disabled

        if(moving && !backend_parking_alarm_state && !main_status && !imu_running)
        {
            if(log_enabled)Serial.println("--- Changing to Riding Mode ----");

            oled_token = oled_taken;
            //if(log_enabled)Serial.println("\n---Sending to OLED---> "); 
            wait(100); 
            oled_riding_mode_enabled();
           
            //Raising Flags for Riding Mode
            main_status = true;
            if(task_imu_status == task_imu_not_needed) task_imu_status = task_imu_needed;
            
            if(!backend_led_status)
            {
                if(log_enabled) Serial.print("Forcing LEDS to get ON while on Riding Mode Initial Loop! ");
                backend_led_status = true;
                backend_led_color = 'r';
                backend_led_brightness = 100;

                //Overriding leds 
                firebase_inputs_need_manual_refresh = true;
                if(firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.printf("\n--- leds forcing an input manual refresh on firebase ..");
                } 
            } 

            wait(100);
            oled_token = oled_free;

            //After all flags were set we start the imu task to check movement
            //Here the IMU will start in riding mode to detect acidents
            if(task_imu_status == task_imu_needed) create_task_imu();   
            wait(100); 

            //Raise the flag to update Firestore as well after the imu finished booting    
            firestore_moving_needs_update = true; 
            firestore_needs_update = true;
        }

        //Already on Riding Mode but restarting from an acident dismiss 

        else if(moving && accident_dismissed && main_status && !imu_running)
        {
            if(log_enabled)Serial.println("--- Riding Mode : Dismissing Accident by Moving Again----");

            //Restarting IMU Task
            task_imu_status = task_imu_needed;
            
            if(!backend_led_status)
            {
                if(log_enabled) Serial.print("Forcing LEDS to get ON while on Riding Mode Initial Loop! ");
                backend_led_status = true;
                backend_led_color = 'r';
                backend_led_brightness = 100;
            }        

            
            //Here the IMU will start in riding mode 
            create_task_imu();      

            accident_dismissed = false;
            //Overriding Dismiss Input to start a normal riding task from here 
            firebase_inputs_need_manual_refresh = true;
            if(firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.printf("\n---leds forcing an input manual refresh on firebase ..");
            } 
            //Raise the flag to update Firestore as well    
            firestore_moving_needs_update = true;
            firestore_needs_update = true; 
        }
        
        //Wont update if the riding mode is active and already checking movement behaviour
        else if(!imu_running) 
        {
            if(log_enabled) Serial.print("-> Publishing to Firebase ");

            firebase_update_engel_main_output(firebase_out_status_moving,firebase_log_mode);

            firebase_cycle_nr++;

            //Raise the flag to update Firestore as well    
            firestore_moving_needs_update = true; 
            firestore_needs_update = true;
        }        
    }

    //USB_CONNECTED
    if(usb_connected != last_known_usb_connected)
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("\n---usb_connected changed to : %d -> Publishing to Firebase ",usb_connected);
        }

        firebase_update_engel_main_output(firebase_out_status_usb_connected,firebase_log_mode);

        last_known_usb_connected = usb_connected;

        firebase_cycle_nr++;

        //Raise the flag to update Firestore as well    
        firestore_usb_connected_needs_update = true; 
        firestore_needs_update = true;
    }

    //LOW_BAT
    if(low_bat != last_known_low_bat)
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("\n---low_bat changed to : %d -> Publishing to Firebase ",low_bat);
        }

        firebase_update_engel_main_output(firebase_out_status_low_bat,firebase_log_mode);

        last_known_low_bat = low_bat;

        firebase_cycle_nr++;

        //Raise the flag to update Firestore as well    
        firestore_low_bat_needs_update = true; 
        firestore_needs_update = true;

    }

    //MAIN_STATUS
    if(main_status != last_known_main_status)
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("\n---main_status changed to : %d -> Publishing to Firebase ",main_status);
        }

        firebase_update_engel_main_output(firebase_out_status_main_status,firebase_log_mode);

        last_known_main_status = main_status;

        firebase_cycle_nr++;

        //Raise the flag to update Firestore as well    
        firestore_main_status_needs_update = true;
        firestore_needs_update = true; 
    }

    //IMU_RUNNING
    if(imu_running != last_known_imu_running)
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("\n---imu_running changed to : %d -> Publishing to Firebase ",imu_running);
        }

        firebase_update_engel_main_output(firebase_out_status_imu_running,firebase_log_mode);

        last_known_imu_running = imu_running;

        firebase_cycle_nr++;

        //Raise the flag to update Firestore as well    
        firestore_imu_running_needs_update = true; 
        firestore_needs_update = true;
    }


    //ACCIDENT

    //DETECTED
    if(accident_detected != last_known_accident_detected) 
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("\n---accident_detected changed to : %d -> Publishing to Firebase ",accident_detected);
        }

        firebase_update_engel_main_output(firebase_out_accident_detected,firebase_log_mode);

        last_known_accident_detected = accident_detected;

        firebase_cycle_nr++;

        //Raise the flag to update Firestore as well    
        firestore_accident_detected_needs_update = true; 
        firestore_needs_update = true;
    }

    //CONFIRMED
    if(accident_confirmed != last_known_accident_confirmed) 
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("\n---accident_confirmed changed to : %d -> Publishing to Firebase ",accident_confirmed);
        }

        firebase_update_engel_main_output(firebase_out_accident_confirmed,firebase_log_mode);

        last_known_accident_confirmed = accident_confirmed;

        firebase_cycle_nr++;

        //Raise the flag to update Firestore as well    
        firestore_accident_confirmed_needs_update = true; 
        firestore_needs_update = true;
    }

    //PARKING_ALARM

    //MOVEMENT_DETECTED

    if(backend_parking_alarm_movement_detected != last_known_parking_alarm_movement_detected)
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("\n---parking_alarm_movement_detected changed to : %d -> Publishing to Firebase ",backend_parking_alarm_movement_detected);
        }

        wait(100);

        firebase_update_engel_main_output(firebase_out_park_alarm_movement_detected,firebase_log_mode);

        last_known_parking_alarm_movement_detected = backend_parking_alarm_movement_detected;

        firebase_cycle_nr++;

        //Raise the flag to update Firestore as well    
        firestore_park_alarm_movement_needs_update = true; 
        firestore_needs_update = true;

    }


    //TRIGGERED

    if(backend_parking_alarm_triggered != last_known_parking_alarm_triggered)
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("\n---parking_alarm_triggered changed to : %d -> Publishing to Firebase ",backend_parking_alarm_triggered);
        }

        wait(100);

        firebase_update_engel_main_output(firebase_out_park_alarm_triggered,firebase_log_mode);

        last_known_parking_alarm_triggered = backend_parking_alarm_triggered;

        firebase_cycle_nr++;

        //Raise the flag to update Firestore as well    
        firestore_park_alarm_triggered_needs_update = true; 
        firestore_needs_update = true;

    }

    //HW_INFO and variant will never change during execution


    //GPS
    //Changes will be checked just if gps_upload is active    
    if(gps_upload)
    {
        //GPS_LAT
        if(gps_latitude != last_known_gps_lat) 
        {
            if(firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.printf("\n--- gps_lat changed to : %d -> Publishing to Firebase ",gps_latitude);
            }

            firebase_update_engel_main_output(firebase_out_gps_lat,firebase_log_mode);

            last_known_gps_lat = gps_latitude;

            firebase_cycle_nr++;

            //Raise the flag to update Firestore as well    
            firestore_gps_lat_needs_update = true; 
            firestore_needs_update = true;
        }

        //GPS_LON
        if(gps_longitude != last_known_gps_lon) 
        {
            if(firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.printf("\n--- gps_lon changed to : %d -> Publishing to Firebase ",gps_longitude);
            }

            firebase_update_engel_main_output(firebase_out_gps_lon,firebase_log_mode);

            last_known_gps_lon = gps_longitude;

            firebase_cycle_nr++;

            //Raise the flag to update Firestore as well    
            firestore_gps_lon_needs_update = true; 
            firestore_needs_update = true;
        }

        //GPS_KPH
        if(gps_speed_kmh != last_known_gps_kph) 
        {
            if(firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.printf("\n--- gps_kph changed to : %d -> Publishing to Firebase ",gps_speed_kmh);
            }

            firebase_update_engel_main_output(firebase_out_gps_kph,firebase_log_mode);

            last_known_gps_kph = gps_speed_kmh;

            firebase_cycle_nr++;

            //Raise the flag to update Firestore as well    
            firestore_gps_kph_needs_update = true; 
            firestore_needs_update = true;
        }

        //GPS_KPH
        if(gps_speed_mph != last_known_gps_mph) 
        {
            if(firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.printf("\n--- gps_mph changed to : %d -> Publishing to Firebase ",gps_speed_mph);
            }

            firebase_update_engel_main_output(firebase_out_gps_mph,firebase_log_mode);

            last_known_gps_mph = gps_speed_mph;

            firebase_cycle_nr++;

            //Raise the flag to update Firestore as well    
            firestore_gps_mph_needs_update = true; 
            firestore_needs_update = true;
        }

        //GPS_HEA
        if(gps_heading != last_known_gps_hea) 
        {
            if(firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.printf("\n--- gps_hea changed to : %d -> Publishing to Firebase ",gps_heading);
            }

            firebase_update_engel_main_output(firebase_out_gps_hea,firebase_log_mode);

            last_known_gps_hea = gps_heading;

            firebase_cycle_nr++;

            //Raise the flag to update Firestore as well    
            firestore_gps_hea_needs_update = true; 
            firestore_needs_update = true;
        }

        //GPS_ALT
        if(gps_altitude != last_known_gps_alt) 
        {
            if(firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.printf("\n--- gps_alt changed to : %d -> Publishing to Firebase ",gps_altitude);
            }

            firebase_update_engel_main_output(firebase_out_gps_alt,firebase_log_mode);

            last_known_gps_alt = gps_altitude;

            firebase_cycle_nr++;

            //Raise the flag to update Firestore as well    
            firestore_gps_alt_needs_update = true; 
            firestore_needs_update = true;
        }

    }
    
    //CAN

    //CAN_VEL
    if(can_vel != last_known_can_vel) 
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("\n--- can_vel changed to : %d -> Publishing to Firebase ",can_vel);
        }

        firebase_update_engel_main_output(firebase_out_can_vel,firebase_log_mode);

        last_known_can_vel = can_vel;

        firebase_cycle_nr++;

        //Raise the flag to update Firestore as well    
        firestore_can_vel_needs_update = true; 
        firestore_needs_update = true;
    }

    //CAN_ACC
    if(can_rpm != last_known_can_rpm) 
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("\n--- can_rpm changed to : %d -> Publishing to Firebase ",can_rpm);
        }

        firebase_update_engel_main_output(firebase_out_can_rpm,firebase_log_mode);

        last_known_can_rpm = can_rpm;

        firebase_cycle_nr++;

        //Raise the flag to update Firestore as well    
        firestore_can_rpm_needs_update = true; 
        firestore_needs_update = true;
    }

    //CAN_ODO
    if(can_odo != last_known_can_odo) 
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("\n--- can_odo changed to : %d -> Publishing to Firebase ",can_odo);
        }

        firebase_update_engel_main_output(firebase_out_can_odo,firebase_log_mode);

        last_known_can_odo = can_odo;

        firebase_cycle_nr++;

        //Raise the flag to update Firestore as well    
        firestore_can_odo_needs_update = true; 
        firestore_needs_update = true;
    }

    //CAN_SOC
    if(can_soc != last_known_can_soc) 
    {
        if(firebase_log_mode > firebase_log_mode_silent)
        {
            Serial.printf("\n--- can_soc changed to : %d -> Publishing to Firebase ",can_soc);
        }

        firebase_update_engel_main_output(firebase_out_can_soc,firebase_log_mode);

        last_known_can_soc = can_soc;

        firebase_cycle_nr++;

        //Raise the flag to update Firestore as well    
        firestore_can_soc_needs_update = true; 
        firestore_needs_update = true;
    }

}

//Only ones allowed are included on the loop
bool firebase_override_inputs(int firebase_log_mode)
{
    int errors_detected = 0 ;
    //will override inputs that were changed from the device and do not match the db
    //Only Scenarios in where this fns is allowed are:

    //Alarm Dismiss or Deactivate from buttons(combinations)
    //Alarm Dismiss or Deactivate from LORA

    //This Fn will detect what changed and rewrite the change to DB
    
    if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
    {
        Serial.printf("\n\n---Overriding Unsyncronized Inputs ");
    }

    //Generic Placeholders

    char firebase_path[firebase_buffer] = ""; 
            
    int char_size =0;
        
    //PARK_ALARM_STATUS-------------------------------------------------------------

    //Getting value from DB

    char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/park_alarm/on",esp_id);

    if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
    {
        Serial.printf("\n\n---Getting park_alarm_on from Firebase ");
        Serial.printf("topic:%s , Path_size: %d chars", firebase_path,char_size);    
    }

    if(Firebase.RTDB.getBool(&fbdo,firebase_path))
    {
        wait(100);

        //Guard to check that data format is correct
        if(fbdo.dataType() == "boolean")
        {   
            if(backend_parking_alarm_state != fbdo.boolData())
            {
                wait(100);

                if(log_enabled && firebase_log_mode >nvs_log_mode_silent)
                {
                    Serial.print("\n---parking_alarm_on is not syncronized");
                }
                               
                //Setting Value into DB

                wait(100);
                
                if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
                {
                    Serial.printf("\n\n---Sending backend_parking_alarm_state: %d to Firebase ",backend_parking_alarm_state);
                    Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
                }

                wait(100);

                
                if(Firebase.RTDB.setBool(&fbdo,firebase_path,backend_parking_alarm_state))
                {
                    wait(100);

                    if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                    {
                        Serial.print("\n---parking_alarm_state updated to : ");
                        Serial.print(backend_parking_alarm_state);
                        Serial.print(" as ");
                        Serial.print(fbdo.dataType());
                        Serial.print(" on "); 
                        Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                    }

                    firestore_park_alarm_on_needs_update = true;
                    firestore_needs_update = true;

                }
                else 
                {
                    wait(100);
                    //Will print always as is a big error
                    if(log_enabled)
                    {
                        Serial.print("\nFAILED TO PUBLISH : ");
                        Serial.print(fbdo.errorReason());
                        
                    }

                    errors_detected++;
                }

            }

            else //Flag is on Sync with Firebase
            {
                if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
                {
                    Serial.printf("\n\n--- park_alarm_status is on sync with Firebase --- OK ");   
                }
            }
        }
        else
        {
            if(log_enabled)
            {
                Serial.print("\n\n---Error : The data is not boolean,is :");
                Serial.print(fbdo.dataType());
                
            }
            
            errors_detected++;
        }                
    }
    else 
    {
        wait(100);
        //Will print always as is a big error
        if(log_enabled)
        {
            Serial.print("\n---FAILED TO READ! : ");
            Serial.print(fbdo.errorReason());              
        }

        errors_detected++;
    }

    //PARK_ALARM_SNOOZE-------------------------------------------------------------------------

    //Getting value from DB

    char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/park_alarm/snooze",esp_id);

    if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
    {
        Serial.printf("\n\n---Getting park_alarm_snooze from Firebase ");
        Serial.printf("topic:%s , Path_size: %d chars", firebase_path,char_size);    
    }

    if(Firebase.RTDB.getBool(&fbdo,firebase_path))
    {
        wait(100);

        //Guard to check that data format is correct
        if(fbdo.dataType() == "boolean")
        {   
            if(parking_alarm_snooze != fbdo.boolData())
            {
                wait(100);

                if(log_enabled && firebase_log_mode >nvs_log_mode_silent)
                {
                    Serial.print("\n---parking_alarm_snooze is not syncronized");
                }
                               
                //Setting Value into DB

                wait(100);
                
                if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
                {
                    Serial.printf("\n\n---Sending parking_alarm_snooze: %d to Firebase ",parking_alarm_snooze);
                    Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
                }

                wait(100);

                
                if(Firebase.RTDB.setBool(&fbdo,firebase_path,parking_alarm_snooze))
                {
                    wait(100);

                    if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                    {
                        Serial.print("\n---parking_alarm_snooze updated to : ");
                        Serial.print(parking_alarm_snooze);
                        Serial.print(" as ");
                        Serial.print(fbdo.dataType());
                        Serial.print(" on "); 
                        Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                    }

                    firestore_park_alarm_snooze_needs_update = true;
                    firestore_needs_update = true;

                }
                else 
                {
                    wait(100);
                    //Will print always as is a big error
                    if(log_enabled)
                    {
                        Serial.print("\nFAILED TO PUBLISH : ");
                        Serial.print(fbdo.errorReason());
                        
                    }
                    errors_detected++;
                }

            }
            else //Flag is on Sync with Firebase
            {
                if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
                {
                    Serial.printf("\n\n--- park_alarm_snooze is on sync with Firebase --- OK ");
                }
            }
        }
        else
        {
            if(log_enabled)
            {
                Serial.print("\n\n---Error : The data is not boolean,is :");
                Serial.print(fbdo.dataType());
                
            }
            errors_detected++;
        }                
    }
    else 
    {
        wait(100);
        //Will print always as is a big error
        if(log_enabled)
        {
            Serial.print("\n---FAILED TO READ! : ");
            Serial.print(fbdo.errorReason());              
        }

        errors_detected++;
    }   


    //ACCIDENT_DISMISSED ------------------------------------------------------------------

    //Getting value from DB

    char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/accident/dismissed",esp_id);

    if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
    {
        Serial.printf("\n\n---Getting accident_dismissed from Firebase ");
        Serial.printf("topic:%s , Path_size: %d chars", firebase_path,char_size);    
    }
    
    if(Firebase.RTDB.getBool(&fbdo,firebase_path))
    {
        wait(100);

        //Guard to check that data format is correct
        if(fbdo.dataType() == "boolean")
        {   
            if(accident_dismissed != fbdo.boolData())
            {
                wait(100);

                if(log_enabled && firebase_log_mode >nvs_log_mode_silent)
                {
                    Serial.print("\n---accident_dismissed is not syncronized");
                }
                               
                //Setting Value into DB

                wait(100);
                
                if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
                {
                    Serial.printf("\n\n---Sending accident_dismissed: %d to Firebase ",accident_dismissed);
                    Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
                }

                wait(100);

                
                if(Firebase.RTDB.setBool(&fbdo,firebase_path,accident_dismissed))
                {
                    wait(100);

                    if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
                    {
                        Serial.print("\n---accident_dismissed updated to : ");
                        Serial.print(accident_dismissed);
                        Serial.print(" as ");
                        Serial.print(fbdo.dataType());
                        Serial.print(" on "); 
                        Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                    }

                    firestore_accident_dismissed_needs_update = true;
                    firestore_needs_update = true;
                }
                else 
                {
                    wait(100);
                    //Will print always as is a big error
                    if(log_enabled)
                    {
                        Serial.print("\nFAILED TO PUBLISH : ");
                        Serial.print(fbdo.errorReason());
                        
                    }

                    errors_detected++;
                }

            }
            else //Flag is on Sync with Firebase
            {
                if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
                {
                    Serial.printf("\n\n--- accident_dismissed is on sync with Firebase --- OK ");   
                }
            }
        }
        else
        {
            if(log_enabled)
            {
                Serial.print("\n\n---Error : The data is not boolean,is :");
                Serial.print(fbdo.dataType());
                
            }
            
            errors_detected++;
        }                
    }
    else 
    {
        wait(100);
        //Will print always as is a big error
        if(log_enabled)
        {
            Serial.print("\n---FAILED TO READ! : ");
            Serial.print(fbdo.errorReason());              
        }

        errors_detected++;
    }

    //Summarizing Errors -------------------------------------------------------------

    if (errors_detected > 0)
    {
        if(log_enabled)Serial.print("\n--- ERRORS DETECTED! , TRYING AGAIN");
        wait(500);
        return false;

    }
    else
    {
        if(log_enabled)Serial.print("\n--- Success Overriding the Inputs on DB --- ");
        return true;
    }                     

    //TOD , check if here the other inputs (can and gps) are also needed
}



//Will check all inputs and initialize if the path doesn't exist
//can also be used to force a refresh on all inputs on demand (e.g. upon reboot)   
//This fn won't change any input , just refresh from DB
//Reminder: all inputs have been deprecated from here and must update just on the callback function

//this is just an emergency function, should not run more than once

void firebase_update_engel_main_input_all(int firebase_log_mode)
{

    if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
    {
        Serial.print("\n -- Forcing Update on ALL INPUTS on Firebase");
    }


    wait(100);
    //Read the chi_id if was not updated before
    if(!esp_id)get_esp_id(); 
    wait(100);

    //syncing Firestore after this refresh as well
    firestore_update_all = true;
    firestore_needs_update = true; 
    

    //Generic Placeholders

    char firebase_path[firebase_buffer] = ""; 
            
    int char_size =0;

    //Going One by one through all inputs
    

    //LEDS ------------------------------------------------------------

    //ON

    char_size= snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/leds/on",esp_id);

    if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
    {
        Serial.printf("\n\n---Getting leds_on from Firebase ");
        Serial.printf("topic:%s , Path_size: %d chars", firebase_path,char_size);    
    }
    
    if(Firebase.RTDB.getBool(&fbdo,firebase_path))
    {
        wait(100);

        //Guard to check that data format is correct
        if(fbdo.dataType() == "boolean")
        {   
            backend_led_status = fbdo.boolData();

            if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.print("\n---backend_led_status updated to : ");
                Serial.print(backend_led_status);
                Serial.print(" as ");
                Serial.print(fbdo.dataType());
                Serial.print(" on "); 
                Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
            }
        }
        else
        {
            Serial.print("\n\n---Error : The data is not boolean,is :");
            Serial.print(fbdo.dataType());
            Serial.print("\n---Discarding Value and setting backend_led_status to default--");
            backend_led_status = false;
        }                
    }
    else 
    {
        wait(100);
        //Will print always as is a big error
        if(log_enabled)
        {
            Serial.print("\n---FAILED TO READ! : ");
            Serial.print(fbdo.errorReason());

            if(fbdo.errorReason() == "path not exist")
            {
                if(log_enabled) Serial.println("---PUBLISHING DEFAULT VALUE TO INITIALIZE THE VARIABLE ----");

                char firebase_path[firebase_buffer] = ""; 
    
                int char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/leds/on",esp_id);

                if(log_enabled)
                {
                    Serial.printf("\n\n---Sending backend_led_status: %d to Firebase ",backend_led_status);
                    Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
                }

                
                if(Firebase.RTDB.setBool(&fbdo,firebase_path,backend_led_status))
                {
                    wait(100);

                    if(log_enabled)
                    {
                        Serial.print("\n---backend_led_status updated to : ");
                        Serial.print(backend_led_status);
                        Serial.print(" as ");
                        Serial.print(fbdo.dataType());
                        Serial.print(" on "); 
                        Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                    }
                }
                else 
                {
                    wait(100);
                    //Will print always as is a big error
                    if(log_enabled)
                    {
                        Serial.print("\nFAILED TO PUBLISH : ");
                        Serial.print(fbdo.errorReason());
                    }
                }
            }                
        }
    }




    //TODO copy this and make for gps and also can and then change them to their own values


    
    //BRIGHTNESS

    char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/leds/brightness",esp_id);

    if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
    {
        Serial.printf("\n\n---Getting leds_brightness from Firebase ");
        Serial.printf("topic:%s , Path_size: %d chars", firebase_path,char_size);    
    }

    if(Firebase.RTDB.getInt(&fbdo,firebase_path))
    {
        wait(100);

        //Guard to check that data format is correct
        if(fbdo.dataType() == "int")
        {   
            backend_led_brightness = fbdo.intData();

            if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.print("\n---backend_led_brightness updated to : ");
                Serial.print(backend_led_brightness);
                Serial.print(" as ");
                Serial.print(fbdo.dataType());
                Serial.print(" on "); 
                Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
            }
        }
        else
        {
            Serial.print("\n\n---Error : The data is not integer,is :");
            Serial.print(fbdo.dataType());
            Serial.print("\n---Discarding Value and setting backend_led_brightness to 100--");
            backend_led_brightness = 100;
        }                
    }
    else 
    {
        wait(100);
        //Will print always as is a big error
        if(log_enabled)
        {
            Serial.print("\n---FAILED TO READ! : ");
            Serial.print(fbdo.errorReason());

            if(fbdo.errorReason() == "path not exist")
            {
                if(log_enabled) Serial.println("---PUBLISHING DEFAULT VALUE TO INITIALIZE THE VARIABLE ----");

                char firebase_path[firebase_buffer] = ""; 
    
                int char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/leds/brightness",esp_id);

                if(log_enabled)
                {
                    Serial.printf("\n\n---Sending backend_led_brightness: %d to Firebase ",backend_led_brightness);
                    Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
                }

                
                if(Firebase.RTDB.setInt(&fbdo,firebase_path,backend_led_brightness))
                {
                    wait(100);

                    if(log_enabled)
                    {
                        Serial.print("\n---backend_led_brightness updated to : ");
                        Serial.print(backend_led_brightness);
                        Serial.print(" as ");
                        Serial.print(fbdo.dataType());
                        Serial.print(" on "); 
                        Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                    }
                }
                else 
                {
                    wait(100);
                    //Will print always as is a big error
                    if(log_enabled)
                    {
                        Serial.print("\nFAILED TO PUBLISH : ");
                        Serial.print(fbdo.errorReason());
                    }
                }
            }                
        }
    }

    //COLOR

    char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/leds/color",esp_id);

    if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
    {
        Serial.printf("\n\n---Getting leds_color from Firebase ");
        Serial.printf("topic:%s , Path_size: %d chars", firebase_path,char_size);    
    }

    
    if(Firebase.RTDB.getString(&fbdo,firebase_path))
    {
        wait(100);

        //Guard to check that data format is correct
        if(fbdo.dataType() == "string")
        {   

            //from string back to char
            if(fbdo.stringData() == "red")
            {
                backend_led_color = 'r';
            }
            else if(fbdo.stringData() == "green")
            {
                backend_led_color = 'g';
            }
            else if(fbdo.stringData() == "blue")
            {
                backend_led_color = 'b';
            }
            else if(fbdo.stringData() == "white")
            {
                backend_led_color = 'w';
            }
            else
            {
                Serial.print("\n--Color not recognized! , defaulting to red");
                backend_led_color = 'r';
            }


            if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.print("\n---backend_led_color updated to : ");
                Serial.print(fbdo.stringData());
                Serial.print(" as ");
                Serial.print(fbdo.dataType());
                Serial.print(" on "); 
                Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
            }
        }
        else
        {
            Serial.print("\n\n---Error : The data is not string,is :");
            Serial.print(fbdo.dataType());
            Serial.print("\n---Discarding Value and setting backend_led_color to default--");
            backend_led_color = 'r';
        }                
    }
    else 
    {
        wait(100);
        //Will print always as is a big error
        if(log_enabled)
        {
            Serial.print("\n---FAILED TO READ! : ");
            Serial.print(fbdo.errorReason());

            if(fbdo.errorReason() == "path not exist")
            {
                if(log_enabled) Serial.println("---PUBLISHING DEFAULT VALUE TO INITIALIZE THE VARIABLE ----");

                char firebase_path[firebase_buffer] = ""; 
    
                int char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/leds/color",esp_id);

                if(log_enabled)
                {
                    Serial.print("\n\n---Sending led_color: red  as String to Firebase ");
                    Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
                }

                
                if(Firebase.RTDB.setString(&fbdo,firebase_path,"red"))
                {
                    wait(100);

                    if(log_enabled)
                    {
                        Serial.print("\n---backend_led_status updated to : red ");
                        //Serial.print(backend_led_status);
                        Serial.print(" as ");
                        Serial.print(fbdo.dataType());
                        Serial.print(" on "); 
                        Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                    }
                }
                else 
                {
                    wait(100);
                    //Will print always as is a big error
                    if(log_enabled)
                    {
                        Serial.print("\nFAILED TO PUBLISH : ");
                        Serial.print(fbdo.errorReason());
                    }
                }
            }                
        }
    }

    //PATTERN --- NOT IMPLEMENTED , MAYBE LATER

    //PARK_ALARM ------------------------------------------------------

    //ON

    char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/park_alarm/on",esp_id);

    if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
    {
        Serial.printf("\n\n---Getting park_alarm_on from Firebase ");
        Serial.printf("topic:%s , Path_size: %d chars", firebase_path,char_size);    
    }

    
    if(Firebase.RTDB.getBool(&fbdo,firebase_path))
    {
        wait(100);

        //Guard to check that data format is correct
        if(fbdo.dataType() == "boolean")
        {   
            backend_parking_alarm_state = fbdo.boolData();

            if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.print("\n---parking_alarm_on updated to : ");
                Serial.print(backend_parking_alarm_state);
                Serial.print(" as ");
                Serial.print(fbdo.dataType());
                Serial.print(" on "); 
                Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
            }
        }
        else
        {
            Serial.print("\n\n---Error : The data is not boolean,is :");
            Serial.print(fbdo.dataType());
            Serial.print("\n---Discarding Value and setting backend_parking_alarm_state to default--");
            backend_parking_alarm_state = false;
        }                
    }
    else 
    {
        wait(100);
        //Will print always as is a big error
        if(log_enabled)
        {
            Serial.print("\n---FAILED TO READ! : ");
            Serial.print(fbdo.errorReason());

            if(fbdo.errorReason() == "path not exist")
            {
                if(log_enabled) Serial.println("---PUBLISHING DEFAULT VALUE TO INITIALIZE THE VARIABLE ----");

                char firebase_path[firebase_buffer] = ""; 
    
                int char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/park_alarm/on",esp_id);

                if(log_enabled)
                {
                    Serial.printf("\n\n---Sending parking_alarm_state: %d to Firebase ",backend_parking_alarm_state);
                    Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
                }

                
                if(Firebase.RTDB.setBool(&fbdo,firebase_path,backend_parking_alarm_state))
                {
                    wait(100);

                    if(log_enabled)
                    {
                        Serial.print("\n---parking_alarm_on updated to : ");
                        Serial.print(backend_parking_alarm_state);
                        Serial.print(" as ");
                        Serial.print(fbdo.dataType());
                        Serial.print(" on "); 
                        Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                    }
                }
                else 
                {
                    wait(100);
                    //Will print always as is a big error
                    if(log_enabled)
                    {
                        Serial.print("\nFAILED TO PUBLISH : ");
                        Serial.print(fbdo.errorReason());
                    }
                }
            }                
        }
    }

    //MODE

    char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/park_alarm/mode",esp_id);

    if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
    {
        Serial.printf("\n\n---Getting park_alarm_mode from Firebase ");
        Serial.printf("topic:%s , Path_size: %d chars", firebase_path,char_size);    
    }

    
    if(Firebase.RTDB.getBool(&fbdo,firebase_path))
    {
        wait(100);

        //Guard to check that data format is correct
        if(fbdo.dataType() == "boolean")
        {   
            backend_parking_alarm_mode = fbdo.boolData();

            if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.print("\n---parking_alarm_mode updated to : ");
                Serial.print(backend_parking_alarm_mode);
                Serial.print(" as ");
                Serial.print(fbdo.dataType());
                Serial.print(" on "); 
                Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
            }
        }
        else
        {
            Serial.print("\n\n---Error : The data is not boolean,is :");
            Serial.print(fbdo.dataType());
            Serial.print("\n---Discarding Value and setting backend_parking_alarm_mode to default--");
            backend_parking_alarm_mode = false;
        }                
    }
    else 
    {
        wait(100);
        //Will print always as is a big error
        if(log_enabled)
        {
            Serial.print("\n---FAILED TO READ! : ");
            Serial.print(fbdo.errorReason());

            if(fbdo.errorReason() == "path not exist")
            {
                if(log_enabled) Serial.println("---PUBLISHING DEFAULT VALUE TO INITIALIZE THE VARIABLE ----");

                char firebase_path[firebase_buffer] = ""; 
    
                int char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/park_alarm/mode",esp_id);

                if(log_enabled)
                {
                    Serial.printf("\n\n---Sending parking_alarm_mode: %d to Firebase ",backend_parking_alarm_mode);
                    Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
                }

                
                if(Firebase.RTDB.setBool(&fbdo,firebase_path,backend_parking_alarm_mode))
                {
                    wait(100);

                    if(log_enabled)
                    {
                        Serial.print("\n---backend_parking_alarm_mode updated to : ");
                        Serial.print(backend_parking_alarm_mode);
                        Serial.print(" as ");
                        Serial.print(fbdo.dataType());
                        Serial.print(" on "); 
                        Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                    }
                }
                else 
                {
                    wait(100);
                    //Will print always as is a big error
                    if(log_enabled)
                    {
                        Serial.print("\nFAILED TO PUBLISH : ");
                        Serial.print(fbdo.errorReason());
                    }
                }
            }                
        }
    }

    //SNOOZE

    char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/park_alarm/snooze",esp_id);

    if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
    {
        Serial.printf("\n\n---Getting park_alarm_snooze from Firebase ");
        Serial.printf("topic:%s , Path_size: %d chars", firebase_path,char_size);    
    }

    
    if(Firebase.RTDB.getBool(&fbdo,firebase_path))
    {
        wait(100);

        //Guard to check that data format is correct
        if(fbdo.dataType() == "boolean")
        {   
            backend_parking_alarm_state = fbdo.boolData();

            if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.print("\n---parking_alarm_snooze updated to : ");
                Serial.print(parking_alarm_snooze);
                Serial.print(" as ");
                Serial.print(fbdo.dataType());
                Serial.print(" on "); 
                Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
            }
        }
        else
        {
            Serial.print("\n\n---Error : The data is not boolean,is :");
            Serial.print(fbdo.dataType());
            Serial.print("\n---Discarding Value and setting backend_parking_alarm_snooze to default--");
            parking_alarm_snooze = false;
        }                
    }
    else 
    {
        wait(100);
        //Will print always as is a big error
        if(log_enabled)
        {
            Serial.print("\n---FAILED TO READ! : ");
            Serial.print(fbdo.errorReason());

            if(fbdo.errorReason() == "path not exist")
            {
                if(log_enabled) Serial.println("---PUBLISHING DEFAULT VALUE TO INITIALIZE THE VARIABLE ----");

                char firebase_path[firebase_buffer] = ""; 
    
                int char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/park_alarm/snooze",esp_id);

                if(log_enabled)
                {
                    Serial.printf("\n\n---Sending parking_alarm_snooze: %d to Firebase ",parking_alarm_snooze);
                    Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
                }

                
                if(Firebase.RTDB.setBool(&fbdo,firebase_path,parking_alarm_snooze))
                {
                    wait(100);

                    if(log_enabled)
                    {
                        Serial.print("\n---parking_alarm_snooze updated to : ");
                        Serial.print(parking_alarm_snooze);
                        Serial.print(" as ");
                        Serial.print(fbdo.dataType());
                        Serial.print(" on "); 
                        Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                    }
                }
                else 
                {
                    wait(100);
                    //Will print always as is a big error
                    if(log_enabled)
                    {
                        Serial.print("\nFAILED TO PUBLISH : ");
                        Serial.print(fbdo.errorReason());
                    }
                }
            }                
        }
    }


    //ACCIDENT_DISMISSED

    char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/accident/dismissed",esp_id);

    if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
    {
        Serial.printf("\n\n---Getting accident_dismissed from Firebase ");
        Serial.printf("topic:%s , Path_size: %d chars", firebase_path,char_size);    
    }

    
    if(Firebase.RTDB.getBool(&fbdo,firebase_path))
    {
        wait(100);

        //Guard to check that data format is correct
        if(fbdo.dataType() == "boolean")
        {   
            accident_dismissed = fbdo.boolData();

            if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.print("\n---accident_dismissed updated to : ");
                Serial.print(accident_dismissed);
                Serial.print(" as ");
                Serial.print(fbdo.dataType());
                Serial.print(" on "); 
                Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
            }
        }
        else
        {
            Serial.print("\n\n---Error : The data is not boolean,is :");
            Serial.print(fbdo.dataType());
            Serial.print("\n---Discarding Value and setting acident_dismissed to default--");
            accident_dismissed = false;
        }                
    }
    else 
    {
        wait(100);
        //Will print always as is a big error
        if(log_enabled)
        {
            Serial.print("\n---FAILED TO READ! : ");
            Serial.print(fbdo.errorReason());

            if(fbdo.errorReason() == "path not exist")
            {
                if(log_enabled) Serial.println("---PUBLISHING DEFAULT VALUE TO INITIALIZE THE VARIABLE ----");

                char firebase_path[firebase_buffer] = ""; 
    
                int char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/accident/dismissed",esp_id);

                if(log_enabled)
                {
                    Serial.printf("\n\n---Sending accident_dismissed: %d to Firebase ",accident_dismissed);
                    Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
                }

                
                if(Firebase.RTDB.setBool(&fbdo,firebase_path,accident_dismissed))
                {
                    wait(100);

                    if(log_enabled)
                    {
                        Serial.print("\n---accident_dismissed updated to : ");
                        Serial.print(backend_parking_alarm_mode);
                        Serial.print(" as ");
                        Serial.print(fbdo.dataType());
                        Serial.print(" on "); 
                        Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                    }
                }
                else 
                {
                    wait(100);
                    //Will print always as is a big error
                    if(log_enabled)
                    {
                        Serial.print("\nFAILED TO PUBLISH : ");
                        Serial.print(fbdo.errorReason());
                    }
                }
            }                
        }
    }

    //GPS ------------------------------------------------------------

    //ENABLED

    char_size= snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/config/gps/enabled",esp_id);

    if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
    {
        Serial.printf("\n\n---Getting gps_enabled from Firebase ");
        Serial.printf("topic:%s , Path_size: %d chars", firebase_path,char_size);    
    }
    
    if(Firebase.RTDB.getBool(&fbdo,firebase_path))
    {
        wait(100);

        //Guard to check that data format is correct
        if(fbdo.dataType() == "boolean")
        {   
            gps_enabled = fbdo.boolData();

            if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.print("\n---gps_enabled updated to : ");
                Serial.print(gps_enabled);
                Serial.print(" as ");
                Serial.print(fbdo.dataType());
                Serial.print(" on "); 
                Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
            }
        }
        else
        {
            Serial.print("\n\n---Error : The data is not boolean,is :");
            Serial.print(fbdo.dataType());
            Serial.print("\n---Discarding Value and setting gps_enabled to default--");
            gps_enabled = true;
        }                
    }
    else 
    {
        wait(100);
        //Will print always as is a big error
        if(log_enabled)
        {
            Serial.print("\n---FAILED TO READ! : ");
            Serial.print(fbdo.errorReason());

            if(fbdo.errorReason() == "path not exist")
            {
                if(log_enabled) Serial.println("---PUBLISHING DEFAULT VALUE TO INITIALIZE THE VARIABLE ----");

                char firebase_path[firebase_buffer] = ""; 
    
                int char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/config/gps/enabled",esp_id);

                if(log_enabled)
                {
                    Serial.printf("\n\n---Sending gps_enabled: %d to Firebase ",gps_enabled);
                    Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
                }

                
                if(Firebase.RTDB.setBool(&fbdo,firebase_path,gps_enabled))
                {
                    wait(100);

                    if(log_enabled)
                    {
                        Serial.print("\n---gps_enabled updated to : ");
                        Serial.print(gps_enabled);
                        Serial.print(" as ");
                        Serial.print(fbdo.dataType());
                        Serial.print(" on "); 
                        Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                    }
                }
                else 
                {
                    wait(100);
                    //Will print always as is a big error
                    if(log_enabled)
                    {
                        Serial.print("\nFAILED TO PUBLISH : ");
                        Serial.print(fbdo.errorReason());
                    }
                }
            }                
        }
    }

    //UPLOAD

    char_size= snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/config/gps/upload",esp_id);

    if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
    {
        Serial.printf("\n\n---Getting gps_upload from Firebase ");
        Serial.printf("topic:%s , Path_size: %d chars", firebase_path,char_size);    
    }
    
    if(Firebase.RTDB.getBool(&fbdo,firebase_path))
    {
        wait(100);

        //Guard to check that data format is correct
        if(fbdo.dataType() == "boolean")
        {   
            gps_upload  = fbdo.boolData();

            if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.print("\n---gps_upload  updated to : ");
                Serial.print(gps_upload );
                Serial.print(" as ");
                Serial.print(fbdo.dataType());
                Serial.print(" on "); 
                Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
            }
        }
        else
        {
            Serial.print("\n\n---Error : The data is not boolean,is :");
            Serial.print(fbdo.dataType());
            Serial.print("\n---Discarding Value and setting gps_upload  to default--");
            gps_upload  = true;
        }                
    }
    else 
    {
        wait(100);
        //Will print always as is a big error
        if(log_enabled)
        {
            Serial.print("\n---FAILED TO READ! : ");
            Serial.print(fbdo.errorReason());

            if(fbdo.errorReason() == "path not exist")
            {
                if(log_enabled) Serial.println("---PUBLISHING DEFAULT VALUE TO INITIALIZE THE VARIABLE ----");

                char firebase_path[firebase_buffer] = ""; 
    
                int char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/config/gps/upload",esp_id);

                if(log_enabled)
                {
                    Serial.printf("\n\n---Sending gps_upload : %d to Firebase ",gps_upload );
                    Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
                }

                
                if(Firebase.RTDB.setBool(&fbdo,firebase_path,gps_upload ))
                {
                    wait(100);

                    if(log_enabled)
                    {
                        Serial.print("\n---gps_upload  updated to : ");
                        Serial.print(gps_upload );
                        Serial.print(" as ");
                        Serial.print(fbdo.dataType());
                        Serial.print(" on "); 
                        Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                    }
                }
                else 
                {
                    wait(100);
                    //Will print always as is a big error
                    if(log_enabled)
                    {
                        Serial.print("\nFAILED TO PUBLISH : ");
                        Serial.print(fbdo.errorReason());
                    }
                }
            }                
        }
    }

    //GPS_REFRESH_SECONDS 

    char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/config/gps/refresh_seconds",esp_id);

    if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
    {
        Serial.printf("\n\n---Getting gps_refresh_seconds from Firebase ");
        Serial.printf("topic:%s , Path_size: %d chars", firebase_path,char_size);    
    }

    if(Firebase.RTDB.getInt(&fbdo,firebase_path))
    {
        wait(100);

        //Guard to check that data format is correct
        if(fbdo.dataType() == "int")
        {   
            gps_refresh_seconds = fbdo.intData();

            if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.print("\n---gps_refresh_seconds updated to : ");
                Serial.print(gps_refresh_seconds);
                Serial.print(" as ");
                Serial.print(fbdo.dataType());
                Serial.print(" on "); 
                Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
            }
        }
        else
        {
            Serial.print("\n\n---Error : The data is not integer,is :");
            Serial.print(fbdo.dataType());
            Serial.print("\n---Discarding Value and setting gps_refresh_seconds to default--");
            gps_refresh_seconds = 10;
        }                
    }
    else 
    {
        wait(100);
        //Will print always as is a big error
        if(log_enabled)
        {
            Serial.print("\n---FAILED TO READ! : ");
            Serial.print(fbdo.errorReason());

            if(fbdo.errorReason() == "path not exist")
            {
                if(log_enabled) Serial.println("---PUBLISHING DEFAULT VALUE TO INITIALIZE THE VARIABLE ----");

                char firebase_path[firebase_buffer] = ""; 
    
                int char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/config/gps/refresh_seconds",esp_id);

                if(log_enabled)
                {
                    Serial.printf("\n\n---Sending gps_refresh_seconds: %d to Firebase ",gps_refresh_seconds);
                    Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
                }

                
                if(Firebase.RTDB.setInt(&fbdo,firebase_path,gps_refresh_seconds))
                {
                    wait(100);

                    if(log_enabled)
                    {
                        Serial.print("\n---gps_refresh_seconds updated to : ");
                        Serial.print(gps_refresh_seconds);
                        Serial.print(" as ");
                        Serial.print(fbdo.dataType());
                        Serial.print(" on "); 
                        Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                    }
                }
                else 
                {
                    wait(100);
                    //Will print always as is a big error
                    if(log_enabled)
                    {
                        Serial.print("\nFAILED TO PUBLISH : ");
                        Serial.print(fbdo.errorReason());
                    }
                }
            }                
        }
    }

    //CAN ------------------------------------------------------------

    //ENABLED

    char_size= snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/config/can/enabled",esp_id);

    if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
    {
        Serial.printf("\n\n---Getting can_enabled from Firebase ");
        Serial.printf("topic:%s , Path_size: %d chars", firebase_path,char_size);    
    }
    
    if(Firebase.RTDB.getBool(&fbdo,firebase_path))
    {
        wait(100);

        //Guard to check that data format is correct
        if(fbdo.dataType() == "boolean")
        {   
            can_enabled = fbdo.boolData();

            if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.print("\n---can_enabled updated to : ");
                Serial.print(can_enabled);
                Serial.print(" as ");
                Serial.print(fbdo.dataType());
                Serial.print(" on "); 
                Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
            }
        }
        else
        {
            Serial.print("\n\n---Error : The data is not boolean,is :");
            Serial.print(fbdo.dataType());
            Serial.print("\n---Discarding Value and setting can_enabled to default--");
            can_enabled = true;
        }                
    }
    else 
    {
        wait(100);
        //Will print always as is a big error
        if(log_enabled)
        {
            Serial.print("\n---FAILED TO READ! : ");
            Serial.print(fbdo.errorReason());

            if(fbdo.errorReason() == "path not exist")
            {
                if(log_enabled) Serial.println("---PUBLISHING DEFAULT VALUE TO INITIALIZE THE VARIABLE ----");

                char firebase_path[firebase_buffer] = ""; 
    
                int char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/config/can/enabled",esp_id);

                if(log_enabled)
                {
                    Serial.printf("\n\n---Sending can_enabled: %d to Firebase ",can_enabled);
                    Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
                }

                
                if(Firebase.RTDB.setBool(&fbdo,firebase_path,can_enabled))
                {
                    wait(100);

                    if(log_enabled)
                    {
                        Serial.print("\n---can_enabled updated to : ");
                        Serial.print(can_enabled);
                        Serial.print(" as ");
                        Serial.print(fbdo.dataType());
                        Serial.print(" on "); 
                        Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                    }
                }
                else 
                {
                    wait(100);
                    //Will print always as is a big error
                    if(log_enabled)
                    {
                        Serial.print("\nFAILED TO PUBLISH : ");
                        Serial.print(fbdo.errorReason());
                    }
                }
            }                
        }
    }

    //CAN_REFRESH_SECONDS 

    char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/config/can/refresh_seconds",esp_id);

    if(log_enabled && firebase_log_mode == firebase_log_mode_verbose)
    {
        Serial.printf("\n\n---Getting can_refresh_seconds from Firebase ");
        Serial.printf("topic:%s , Path_size: %d chars", firebase_path,char_size);    
    }

    if(Firebase.RTDB.getInt(&fbdo,firebase_path))
    {
        wait(100);

        //Guard to check that data format is correct
        if(fbdo.dataType() == "int")
        {   
            can_refresh_seconds = fbdo.intData();

            if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
            {
                Serial.print("\n---can_refresh_seconds updated to : ");
                Serial.print(can_refresh_seconds);
                Serial.print(" as ");
                Serial.print(fbdo.dataType());
                Serial.print(" on "); 
                Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
            }
        }
        else
        {
            Serial.print("\n\n---Error : The data is not integer,is :");
            Serial.print(fbdo.dataType());
            Serial.print("\n---Discarding Value and setting can_refresh_seconds to default--");
            can_refresh_seconds = 10;
        }                
    }
    else 
    {
        wait(100);
        //Will print always as is a big error
        if(log_enabled)
        {
            Serial.print("\n---FAILED TO READ! : ");
            Serial.print(fbdo.errorReason());

            if(fbdo.errorReason() == "path not exist")
            {
                if(log_enabled) Serial.println("---PUBLISHING DEFAULT VALUE TO INITIALIZE THE VARIABLE ----");

                char firebase_path[firebase_buffer] = ""; 
    
                int char_size = snprintf(firebase_path,firebase_buffer,"/devices/taillight/%d/config/can/refresh_seconds",esp_id);

                if(log_enabled)
                {
                    Serial.printf("\n\n---Sending can_refresh_seconds: %d to Firebase ",can_refresh_seconds);
                    Serial.printf("Topic:%s , Path_size: %d chars", firebase_path,char_size);    
                }

                
                if(Firebase.RTDB.setInt(&fbdo,firebase_path,can_refresh_seconds))
                {
                    wait(100);

                    if(log_enabled)
                    {
                        Serial.print("\n---can_refresh_seconds updated to : ");
                        Serial.print(can_refresh_seconds);
                        Serial.print(" as ");
                        Serial.print(fbdo.dataType());
                        Serial.print(" on "); 
                        Serial.print(fbdo.dataPath()); Serial.print(" <-- OK ");
                    }
                }
                else 
                {
                    wait(100);
                    //Will print always as is a big error
                    if(log_enabled)
                    {
                        Serial.print("\nFAILED TO PUBLISH : ");
                        Serial.print(fbdo.errorReason());
                    }
                }
            }                
        }
    }
}


void firebase_update_engel_main_output_all_sensors(int firebase_log_mode)
{
    if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
    {
      Serial.print("\n\n---Updating ALL Sensors\n");
    }

    firebase_update_engel_main_output(firebase_out_sensors_lux,firebase_log_mode);           
    firebase_update_engel_main_output(firebase_out_sensors_temp,firebase_log_mode); 
    firebase_update_engel_main_output(firebase_out_sensors_soc,firebase_log_mode); 

}

void firebase_update_engel_main_output_all_status(int firebase_log_mode)
{
    if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
    {
      Serial.print("\n\n---Updating ALL Status\n");
    }

    firebase_update_engel_main_output(firebase_out_status_charging,firebase_log_mode);           
    firebase_update_engel_main_output(firebase_out_status_moving,firebase_log_mode); 
    firebase_update_engel_main_output(firebase_out_status_usb_connected,firebase_log_mode); 
    firebase_update_engel_main_output(firebase_out_status_low_bat,firebase_log_mode); 
    firebase_update_engel_main_output(firebase_out_status_main_status,firebase_log_mode); 
    firebase_update_engel_main_output(firebase_out_status_imu_running,firebase_log_mode); 

}

void firebase_update_engel_main_output_all_hw_info(int firebase_log_mode)
{
    if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
    {
      Serial.print("\n\n---Updating ALL HW_Info\n");
    }

    firebase_update_engel_main_output(firebase_out_hw_info_version,firebase_log_mode);           
    firebase_update_engel_main_output(firebase_out_hw_info_variant,firebase_log_mode); 
}

void firebase_update_engel_main_output_all_park_alarm(int firebase_log_mode)
{
    if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
    {
      Serial.print("\n\n---Updating ALL park_alarm\n");
    }

    //just for  output
    firebase_update_engel_main_output(firebase_out_park_alarm_movement_detected,firebase_log_mode);
    firebase_update_engel_main_output(firebase_out_park_alarm_triggered,firebase_log_mode);
    //INPUTS UPDATE JUST PER CALLBACK           

}

void firebase_update_engel_main_output_all_accident(int firebase_log_mode)
{
    if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
    {
      Serial.print("\n\n---Updating ALL accident\n");
    }

    //just for  output
    firebase_update_engel_main_output(firebase_out_accident_detected,firebase_log_mode);
    firebase_update_engel_main_output(firebase_out_accident_confirmed,firebase_log_mode);
    //INPUTS UPDATE JUST PER CALLBACK           
}

void firebase_update_engel_main_output_all_gps(int firebase_log_mode)
{
    //Enter here just if gps_upload is enabled
    if(gps_upload)
    {
        if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
        {
          Serial.print("\n\n---Updating ALL GPS\n");
        }
    
        //just for  output
        firebase_update_engel_main_output(firebase_out_gps_lat,firebase_log_mode);
        firebase_update_engel_main_output(firebase_out_gps_lon,firebase_log_mode);
        firebase_update_engel_main_output(firebase_out_gps_kph,firebase_log_mode);
        firebase_update_engel_main_output(firebase_out_gps_mph,firebase_log_mode);
        firebase_update_engel_main_output(firebase_out_gps_hea,firebase_log_mode);
        firebase_update_engel_main_output(firebase_out_gps_alt,firebase_log_mode);    
    
        //INPUTS UPDATE JUST PER CALLBACK           
    }
    else
    {
        if(log_enabled) Serial.print("\n---- gps_upload is disabled , values won't be updated on Firebase ");
    }
}

void firebase_update_engel_main_output_all_can(int firebase_log_mode)
{
    if(log_enabled && firebase_log_mode > firebase_log_mode_silent)
    {
      Serial.print("\n\n---Updating ALL can\n");
    }

    //just for  output
    firebase_update_engel_main_output(firebase_out_can_vel,firebase_log_mode);
    firebase_update_engel_main_output(firebase_out_can_rpm,firebase_log_mode);
    firebase_update_engel_main_output(firebase_out_can_odo,firebase_log_mode);
    firebase_update_engel_main_output(firebase_out_can_soc,firebase_log_mode); 

    //INPUTS UPDATE JUST PER CALLBACK           
}


void firebase_update_engel_main_output_all(int firebase_log_mode)
{
  //Compilation of al Output topics

  firebase_update_engel_main_output_all_hw_info(firebase_log_mode);
  wait(10);

  firebase_update_engel_main_output_all_sensors(firebase_log_mode);
  wait(10);

  firebase_update_engel_main_output_all_status(firebase_log_mode);
  wait(10);

  firebase_update_engel_main_output_all_park_alarm(firebase_log_mode);
  wait(10);

  firebase_update_engel_main_output_all_accident(firebase_log_mode);
  wait(10);  

  if(gps_upload)firebase_update_engel_main_output_all_gps(firebase_log_mode);
  wait(10);  

  firebase_update_engel_main_output_all_can(firebase_log_mode);
  wait(10);  

}









//Firebase Task---------------------------------------------------------------------------------------------------- 
//Main Task to keep Firebase alive

TaskHandle_t task_firebase_handle = NULL;

void create_task_firebase() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n--creating task_firebase--");
    wait(100);
    

    task_firebase_i2c_declare();
    wait(100);

    xTaskCreate
    (
        task_firebase,   //Function Name (must be a while(1))
        "task_firebase", //Logging Name
        10000,           //Stack Size
        NULL,            //Passing Parameters
        11,              //Task Priority
        &task_firebase_handle
    );   

    task_firebase_active = true;

    if(log_enabled) Serial.print("-- done --\n");
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
  
    if(log_enabled)Serial.println("Running firebase_task ");
    wait(100);  
    
    //OUT with BTN2 (Disable for the moment on testing)
    //btn_2_interr_enable_on_press();
    wait(100);

    int interval = 1000;

    unsigned long counter = 0;




    //To Check Heap Size
    if(display_heap && log_enabled)
    {
        Serial.print("\nFree Heap Size: ");
        Serial.print(esp_get_free_heap_size());
    }
    
    while(1)
    {
        
        if(  !task_firebase_active ) // || btn_2.is_pressed  Getting Out (btn_2.is_pressed Deactivated on Mubea)
        {
            //btn_2_interr_disable();
            
            if(log_enabled) Serial.print("\n ----- Destroying task_firebase! ");   

            wifi_disconnect();
            wifi_off();

            //Clearing Flags
            wifi_connected   = false;
            firebase_initialized = false;
            firebase_connected   = false;
                
            //Deleted for Mubea
            //wait_for_btn_2_release(); //will reset the btn.pressed flag also
            //wait(100);
            //oled_clear();
            //wait(100);
            //create_task_devel_menu();
            //wait(500);
            //task_firebase_active = false;
            
            task_firebase_i2c_release();
            wait(10);

            vTaskDelete(NULL);//Delete itself
        }
        
        
        else if (WiFi.status() != WL_CONNECTED) 
        {
            //To Check Heap Size
            if(display_heap && log_enabled)
            {
                Serial.print("\nFree Heap Size: ");
                Serial.print(esp_get_free_heap_size());
            } 

            if(log_enabled)Serial.print("\n---Connecting to WIFI---\n");
            
            //if(wifi_has_credentials) wifi_connect();
            //TODO , later corret the statement above


            //TO DO , DELETE THIS ONE LATER
            
            //POINT A LAMP TO CONNECT TO HQ , OTHERWISE TO MOTIONLAB
            if(lux_get() > 1000) 
            {
                Serial.print("Connecting to HQ");
                wifi_ssid = wifi_ssid_hq;
                wifi_pass = wifi_pass_hq; 
            }
            else
            {
                Serial.print("Connecting to MotionLab");
                wifi_ssid = wifi_ssid_motionlab;
                wifi_pass = wifi_pass_motionlab;    
            }


            wifi_connect();
            wait(100);


            //To Check Heap Size
            if(display_heap && log_enabled)
            {
                Serial.print("\nFree Heap Size: ");
                Serial.print(esp_get_free_heap_size());
            } 
        
        }
        
        //If never connected or connection broken
        else if(!firebase_connected && WiFi.status() == WL_CONNECTED)
        {            
            if(log_enabled)Serial.print("\n---Connecting to Firebase ---\n");
            
            firebase_initialized= true;
            firebase_connect(); //TODO , maybe just proceed in case connection succeed.
            wait(100);

            if(log_enabled)Serial.print("\n--- Connected to Firebase via WIFI"); 

            
            //At the beginning we will get and set all the info to the DB 
            //if it does not exist we will create it and if it does will read and refresh if needed

            //Here Configuring the Callbacks for the Inputs and populating if empty
            firebase_declare_input_callbacks(firebase_log_mode_moderate);   

           
            //Here refreshing all outputs on DB    
            //firebase_update_engel_main_output_all(firebase_log_mode_moderate);

            wait(500);
        }

        //All successfull
        if (firebase_connected && WiFi.status() == WL_CONNECTED)
        {        
            if(log_enabled && firebase_first_loop)
            {
                Serial.print("\n\n--- Firebase Loop now Running!");
                //Serial.print("\n--- From now on updating Firebase just upon change ---\n\n");       
               
                //Serial.print("\n--- Forcing Firestore Refresh to Sync first loop ---\n");
                //wait(100);  
                //Serial.print("\n--- Forcing Firebase and Firestore Refresh to Sync on first loop all parameters ---\n");
                //Updating Firestore to Sync the first loop
                //firestore_update_all = true;
                //firestore_needs_update = true;        
                
                
            }

            if(oled_enabled)
            {
                //if(log_enabled)Serial.println("\n---Sending to OLED---> "); 
                //wait(100); 
                //TODO REMOVE THIS AFTER MUBEA TEST  oled_firebase(firebase_cycle_nr);
            }

            //wait(100);
       
            
            //All underneath nested here before Mubea , check if needed 
            /*
            while(!btn_2.is_pressed)
            {
                
            }
            */  
            
            //Main Loop (All Firebase interaction must be handled here sequentially)
            //No DB connection on other tasks or loops

            /*
            if(firebase_connected && Firebase.ready() && WiFi.status() == WL_CONNECTED)
            {
                if(firebase_inputs_need_manual_refresh)
                {
                    if(log_enabled)Serial.print("\n--- Forcing Update on all firebase_inputs");
                    
                    //Forcing Update on all firebase_inputs (Use just for debugging)
                    if(firebase_override_inputs(firebase_log_mode_moderate))
                    {
                        //ALL OK ! Setting flag down
                        firebase_inputs_need_manual_refresh = false;          
                    }
                    //else will try it again on the next loop   

                    else
                    {
                        if(log_enabled)Serial.println("\n--Error while overriding inputs to Firebase");
                    }                    
                    //check if we can do it parallel or set the flag just once and wait                                     
                }  

                //To Check Heap Size
                
                if(display_heap && log_enabled)
                {
                    Serial.print("\nFree Heap Size: ");
                    Serial.print(esp_get_free_heap_size());
                } 
                
                //before continuing upload all relevant info to firestore

                wait(10);       

                //Callback-based reaction for inputs if they changed on DB  
                firebase_check_for_input_change(firebase_log_mode_moderate);

                wait(10);

                //If a change is detected will be sent to database
                firebase_check_for_output_change(firebase_log_mode_moderate);

                wait(10);

                //TODO later eliminate this as this will be a background taks

                //cycle nr is the times we have refreshed the outputs
                if(oled_enabled && oled_token == oled_free && firebase_cycle_nr != firebase_cycle_last_known) 
                {
                    //if(log_enabled)Serial.println("\n---Sending to OLED---> "); 
                    wait(10); 
                    //TODO REMOVE THIS AFTER MUBEA TESToled_firebase(firebase_cycle_nr);
                    firebase_cycle_last_known = firebase_cycle_nr;
                }



                //TODO Reactivate this later , IGNORING FOR MUBEA -> firestore enabled = false;

                //Firestore-Related--------------------------------------------------------------------------------
                if(firestore_enabled && !firestore_initialized)
                {
                    wait(100);

                    if(log_enabled) Serial.print("\n--- Creating Firestore Task --- ");
                    
                    create_task_firestore();

                    //To Check Heap Size
                    if(display_heap && log_enabled)
                    {
                        Serial.print("\nFree Heap Size: ");
                        Serial.print(esp_get_free_heap_size());
                    } 

                    //Wait until fully initialized
                    if(log_enabled) Serial.print("\n--- Waiting for Firestore Initialization ");

                    while (!firestore_initialized)
                    {
                        if(log_enabled)Serial.print(".");
                        wait(1000);
                    } 

                    if(firestore_initialized && log_enabled) Serial.print("\n--- Firestore Task Initialized --- ");                         
                }

                //If there was a change on the parameters here wait for firestore to upload the new JSON
                if(firestore_enabled && firestore_needs_update)
                {
                    
                    if(log_enabled)Serial.print("\n---Waiting for Firestore Task to release the token---");
                    
                    wait(100);
                    //Clearing the Flag for accumulator
                    firestore_needs_update = false;
                    //Setting new flag for task
                    firestore_update_in_progress = true;

                    while(firestore_update_in_progress)
                    {
                        wait(10000);
                        if(log_enabled)Serial.print(" . ");
                        //Wait until the firestore task finish updating
                    } 

                    //To Check Heap Size
                    if(display_heap && log_enabled)
                    {
                        Serial.print("\nFree Heap Size: ");
                        Serial.print(esp_get_free_heap_size());
                    } 

                    if(log_enabled)Serial.print("\n--- Firestore Token Released !---");
                }

                // END OF Firestore Related ----------------------------------------------------------
                if(firebase_first_loop)
                {
                    if(log_enabled)Serial.print("\n--- End of First Full Firebase Loop --- ");
                    firebase_first_loop = false;
                }
                
            }
            else
            {
                Serial.print("\nERROR:");
                Serial.printf("firebase_connected : %d", firebase_connected);
                Serial.printf("Firebase.ready() : %d", Firebase.ready());
                Serial.printf("Wifi connected? (3 is ok) : %d", WiFi.status());    
                wait(1000);
            } 
        }
        wait(100);
    }  
}


















/*

// Callback function for Firestore updates
void firestoreCallback(FirebaseStream data) 
{
    Serial.println("\n🔥 Firestore data changed!");

    FirebaseJson json = data.jsonObject();
    FirebaseJsonData jsonData;

    if (json.get(jsonData, "leds_on")) leds_on = jsonData.boolValue;
    if (json.get(jsonData, "leds_brightness")) leds_brightness = jsonData.intValue;
    if (json.get(jsonData, "leds_color")) leds_color = jsonData.stringValue;
    if (json.get(jsonData, "park_alarm_on")) park_alarm_on = jsonData.boolValue;
    if (json.get(jsonData, "park_alarm_mode")) park_alarm_mode = jsonData.stringValue;
    if (json.get(jsonData, "park_alarm_snooze")) park_alarm_snooze = jsonData.intValue;
    if (json.get(jsonData, "accident_dismissed")) accident_dismissed = jsonData.boolValue;
    if (json.get(jsonData, "gps_enabled")) gps_enabled = jsonData.boolValue;
    if (json.get(jsonData, "gps_upload")) gps_upload = jsonData.boolValue;
    if (json.get(jsonData, "gps_refresh_seconds")) gps_refresh_seconds = jsonData.intValue;
    if (json.get(jsonData, "can_enabled")) can_enabled = jsonData.boolValue;
    if (json.get(jsonData, "can_refresh_seconds")) can_refresh_seconds = jsonData.intValue;

    Serial.println("🔄 Updated Local Variables:");
    Serial.printf("LEDs: %d, Brightness: %d, Color: %s\n", leds_on, leds_brightness, leds_color.c_str());
    Serial.printf("Park Alarm: %d, Mode: %s, Snooze: %d\n", park_alarm_on, park_alarm_mode.c_str(), park_alarm_snooze);
    Serial.printf("GPS: %d, Upload: %d, Refresh: %d sec\n", gps_enabled, gps_upload, gps_refresh_seconds);
    Serial.printf("CAN: %d, Refresh: %d sec\n", can_enabled, can_refresh_seconds);
}

void setup() {
    Serial.begin(115200);

    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n✅ Connected to WiFi!");

    // Configure Firebase
    config.api_key = API_KEY;
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    // Listen for Firestore changes on "callback_vars"
    if (Firebase.Firestore.beginStream(&fbdo, FIREBASE_PROJECT_ID, "callback_vars")) {
        Firebase.Firestore.setStreamCallback(&fbdo, firestoreCallback);
        Serial.println("✅ Listening for Firestore updates on /callback_vars...");
    } else {
        Serial.println("❌ Failed to start Firestore listener: " + fbdo.errorReason());
    }
}
*/





/*
JSON IMPLEMENTATION

// Refactored firebase_update_outputs() with batched JSON update

#include <FirebaseJson.h>

void firebase_update_outputs(int firebase_log_mode, bool force_update)
{
    if (firebase_log_mode > firebase_log_mode_silent)
    {
        if (force_update)
            Serial.println("\n---Forcing an Output Update (first loop or manual override) ----");
        else
            Serial.println();
    }

    FirebaseJson json;
    firebase_something_changed = false;

    // SENSOR DATA
    if (force_update || abs(board_temp - last_known_temp) >= firebase_temp_threshold)
    {
        json.set("sensors/temp", board_temp);
        last_known_temp = board_temp;
        firebase_something_changed = true;
    }

    if (force_update || abs(lux_val - last_known_lux) >= firebase_lux_threshold)
    {
        json.set("sensors/lux", lux_val);
        last_known_lux = lux_val;
        firebase_something_changed = true;
    }

    if (force_update || abs(bat_percent - last_known_soc) >= firebase_soc_threshold)
    {
        json.set("sensors/soc", bat_percent);
        last_known_soc = bat_percent;
        firebase_something_changed = true;
    }

    // STATUS
    if (force_update || charging != last_known_charging)
    {
        json.set("status/charging", charging);
        last_known_charging = charging;
        firebase_something_changed = true;
    }

    if (force_update || moving != last_known_moving)
    {
        json.set("status/moving", moving);
        last_known_moving = moving;
        firebase_something_changed = true;
    }

    if (force_update || usb_connected != last_known_usb_connected)
    {
        json.set("status/usb_connected", usb_connected);
        last_known_usb_connected = usb_connected;
        firebase_something_changed = true;
    }

    if (force_update || low_bat != last_known_low_bat)
    {
        json.set("status/low_bat", low_bat);
        last_known_low_bat = low_bat;
        firebase_something_changed = true;
    }

    if (force_update || main_status != last_known_main_status)
    {
        json.set("status/main_status", main_status);
        last_known_main_status = main_status;
        firebase_something_changed = true;
    }

    if (force_update || imu_running != last_known_imu_running)
    {
        json.set("status/imu_running", imu_running);
        last_known_imu_running = imu_running;
        firebase_something_changed = true;
    }

    // ACCIDENT
    if (force_update || accident_detected != last_known_accident_detected)
    {
        json.set("accident/detected", accident_detected);
        last_known_accident_detected = accident_detected;
        firebase_something_changed = true;
    }

    if (force_update || accident_confirmed != last_known_accident_confirmed)
    {
        json.set("accident/confirmed", accident_confirmed);
        last_known_accident_confirmed = accident_confirmed;
        firebase_something_changed = true;
    }

    // PARK ALARM
    if (force_update || backend_parking_alarm_triggered != last_known_parking_alarm_triggered)
    {
        json.set("park_alarm/triggered", backend_parking_alarm_triggered);
        last_known_parking_alarm_triggered = backend_parking_alarm_triggered;
        firebase_something_changed = true;
    }

    if (force_update || backend_parking_alarm_movement_detected != last_known_parking_alarm_movement_detected)
    {
        json.set("park_alarm/movement", backend_parking_alarm_movement_detected);
        last_known_parking_alarm_movement_detected = backend_parking_alarm_movement_detected;
        firebase_something_changed = true;
    }

    // GPS (example, limited set)
    if (gps_enabled && gps_upload)
    {
        if (force_update || abs(gps_latitude - last_known_gps_lat) >= firebase_gps_lat_threshold)
        {
            json.set("gps/lat", gps_latitude);
            last_known_gps_lat = gps_latitude;
            firebase_something_changed = true;
        }
        if (force_update || abs(gps_longitude - last_known_gps_lon) >= firebase_gps_lon_threshold)
        {
            json.set("gps/lon", gps_longitude);
            last_known_gps_lon = gps_longitude;
            firebase_something_changed = true;
        }
    }

    // HW INFO
    if (force_update)
    {
        json.set("hw_info/version", hw_version);
        json.set("hw_info/variant", hw_variant_string);
        firebase_something_changed = true;
    }

    // Final batch update
    if (firebase_something_changed)
    {
        if (Database.updateNode(aClient, base_path_with_id, json, process_data, "firebase_update_all"))
        {
            Serial.println("✅ Firebase updated with batched JSON.");
        }
        else
        {
            Serial.printf("❌ Firebase update failed: %s\n", aClient.error().message().c_str());
        }
    }
    else if (firebase_log_mode > firebase_log_mode_silent)
    {
        Serial.println("---No changes to update to Firebase---");
    }
}



*/

