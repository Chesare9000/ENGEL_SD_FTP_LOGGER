
#include <Arduino.h>
#include <I2Cdev.h>

#include <vars.h>
#include <Wire.h>
#include <imu.h>
#include <tools.h>
#include <oled.h>
#include <rgb.h>
#include <buzzer.h>
#include <logger.h>
#include <interrupts.h>
#include <ota.h>
#include <wifi.h>
#include <sd.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <nvs.h>

bool rgb_active_for_logger  = true;


bool task_logger_sd_ftp_running = false;

bool task_wifi_selector_running = false;

bool task_logger_ms_selector_running = false;

bool task_button_mapper_for_oled_dev_screen_nr_running = false;

int wifi_selected = 0;

//defaults
String logger_wifi_ssid = "wifi";
String logger_wifi_password = "wifi1234";

//Logger Task (SD+FTP) version--------------------------------- 

TaskHandle_t task_logger_sd_ftp_handle = NULL;

void create_task_logger_sd_ftp() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n--creating task_logger_sd_ftp--");
    wait(100);
    
    task_logger_sd_ftp_declare();
    wait(100);

    xTaskCreate
    (
        task_logger_sd_ftp,           //Function Name (must be a while(1))
        "task_logger_sd_ftp", //Logging Name
        8192,                //Stack Size
        NULL,                //Passing Parameters
        8,                   //Task Priority
        &task_logger_sd_ftp_handle
    );   

    task_logger_sd_ftp_running = true;

    if(log_enabled) Serial.print("-- done --\n");
}

void  task_logger_sd_ftp_declare()
{
    if(log_enabled)Serial.print("\ntask_logger_sd_ftp_declared\n");
    wait(100);
    //This Taks will use the following I2C_Devs
    
    //imu_needed++;
    //rgb_needed++;
    //temp_needed++;
    //lux_needed++;
    rtc_needed++;
    //fuelgauge_needed++;
    oled_needed++;
}

void task_logger_sd_ftp_release()
{
    if(log_enabled)Serial.print("\ntask_logger_sd_ftp_released\n");
    wait(100);
    
    //This Taks will release the following I2C_Devs
    
    //imu_needed--;
    //rgb_needed--;
    //temp_needed--;
    //lux_needed--;
    rtc_needed--;
    //fuelgauge_needed--;
    oled_needed--;
}

