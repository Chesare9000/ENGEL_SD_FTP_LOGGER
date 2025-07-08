








//NEED TO GET THE UPDATED CALLBACKS AND VALUES FROM GPS AND CAN
//ALSO THE NEW ORDER FROM THE STRUCTURE OF THE CONFIG 
//AND FIGURE OUT WHY THE VALUES OF GPS AND CACN ARE STILL NOT THERE

#include<Arduino.h>

//#include <Firebase_ESP_Client.h>
//#include <addons/TokenHelper.h>
//#include <addons/RTDBHelper.h>

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


unsigned int firestore_buffer = 128;

bool firestore_initialized = false;
//Flags
bool firestore_update_all = false;
//OUTPUTS
bool firestore_temp_needs_update = false; 
bool firestore_lux_needs_update = false; 
bool firestore_soc_needs_update = false; 
bool firestore_charging_needs_update = false; 
bool firestore_moving_needs_update = false; 
bool firestore_usb_connected_needs_update = false; 
bool firestore_low_bat_needs_update = false; 
bool firestore_main_status_needs_update = false; 
bool firestore_imu_running_needs_update = false; 
bool firestore_accident_detected_needs_update = false; 
bool firestore_accident_confirmed_needs_update = false; 
bool firestore_park_alarm_movement_needs_update = false; 
bool firestore_park_alarm_triggered_needs_update = false;
bool firestore_hw_info_version_needs_update = false; 
bool firestore_hw_info_variant_needs_update = false; 

bool firestore_gps_lat_needs_update = false;
bool firestore_gps_lon_needs_update = false;
bool firestore_gps_kph_needs_update = false;
bool firestore_gps_mph_needs_update = false;
bool firestore_gps_hea_needs_update = false;
bool firestore_gps_alt_needs_update = false;

bool firestore_can_vel_needs_update = false;
bool firestore_can_rpm_needs_update = false;
bool firestore_can_odo_needs_update = false;
bool firestore_can_soc_needs_update = false;


//INPUTS
bool firestore_leds_on_needs_update = false; 
bool firestore_leds_brightness_needs_update = false; 
bool firestore_leds_color_needs_update = false; 
bool firestore_park_alarm_on_needs_update = false; 
bool firestore_park_alarm_mode_needs_update = false; 
bool firestore_park_alarm_snooze_needs_update = false; 
bool firestore_accident_dismissed_needs_update = false;

bool firestore_gps_enabled_needs_update = false;
bool firestore_gps_upload_needs_update = false;
bool firestore_gps_refresh_seconds_needs_update = false;

bool firestore_can_enabled_needs_update = false;
bool firestore_can_refresh_seconds_needs_update = false;


//For Individual Ones
// Define an enum for different sensor types
enum firebase_id 
{
    TEMP,
    LUX,
    SOC,
    HW_INFO_VARIANT,
    HW_INFO_VERSION,
    LEDS_ON,
    LEDS_BRIGHTNESS,
    LEDS_COLOR,
    PARK_ALARM_MODE,
    PARK_ALARM_ON,
    PARK_ALARM_MOVEMENT,
    PARK_ALARM_TRIGGERED,
    PARK_ALARM_SNOOZE,
    STATUS_CHARGING,
    STATUS_LOW_BAT,
    STATUS_MOVING,
    STATUS_USB_CONNECTED,
    STATUS_MAIN_STATUS,
    STATUS_IMU_RUNNING,
    ACCIDENT_DETECTED,
    ACCIDENT_DISMISSED,
    ACCIDENT_CONFIRMED,
    GPS_LAT,
    GPS_LON,
    GPS_KPH,
    GPS_MPH,
    GPS_HEA,
    GPS_ALT,
    GPS_ENABLED,
    GPS_UPLOAD,
    GPS_REFRESH_SECONDS,
    CAN_VEL,
    CAN_ACC,
    CAN_ODO,
    CAN_SOC,
    CAN_ENABLED,
    CAN_REFRESH_SECONDS
};

