#include <Arduino.h>

#include <vars.h>
#include <tools.h>

#include <temp.h>
#include <lux.h>
#include <tasks.h>
#include <imu.h>
#include <rtc.h>
#include <interrupts.h>
#include <oled.h>
#include <demos.h>
#include <gpio_exp.h>
#include <rgb.h>

#include "i2c.h"



void i2c_init()
{
    if(!i2c_initialized)
    {
        Wire.begin();
        //Wire.begin(esp_sda,esp_scl);
        //Wire.setClock(400000);
        i2c_initialized = true;
        if(log_enabled)Serial.println("\n ---- I2C Enabled with Default Pins----- \n");    
    }
    else
    {
        if (log_enabled)Serial.println("\nERROR on INIT_I2C() -> I2C Already Enabled!\n");    
    }
}

void i2c_scanner_raw() //log_enabled must be active to use this tool
{
    if(!log_enabled)return;

    if(!i2c_initialized)i2c_init();

    Serial.println("\n---- Running I2C Scanner ------ \n");

    byte error, address;
    
    int nDevices;rgb_needed++;
    
    Serial.println("Scanning...");
    
    nDevices = 0;
    for(address = 1; address < 127; address++ )
    {
        // The i2c_scanner uses the return value of
        // the Write.endTransmisstion to see if
        // a device did acknowledge to the address.

        Wire.beginTransmission(address);
        error = Wire.endTransmission();
    
        if (error == 0)
        {
            Serial.print("I2C device found at address 0x");
            
            if (address<16) Serial.print("0");
            
            Serial.print(address,HEX);
            Serial.println("  !");

            nDevices++;
        }
        else if (error==4)
        {
            Serial.print("Unknown error at address 0x");
            
            if (address<16)Serial.print("0");
            
            Serial.println(address,HEX);
        }    
    }
    if (nDevices == 0)
        Serial.println("No I2C devices found\n");
    else
        Serial.println("\n-----I2C Scanner Finished----\n");
        
}

void i2c_scanner_with_names() //Print the known found sensors on the Rail  
{
    if(!log_enabled)return;

    if(!i2c_initialized)i2c_init();

    Serial.println("\n---- Running I2C Scanner ------ \n");

    byte error, address;
    
    int nDevices;
    
    Serial.println("Scanning...");
    
    nDevices = 0;
    for(address = 1; address < 127; address++ )
    {
        // The i2c_scanner uses the return value of
        // the Write.endTransmisstion to see if
        // a device did acknowledge to the address.

        Wire.beginTransmission(address);
        error = Wire.endTransmission();
    
        if (error == 0)
        {
            Serial.print("I2C device found at address 0x");
            
            if (address<16) Serial.print("0");           
            Serial.print(address,HEX);

            Serial.print("  ! ");

            //We just correlate this trigger with the corresponding hex adress
            if     (address == lux_sens_addr_hex)   Serial.print("<- LUX");
            else if(address == temp_sens_addr_hex)  Serial.print("<- TEMP");
            else if(address == gpio_exp_addr_hex)   Serial.print("<- GPIO_EXP");
            else if(address == imu_addr_hex)        Serial.print("<- IMU");
            else if(address == rtc_addr_hex)        Serial.print("<- RTC");
            else if(address == oled_addr_hex)       Serial.print("<- OLED");
            else if(address == fuel_gauge_addr_hex) Serial.print("<- FUEL_GAUGE"); 
            else if(address == gps_q_lc29h_addr_hex)Serial.print("<- GPS");

            nDevices++;
            Serial.println();
        }
        else if (error==4)
        {
            Serial.print("Unknown error at address 0x");
            
            if (address<16)Serial.print("0");
            
            Serial.println(address,HEX);
        }    
    }
    if (nDevices == 0)
        Serial.println("No I2C devices found\n");
    else
        Serial.println("\n-----I2C Scanner Finished----\n");
}

//AUX counter for refreshing sensors inside i2c manager;
int check_temp=0;
int refresh_count_temp = 50;

int check_lux = 0;
int refresh_count_lux = 10;

int check_rtc = 0;
int refresh_count_rtc = 20;

int refresh_oled = 0;
int refresh_count_oled = 3;
int refresh_count_oled_dynamic = refresh_count_oled;

bool i2c_manager_running = false;



// can be used for stopping the tasks
TaskHandle_t i2c_manager_handle = NULL;


void create_task_i2c_manager() //once created it will automatically run
{
    if(log_enabled)Serial.print("\n-- Creating I2C Manager --");

    xTaskCreate
    (
        i2c_manager,           //Function Name (must be inf.loop)
        "i2c_manager", //Logging Name (Just for ID)
        4096,                //Stack Size 
        NULL,                //Passing Parameters
        10,                  //Task Priority
        &i2c_manager_handle
    );

    i2c_manager_running = true;
    
    if(log_enabled)Serial.print("-- done --\n");
}



//HERE ALL THE I2C and nowhere else (maybe later with mutex)

//make vars (_needed) to ena or disa the measurements


