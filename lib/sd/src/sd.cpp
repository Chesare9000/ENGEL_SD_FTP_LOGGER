//ORIGINAL BLACK BOX IS ON THE MUBEA PILOT FW , THIS IS A MINIMAL VERSION



#include <Arduino.h>

#include <vars.h>
#include <sd.h>
#include <rtc.h>
#include <tools.h>
#include <imu.h>
#include <WiFi.h>
#include <oled.h>


//Setting correct pins to the SD
int esp_pin_sd_v_miso = 19;
int esp_pin_sd_v_mosi = 23;
int esp_pin_sd_v_clk = 18;
int esp_pin_sd_v_cs = 5;

int sd_miso = esp_pin_sd_v_miso;
int sd_mosi = esp_pin_sd_v_mosi;
int sd_clk = esp_pin_sd_v_clk;
int sd_cs = esp_pin_sd_v_cs;

double sd_space_total_bytes = 0;
double sd_space_used_bytes = 0;
double sd_space_left_bytes = 0;

double sd_space_total_mb =0;
double sd_space_used_mb =0;
double sd_space_available_mb =0;

float sd_space_used_percent =0;

int sd_space_used_percent_int = 0; //Percentage of space used on the SD card
int sd_space_total_gb_int = 0; //Total space on the SD card in GB



bool sd_initialized = false;

//bool black_box_mode_enabled = false;

bool black_box_enabled = true; //Black Box is enabled by default

bool task_sd_active = false;

//Black Box Logging Interval     
int black_box_logging_interval_milliseconds_default = 100; //Default value for the black box logging interval
int black_box_logging_interval_milliseconds = black_box_logging_interval_milliseconds_default; //Default value for the black box logging interval


uint64_t black_box_log_nr = 0;

int black_box_log_nr_serial_counter = 0; //serial monitor gap betwen logs
int black_box_log_nr_firebase_counter = 0; //serial monitor gap betwen logs

bool black_box_need_firebase_update = false;

bool black_box_first_loop = true;

//Later make a nice struct or something

//Name of the file will be defined on the first loop
String sd_file_name = "";

//Placeholder to append data here
String sd_data_header = ""; 
String sd_data_payload = "";

int black_box_serial_iterations_gap_default = 10000;
int black_box_serial_iterations_gap = black_box_serial_iterations_gap_default;

int black_box_firebase_iterations_gap_default = 10000;
int black_box_firebase_iterations_gap = black_box_firebase_iterations_gap_default;




int black_box_serial_mode = black_box_serial_formatted;

//VALUES TO APPEND

bool black_box_log_nr_enabled = true;

bool black_box_timestamp_log_enabled  = true;

bool black_box_imu_pitch_log_enabled = true;
bool black_box_imu_roll_log_enabled = true;
bool black_box_imu_yaw_log_enabled = true;

bool black_box_imu_acc_x_log_enabled  = true;
bool black_box_imu_acc_y_log_enabled  = true;
bool black_box_imu_acc_z_log_enabled  = true;

bool black_box_soc_log_enabled       = true;
bool black_box_lux_log_enabled       = true;
bool black_box_temp_log_enabled      = true;


bool black_box_mubea_can_motor_power_log_enabled    = true;  
bool black_box_mubea_can_motor_rpm_log_enabled      = true;  
bool black_box_mubea_can_motor_temp_log_enabled     = true; 
bool black_box_mubea_can_gen_power_log_enabled      = true;  
bool black_box_mubea_can_assist_level_log_enabled   = true;

bool black_box_mubea_can_soc_log_enabled           = true;   
bool black_box_mubea_can_soh_log_enabled           = true;
bool black_box_mubea_can_power_log_enabled         = true;   
bool black_box_mubea_can_voltage_log_enabled       = true;
bool black_box_mubea_can_temperature_log_enabled   = true;

bool black_box_mubea_can_speed_log_enabled         = true; 
bool black_box_mubea_can_direction_log_enabled     = true; 
bool black_box_mubea_can_gear_log_enabled          = true;
bool black_box_mubea_can_mileage_log_enabled       = true;
        
bool black_box_mubea_can_error_code_log_enabled     = true;
bool black_box_mubea_can_recuperation_log_enabled  = true; 

bool black_box_update_gap_ms_enabled  = true; 


bool black_box_running = false;

//TODO NOW --ADD TO FIREBASE THE CURRENT LOG FILE AND LOG NUMBER
//TODO NOW --ALSO ADD THE LAST WAKEUP DATE AND TIME


int last_logged_day = -1;
int last_logged_month = -1;
int last_logged_year = -1;


//minimum space left before start overwritting on sd card
int sd_minimimum_free_space_mb = 1; //at 1MB left we will start overwritting the oldest file

//TODO : Check For Internet Time
//https://randomnerdtutorials.com/esp32-microsd-card-arduino/


//TASK SD -----------------------------------------------------

TaskHandle_t task_sd_handle = NULL;

int task_priority_sd = 10;


void create_task_sd() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n-- creating task_sd --");

    task_sd_i2c_declare();

    xTaskCreate
    (
        task_sd,           //Function Name
        "task_sd",    //Logging Name
        8192,                //Stack Size
        NULL,                //Passing Parameters
        task_priority_sd,//Task Priority
        &task_sd_handle
    );

    task_sd_active = true;

    if(log_enabled) Serial.print("-- done --\n");

}