/*
// Utility function to format Firestore path using snprintf
void format_firestore_path(char* path_buffer, const char* format, int id)
{
    snprintf(path_buffer, firestore_buffer, format, id);
}

// Function to update Firestore (patch or create document)
void update_firestore(const char* path, FirebaseJson& content, const char* fields,firestore_log_mode log_mode)
{
    if (Firebase.Firestore.patchDocument(&fbdo, firebase_project_id, "", path, content.raw(), fields)) {
        if (log_enabled && log_mode > firestore_log_mode_silent) {
            Serial.printf("\n--JSON Successfully patched on Firestore: %s\n", path);
            if (log_mode > firestore_log_mode_moderate) {
                Serial.println(fbdo.payload().c_str());
            }
        }
    } else {
        if (log_enabled && log_mode > firestore_log_mode_silent) {
            Serial.printf("\nERROR ON JSON PATCH for %s: %s\n", path, fbdo.errorReason());
        }
        if (Firebase.Firestore.createDocument(&fbdo, firebase_project_id, "", path, content.raw())) {
            if (log_enabled && log_mode > firestore_log_mode_silent) {
                Serial.printf("\n--JSON Successfully created on Firestore: %s\n", path);
                if (log_mode > firestore_log_mode_moderate) {
                    Serial.println(fbdo.payload().c_str());
                }
            }
        } else {
            if (log_enabled && log_mode > firestore_log_mode_silent) {
                Serial.printf("\nERROR ON JSON CREATION for %s: %s\n", path, fbdo.errorReason());
            }
        }
    }
}

// Function to log the Firestore path
void log_firestore_path(const char* path, const char* section,firestore_log_mode log_mode)
{
    if (log_enabled && log_mode > firestore_log_mode_moderate) {
        Serial.printf("\n  %s Path : %s\n", section, path);
        wait(100);
    }
}

// Function to set FirebaseJson content based on provided fields
void set_firestore_content(FirebaseJson& content, const std::map<String, String>& fields)
{
    content.clear();
    for (std::map<String, String>::const_iterator it = fields.begin(); it != fields.end(); ++it) {
        content.set(it->first, it->second);
    }
}



//This fns will be used to update the firestore
//after the firebase has been updated

//We try to patch the document first , 
//if it has an error then we will create the document 

// Generic function to update Individual Firestore JSON Properties

bool firestore_update_json(firebase_id firebase_id, const String& value, firestore_log_mode log_mode) 
{
    if(log_enabled && log_mode > firestore_log_mode_silent) 
    {
        Serial.printf("\n ---- Updating Firestore JSON for %s", String(firebase_id));
    }

    char firestore_path[firestore_buffer] = "";
    int char_size = 0;
    FirebaseJson content;

    // Construct the Firestore path based on sensor type
    switch(firebase_id) 
    {
        case TEMP:
        case LUX:
        case SOC:
            char_size = snprintf(firestore_path, firestore_buffer, "devices/taillight/%d/sensors", esp_id);
            content.set("fields/" + std::string((firebase_id == TEMP) ? "temp" : (firebase_id == LUX) ? "lux" : "soc") + "/doubleValue", value);
        break;

        case HW_INFO_VARIANT:
        case HW_INFO_VERSION:
            char_size = snprintf(firestore_path, firestore_buffer, "devices/taillight/%d/hw_info", esp_id);
            
            if (firebase_id == HW_INFO_VARIANT) content.set("fields/variant/stringValue", value);      
            else content.set("fields/version/doubleValue", value);
    
        break;

        case LEDS_ON:
        case LEDS_COLOR:
        case LEDS_BRIGHTNESS:

            char_size = snprintf(firestore_path, firestore_buffer, "devices/taillight/%d/leds", esp_id);

            if     (firebase_id == LEDS_ON)          content.set("fields/on/booleanValue", value);
            else if(firebase_id == LEDS_COLOR)       content.set("fields/color/stringValue", value);
            else if (firebase_id == LEDS_BRIGHTNESS) content.set("fields/brightness/doubleValue", value);
            else {if(log_enabled)Serial.println(" --- ERROR ON Firestore update request for LEDS --");}

        break;

        case PARK_ALARM_ON:
        case PARK_ALARM_MODE:
        case PARK_ALARM_MOVEMENT:
        case PARK_ALARM_TRIGGERED:
        case PARK_ALARM_SNOOZE:

            char_size = snprintf(firestore_path,firestore_buffer,"devices/taillight/%d/park_alarm",esp_id);     

            if     (firebase_id == PARK_ALARM_ON)        content.set("fields/on/booleanValue", value);
            else if(firebase_id == PARK_ALARM_MODE)      content.set("fields/mode/booleanValue", value);
            else if(firebase_id == PARK_ALARM_MOVEMENT)  content.set("fields/movement/booleanValue", value);
            else if(firebase_id == PARK_ALARM_TRIGGERED) content.set("fields/triggered/booleanValue", value);
            else if(firebase_id == PARK_ALARM_SNOOZE)    content.set("fields/snooze/booleanValue", value);
            else {if(log_enabled)Serial.println(" --- ERROR ON Firestore update request for PARK_ALARM --");}

        break;

        case GPS_LAT:
        case GPS_LON:
        case GPS_KPH:
        case GPS_MPH:
        case GPS_HEA:
        case GPS_ALT:
        case GPS_ENABLED:
        case GPS_UPLOAD:
        case GPS_REFRESH_SECONDS:

            char_size = snprintf(firestore_path,firestore_buffer,"devices/taillight/%d/gps",esp_id); 
            
            if     (firebase_id == GPS_LAT)             content.set("fields/lat/doubleValue", value);
            else if(firebase_id == GPS_LON)             content.set("fields/lon/doubleValue", value);
            else if(firebase_id == GPS_KPH)             content.set("fields/kph/doubleValue", value);
            else if(firebase_id == GPS_MPH)             content.set("fields/mph/doubleValue", value);
            else if(firebase_id == GPS_HEA)             content.set("fields/hea/doubleValue", value);
            else if(firebase_id == GPS_ALT)             content.set("fields/alt/doubleValue", value);
            else if(firebase_id == GPS_ENABLED)         content.set("fields/enabled/booleanValue", value);
            else if(firebase_id == GPS_UPLOAD)          content.set("fields/upload/booleanValue", value);
            else if(firebase_id == GPS_REFRESH_SECONDS) content.set("fields/refresh_seconds/doubleValue", value);

            else {if(log_enabled)Serial.println(" --- ERROR ON Firestore update request for GPS --");}

        break;


        case CAN_VEL:
        case CAN_ACC:
        case CAN_ODO:
        case CAN_SOC:
        case CAN_ENABLED:
        case CAN_REFRESH_SECONDS:

            char_size = snprintf(firestore_path,firestore_buffer,"devices/taillight/%d/can",esp_id); 
            
            if     (firebase_id == CAN_VEL)             content.set("fields/vel/doubleValue", value);
            else if(firebase_id == CAN_ACC)             content.set("fields/acc/doubleValue", value);
            else if(firebase_id == CAN_ODO)             content.set("fields/odo/doubleValue", value);
            else if(firebase_id == CAN_SOC)             content.set("fields/soc/doubleValue", value);
            else if(firebase_id == CAN_ENABLED)         content.set("fields/enabled/booleanValue", value);
            else if(firebase_id == CAN_REFRESH_SECONDS) content.set("fields/refresh_seconds/doubleValue", value);

            else {if(log_enabled)Serial.println(" --- ERROR ON Firestore update request for CAN --");}

        break;



        case STATUS_CHARGING:
        case STATUS_LOW_BAT:
        case STATUS_MOVING:
        case STATUS_USB_CONNECTED:
        case STATUS_MAIN_STATUS:
        case STATUS_IMU_RUNNING:
  
            char_size = snprintf(firestore_path,firestore_buffer,"devices/taillight/%d/status",esp_id);

            if     (firebase_id == STATUS_CHARGING)      content.set("fields/charging/booleanValue", value);
            else if(firebase_id == STATUS_LOW_BAT)       content.set("fields/low_bat/booleanValue", value);
            else if(firebase_id == STATUS_MOVING)        content.set("fields/moving/booleanValue", value);
            else if(firebase_id == STATUS_USB_CONNECTED) content.set("fields/usb_connected/booleanValue", value);
            else if(firebase_id == STATUS_MAIN_STATUS)   content.set("fields/main_status/booleanValue", value);
            else if(firebase_id == STATUS_IMU_RUNNING)   content.set("fields/imu_running/booleanValue", value);
            else {if(log_enabled)Serial.println(" --- ERROR ON Firestore update request for STATUS --");}        
        
        break;

        case ACCIDENT_DETECTED:
        case ACCIDENT_CONFIRMED:
        case ACCIDENT_DISMISSED:

            char_size = snprintf(firestore_path,firestore_buffer,"devices/taillight/%d/status",esp_id);

            if     (firebase_id == ACCIDENT_DETECTED)  content.set("fields/detected/booleanValue", value);
            else if(firebase_id == ACCIDENT_CONFIRMED) content.set("fields/confirmed/booleanValue", value);
            else if(firebase_id == ACCIDENT_DISMISSED) content.set("fields/dismissed/booleanValue", value);
            else {if(log_enabled)Serial.println(" --- ERROR ON Firestore update request for ACCIDENT --");}                
        
        break;

        default:
            
            if(log_enabled)Serial.println(" --- ERROR ON Firestore update request : INVALID ID --");
            return false; // Invalid sensor type
    }

    if(log_enabled && log_mode > firestore_log_mode_moderate) 
    {
        Serial.print("\n  Sensor Path : ");
        Serial.print(firestore_path);
        Serial.printf("\nPath_size: %d chars", char_size);
    }

    content.clear();
    
    // First we will patch (if the document already exists, if not then we will create the new doc)
    if (Firebase.Firestore.patchDocument(&fbdo, firebase_project_id, "", firestore_path, content.raw(), "sensor_data"))
    {
        if(log_enabled && log_mode > firestore_log_mode_silent) 
        {
            Serial.print("\n-- Data successfully patched on Firestore");

            if(log_mode > firestore_log_mode_moderate) 
            {
                Serial.println();
                Serial.print(fbdo.payload().c_str());
            }
            return true;
        }
    } 
    else 
    { // Error handling
        if(log_enabled && log_mode > firestore_log_mode_silent) 
        {
            Serial.print("\n ERROR on patch! , Trying to create field : ");
            Serial.println(fbdo.errorReason());
        }
        
        // Try to create a new document
        if (Firebase.Firestore.createDocument(&fbdo, firebase_project_id, "", firestore_path, content.raw())) 
        {
            if(log_enabled && log_mode > firestore_log_mode_silent) 
            {
                Serial.print("\n-- Data successfully created on Firestore");
                if(log_mode > firestore_log_mode_moderate) 
                {
                    Serial.println();
                    Serial.print(fbdo.payload().c_str());
                }
                return true;
            }
        } 
        else 
        {
            if(log_enabled && log_mode > firestore_log_mode_silent) 
            {
                Serial.print("\nERROR on creation! : ");
                Serial.println(fbdo.errorReason());
            }
            return false;
        }
    }
    return false; // Default return value if not successful
}


//TODO : ON CRITICAL SIGNALS PERSIST IF FAILED

void check_firestore_request(firestore_log_mode firestore_log_mode)
{
    //ALL
    if(firestore_update_all)
    {
        if (firestore_log_mode > firestore_log_mode_silent) 
        {
            Serial.print("\n---Updating All Values on Firestore---");          
        }
        //here we dont check for return (TODO maybe later : integrate into individual functions and log errors without stopping )
        //firestore_update_json_all(firestore_log_mode); 

        firestore_temp_needs_update = true;
        firestore_lux_needs_update = true;
        firestore_soc_needs_update = true;
        firestore_hw_info_version_needs_update = true;
        firestore_hw_info_variant_needs_update = true;
        firestore_leds_on_needs_update = true;
        firestore_leds_brightness_needs_update = true;
        firestore_leds_color_needs_update = true;
        firestore_park_alarm_on_needs_update = true;
        firestore_park_alarm_mode_needs_update = true;
        firestore_park_alarm_movement_needs_update = true;
        firestore_park_alarm_triggered_needs_update = true;
        firestore_park_alarm_snooze_needs_update = true;
        firestore_charging_needs_update = true;
        firestore_moving_needs_update = true;
        firestore_usb_connected_needs_update = true;
        firestore_low_bat_needs_update = true;
        firestore_main_status_needs_update  = true;
        firestore_imu_running_needs_update = true;
        firestore_accident_detected_needs_update = true;
        firestore_accident_confirmed_needs_update = true;
        firestore_accident_dismissed_needs_update = true;

        firestore_gps_lat_needs_update = true;
        firestore_gps_lon_needs_update = true;
        firestore_gps_kph_needs_update = true;
        firestore_gps_mph_needs_update = true;
        firestore_gps_hea_needs_update = true;
        firestore_gps_alt_needs_update = true;

        firestore_can_vel_needs_update = true;
        firestore_can_rpm_needs_update = true;
        firestore_can_odo_needs_update = true;
        firestore_can_soc_needs_update = true;

        firestore_update_all = false;
    }

    //SENSORS

    //TEMP
    if (firestore_temp_needs_update)
    {
        if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
        {
            Serial.print("\n--- Updating Temp on Firestore ");
        }
        
        if(firestore_update_json(TEMP, String(board_temp), firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---temp updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_temp_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on temp update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    } 

    //LUX
    if (firestore_lux_needs_update)
    {
        if(firestore_update_json(LUX, String(lux_val), firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---lux updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_lux_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on lux update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }   
    //SOC
    if (firestore_soc_needs_update)
    {
        if(firestore_update_json(SOC, String(bat_percent), firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---soc updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_soc_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on soc update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }     
    //HW_INFO
    //HW_VERSION
    if (firestore_hw_info_version_needs_update)
    {
        if(firestore_update_json(HW_INFO_VERSION, String(hw_version), firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---hw_info_version updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_hw_info_version_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on hw_info_version update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }

    } 
    //HW_VARIANT
    if (firestore_hw_info_variant_needs_update) 
    {

        update_hw_variant_string(); // Make sure to call this function before updating variant
    
        if(firestore_update_json(HW_INFO_VARIANT, hw_variant_string, firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---hw_info_variant updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_hw_info_variant_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on hw_info_variant update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }
    
    //LEDS
    //ON
    if (firestore_leds_on_needs_update)
    {
        if(firestore_update_json(LEDS_ON, String(backend_led_status), firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---leds_on updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_leds_on_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on leds_on update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }

    }        
    
    if(firestore_leds_brightness_needs_update)
    {
        if(firestore_update_json(LEDS_BRIGHTNESS, String(backend_led_brightness), firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---leds_brightness updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_leds_brightness_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on leds_on update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }

    }
    
    if(firestore_leds_color_needs_update)        
    {
        // Make sure to call this function before updating color
        update_backend_led_color_string();
        if(firestore_update_json(LEDS_COLOR, backend_led_color_string , firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---leds_color updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_leds_color_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on leds_color update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }

    //PARK_ALARM
    if (firestore_park_alarm_on_needs_update)
    {
        if(firestore_update_json(PARK_ALARM_ON,String(backend_parking_alarm_state),firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---park_alarm_on updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_park_alarm_on_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on park_alarm_on update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }

    if(firestore_park_alarm_mode_needs_update)
    {
        if(firestore_update_json(PARK_ALARM_MODE,String(backend_parking_alarm_mode),firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---park_alarm_mode updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_park_alarm_mode_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on park_alarm_mode update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }

    }   
    
    if(firestore_park_alarm_movement_needs_update)
    {
        if(firestore_update_json(PARK_ALARM_MOVEMENT,String(backend_parking_alarm_movement_detected),firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---park_alarm_movement updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_park_alarm_movement_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on park_alarm_movement update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }

    if(firestore_park_alarm_triggered_needs_update)
    {
        if(firestore_update_json(PARK_ALARM_TRIGGERED,String(backend_parking_alarm_triggered),firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---park_alarm_triggered updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_park_alarm_triggered_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on park_alarm_triggered update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }

    }

    if(firestore_park_alarm_snooze_needs_update)
    {
        if(firestore_update_json(PARK_ALARM_SNOOZE,String(parking_alarm_snooze),firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---park_alarm_snooze updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_park_alarm_snooze_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on park_alarm_snooze update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }
    
    //STATUS
    if(firestore_charging_needs_update)
    {
        if(firestore_update_json(STATUS_CHARGING , String(charging) ,firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---status_charging updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_charging_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on status_charging update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }      
    
    if(firestore_moving_needs_update)
    {
        if(firestore_update_json(STATUS_MOVING , String(moving) ,firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---status_moving updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_moving_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on status_moving update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }   

    if(firestore_usb_connected_needs_update)
    {
        if(firestore_update_json(STATUS_USB_CONNECTED , String(usb_connected) ,firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---status_usb_connected updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_usb_connected_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on status_usb_connected update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }

    } 
    
    if(firestore_low_bat_needs_update)
    {
        if(firestore_update_json(STATUS_LOW_BAT , String(low_bat) ,firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---status_low_bat updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_low_bat_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on status_low_bat update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }   

    if(firestore_main_status_needs_update)
    {
        if(firestore_update_json(STATUS_MAIN_STATUS , String(main_status) ,firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---status_main_status updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_main_status_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on status_main_status update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }

    }   
    
    if(firestore_imu_running_needs_update)   
    {
        if(firestore_update_json(STATUS_IMU_RUNNING , String(imu_running) ,firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---status_imu_running updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_imu_running_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on status_imu_running update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }

    //ACCIDENT
    if(firestore_accident_detected_needs_update)
    {
        if(firestore_update_json(ACCIDENT_DETECTED ,String(accident_detected),firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---accident_detected updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_accident_detected_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on accident_detected update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }

    if(firestore_accident_confirmed_needs_update)
    {
        if(firestore_update_json(ACCIDENT_CONFIRMED , String(accident_confirmed) ,firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---accident_confirmed updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_accident_confirmed_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on accident_confirmed update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }

    }

    if(firestore_accident_dismissed_needs_update)
    {
        if(firestore_update_json(ACCIDENT_DISMISSED , String(accident_dismissed) ,firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---accident_dismissed updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_accident_dismissed_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on accident_dismissed update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }

    //GPS

    if(firestore_gps_lat_needs_update)
    {
        if(firestore_update_json(GPS_LAT , String(gps_latitude) ,firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---gps_lat updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_gps_lat_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on gps_lat update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }

    if(firestore_gps_lon_needs_update)
    {
        if(firestore_update_json(GPS_LON , String(gps_longitude) ,firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---gps_lon updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_gps_lon_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on gps_lon update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }

    if(firestore_gps_kph_needs_update)
    {
        if(firestore_update_json(GPS_KPH , String(gps_speed_kmh) ,firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---gps_kph updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_gps_kph_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on gps_lon update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }

    if(firestore_gps_mph_needs_update)
    {
        if(firestore_update_json(GPS_MPH , String(gps_speed_mph) ,firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---gps_mph updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_gps_mph_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on gps_mph update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }

    if(firestore_gps_hea_needs_update)
    {
        if(firestore_update_json(GPS_HEA , String(gps_heading) ,firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---gps_hea updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_gps_hea_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on gps_hea update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }

    if(firestore_gps_alt_needs_update)
    {
        if(firestore_update_json(GPS_ALT , String(gps_altitude) ,firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---gps_alt updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_gps_alt_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on gps_alt update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }

    if(firestore_gps_enabled_needs_update)
    {
        if(firestore_update_json(GPS_ENABLED , String(gps_enabled) ,firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---gps_enabled updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_gps_enabled_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on gps_enabled update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }

    if(firestore_gps_upload_needs_update)
    {
        if(firestore_update_json(GPS_UPLOAD , String(gps_upload) ,firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---gps_upload updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_gps_upload_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on gps_upload update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }

    if(firestore_gps_refresh_seconds_needs_update)
    {
        if(firestore_update_json(GPS_REFRESH_SECONDS , String(gps_refresh_seconds) ,firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---gps_refresh_seconds updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_gps_refresh_seconds_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on gps_refresh_seconds update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }

    //CAN

    if(firestore_can_vel_needs_update)
    {
        if(firestore_update_json(CAN_VEL , String(can_vel) ,firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---can_vel updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_can_vel_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on can_vel update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }

    if(firestore_can_rpm_needs_update)
    {
        if(firestore_update_json(CAN_ACC , String(can_rpm) ,firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---can_rpm updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_can_rpm_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on can_rpm update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }

    if(firestore_can_odo_needs_update)
    {
        if(firestore_update_json(CAN_ODO , String(can_odo) ,firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---can_odd updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_can_odo_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on can_odo update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }

    if(firestore_can_soc_needs_update)
    {
        if(firestore_update_json(CAN_SOC , String(can_soc) ,firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---can_soc updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_can_soc_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on can_soc update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }

    if(firestore_can_enabled_needs_update)
    {
        if(firestore_update_json(CAN_ENABLED , String(can_enabled) ,firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---can_enabled updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_can_enabled_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on can_enabled update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }

    if(firestore_can_refresh_seconds_needs_update)
    {
        if(firestore_update_json(CAN_REFRESH_SECONDS , String(can_refresh_seconds) ,firestore_log_mode_moderate))
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_moderate)
            {
                Serial.print("\n---can_refresh_seconds updated in Firestore");
            }
            //Resetting Flag to indicate success
            firestore_can_refresh_seconds_needs_update = false;
        }
        else
        {
            if(log_enabled && firestore_log_mode > firestore_log_mode_silent)
            {
                Serial.print("\n---error on can_refresh_seconds update in Firestore");
            }
            //Try again next iteration (keep flag up but not persist)
        }
    }


}

//TASK that will run concurrently with Firebase

//Firestore Task---------------------------------------------------------------------------------------------------- 
//Main Task to keep Firestore alive

TaskHandle_t task_firestore_handle = NULL;

void create_task_firestore() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n--- Creating task_firestore--");
    wait(100);
    
    task_firestore_i2c_declare();
    wait(100);

    BaseType_t result = xTaskCreate
    (
        task_firestore,   //Function Name (must be a while(1))
        "task_firestore", //Logging Name
        8500,                //Stack Size
        NULL,                //Passing Parameters
        13,                   //Task Priority 
        &task_firestore_handle
    );  

    if (result != pdPASS) 
    {
        Serial.print("Task creation failed! Reason: ");
        
        switch (result) 
        {
            case errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY:
                Serial.println("Could not allocate required memory.");
                break;
            case errQUEUE_BLOCKED:
                Serial.println("Queue blocked.");
                break;
            default:
                Serial.println("Other error.");
                break;
        }
    } 
    else
    {
       if(log_enabled) Serial.print("-- done --\n");
    }

    wait(100);
    
}

void task_firestore_i2c_declare()
{
    if(log_enabled)Serial.print("\n--- Task_firestore_i2c_declared\n");
    wait(100);
    //This Taks will use the following I2C_Devs
    

    //imu_needed++;
    //rgb_needed++;
    //temp_needed++;
    //lux_needed++;
    //rtc_needed++;
    //fuelgauge_needed++;
    oled_needed++;
}

void task_firestore_i2c_release()
{
    if(log_enabled)Serial.print("\ntask_firestore_i2c_released\n");
    wait(100);
    
    //This Taks will release the following I2C_Devs
    
    //imu_needed--;
    //rgb_needed--;
    //temp_needed--;
    //lux_needed--;
    //rtc_needed--;
    //fuelgauge_needed--;
    oled_needed--;
}



void task_firestore(void * parameters)
{   
    if(log_enabled)Serial.print("\n---Running task_firestore---");
    wait(100);  

    //Serial.print("\nFree Heap Size: ");
    //Serial.print(esp_get_free_heap_size());

    if(log_enabled)Serial.print("\n--- task_firestore idle until request is received ---");   
    wait(100);

    firestore_initialized = true; 

    while(1)
    {           
        if(!task_firebase_active) //Getting Out if the Firebase task gets destroyed
        {
            wait(500);
            if(log_enabled)Serial.print("\n---Firestore Task Destroyed due to Firebase Deactivation---\n");
            task_firestore_i2c_release();
            firestore_initialized = false;
            wait(100);
            vTaskDelete(task_firestore_handle);//Delete itself
        }

        //If we are here Firebase is running so now lets check for request to update    
        //Update requested if true 
        else if(firestore_update_in_progress)
        {
            if(Firebase.ready() && WiFi.status() == WL_CONNECTED)
            {
                if(log_enabled) Serial.print("\n---Updating Firestore JSON upon Request---");
                    
                //Main Function
                check_firestore_request(firestore_log_mode_moderate);
                
                //TODO LATER RETRY ON ERROR FOR CRITICAL PARAMETERS (convert the fn into a bool)
                firestore_update_in_progress = false;   

                if(log_enabled) Serial.print("\n--- Firestore JSON Update Routine Finished ---"); 
                
            }

            //LOGS 
            else if (!Firebase.ready()) //Firebase Not Ready
            {
                if(log_enabled)
                {
                    Serial.print("\n-- Waiting for Firestore to get Ready --");
                }
            }

            else if (WiFi.status() != WL_CONNECTED)
            {
                if(log_enabled)
                {
                    Serial.print("\n-- Firestore : Waiting for WIFI to connect --");
                }
            }

            else //Something Weird Happened , printing status
            {
                if(log_enabled)
                {
                    Serial.print("\n--- Error ---"); 
                    Serial.printf("\nFirebase.ready()? (true) :%d",Firebase.ready());
                    Serial.printf("\n WiFi.status() == 3? (connected) :%d",WiFi.status());
                    Serial.printf("\ntask_firebase_active? (false) :%d", task_firebase_active );
                }
            }
            wait(100);
        }
        else wait(100);           
    }
}

*/