void task_logger_sd_ftp(void * parameters)
{   
    //MENU BASED TASK 

    if(log_enabled)Serial.println("Running task_logger_sd_ftp ");
    
    buzzer_ok();

    wait(500); 

    //Relation Cursor-> Mode
    #define wifi_list    50
    #define wifi_connect 60
    #define logger_ms    70
    #define start_logger 80 //recalibrate and start 
    #define ftp_hotspot  90 
    #define ftp_wifi    100 
    #define leds_imu    110  // gyro and acc on x y or z
    #define ota         120  

    int cursor_position = 50 ;
    bool menu_needs_refresh = true;

    btn_1_interr_enable_on_press();
    btn_2_interr_enable_on_press();

    //Getting wifi credentials from NVS
    nvs_get_logger_wifi_credentials(nvs_log_mode_verbose);
    
    oled_dev_mode_enabled = false; //This task manage its own oled fns.

    while(1)
    {
        if(!task_logger_sd_ftp_running)
        {
            if(log_enabled) Serial.print(" \n --- KILLING task_logger_sd_ftp ! ");
            wait(500);
            vTaskDelete(NULL);
        }

        //Selecting once ssomething was chosen    
        if(btn_2.is_pressed) 
        {
            btn_1_interr_disable();
            btn_2_interr_disable();

            wait_for_btn_2_release();
                      
            if(log_enabled) Serial.print(" \n --- STARTING ");
            

            switch(cursor_position)
            {
                case wifi_list:  
                {
                    Serial.print("WIFI LIST\n"); 
                    
                   
                    create_task_wifi_selector();    

                    //Killing task
                    task_logger_sd_ftp_running = false;

                    break; 
                }

             
             
               //TODO complete all this

                case wifi_connect: 
                { 
                    Serial.print("\n --- WIFI CONNECT ");  
                    
                    //TODO
                    
                    
                    wait(3000);
                    cursor_position = 50;
                    menu_needs_refresh = true;
                    btn_1_interr_enable_on_press();
                    btn_2_interr_enable_on_press();

                    break; 
                }



                case logger_ms:  
                { 
                    Serial.print("\n --- logger_ms");  

                    create_task_logger_ms_selector();    

                    //Killing task
                    task_logger_sd_ftp_running = false;

                    break; 

                    
                }

                
                case start_logger:  
                { 
                   
                    Serial.print("start_logger\n"); 

                    oled_black_box_starting();

                    buzzer_drive_click();

                    wait(1000);

                    if (WiFi.status() != WL_CONNECTED)
                    {
                        if(!wifi_connect_to(logger_wifi_ssid,logger_wifi_password))
                        {
                            Serial.print("ERROR : WIFI COULDNT CONNECT,EXITING");
                            firebase_file_initialized = false;
                            ftp_wifi_running = false;
                            
                            oled_logger_wifi_failed();
                            buzzer_error();

                            wait(3000);
                            cursor_position = 50;
                            menu_needs_refresh = true;
                            btn_1_interr_enable_on_press();
                            btn_2_interr_enable_on_press();
                    
                            break;
                        }
                    } 
                                       
                    
                    if(!rtc_calibrated)
                    {                    
                        if(update_time_via_wifi())
                        {
                           Serial.println("\n--- RTC Calibrated ---\n");
                        }
                        else 
                        {
                            Serial.println("\n--- RTC Calibration Problems ---\n");
                            //Serial.println("\n--- Retry?->BTN_1 | Proceed?->BTN 2 ---\n");
                            Serial.println("\n--- Returning to Menu ---\n");

                            wait(1000);

                            buzzer_error();

                            oled_error_rtc_not_calibrated();

                            wait(2000);

                            cursor_position = 50;

                            menu_needs_refresh = true;

                            btn_1_interr_enable_on_press();
                            btn_2_interr_enable_on_press();
                        }
                    }
                    
                    if(rtc_calibrated)
                    {
                        if(firebase_connection_method == connection_via_wifi)
                        {
                            Serial.println("\n---Disconnecting WiFi to Enable SD Init---\n");
                            wifi_disconnect();
                            wait(1000);
                        }

                        //For all cases (Wifi and LTE)
                        Serial.println("\n---Turning ON BLACK BOX ---\n");

                        buzzer_ok();                       

                        create_task_sd();

                        create_task_button_mapper_for_oled_dev_screen_nr();//For the menu

                        wait(3000);

                        //we will need the oled in developer mode so acctivating here
                        oled_dev_mode_enabled = true;
                        oled_dev_screen_nr = 1; //Black_Box Screen

                        //Killing this task_logger_sd_ftp as well
                        task_logger_sd_ftp_running = false;
                    }                     

                    break; 
                
                }
                
                case ftp_hotspot:   
                
                { 
                    Serial.print("ftp_hotspot\n");   

                    //TODO
                    
                    
                    wait(3000);
                    cursor_position = 50;
                    menu_needs_refresh = true;
                    btn_1_interr_enable_on_press();
                    btn_2_interr_enable_on_press();
                    
                    break; 
                
                }
                                    
                
                case ftp_wifi:  
                
                {       
                    Serial.print("ftp_wifi\n");  

                    //here make an oled for confitming 

                    
                    ftp_wifi_running = true;

                    


                    
                    i2c_manager_running = false;
                    
                    wait(500);

                    oled_starting_ftp_via_wifi();

                    if(!i2c_manager_running)oled_refresh();

                    //Killing this task_logger_sd_ftp as well
                    task_logger_sd_ftp_running = false;
                
                    break; 
                }
                
                case leds_imu:  
                { 
                    Serial.print("leds_imu \n");  

                    //TODO
                    
                    
                    wait(3000);
                    cursor_position = 50;
                    menu_needs_refresh = true;
                    btn_1_interr_enable_on_press();
                    btn_2_interr_enable_on_press();
                    

                    break; 
                }

                case ota:
                {
                    Serial.print("ota\n"); 

                    Serial.print("\n---Creating ota_task");

                    create_task_ota();

                    //Deactivate developer mode as we will kill i2c_manager
                    oled_dev_mode_enabled = false;

                    //Killing this task_logger_sd_ftp as well
                    task_logger_sd_ftp_running = false;

                    break; 
                }
                
                default: {Serial.printf("\n ERROR -> VAL: ",cursor_position); break; }
            }            
        }

        //Select Next Option
        if (btn_1.is_pressed) //Next Pattern was selected
        {
            if(cursor_position>=120) cursor_position = 50;
            else cursor_position+=10;
            
            menu_needs_refresh = true;
            wait_for_btn_1_release();
        }

        if(menu_needs_refresh) //Refreshing Serial & OLED
        {
            if(log_enabled)
            {
                Serial.print("\nCurrent Selection -> "); 
                switch(cursor_position)
                {
                    case wifi_list:    { Serial.print(" wifi_list");    break; }
                    case wifi_connect: { Serial.print(" wifi_connect"); break; }
                    case logger_ms:    { Serial.print(" logger_ms");    break; }
                    case start_logger: { Serial.print(" start_logger"); break; }
                    case ftp_hotspot:  { Serial.print(" ftp_hotspot");  break; }
                    case ftp_wifi:     { Serial.print(" ftp_wifi");     break; }
                    case leds_imu:     { Serial.print(" leds_imu");     break; }
                    case ota:          { Serial.print(" ota");          break; }
                    
                    default: {Serial.printf("\n ERROR -> VAL: ",cursor_position); break; }
                }                
            }
           
            if(oled_enabled) oled_template_logger_sd_ftp(cursor_position);

            menu_needs_refresh = false;
        }
        else wait(10);
    }
}




