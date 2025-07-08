
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

bool task_logger_sd_ftp_running = false;

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
        4096,                //Stack Size
        NULL,                //Passing Parameters
        5,                   //Task Priority
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
    
    imu_needed++;
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
    
    imu_needed--;
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
    wait(100);  

    buzzer_ok();

    //Relation Cursor->Mode
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
            
            //wipe the screen
            oled_clear();

            switch(cursor_position)
            {
                case wifi_list:  
                {
                    Serial.print("WIFI LIST\n"); 
                    
                    //TODO

                    oled_clear();
                    wait(3000);
                    cursor_position = 50;
                    menu_needs_refresh = true;
                    btn_1_interr_enable_on_press();
                    btn_2_interr_enable_on_press();

                    break; 
                }

             
             
               //TODO complete all this

                case wifi_connect: 
                { 
                    Serial.print("\n --- WIFI CONNECT ");  
                    
                    //TODO
                    
                    oled_clear();
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

                    //TODO delte afterwards
                    rtc_calibrated = true;


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

                        create_task_sd();

                        //Killing this task_logger_sd_ftp as well
                        task_logger_sd_ftp_running = false;
                    }                 

                    break; 
                }

                
                case start_logger:  
                { 
                    Serial.print("start_logger\n"); 
                                       
                    //TODO
                    
                    oled_clear();
                    wait(3000);
                    cursor_position = 50;
                    menu_needs_refresh = true;
                    btn_1_interr_enable_on_press();
                    btn_2_interr_enable_on_press();

                    break; 
                
                }
                
                case ftp_hotspot:   
                
                { 
                    Serial.print("ftp_hotspot\n");   

                    //TODO
                    
                    oled_clear();
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

                    //TODO
                    
                    oled_clear();
                    wait(3000);
                    cursor_position = 50;
                    menu_needs_refresh = true;
                    btn_1_interr_enable_on_press();
                    btn_2_interr_enable_on_press();
                    


                    break; 
                }
                
                case leds_imu:  
                { 
                    Serial.print("leds_imu \n");  

                    //TODO
                    
                    oled_clear();
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