void task_sd_i2c_declare()
{
    if(log_enabled)Serial.print("\ntask_sd_i2c_declared\n");
    //This Taks will use the following I2C_Devs

    imu_needed++;
    //rgb_needed++;
    temp_needed++;
    lux_needed++;
    rtc_needed++;
    //fuelgauge_needed++;
    oled_needed++;

    //DEFINING THIS TASK TIME CRITICAL TO REFRESH THE OLED ASAP
    //time_critical_tasks_running++;

}

void task_sd_i2c_release()
{
    if(log_enabled)Serial.print("\ntask_sd_i2c_released\n");
    //This Taks will release the following I2C_Devs
    
    imu_needed--; 
    //rgb_needed--;
    temp_needed--;
    lux_needed--;
    rtc_needed--;
    //fuelgauge_needed--;
    oled_needed--;

    //RELEASING THE TIME_CRITICAL FLAG TO ALLOW FOR NON-CRITICAL REFRESH
    //time_critical_tasks_running--;
}


void task_sd(void * parameters)
{ 

    if(log_enabled) Serial.print("\n\n------Starting Task SD -----");

    sd_initialized = sd_init();

    if(!sd_initialized)
    {
        Serial.print("\n\n------ERROR: SD Not Detected -----");
        Serial.print("\n------Retrying in 5 seconds -----");
        wait(5000);
        sd_initialized = sd_init();        
    }

    if(!sd_initialized)
    {
        //TEST : DELETE THE TASK AND RETRY
        Serial.print("\n------Killing task_sd -----\n");
        task_sd_active = false;
        sd_initialized = false;
        black_box_running = false;
        //Here close the SD before exiting

        Serial.print("---\n task_sd terminated ----");

        vTaskDelete(NULL); 
    }

    if(sd_initialized)
    {
        Serial.println("\n---SD OK---");
        Serial.println("\n---Initializing Black_Box --");

        //Reset Log number every time the task restarts
        black_box_log_nr = 0;
        black_box_first_loop = true; 
    }  

    //In case the IMU is still not ready
    if(imu_needed > 0 && (!imu_running || !imu_initialized))
    {
        unsigned long timer = millis();

        while (1)
        { 
            wait(10);

            if(imu_running) break;
            if(imu_initialized) break;
            
            if(millis() > timer + 10000)
            {
                Serial.print("---ERROR : IMU not ready after waiting time while in black_box , proceeding anyway ");
                break;
            }   
        }
    }
    
    //If the imu was already running before starting sd_-task()
    //The very moment before starting we will recallibrate the IMU
    if(imu_needed > 1)
    {
        recalibrate_imu_via_i2c_manager(imu_recalibrate_log_handler_moderate);
    }

    Serial.println("\n--Running Black_Box--\n");

    black_box_running = true;

    while(1) //SD initialized correctly
    {
        if(!task_sd_active)//Order was sent to terminate the task
        {
            task_sd_active = false;
            sd_initialized = false;
            black_box_running = false;
            //Here close the SD before exiting

            Serial.print("---\n task_sd terminated ----");

            vTaskDelete(NULL); 
        }

        else
        {
            //Taking global_vars_mutex and running the black_blox
            if (xSemaphoreTake(global_vars_mutex, portMAX_DELAY) == pdTRUE)
            {
                if(ensure_sd_space(sd_minimimum_free_space_mb * 1024 * 1024, false)) // MB in bytes
                {
                    run_black_box();
                    // Release the mutex after SD card operations
                    xSemaphoreGive(global_vars_mutex);
                    //Guarantee Toggle on the refresh_interval
                    wait(black_box_logging_interval_milliseconds); 
                }
                else
                {
                    Serial.println("---ERROR : No space left even after deletion.");
                    wait(10000);
                }                
            }
            else
            {
                //Serial.print("\n---global_var_mutex not available ---");
                wait(10);
            }
        }
    }        
}