//WIFI SELECTION TASK------------------------------------------------

TaskHandle_t task_wifi_selector_handle = NULL;

void create_task_wifi_selector() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n--creating task_wifi_selector--");
    wait(100);

    oled_dev_mode_enabled = false; //This tassk manage its own oled fns.
    
    task_wifi_selector_declare();
    wait(100);

    xTaskCreate
    (
        task_wifi_selector,           //Function Name (must be a while(1))
        "task_wifi_selector", //Logging Name
        4096,                //Stack Size
        NULL,                //Passing Parameters
        5,                   //Task Priority
        &task_wifi_selector_handle
    );   

    task_wifi_selector_running = true;

    if(log_enabled) Serial.print("-- done --\n");
}

void  task_wifi_selector_declare()
{
    if(log_enabled)Serial.print("\nttask_wifi_selector_declared\n");
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

void task_wifi_selector_release()
{
    if(log_enabled)Serial.print("\ntask_wifi_selector_released\n");
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

void task_wifi_selector(void * parameters)
{   
    //MENU BASED TASK 

    if(log_enabled)Serial.println("Running task_wifi_selector ");
    wait(100);  

    buzzer_notification();

    //Relation Cursor->Mode
    #define cesar  50
    #define marvin 60
    #define motionlab 70
    #define hq 80 
    #define ota 90


    bool wifi_selected = false;

    int cursor_position = 50 ;
    bool menu_needs_refresh = true;

    btn_1_interr_enable_on_press();
    btn_2_interr_enable_on_press();
    
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nTurning OFF WiFi before WiFi selection...");
        wifi_off(); 
        WiFi.disconnect(true); // Disconnect from any previous WiFi connection
        WiFi.mode(WIFI_OFF); // Set WiFi mode to OFF
        wait(1000);

    } 

    while(1)
    {
        if(!task_wifi_selector_running)
        {
            if(log_enabled) Serial.print(" \n --- KILLING task_wifi_selector and returning to menu! ");
            
            create_task_logger_sd_ftp();
    
            vTaskDelete(NULL);
        }

        //Selecting once ssomething was chosen    
        if(btn_2.is_pressed) 
        {
            btn_1_interr_disable();
            btn_2_interr_disable();

            wait_for_btn_2_release();
                      
            if(log_enabled) Serial.print(" \n --- CONNECTING TO ");
            
            //wipe the screen
            
            switch(cursor_position)
            {
                case cesar:  
                {

                    Serial.print("CESAR WIFI HOTSPOT \n"); 
                    
                    //TODO

                    


                    logger_wifi_ssid = "wifi";
                    logger_wifi_password = "wifi1234"; 

                    wifi_selected = true;

                    break; 
                }
             
             
               //TODO complete all this

                case marvin: 
                { 
                    Serial.print("MARVIN WIFI HOTSPOT\n");  
                    
                    //TODO
                    
                    
                    
                    logger_wifi_ssid = "marvin";
                    logger_wifi_password = "marvin1234"; 

                    wifi_selected = true;

                    break; 
                }

                case motionlab: 
                { 
                    Serial.print("MOTIONLAB\n");  
                    
                    logger_wifi_ssid = "Motionlab-Member";
                    logger_wifi_password = "always.in.motion"; 

                    wifi_selected = true;

                    break; 
                }

                case hq: 
                { 
                    Serial.print("HQ WIFI\n");  
                    
                    //TODO
                    
                    
                    
                    logger_wifi_ssid = "Not_Your_Hotspot";
                    logger_wifi_password = "wifirocks"; 

                    wifi_selected = true;

                    break; 
                }

                case ota: 
                { 
                    Serial.print("OTA WIFI\n");  
                    
                    //TODO
                    
                    
                    
                    logger_wifi_ssid = "ota";
                    logger_wifi_password = "ota"; 

                    wifi_selected = true;

                    break; 
                }              

                default: {Serial.printf("\n ERROR -> VAL: %d NOT VALID ",cursor_position); break; }
            }
        }

        //Select Next Option
        if (btn_1.is_pressed) //Next Pattern was selected
        {

            //Relation Cursor->Mode
            //#define cesar  50
            //#define marvin 60
            //#define motionlab 70
            //#define hq 80 
            //#define ota 90

            cursor_position+=10;

            if(cursor_position > 90) cursor_position = 50;
                                  
            menu_needs_refresh = true;

            wait_for_btn_1_release();            
        }

        if(menu_needs_refresh)
        {
            if(log_enabled)
            {
                Serial.print("\nCurrent Selection -> "); 
                switch(cursor_position)
                {
                    case cesar:    { Serial.print(" cesar");    break; }
                    case marvin:   { Serial.print(" marvin");   break; }
                    case motionlab:{ Serial.print(" motionlab");break; }
                    case hq:       { Serial.print(" hq");       break; }
                    case ota:      { Serial.print(" ota");      break; }
                    
                    default: {Serial.printf("\n ERROR -> VAL: ",cursor_position); break; }
                }                
            }
           
            if(oled_enabled) oled_template_logger_wifi_menu(cursor_position);

            menu_needs_refresh = false;

        }

        if(wifi_selected)
        {
            Serial.print("\n---Saving New Credentials as Default to NVS");
            nvs_set_logger_wifi_credentials(logger_wifi_ssid,logger_wifi_password,nvs_log_mode_verbose);

            Serial.print("\n---Connecting to Wifi using new credentials");
            WiFi.mode(WIFI_STA);
            WiFi.begin(logger_wifi_ssid, logger_wifi_password);
            Serial.println("");

            buzzer_startup();

            //TODO later add more interesting things to the OLED_OTA
            oled_logger_wifi(logger_wifi_ssid);

            unsigned long waiting_for_wifi = millis();

            int logger_waiting_for_wifi_timeout = 60000;

            // Wait for connection and if not then restart everything and try again
            while (WiFi.status() != WL_CONNECTED) 
            {
                wait(1000);
                Serial.print(".");

                if (millis() > waiting_for_wifi + logger_waiting_for_wifi_timeout ) // 60 seconds timeout
                {
                    Serial.printf("\n--- Failed to connect to WiFi within %d seconds, restarting ...", int(logger_waiting_for_wifi_timeout / 1000));
                    //todo later save on nvs the cause of restart as ota_wifi_waiting_timeout
                    
                    oled_logger_wifi_failed();
                    buzzer_error();

                    wait(3000);
                    ESP.restart(); // Restart the ESP if it takes too long to connect
                }
                //TODO if waiting for too much just restart the ESP and try again
            }

            Serial.println("");
            Serial.print("Connected to ");
            Serial.println(logger_wifi_ssid);
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());

            //TODO later add more interesting things to the OLED_OTA
            oled_logger_wifi(logger_wifi_ssid);
            buzzer_ok();
            wait(1000);

            //killing the task and returning to menu
            task_wifi_selector_running = false;
        }

        else wait(10);
    }
}