void i2c_manager(void * parameters) //Whenever nothing critical is happening 
{
    u_int8_t active_i2c_devs = 0;  //INternal counter to refresh oled_always at the same raate

    if (!i2c_initialized) i2c_init();  //Just Double check

    unsigned long timer = 1000;

    while(1) //This will run forever while the task is active
    {
        if(i2c_manager_running == false)
        {
            Serial.print("\n---Destroying I2C Task Manager & mwr Counter---");
            vTaskDelete(NULL);
        } 

        else //The I2c_Manager is alowed to continue looping
        {
            //Checking Mutex to protect any share resources
            //If the mutex is free

            // Take the mutex before accessing the I2C bus
            if (xSemaphoreTake(global_vars_mutex, portMAX_DELAY) == pdTRUE)
            {

                //RULES AND RECOMMENDATIUONS 

                //Always when creating & deleting tasks mind the _needed flags

                //OLED fns just in one task at the time avoid buffer overwriting 

                //Check Mutex and all that stuff in the future to improve rail stability
                    
                //Check eTaskGetState(here_task_handle)== 0 for task info

                //Processing in Priority order

                //SENSORS-----------------------------------------------------------
                
                
                //IMU--------            
                if(imu_needed > 0) //IMU IS NEEDED AT LEAST IN ONE TASK
                {
                    if(!imu_running)//Activating
                    {
                        if(log_enabled)Serial.print("\n-- IMU ENABLED -> NEEDED --\n");
                        non_critical_task_refresh_ms = 5000;
                        if(log_enabled)Serial.printf("\n-- Changed NON-Critical Task to %d ms->  --\n",non_critical_task_refresh_ms);
                        imu_running = true;   
                        active_i2c_devs++;
                    }

                    imu_run();            
                }

                else //IMU is NOT Needed
                {
                    if(imu_running)
                    {
                        if(log_enabled)Serial.print("\n-- IMU DISABLED -> NOT NEEDED --\n");
                        non_critical_task_refresh_ms = 1000;
                        if(log_enabled)Serial.printf("\n-- Changed NON-Critical Task to %d ms->  --\n",non_critical_task_refresh_ms);
                        //TODO DISABLE THE SENSOR TO SAVE POWER
                        imu_running = false;
                        imu_initialized = false;
                        active_i2c_devs--;
                    }
                }       

                //NON-CRITICAL Sensors will refresh fast but not crazy fast


                //LUX---------------------------

                if(lux_needed > 0) //LUX IS NEEDED AT LEAST IN ONE TASK
                {
                    if(!lux_running)//Activating
                    {
                        if(log_enabled)Serial.print("\n-- LUX ENABLED -> NEEDED --\n");
                        lux_running = true;  
                        active_i2c_devs++; 
                    }

                    //LUX
                    if(check_lux > refresh_count_lux)
                    {
                        lux_val = lux_get();
                        check_lux = 0;
                    }
                    else check_lux++;            
                }

                else //LUX is NOT Needed
                {
                    if(lux_running)
                    {
                        if(log_enabled)Serial.print("\n-- LUX DISABLED -> NOT NEEDED --\n");
                        //TODO DISABLE THE SENSOR TO SAVE POWER
                        lux_running = false;
                        active_i2c_devs--;
                    }
                }

                //TEMP---------------------------

                if(temp_needed > 0) //TEMP IS NEEDED AT LEAST IN ONE TASK
                {
                    if(!temp_running)//Activating
                    {
                        if(log_enabled)Serial.print("\n-- TEMP ENABLED -> NEEDED --\n");
                        temp_running = true;   
                        active_i2c_devs++;
                    }

                    //TEMP (every refresh_count_temp cycles)
                    if(check_temp > refresh_count_temp)
                    {
                        board_temp = temp_get();
                        check_temp = 0;
                    }
                    else check_temp++;             
                }

                else //TEMP is NOT Needed
                {
                    if(temp_running)
                    {
                        if(log_enabled)Serial.print("\n-- TEMP DISABLED -> NOT NEEDED --\n");
                        //TODO DISABLE THE SENSOR TO SAVE POWER
                        temp_running = false;
                        active_i2c_devs--;
                    }
                }


                //RTC---------------------------
                if(rtc_needed > 0) //RTC IS NEEDED AT LEAST IN ONE TASK
                {
                    if(!rtc_running)//Activating
                    {
                        if(log_enabled)Serial.print("\n-- RTC ENABLED -> NEEDED --\n");
                        rtc_running = true;   
                        active_i2c_devs++;
                    }

                    //RTC
                    if(check_rtc > refresh_count_rtc)
                    {
                        rtc_update();
                        check_rtc = 0;
                    }
                    else check_rtc++;         
                }

                else //TEMP is NOT Needed
                {
                    if(rtc_running)
                    {
                        if(log_enabled)Serial.print("\n-- RTC DISABLED -> NOT NEEDED --\n");
                        //TODO DISABLE THE SENSOR TO SAVE POWER
                        rtc_running = false;
                        active_i2c_devs--;
                    }
                }
                
                //OLED--------------------------------------------------------------
                if(oled_enabled)
                {
                    if(oled_needed > 0) //RTC IS NEEDED AT LEAST IN ONE TASK
                    {
                        if(!oled_running)//Activating
                        {
                            if(log_enabled)Serial.print("\n--OLED ENABLED -> NEEDED --\n");
                            oled_running = true;   
                        }

                        //TODO : REWORK THIS
                        /*
                        //adjust the waiting time depending on the total of active devs
                        if(active_i2c_devs>1)refresh_count_oled_dynamic = (int) (refresh_count_oled / active_i2c_devs); 
                        else refresh_count_oled_dynamic = refresh_count_oled;

                        //OLED

                        //FOR IMU set to max refresh_rate
                        if(imu_running || lux_running) refresh_count_oled_dynamic = 0;

                        if(refresh_oled > refresh_count_oled_dynamic)
                        {
                            if(oled_needs_refresh)oled_refresh();
                            //Resetting flag on the fn.
                            refresh_oled = 0;
                        }
                        else refresh_oled++;     
                        */
                        //TODO TEST THIS AND CHECK IF IT IS STABLE 

                        //If the oled_developer_screen is active (this will override any other oled so important to have it on -1 until changed on firebase)
                        
                        if(oled_initialized && oled_dev_mode_enabled && millis() - firebase_oled_last_update > firebase_oled_update_interval && (oled_dev_screen_nr != -1 || oled_needs_clear || firebase_first_loop))
                        {
                            if(oled_needs_clear || (oled_dev_screen_nr == 0 && firebase_first_loop && firebase_initialized))
                            {
                                Serial.println("\n---Deactivating OLED");
                            }
                            //else Serial.println("\n---Updating OLED");
                            //Keeping the Timer Accurate

                            firebase_oled_last_update = millis();
                            //1 to 5 , -1 is off
                            oled_dev_info(oled_dev_screen_nr);
                        }
                        

                        if(oled_needs_refresh)
                        {
                            oled_refresh();               
                        } 
                    }

                    else //OLED is NOT Needed
                    {
                        if(oled_running)
                        {
                            if(log_enabled)Serial.print("\n-- OLED DISABLED -> NOT NEEDED --\n");
                            //TODO DISABLE THE SENSOR TO SAVE POWER
                            oled_running = false;
                        }
                    }

                }

                //TODO , THINK IF WE WANT TO MANAGE AND TURN OFF THE RGB IF NOT NEEDED BY ANYONE 
                //There is one exception, RGB_LEDS is not part of the I2C Rail
                //But we turn off the 5v rail here if its not in use
                //so far the rgb_leds are the only ones who use the 5v_reg, this might
                //change in the future 
                //RGB_LEDS---------------------------
                
                if(rgb_needed > 0 && !rgb_bypassed) //RTC IS NEEDED AT LEAST IN ONE TASK
                {
                    if(!rgb_running)//Activating
                    {
                        if(log_enabled)Serial.print("\n-- RGB LEDS ENABLED -> NEEDED --\n");
                        rgb_running = true;   
                    }

                    if(!rgb_leds_enabled) rgb_leds_enable(); 

                }
                else //RGB NOT Needed
                {
                    if(rgb_leds_enabled || rgb_running && !rgb_bypassed)
                    {
                        if(log_enabled)Serial.print("\n-- RGB DISABLED -> NOT NEEDED --\n");
                        //DISABLE ALL 5V Rail to save power
                        if(rgb_leds_enabled)rgb_leds_disable();
                        rgb_running = false;
                    }
                }

                //TODO, ASAP Here all directives for GPIO_EXP so we dont interfere with i2c rail

                //e.g. disable_5v_reg = true; will be activated anywhere but 5v_reg_disable()-> 5v_reg_enabled=false will be executed here 

                //HERE ADD ALL THE OTHER I2C COMMANDS , e.g GPIOEXP

                wait(10);

                //NON-CRITICAL UPDATES 
                //WONT RUN WHILE SCROLLING OR ON TIME CRITICAL_TASKSTHE DEV_MENU 
                //AS THEY SLOW DOWN THE OLED

                if(millis() > timer + non_critical_task_refresh_ms && time_critical_tasks_running == 0)
                {
                    non_critical_refresh(1);//change to 1 for some logs
                    timer = millis();
                }
                //TODO (MAKE A TIME CRITICAL TASK UPDATE FEATURE, update maybe once the bike is stopped )
            
                // Release the mutex after I2C operations
                xSemaphoreGive(global_vars_mutex);
                //Guarantee Toggle
                wait(50);
            
            } //end of Mutex is free
            
            else if (xSemaphoreTake(global_vars_mutex, portMAX_DELAY) == pdFALSE)  //Mutex is taken , wait
            {
                //Serial.print("\n--Mutex is taken --\n");
                //delay here to avoid flooding the serial
                wait(10);
            }

            //Quickly update the minutes without reset
            //get_mwr(mwr_log_mode_moderate);

        } //(i2c_manager_running == true) bracket
    
    }//while(1)
}//end of function

//----------------------------------------------------------------