bool ensure_sd_space(int minFreeBytes, bool log_to_serial)// 1 MB = 1024 * 1024 bytes
{ 
    //Might not consider partitions so better check free size
    double cardSize = (double)SD.cardSize() / (1024.0 * 1024.0 * 1024.0); 

    sd_space_total_bytes = SD.totalBytes();
    sd_space_used_bytes  = SD.usedBytes();

    sd_space_left_bytes = sd_space_total_bytes - sd_space_used_bytes;

    sd_space_total_mb = (double)sd_space_total_bytes/ (1024.0 * 1024.0);
    sd_space_used_mb = (double)sd_space_used_bytes / (1024.0 * 1024.0);
    sd_space_available_mb = (double)sd_space_left_bytes / (1024.0 * 1024.0);

    sd_space_used_percent = (double)(sd_space_used_mb * 100.0) / (double)sd_space_total_mb ;

    //DATA for FIREBASE
    
    //Percentage of space used on the SD card
    sd_space_used_percent_int = (int)sd_space_used_percent;
    //Total space on the SD card in GB
    sd_space_total_gb_int = (int)((double)sd_space_total_bytes/ (1024.0 * 1024.0 * 1024.0)); 


    if(black_box_first_loop || log_to_serial)
    {
        // Optional: minimal print
        /*
        Serial.printf("SD Space: %.2f MB / %.2f MB : used (%.2f%%)\n",
             sd_space_used_mb, sd_space_total_mb, sd_space_used_percent);
        */        
        
        Serial.println("\n--- SD Storage Info ---");
        Serial.printf("\nSD Card Size: %.2f GB", cardSize);
        Serial.printf("\nTotal Size  : %.2f MB", sd_space_total_mb);
        Serial.printf("\nUsed Space  : %.2f MB", sd_space_used_mb);
        Serial.printf("\nFree Space  : %.2f MB", sd_space_available_mb);

        Serial.printf("\nUsed Percent : %.2f %%", sd_space_used_percent);

        Serial.printf("\n\n--- Data to Firebase ---");

        Serial.printf("\n---Used Percent : %d %%", sd_space_used_percent_int);
        Serial.printf("\n---Total Size : %d GB", sd_space_total_gb_int);
        Serial.println();
    }

    if (sd_space_left_bytes >= minFreeBytes) return true;

    //IF WE DETECT PROBLEMS WE CONTINUE HERE

    Serial.println("\n--- Low space, deleting oldest log file --");

    File root = SD.open("/logs");

    if (!root || !root.isDirectory()) 
    {
        Serial.println("\n--- ERROR ON DELETE : !root || !root.isDirectory() ---");
        return false;
    }

    File oldest;
    while (File entry = root.openNextFile())
    {
        if (!oldest || strcmp(entry.name(), oldest.name()) < 0) 
        {
            if (oldest) oldest.close();
            oldest = entry;
        } 
        else 
        {
            entry.close();
        }
    }

    if (oldest) 
    {
        Serial.print("\n---Deleting: "); Serial.println(oldest.name());
        SD.remove(oldest.name());
        oldest.close();
        return true; // Assume success
    }

    Serial.println("---ERROR : No file deleted — logs empty?");
    return false;
}


bool sd_init()
{
    Serial.println("\n--Initializing SD Module");

    wait(100);
    //Initialize the SD Card

    if(!SD.begin())
    {
        Serial.println("\n---Card Mount Failed");
        return false;
    }

    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE)
    {
        Serial.println("\n--No SD card attached");
        return false;
    }
    
    Serial.print("\n---SD Card Type: ");
    if(cardType == CARD_MMC)
    {
        Serial.println("MMC");
    }
    else if(cardType == CARD_SD)
    {
        Serial.println("SD_SC");
    } 
    else if(cardType == CARD_SDHC)
    {
        Serial.println("SD_HC");
    } 
    else 
    {
        Serial.println("UNKNOWN");
        return false;
    }

    //uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    //Serial.printf("SD Card Size: %lluMB\n", cardSize);

    //updating the SD Space Values and guaranteeing the minimum space is available  
    ensure_sd_space(sd_minimimum_free_space_mb * 1024 * 1024,true); // MB in bytes


    if (!SD.exists("/logs")) 
    {
        Serial.print("\n---Creating /logs directory---\n");
        SD.mkdir("/logs");
    }
    else
    {
        Serial.print("\n---/logs directory already exists---\n");
    }

    //Calibrated on LTE so no need here at the moment TODO , check with WIFI later
    if(black_box_timestamp_log_enabled && 
       (WiFi.status() == WL_CONNECTED || simcom_lte_initialized))
    {
        if(!rtc_calibrated)
        {
            if(log_enabled) Serial.println("\n --- Calibrating RTC Clock --");
            rtc_calib();
        } 
    }
    else
    {
        Serial.println("\n--- No WiFi or LTE connection for RTC calibration at the moment---");
        Serial.println(" until network is available, timestamps may be inaccurate.");
    }
    
    return true;
} 

//On first loop we fill the header, later just the payload