// task_logger_ms_selector ------------------------------------------------

TaskHandle_t task_logger_ms_selector_handle = NULL;

void create_task_logger_ms_selector() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n--task_logger_ms_selector--");
    wait(100);

    oled_dev_mode_enabled = false; //This tassk manage its own oled fns.
    
    task_logger_ms_selector_declare();
    wait(100);

    xTaskCreate
    (
        task_logger_ms_selector,           //Function Name (must be a while(1))
        "task_logger_ms_selector", //Logging Name
        4096,                //Stack Size
        NULL,                //Passing Parameters
        5,                   //Task Priority
        &task_logger_ms_selector_handle
    );   

    task_logger_ms_selector_running = true;

    if(log_enabled) Serial.print("-- done --\n");
}

void task_logger_ms_selector_declare()
{
    if(log_enabled)Serial.print("\ntask_logger_ms_selector_declared\n");
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

void task_logger_ms_selector_release()
{
    if(log_enabled)Serial.print("\ntask_logger_ms_selector_released\n");
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

void task_logger_ms_selector(void * parameters)
{   
    //MENU BASED TASK 

    if(log_enabled)Serial.println("Running task_logger_ms_selector ");
    wait(100);  

    buzzer_notification();

    //Relation Cursor->Mode
    #define ten  50
    #define twenty 60
    #define fifty 70
    #define hundred 80 
    #define twousand 90

    int cursor_position = 50 ;
    bool menu_needs_refresh = true;

    btn_1_interr_enable_on_press();
    btn_2_interr_enable_on_press();
    
    while(1)
    {
        if(!task_logger_ms_selector_running)
        {
            if(log_enabled) Serial.print(" \n --- KILLING task_logger_ms_selector and returning to menu! ");
           
            create_task_logger_sd_ftp();
    
            vTaskDelete(NULL);
        }

        //Selecting once ssomething was chosen    
        if(btn_2.is_pressed) 
        {
            btn_1_interr_disable();
            btn_2_interr_disable();

            wait_for_btn_2_release();
                      
            if(log_enabled) Serial.print(" \n --- Setting BlackBox refresh rate to ");
            
            //wipe the screen
            

            switch(cursor_position)
            {
                case ten:  
                {

                    Serial.print("10 ms \n"); 

                    black_box_logging_interval_milliseconds = 10;
                    
                    

                    //killing the task and returning to menu
                    task_logger_ms_selector_running = false;

                    break; 
                }
             
             
               //TODO complete all this

                case twenty: 
                { 
                    Serial.print("20 ms \n"); 

                    black_box_logging_interval_milliseconds = 20;
                    
                    

                    //killing the task and returning to menu
                    task_logger_ms_selector_running = false;

                    break; 
                }

                case fifty: 
                { 
                    Serial.print("50 ms \n"); 

                    black_box_logging_interval_milliseconds = 50;
                    
                    

                    //killing the task and returning to menu
                    task_logger_ms_selector_running = false;

                    break; 
                }

                case hundred: 
                { 
                    Serial.print("100 ms \n"); 

                    black_box_logging_interval_milliseconds = 100;
                    
                    

                    //killing the task and returning to menu
                    task_logger_ms_selector_running = false;

                    break; 
                }

                case twousand: 
                { 
                    Serial.print("1000 ms \n"); 

                    black_box_logging_interval_milliseconds = 1000;
                    
                    

                    //killing the task and returning to menu
                    task_logger_ms_selector_running = false;

                    break; 
                }              

                default: {Serial.printf("\n ERROR -> VAL: ",cursor_position); break; }
            }

            //In case Firebase is running we need to sync that overriden input
            if(task_firebase_active)
            {
                firebase_black_box_refresh_milliseconds_needs_override = true;
                inputs_are_missing = true;
            }

        }

        //Select Next Option
        if (btn_1.is_pressed) //Next Pattern was selected
        {

            //Relation Cursor->Mode
            //#define cesar  50
            //#define marvin 60
            //#define motionlab 70
            //#define hq 80 
            //#define ota 90

            cursor_position+=10;

            if(cursor_position > 90) cursor_position = 50;
                                  
            menu_needs_refresh = true;

            wait_for_btn_1_release();            
        }

        if(menu_needs_refresh)
        {
            if(log_enabled)
            {
                Serial.print("\nCurrent Selection -> "); 
                switch(cursor_position)
                {
                    case ten:     { Serial.print(" 10ms");  break; }
                    case twenty:  { Serial.print(" 20ms");  break; }
                    case fifty:   { Serial.print(" 50ms");  break; }
                    case hundred: { Serial.print(" 100ms"); break; }
                    case twousand:{ Serial.print(" 1000ms");break; }
                    
                    default: {Serial.printf("\n ERROR -> VAL: ",cursor_position); break; }
                }                
            }
           
            if(oled_enabled) oled_template_logger_ms_menu(cursor_position);

            menu_needs_refresh = false;

        }
        else wait(10);
    }
}




//This task will alsso manage the leds for the imu so they arre as fast as possible

//task_button_mapper_for_oled_dev_screen_nr ------------------------------------------------

TaskHandle_t task_button_mapper_for_oled_dev_screen_nr_handle = NULL;

void create_task_button_mapper_for_oled_dev_screen_nr() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n--creating task_button_mapper_for_oled_dev_screen_nr--");
    wait(100);
    
    task_button_mapper_for_oled_dev_screen_nr_declare();
    wait(100);

    xTaskCreate
    (
        task_button_mapper_for_oled_dev_screen_nr,           //Function Name (must be a while(1))
        "task_button_mapper_for_oled_dev_screen_nr", //Logging Name
        4096,                //Stack Size
        NULL,                //Passing Parameters
        5,                   //Task Priority
        &task_button_mapper_for_oled_dev_screen_nr_handle
    );   

    task_button_mapper_for_oled_dev_screen_nr_running = true;

    if(log_enabled) Serial.print("-- done --\n");
}

void  task_button_mapper_for_oled_dev_screen_nr_declare()
{
    if(log_enabled)Serial.print("\ntask_button_mapper_for_oled_dev_screen_nr_declared\n");
    wait(100);
    //This Taks will use the following I2C_Devs
    
    //imu_needed++;
    rgb_needed++;  //for the imu values
    //temp_needed++;
    //lux_needed++;
    //rtc_needed++;
    //fuelgauge_needed++;
    //oled_needed++;
}

void task_button_mapper_for_oled_dev_screen_nr_release()
{
    if(log_enabled)Serial.print("\ntask_button_mapper_for_oled_dev_screen_nr_released\n");
    wait(100);
    
    //This Taks will release the following I2C_Devs
    
    //imu_needed--;
    rgb_needed--;
    //temp_needed--;
    //lux_needed--;
    //rtc_needed--;
    //fuelgauge_needed--;
    //oled_needed--;
}

void task_button_mapper_for_oled_dev_screen_nr(void * parameters)
{   
    //MENU BASED TASK 

    if(log_enabled)Serial.println("Running task_button_mapper_for_oled_dev_screen_nr ");
    wait(100);  

    btn_1_interr_enable_on_press(); //Change Screen
    btn_2_interr_enable_on_press(); //EXIT
    
    while(1)
    {
        if(!task_button_mapper_for_oled_dev_screen_nr_running)
        {
            if(log_enabled) Serial.print(" \n --- KILLING task_button_mapper_for_oled_dev_screen_nr and task_sd and returning to menu! ");
            
            task_sd_active = false;

            create_task_logger_sd_ftp();
    
            vTaskDelete(NULL);
            
        }

        //Selecting once ssomething was chosen    
        if(btn_2.is_pressed) 
        {
            task_button_mapper_for_oled_dev_screen_nr_running = false;
            wait_for_btn_2_release();           
        }

        //Select Next Option
        if (btn_1.is_pressed) //Next Pattern was selected
        {             
            btn_1_interr_disable();
            btn_2_interr_disable();

            if(oled_dev_screen_nr >= oled_dev_screen_nr_max ) oled_dev_screen_nr = 1;
            
            else oled_dev_screen_nr++;

            Serial.printf("\n\n--Switching to oled_dev_screen_nr nr: %d -- \n\n",oled_dev_screen_nr);
            //guard already inside (preventing bouncing or looping if btn_2 is stil hold)
            
            wait_for_btn_1_release();

            btn_1_interr_enable_on_press(); //Change Screen
            btn_2_interr_enable_on_press(); //EXIT
        }

        //we use the global var that is being updated in real-time
        //to control the leds when the system is on 
        switch (oled_dev_screen_nr)
        {
            #define max_pos 70
            #define max_acc 5000

            case oled_dev_gyro_x: 
            {
               
                rgb_leds_imu_pos(false,rgb_leds_brightness,'x',max_pos);
                if(!rgb_active_for_logger ) rgb_active_for_logger  = true;
            }
            break;

            case oled_dev_gyro_y: 
            {
                rgb_leds_imu_pos(false,rgb_leds_brightness,'y',max_pos);
                if(!rgb_active_for_logger ) rgb_active_for_logger  = true;
            }
            break;

            case oled_dev_gyro_z: 
            {
                rgb_leds_imu_pos(false,rgb_leds_brightness,'z',max_pos);
                if(!rgb_active_for_logger ) rgb_active_for_logger  = true;
            }
            break;

            case oled_dev_acc_x: 
            {
                rgb_leds_imu_acc(false,rgb_leds_brightness,'x',5000);
                if(!rgb_active_for_logger ) rgb_active_for_logger  = true;
            }
            break;

            case oled_dev_acc_y: 
            {
                rgb_leds_imu_acc(false,rgb_leds_brightness,'y',5000);
                if(!rgb_active_for_logger ) rgb_active_for_logger  = true;
            }
            break;

            case oled_dev_acc_z: 
            {
                rgb_leds_imu_acc(false,rgb_leds_brightness,'z',5000);
                if(!rgb_active_for_logger ) rgb_active_for_logger  = true;
            }
            break;

            default: 
            {
                if(rgb_active_for_logger)
                {
                    rgb_leds_off();
                    rgb_active_for_logger  = false;
                } 
            }
            break;
            //remember to modify the oled_screen_nr_max accordingly 
        } 
        
        wait(100);
    }
}


//al iniciar logger decir que estamos imprimiendo wifi

//make more precise Hz and change to that for the refresh rate considering the processing times