void sd_populate_all()
{
    //Clean the Placeholder Strings
    if(black_box_first_loop) sd_data_header = "";
 
    sd_data_payload = "";

    //Adding in Order

    if(black_box_log_nr_enabled)
    {
       //sd_log_add_parameter();
       bool ok = sd_get_log_nr();
       if(!ok) Serial.print("ERROR ON sd_get_log_nr()");
    }

    //This is 2 colums (date and time)
    if(black_box_timestamp_log_enabled)
    {
       
       sd_log_add_parameter(); 

       bool ok = sd_get_log_timestamp_date();
       if(!ok) Serial.print("ERROR ON sd_get_log_timestamp_date()");

       sd_log_add_parameter(); 

       ok = sd_get_log_timestamp_time();
       if(!ok) Serial.print("ERROR ON sd_get_log_timestamp_time()");
    }

    //IMU (6 Values)

    if(black_box_imu_pitch_log_enabled)
    {
       sd_log_add_parameter(); 

       bool ok = sd_get_log_imu_pitch();
       if(!ok) Serial.print("ERROR ON sd_get_log_imu_pitch()");

    }

    if(black_box_imu_roll_log_enabled)
    {
       sd_log_add_parameter(); 

       bool ok = sd_get_log_imu_roll();
       if(!ok) Serial.print("ERROR ON sd_get_log_imu_roll()");

    }
    
    if(black_box_imu_yaw_log_enabled)
    {
       sd_log_add_parameter(); 

       bool ok = sd_get_log_imu_yaw();
       if(!ok) Serial.print("ERROR ON sd_get_log_imu_yaw()");

    }

    if(black_box_imu_acc_x_log_enabled)
    {
       sd_log_add_parameter(); 

       bool ok = sd_get_log_imu_acc_x();
       if(!ok) Serial.print("ERROR ON sd_get_log_imu_acc_x()");

    }

    if(black_box_imu_acc_y_log_enabled)
    {
       sd_log_add_parameter(); 

       bool ok = sd_get_log_imu_acc_y();
       if(!ok) Serial.print("ERROR ON sd_get_log_imu_acc_y()");

    }

    if(black_box_imu_acc_z_log_enabled)
    {
       sd_log_add_parameter(); 

       bool ok = sd_get_log_imu_acc_z();
       if(!ok) Serial.print("ERROR ON sd_get_log_imu_acc_z()");

    }

    //TEMP
    if(black_box_temp_log_enabled)
    {
       sd_log_add_parameter();
       bool ok = sd_get_log_temp();
       if(!ok) Serial.print("ERROR ON sd_get_log_temp()");
    }

    //LUX
    if(black_box_lux_log_enabled)
    {
       sd_log_add_parameter();
       bool ok = sd_get_log_lux();
       if(!ok) Serial.print("ERROR ON sd_get_log_lux()");
    }

    //SOC (Internal Bat)
    if(black_box_soc_log_enabled)
    {
       sd_log_add_parameter();
       bool ok = sd_get_log_soc();
       if(!ok) Serial.print("ERROR ON sd_get_log_soc()");
    }

    //MUBEA CAN VALUES
    if(black_box_mubea_can_motor_power_log_enabled)
    {
       sd_log_add_parameter();
       bool ok = sd_get_log_mubea_can_motor_power();
       if(!ok) Serial.print("ERROR ON sd_get_log_mubea_can_motor_power()");
    }

    if(black_box_mubea_can_motor_rpm_log_enabled)
    {
       sd_log_add_parameter();
       bool ok = sd_get_log_mubea_can_motor_rpm();
       if(!ok) Serial.print("ERROR ON sd_get_log_mubea_can_motor_rpm()");
    }

    if(black_box_mubea_can_motor_temp_log_enabled)
    {
       sd_log_add_parameter();
       bool ok = sd_get_log_mubea_can_motor_temp();
       if(!ok) Serial.print("ERROR ON sd_get_log_mubea_can_motor_temp()");
    }

    if(black_box_mubea_can_gen_power_log_enabled)
    {
       sd_log_add_parameter();
       bool ok = sd_get_log_mubea_can_gen_power();
       if(!ok) Serial.print("ERROR ON sd_get_log_mubea_can_gen_power()");
    }

    if(black_box_mubea_can_assist_level_log_enabled)
    {
       sd_log_add_parameter();
       bool ok = sd_get_log_mubea_can_assist_level();
       if(!ok) Serial.print("ERROR ON sd_get_log_mubea_can_assist_level()");
    }

    if(black_box_mubea_can_soc_log_enabled)
    {
       sd_log_add_parameter();
       bool ok = sd_get_log_mubea_can_soc();
       if(!ok) Serial.print("ERROR ON sd_get_log_mubea_can_soc()");
    }

    if(black_box_mubea_can_soh_log_enabled)
    {
       sd_log_add_parameter();
       bool ok = sd_get_log_mubea_can_soh();
       if(!ok) Serial.print("ERROR ON sd_get_log_mubea_can_soh()");
    }

    if(black_box_mubea_can_power_log_enabled )
    {
       sd_log_add_parameter();
       bool ok = sd_get_log_mubea_can_power();
       if(!ok) Serial.print("ERROR ON sd_get_log_mubea_can_power()");
    }

    if (black_box_mubea_can_voltage_log_enabled) 
    {
        sd_log_add_parameter();
        bool ok = sd_get_log_mubea_can_voltage();
        if (!ok) Serial.print("ERROR ON sd_get_log_mubea_can_voltage()");
    }

    if (black_box_mubea_can_temperature_log_enabled) 
    {
        sd_log_add_parameter();
        bool ok = sd_get_log_mubea_can_temperature();
        if (!ok) Serial.print("ERROR ON sd_get_log_mubea_can_temperature()");
    }

    if (black_box_mubea_can_speed_log_enabled) 
    {
        sd_log_add_parameter();
        bool ok = sd_get_log_mubea_can_speed();
        if (!ok) Serial.print("ERROR ON sd_get_log_mubea_can_speed()");
    }

    if (black_box_mubea_can_direction_log_enabled) 
    {
        sd_log_add_parameter();
        bool ok = sd_get_log_mubea_can_direction();
        if (!ok) Serial.print("ERROR ON sd_get_log_mubea_can_direction()");
    }

    if (black_box_mubea_can_gear_log_enabled) 
    {
        sd_log_add_parameter();
        bool ok = sd_get_log_mubea_can_gear();
        if (!ok) Serial.print("ERROR ON sd_get_log_mubea_can_gear()");
    }

    if (black_box_mubea_can_mileage_log_enabled) 
    {
        sd_log_add_parameter();
        bool ok = sd_get_log_mubea_can_mileage();
        if (!ok) Serial.print("ERROR ON sd_get_log_mubea_can_mileage()");
    }

    if (black_box_mubea_can_error_code_log_enabled) 
    {
        sd_log_add_parameter();
        bool ok = sd_get_log_mubea_can_error_code();
        if (!ok) Serial.print("ERROR ON sd_get_log_mubea_can_error_code()");
    }

    if (black_box_mubea_can_recuperation_log_enabled) 
    {
        sd_log_add_parameter();
        bool ok = sd_get_log_mubea_can_recuperation();
        if (!ok) Serial.print("ERROR ON sd_get_log_mubea_can_recuperation()");
    }  
    
    if (black_box_update_gap_ms_enabled) 
    {
        sd_log_add_parameter();
        bool ok = sd_get_black_box_update_gap_ms();
        if (!ok) Serial.print("ERROR ON sd_get_black_box_update_gap_ms()");
    } 

    //Adding the line finish
    sd_log_jump_line();
}


bool sd_get_log_imu_pitch()
{
   if(black_box_first_loop) sd_data_header += "imu_pitch";
   
   sd_data_payload += String(imu_pitch);
    
   return true;
}

bool sd_get_log_imu_roll()
{
   if(black_box_first_loop) sd_data_header += "imu_roll";
   
   sd_data_payload += String(imu_roll);
    
   return true;
}

bool sd_get_log_imu_yaw()
{
    if(black_box_first_loop) sd_data_header += "imu_yaw";
   
    sd_data_payload += String(imu_yaw);
    
    return true;
}

bool sd_get_log_imu_acc_x()
{
   if(black_box_first_loop) sd_data_header += "imu_acc_x";
   
   sd_data_payload += String(imu_acc_comp_grav_x);
    
   return true;

}

bool sd_get_log_imu_acc_y()
{
    if(black_box_first_loop) sd_data_header += "imu_acc_y";
   
    sd_data_payload += String(imu_acc_comp_grav_y);
     
    return true;
}

bool sd_get_log_imu_acc_z()
{
    if(black_box_first_loop) sd_data_header += "imu_acc_z";
   
    sd_data_payload += String(imu_acc_comp_grav_z);
     
    return true;
}

bool sd_get_log_nr()
{
   if(black_box_first_loop) sd_data_header += "log_nr";
   
   sd_data_payload += String(black_box_log_nr);
    
   return true;

}


bool sd_get_log_temp()
{
    if(black_box_first_loop) sd_data_header += "temp_C";
   
    sd_data_payload += String(board_temp);
    
    return true;
}   

bool sd_get_log_lux()
{
    if(black_box_first_loop) sd_data_header += "lux";
   
    sd_data_payload += String(lux_val);
    
    return true;
}

bool sd_get_log_soc()
{
    if(black_box_first_loop) sd_data_header += "soc";
   
    sd_data_payload += String(bat_percent);
    
    return true;
}


//For the Mubea_CAN

bool sd_get_log_mubea_can_motor_power() 
{
    if (black_box_first_loop) 
    {
        sd_data_header += "mubea_can_motor_power";
    }
    sd_data_payload += String(mubea_can_motor_power);
    return true;
}

bool sd_get_log_mubea_can_motor_rpm() 
{
    if (black_box_first_loop) 
    {
        sd_data_header += "mubea_can_motor_rpm";
    }
    sd_data_payload += String(mubea_can_motor_rpm);
    return true;
}

bool sd_get_log_mubea_can_motor_temp() 
{
    if (black_box_first_loop) 
    {
        sd_data_header += "mubea_can_motor_temp";
    }
    sd_data_payload += String(mubea_can_motor_temp);
    return true;
}

bool sd_get_log_mubea_can_gen_power() 
{
    if (black_box_first_loop) 
    {
        sd_data_header += "mubea_can_gen_power";
    }
    sd_data_payload += String(mubea_can_gen_power);
    return true;
}

bool sd_get_log_mubea_can_assist_level() 
{
    if (black_box_first_loop) 
    {
        sd_data_header += "mubea_can_assist_level";
    }
    sd_data_payload += String(mubea_can_assist_level);
    return true;
}

bool sd_get_log_mubea_can_soc() 
{
    if (black_box_first_loop) 
    {
        sd_data_header += "mubea_can_soc";
    }
    sd_data_payload += String(mubea_can_soc);
    return true;
}

bool sd_get_log_mubea_can_soh() 
{
    if (black_box_first_loop) 
    {
        sd_data_header += "mubea_can_soh";
    }
    sd_data_payload += String(mubea_can_soh);
    return true;
}

bool sd_get_log_mubea_can_power() 
{
    if (black_box_first_loop) 
    {
        sd_data_header += "mubea_can_power";
    }
    sd_data_payload += String(mubea_can_power);
    return true;
}

bool sd_get_log_mubea_can_voltage() 
{
    if (black_box_first_loop) 
    {
        sd_data_header += "mubea_can_voltage";
    }
    sd_data_payload += String(mubea_can_voltage);
    return true;
}

bool sd_get_log_mubea_can_temperature() 
{
    if (black_box_first_loop) 
    {
        sd_data_header += "mubea_can_temperature";
    }
    sd_data_payload += String(mubea_can_temperature);
    return true;
}

bool sd_get_log_mubea_can_speed() 
{
    if (black_box_first_loop) 
    {
        sd_data_header += "mubea_can_speed";
    }
    sd_data_payload += String(mubea_can_speed);
    return true;
}

bool sd_get_log_mubea_can_direction() 
{
    if (black_box_first_loop) 
    {
        sd_data_header += "mubea_can_direction";
    }
    sd_data_payload += String(mubea_can_direction);
    return true;
}

bool sd_get_log_mubea_can_gear() 
{
    if (black_box_first_loop) 
    {
        sd_data_header += "mubea_can_gear";
    }
    sd_data_payload += String(mubea_can_gear);
    return true;
}

bool sd_get_log_mubea_can_mileage() 
{
    if (black_box_first_loop) 
    {
        sd_data_header += "mubea_can_mileage";
    }
    sd_data_payload += String(mubea_can_mileage);
    return true;
}

bool sd_get_log_mubea_can_error_code() 
{
    if (black_box_first_loop) 
    {
        sd_data_header += "mubea_can_error_code";
    }
    sd_data_payload += String(mubea_can_error_code);
    return true;
}

bool sd_get_log_mubea_can_recuperation() 
{
    if (black_box_first_loop) 
    {
        sd_data_header += "mubea_can_recuperation";
    }
    sd_data_payload += String(mubea_can_recuperation);
    return true;
}


bool sd_get_black_box_update_gap_ms() 
{
    if (black_box_first_loop) 
    {
        sd_data_header += "update_gap_ms";
    }
    sd_data_payload += String(black_box_logging_interval_milliseconds);
    return true;
}


//For the Timestamp
//TODO , later do it online

bool sd_get_log_timestamp_date()
{
    //TODO , later do it online
    rtc_update();

    if(black_box_first_loop) sd_data_header += "date";
   
    //Structure to Fill Up the Date

    sd_data_payload += String(day);
    sd_data_payload += ".";
    sd_data_payload += String(month);
    sd_data_payload += ".";
    sd_data_payload += String(year);
    
    return true;
}

bool sd_get_log_timestamp_time()
{
    //TODO , later do it online
    rtc_update();

    if(black_box_first_loop) sd_data_header += "time";
   
    //Structure to Fill Up the Time

    sd_data_payload += String(hour);
    sd_data_payload += ":";
    sd_data_payload += String(minute);
    sd_data_payload += ":";
    sd_data_payload += String(second);
    
    return true;
}


void sd_log_jump_line()
{
    //Adding the line finish
    if(black_box_first_loop) sd_data_header += "\r\n";
    sd_data_payload += "\r\n";
}

void sd_log_add_parameter()
{
    //Adding the separation for a new field
    if(black_box_first_loop) sd_data_header += ",";
    sd_data_payload += ",";
}

void run_black_box()
{
    //Serial.print("\n -- Black_box data nr: ");
    //Serial.print(black_box_log_nr);
    //Serial.println();

    //Populate all info and then decide if we will create or append the file

    //Here get all data

    sd_populate_all();

    //If we change date even while still running the log we need to change the file
    if(black_box_first_loop || day != last_logged_day || month != last_logged_month || year != last_logged_year)
    {

        if(black_box_first_loop)Serial.print("\n---Checking Date and creating or appending file---\n");
        else Serial.print("\n---Date Changed-- Changing to a new file ---\n");

       //Check if we are logging on the correct day file
        if (day != last_logged_day || month != last_logged_month || year != last_logged_year) 
        {
            char date_buf[32];
            snprintf(date_buf, sizeof(date_buf), "/logs/%04d-%02d-%02d.txt", year, month, day);
            sd_file_name = String(date_buf);

            // Reset header for new file
            File testFile = SD.open(sd_file_name);
            if (!testFile) 
            {
                writeFile(SD, sd_file_name.c_str(), sd_data_header.c_str());
            } 
            else 
            {
                testFile.close();
            }

            last_logged_day = day;
            last_logged_month = month;
            last_logged_year = year;
        }

        //Check the file
        // If the data.txt file doesn't exist
        // Create a file on the SD card and write the data labels
        File file = SD.open(sd_file_name);

        Serial.print("\n---File to store logs : "); Serial.print(sd_file_name);
    
        if(!file) 
        {
            Serial.println("\n---File doesn't exist");
            Serial.println("\n---Creating file : ");
            Serial.print(sd_file_name);

            //TODO make header

            //writeFile(SD, sd_file_name.c_str(), "temp,lux,soc \r\n");
            writeFile(SD, sd_file_name.c_str(), sd_data_header.c_str());
            
            //TODO , if parameters change for to make a new file
            //TODO , save by date or choose what parameter to save in 
        }

        else //file already exists 
        {
            Serial.println("\n---File already exists , appending old file ... ");  
        }
        file.close();


        Serial.print("\n ----------------- Running Black Box --------------- \n ");

        
        if(black_box_serial_mode == black_box_serial_raw)
        {
            Serial.print("\n ----------------- Black_Box Header--------------- \n ");
            Serial.print(sd_data_header);
            Serial.println();
        }


    }

    //Starting LOG Activities
    
    //Append the data to file
    appendFile(SD, sd_file_name.c_str() , sd_data_payload.c_str());

    
    //Print every black_box_serial_iterations_gap
    if (black_box_log_nr > black_box_log_nr_serial_counter + black_box_serial_iterations_gap)//Time to Serial print
    {
        //based on the mode the function will react
        //Logging to Serial formatted will modify the payload String
        //but is not used anymore anyways in this loop

        black_box_serial_print(black_box_serial_mode);

        //Setting the serial counter to the current log number
        black_box_log_nr_serial_counter = black_box_log_nr;
    } 

    //Send order to next Firebase output loop
    if (black_box_log_nr > black_box_log_nr_firebase_counter + black_box_firebase_iterations_gap)//Time to Serial print
    {
        //based on the mode the function will react
        //Logging to Serial formatted will modify the payload String
        //but is not used anymore anyways in this loop

        black_box_need_firebase_update = true;

        //Setting the serial counter to the current log number
        black_box_log_nr_firebase_counter = black_box_log_nr;
    } 

    if(black_box_first_loop) black_box_first_loop = false;

    black_box_log_nr++;
}

void black_box_serial_print(int given_serial_mode)
{
    if(given_serial_mode == black_box_serial_raw)
    {
        Serial.printf("\n---Black_Box_Payload Nr.%d: ",black_box_log_nr);
        Serial.println(sd_data_payload); 
    }

    else if(given_serial_mode == black_box_serial_formatted)
    {

        Serial.printf("\n--- Black Box Payload Nr.%d (Formatted) ---\n", black_box_log_nr);

        // Debugging output
        //Serial.println("Debug: Header and Payload before processing:");
        //Serial.print("Header: ");  Serial.println(sd_data_header);
        //Serial.print("Payload: "); Serial.println(sd_data_payload);

        // Check if header or payload is empty
        if (sd_data_header.isEmpty() || sd_data_payload.isEmpty()) 
        {
            Serial.println("\nError: Header or Payload is empty.");
            return;
        }

        // Remove trailing newline from payload (so the last element is also correctly formatted)
        //This modifyes the payload String so remove if necessary
        //but the payload string is already copied to the SD at this point 
        if (sd_data_payload.endsWith("\r\n")) 
        {
            sd_data_payload = sd_data_payload.substring(0, sd_data_payload.length() - 2);
        }
        //else Serial.println("\nBoth Strings have at least one value , continuing...");

        //Serial.println("\nCounting the number of items in the header and payload...");

        //Serial.println("\nFor Header:");
        // Count the number of items in the header and payload
        String headerCopy = sd_data_header; // Create a copy of the header
        headerCopy.replace(",", "");        // Remove all commas
        int headerCount = sd_data_header.length() - headerCopy.length() + 1;

        //Serial.printf("Header Count: %d\n", headerCount);
        //Serial.println("\nFor Payload:");
        String payloadCopy = sd_data_payload; // Create a copy of the payload
        payloadCopy.replace(",", "");         // Remove all commas
        int payloadCount = sd_data_payload.length() - payloadCopy.length() + 1;
        //Serial.printf("Payload Count: %d\n", payloadCount);

        if (headerCount != payloadCount) 
        {
            Serial.printf("\nError: Header and Payload lengths do not match (Header: %d, Payload: %d).\n", headerCount, payloadCount);
            return;
        }            
        
        // Split the header and payload into individual items
        String header = sd_data_header;
        String payload = sd_data_payload;

        int headerIndex = 0;
        //Serial.println("\n--- Iterating through the header and payload, separating by commas ---");
        // Iterate through the header and payload, separating by commas
        while (header.length() > 0 && payload.length() > 0) 
        {
            // Extract the next header item
            int headerDelimiterIndex = header.indexOf(',');
            String headerItem;
            if (headerDelimiterIndex != -1) 
            {
                headerItem = header.substring(0, headerDelimiterIndex);
                header = header.substring(headerDelimiterIndex + 1);
            } 
            else 
            {
                headerItem = header;
                header = "";
            }

            // Extract the next payload item
            int payloadDelimiterIndex = payload.indexOf(',');
            String payloadItem;
            if (payloadDelimiterIndex != -1) 
            {
                payloadItem = payload.substring(0, payloadDelimiterIndex);
                payload = payload.substring(payloadDelimiterIndex + 1);
            } 
            else 
            {
                payloadItem = payload;
                payload = "";
            }

            // Print the header and payload item as a formatted line
            Serial.printf("\n%s = %s", headerItem.c_str(), payloadItem.c_str());
            headerIndex++;
            
            wait(1);
        }
        Serial.print("\n----------------------------------------------\n");      
    }
}




// Write to the SD card
void writeFile(fs::FS &fs, const char * path, const char * message) 
{
    //Serial.printf("Writing file: %s\n", path);
  
    File file = fs.open(path, FILE_WRITE);
    if(!file) 
    {
      Serial.println("\nFailed to open file for writing");
      return;
    }
    if(file.print(message)) 
    {
      //Serial.println("File written");
    } 
    else 
    {
      Serial.println("\nWrite failed");
    }
    file.close();
}
  
// Append data to the SD card
void appendFile(fs::FS &fs, const char * path, const char * message) 
{
    //Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file) 
    {
        Serial.println("\nFailed to open file for appending");
        return;
    }
    if(file.print(message)) 
    {
        //Serial.println("Message appended");
    } 
    else 
    {
        Serial.println("\nAppend failed");
    }
    file.close();
}

//To Display the /logs content


int print_sd_log_folder_content() 
{
    int total_files = 0;

    Serial.println("Listing .txt files in /logs:");
    File dir = SD.open("/logs");
    File file = dir.openNextFile();
    while (file) 
    {
        if (!file.isDirectory() && String(file.name()).endsWith(".txt")) 
        {
            Serial.println(file.name());
            total_files++;
        }
        file = dir.openNextFile();
    }    
    Serial.printf("\n---Total Files : %d \n", total_files );

    return total_files;
}

//TODO later separate per day

//Think how to do the cycle SD card flashunbg even with memory ful , so have a counter in place on NVS flash upon boot  

//LATER ADD THE REGISTERED HEAVY BREAKING ETC 


void splitFileIntoChunksIfNeeded(
    const String &filePath,
    std::vector<String> &partFiles,
    size_t maxPartSizeBytes,
    const std::set<String> &existingRemoteFiles
)
{
    // 🧹 Step 1: Clean up leftover part files
    File logDir = SD.open("/logs");
    if (logDir && logDir.isDirectory()) 
    {
        File entry = logDir.openNextFile();
        while (entry) 
        {
            String name = entry.name();
            if (!entry.isDirectory() && name.endsWith(".txt") &&
                name.indexOf("_part") != -1) 
            {
                String fullPath = "/logs/" + name;
                Serial.printf("🧹 Removing leftover chunk: %s\n", name.c_str());
                SD.remove(fullPath.c_str());
            }
            entry = logDir.openNextFile();
        }
        logDir.close();
    }

    // 🟢 Step 2: Check if splitting is needed
    File inputFile = SD.open(filePath.c_str());
    if (!inputFile) 
    {
        Serial.printf("\n❌ Could not open file: %s\n", filePath.c_str());
        return;
    }

    size_t totalSize = inputFile.size();
    if (totalSize <= maxPartSizeBytes) 
    {
        Serial.printf("\n✅ %s is under threshold → upload directly.\n", filePath.c_str());
        partFiles.push_back(filePath);
        inputFile.close();
        return;
    }

    // 📦 Step 3: Setup for chunking
    float totalSizeMB = float(totalSize) / 1000000.0;
    float thresholdMB = float(maxPartSizeBytes) / 1000000.0;
    size_t totalParts = (totalSize + maxPartSizeBytes - 1) / maxPartSizeBytes;

    Serial.printf("\n📦 File: %s is %.2f MB → larger than %.2f MB threshold.\nWill be split into %d parts.\n",
                  filePath.c_str(), totalSizeMB, thresholdMB, totalParts);

    String fileName = filePath.substring(filePath.lastIndexOf("/") + 1);
    String baseName = fileName.substring(0, fileName.lastIndexOf("."));
    String ext = fileName.substring(fileName.lastIndexOf("."));

    size_t partNumber = 1;
    size_t bytesWritten = 0;
    File partFile;
    unsigned long timer = millis();

    while (inputFile.available()) 
    {
        String partFileName = baseName + "_part" + String(partNumber) + "_of_" + String(totalParts) + ext;
        String fullPartPath = "/logs/" + partFileName;

        // ⏩ Skip if already uploaded
        if (existingRemoteFiles.find(partFileName) != existingRemoteFiles.end()) 
        {
            Serial.printf("✅ Skipping chunk already uploaded: %s\n", partFileName.c_str());
            // Drain chunk from original file
            size_t drained = 0;
            while (inputFile.available() && drained < maxPartSizeBytes) 
            {
                String line = inputFile.readStringUntil('\n');
                drained += line.length() + 1;
            }
            partNumber++;
            continue;
        }

        // 📝 Write chunk directly with final name
        partFile = SD.open(fullPartPath.c_str(), FILE_WRITE);
        if (!partFile) 
        {
            Serial.printf("❌ Failed to create chunk: %s\n", fullPartPath.c_str());
            break;
        }

        Serial.printf("✍️  Writing chunk: %s\n", fullPartPath.c_str());
        bytesWritten = 0;

        while (inputFile.available() && bytesWritten < maxPartSizeBytes) 
        {
            String line = inputFile.readStringUntil('\n');
            partFile.print(line + "\n");
            bytesWritten += line.length() + 1;

            if (millis() > timer + 1000) 
            {
                Serial.print(".");
                timer = millis();
            }
        }

        partFile.close();
        partFiles.push_back(fullPartPath);
        Serial.printf("\n✅ Finished chunk %d (%0.2f KB)\n", partNumber, float(bytesWritten) / 1000.0);

        partNumber++;
    }

    inputFile.close();
}
