#include <Arduino.h>

#include <vars.h>
#include <interrupts.h>
#include <tools.h>
#include <rgb.h>

#include <demos.h>
#include <oled.h>
#include <tasks.h>
#include <imu.h>
#include <gpio_exp.h>
#include <fuel_gauge.h>
#include <buzzer.h>


//DEMO MENU

TaskHandle_t task_demo_menu_handle = NULL;

void create_task_demo_menu() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n-- creating task_demo_menu --");

    task_demo_menu_i2c_declare();

    xTaskCreate
    (
        demo_menu,           //Function Name
        "task_demo_menu",    //Logging Name
        4096,                //Stack Size
        NULL,                //Passing Parameters
        2,                   //Task Priority
        &task_demo_menu_handle
    );

    if(log_enabled) Serial.print("-- done --\n");

}

void task_demo_menu_i2c_declare()
{
    if(log_enabled)Serial.print("\ndemo_menu_i2c_declared\n");
    //This Taks will use the following I2C_Devs

    //imu_needed++;
    rgb_needed++;
    //temp_needed++;
    //lux_needed++;
    //rtc_needed++;
    //fuelgauge_needed++;
    oled_needed++;

    //DEFINING THIS TASK TIME CRITICAL TO REFRESH THE OLED ASAP
    //time_critical_tasks_running++;

}

void task_demo_menu_i2c_release()
{
    if(log_enabled)Serial.print("\ndemo_menu_i2c_released\n");
    //This Taks will release the following I2C_Devs
    
    //imu_needed--;
    rgb_needed--;
    //temp_needed--;
    //lux_needed--;
    //rtc_needed--;
    //fuelgauge_needed--;
    oled_needed--;

    //RELEASING THE TIME_CRITICAL FLAG TO ALLOW FOR NON-CRITICAL REFRESH
    //time_critical_tasks_running--;
}


void demo_menu(void * parameters)
{ 
    if(log_enabled) Serial.print("\n\n------Starting Demo Menu----");
    
    //TODO Consider all prereq. and enable them here  if needed
    
    //eg.FOR LEDS (will cascade from 5v_reg and lvl_shftr sso all will activate)
    
    if(!rgb_leds_initialized) rgb_leds_init(); 


    if(log_enabled) Serial.print("\n---Setting BTN. INterrupts---");

    btn_1.number_of_push=0;
    btn_2.number_of_push=0;

    btn_1_interr_enable_on_press();
    btn_2_interr_enable_on_press();

    if(log_enabled) Serial.print("\n------Turning ON RGB Led----");

    rgb_led_on(0,'r',100);

    if(log_enabled) Serial.print("\n----Sending OLED Menu Order--");

    int cursor_pos = 50;

    if(oled_enabled)oled_template_demo(cursor_pos);  

    if(log_enabled) Serial.print("\n---LOOPING--");
  
    while(1)
    {
        if(btn_1.is_pressed)
        {
            rgb_leds_off();    

            if(btn_1.number_of_push>demos_total) // demos + exit 
            {
                //Back to the First Option
                btn_1.number_of_push=0;
                cursor_pos = 50;
            }
            else cursor_pos +=10;

            if(btn_1.number_of_push == demos_total)
            {
                if(log_enabled) Serial.print("\n-- Selected EXIT --- ");
                rgb_leds_off();
            }
            else
            {
                if(log_enabled) Serial.printf("\n-- Selected Demo %u --- ",btn_1.number_of_push);
                rgb_led_on(btn_1.number_of_push,'r',100);
            } 

             
            //TODO Uncomment this if there is more demos(and revisse previos if (modify "demos_total" statement )) 

            /*
            else if (btn_1.number_of_push>4 && btn_1.number_of_push<10)
            {
                rgb_led_on(btn_1.number_of_push-5,'g',100);
            }
  
            else if (btn_1.number_of_push>9 && btn_1.number_of_push<15)
            {
                rgb_led_on(btn_1.number_of_push-10,'b',100);
            }
            */

            oled_template_demo(cursor_pos);

            wait_for_btn_1_release();
                        
            

        }

        if(btn_2.is_pressed)
        {
            wait_for_btn_2_release();

            rgb_leds_off(); 

            if(log_enabled) Serial.printf("--- Running Demo %u---",btn_1.number_of_push);

            //All Ints. Disabled and Deleting Menu

            oled_clear();

            switch (btn_1.number_of_push)
            {
                case 0:
                {
                    create_task_demo_0_rgb_patterns();

                    task_demo_menu_i2c_release();
                    vTaskDelete(NULL);//Delete Itself
                    break;
                }

                case 1:
                {
                    create_task_demo_1_lux();
                    
                    task_demo_menu_i2c_release();
                    vTaskDelete(NULL);//Delete Itself
                    break;

                }

                case 2:
                {
                    create_task_demo_2_imu_pos();

                    task_demo_menu_i2c_release();
                    vTaskDelete(NULL);//Delete Itself
                    break;
                }

                case 3:
                {
                    create_task_demo_3_imu_acc();

                    task_demo_menu_i2c_release();
                    vTaskDelete(NULL);//Delete Itself
                    break;
                }

                case 4:
                {
                    create_task_demo_4_lock();

                    task_demo_menu_i2c_release();
                    vTaskDelete(NULL);//Delete Itself
                    break;
                }

                case 5: 
                { 
                    create_task_demo_5_buzzer();

                    task_demo_menu_i2c_release();
                    vTaskDelete(NULL);//Delete Itself
                    break;
                }

                case 6: 
                { 
                    create_task_demo_6_status();

                    task_demo_menu_i2c_release();
                    vTaskDelete(NULL);//Delete Itself
                    break;
                }

                case 7: 
                {
                    //exit here 
                    create_task_devel_menu();

                    task_demo_menu_i2c_release();
                    vTaskDelete(NULL);//Delete Itself
                    break;
                }

                
                //demo _1 lux
                //demo _2 lock 
                //demo _3 IMU with danger zones
                //demo _4 lock
                //demo _5 buzzer
                //demo _6 status
                //
                //demo _ maybe ble? , near field ? , lora ?
                //option 7 is exit at the moment   
            }
        }

        wait(10); //to Not freak_out if skipping all ifs
    }    
}



//DEMO 0  --- RGB_Patterms 

TaskHandle_t task_demo_0_rgb_patterns_handle = NULL;

void create_task_demo_0_rgb_patterns() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n-- creating task_demo_0_rgb_patterns --");

    task_demo_0_rgb_patterns_i2c_declare();

    xTaskCreate
    (
        demo_0_rgb_patterns,           //Function Name
        "task_demo_0_rgb_patterns", //Logging Name
        4096,                //Stack Size
        NULL,                //Passing Parameters
        3,                   //Task Priority
        &task_demo_0_rgb_patterns_handle
    );
    
    if(log_enabled) Serial.print("-- done --\n");

}

void task_demo_0_rgb_patterns_i2c_declare()
{
    if(log_enabled)Serial.print("\ntask_demo_0_patterns_i2c_declared\n");
    //This Taks will use the following I2C_Devs

    //imu_needed++;
    rgb_needed++;
    //temp_needed++;
    //lux_needed++;
    //rtc_needed++;
    //fuelgauge_needed++;
    oled_needed++;
}

void task_demo_0_rgb_patterns_i2c_release()
{
    if(log_enabled)Serial.print("\ntask_demo_0_patterns_i2c_released\n");
    //This Taks will release the following I2C_Devs
    
    //imu_needed--;
    rgb_needed--;
    //temp_needed--;
    //lux_needed--;
    //rtc_needed--;
    //fuelgauge_needed--;
    oled_needed--;
}

void demo_0_rgb_patterns(void * parameters)
{
    btn_1.number_of_push=0;

    if(log_enabled)
    {
        Serial.println("\n\n----Starting Demo 0 Patterns---\n");
        Serial.printf("\n--Running Pattern %d--",btn_1.number_of_push);
    } 
            
    if(oled_enabled)
    {
        oled_demo_patterns(btn_1.number_of_push);
    }

    bool first_loop = true;

    while(1)
    {
        if(btn_2.is_pressed) //Getting Out
        {
          
            if(log_enabled) Serial.print(" ----- Back to Menu ! \n ");

            wait_for_btn_2_release();

            rgb_leds_off();//Might be redundant 
            oled_clear();

            create_task_demo_menu();

            task_demo_0_rgb_patterns_i2c_release();
            vTaskDelete(NULL);//Delete itself
        }

        if (btn_1.is_pressed) //Next Pattern was selected
        {
            if(btn_1.number_of_push > 5)btn_1.number_of_push=0;

            if(log_enabled) Serial.printf("\n--Running Pattern %d--",btn_1.number_of_push);
            
            if(oled_enabled)
            {
                oled_demo_patterns(btn_1.number_of_push);
            }

            wait_for_btn_1_release();
            first_loop = true;
        }

        if(btn_1.number_of_push == 0 && !btn_1.is_pressed && !btn_2.is_pressed)
        {       
            rgb_leds_demo_fade_multicolor();         
        }
        
        else if(btn_1.number_of_push == 1 && !btn_1.is_pressed && !btn_2.is_pressed)
        {
            rgb_leds_demo_shift(100, 500);           
        }

        else if(btn_1.number_of_push == 2 && !btn_1.is_pressed && !btn_2.is_pressed)
        {
            rgb_led_demo_pong('r',100,100);
            rgb_led_demo_pong('g',100,100);
            rgb_led_demo_pong('b',100,100);
            rgb_led_demo_pong('y',100,100);
            rgb_led_demo_pong('p',100,100);
            rgb_led_demo_pong('w',100,100);
        }

        else if(btn_1.number_of_push == 3 && !btn_1.is_pressed && !btn_2.is_pressed)
        {
            rgb_led_demo_pong('r',100,100);
        }

        else if(btn_1.number_of_push == 4 && !btn_1.is_pressed && !btn_2.is_pressed)
        {
            rgb_leds_fade_once(0,'r',0,50,10);
        }

        else if(btn_1.number_of_push == 5 && !btn_1.is_pressed && !btn_2.is_pressed)
        {
            rgb_leds_on('r',50);
        }
    }
    wait(10);
}

//DEMO 1 --- LUX Sensor

TaskHandle_t task_demo_1_lux_handle = NULL;

void create_task_demo_1_lux() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n-- creating task_demo_1_lux --");

    task_demo_1_lux_i2c_declare();
    
    xTaskCreate
    (
        demo_1_lux,           //Function Name
        "task_demo_1_lux", //Logging Name
        4096,                //Stack Size
        NULL,                //Passing Parameters
        3,                   //Task Priority
        &task_demo_1_lux_handle
    );

    if(log_enabled) Serial.print("-- done --\n");
}

void task_demo_1_lux_i2c_declare()
{
    if(log_enabled)Serial.print("\ntask_demo_1_lux_i2c_declared\n");
    //This Taks will use the following I2C_Devs

    //imu_needed++;
    rgb_needed++;
    //temp_needed++;
    lux_needed++;
    //rtc_needed++;
    //fuelgauge_needed++;
    oled_needed++;
}

void task_demo_1_lux_i2c_release()
{
    if(log_enabled)Serial.print("\ntask_demo_1_lux_i2c_released\n");

    //This Taks will release the following I2C_Devs
    
    //imu_needed--;
    rgb_needed--;
    //temp_needed--;
    lux_needed--;
    //rtc_needed--;
    //fuelgauge_needed--;
    oled_needed--;
}

void demo_1_lux(void * parameters)
{
    btn_1.number_of_push=0;

    if(log_enabled) Serial.print("\n\n------Starting Demo 1 LUX----");

    bool first_loop = true;

    while(1)
    {
        if(btn_2.is_pressed) //Getting Out
        {
            if(log_enabled) Serial.print(" ----- Back to Menu ! \n ");

            
            wait_for_btn_2_release();

            rgb_leds_off();//Might be redundant 

            create_task_demo_menu();

            task_demo_1_lux_i2c_release();
            vTaskDelete(NULL);//Delete itself
        }

        if (btn_1.is_pressed) //Next Pattern was selected
        {
            wait_for_btn_1_release();
            first_loop = true;
        }

        if(btn_1.number_of_push == 0 )
        {
            if(first_loop)
            {
                if(log_enabled) Serial.print("\n--Lux on Red--");
                first_loop = false;
            }
            
            if(!btn_1.is_pressed||!btn_2.is_pressed)
            {
                rgb_leds_demo_lux('r',255,10,100);                
            }    
        }
        
        else if(btn_1.number_of_push == 1 )
        {
            if(first_loop)
            {
                if(log_enabled) Serial.print("\n--Lux on Green--");
                first_loop = false;
            }
            
            if(!btn_1.is_pressed||!btn_2.is_pressed)
            {
                rgb_leds_demo_lux('g',255,10,100);                
            }    
        }

        else if(btn_1.number_of_push == 2 )
        {
            if(first_loop)
            {
                if(log_enabled) Serial.print("\n--Lux on Blue--");
                first_loop = false;
            }
            
            if(!btn_1.is_pressed||!btn_2.is_pressed)
            {
                rgb_leds_demo_lux('b',255,10,100);                
            }    
            
        }

        else if(btn_1.number_of_push == 3 )
        {
           if(first_loop)
            {
                if(log_enabled) Serial.print("\n--Lux on White--");
                first_loop = false;
            }
            
            if(!btn_1.is_pressed||!btn_2.is_pressed)
            {
                rgb_leds_demo_lux('w',200,10,100);                
            }    
        }

        else if(btn_1.number_of_push == 4 )
        {
            btn_1.number_of_push =0;
            //Back to Lux R
        }
    }
    wait(10);
}



//DEMO 2 --- IMU Positions

TaskHandle_t task_demo_2_imu_pos_handle = NULL;

void create_task_demo_2_imu_pos() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n-- creating task_demo_2_pos_imu --");

    task_demo_2_imu_pos_i2c_declare();

    xTaskCreate
    (
        demo_2_imu_pos,           //Function Name
        "task_demo_2_pos_imu", //Logging Name
        4096,                //Stack Size
        NULL,                //Passing Parameters
        3,                   //Task Priority
        &task_demo_2_imu_pos_handle
    );

    if(log_enabled) Serial.print("-- done --\n");
}

void task_demo_2_imu_pos_i2c_declare()
{
    if(log_enabled)Serial.print("\ntask_demo_2_imu_pos_i2c_declared\n");
    //This Taks will use the following I2C_Devs

    imu_needed++;
    rgb_needed++;
    //temp_needed++;
    //lux_needed++;
    //rtc_needed++;
    //fuelgauge_needed++;
    oled_needed++;
}

void task_demo_2_imu_pos_i2c_release()
{
    if(log_enabled)Serial.print("\ntask_demo_2_imu_pos_i2c_released\n");
    //This Taks will release the following I2C_Devs
    
    imu_needed--;
    rgb_needed--;
    //temp_needed--;
    //lux_needed--;
    //rtc_needed--;
    //fuelgauge_needed--;
    oled_needed--;
}

void demo_2_imu_pos(void * parameters)
{

    #define max_pos 70
    #define led_brightness 100 

    btn_1.number_of_push=0;

    if(log_enabled) Serial.println("\n\n------Starting Demo 2 IMU POS----\n");

    bool first_loop = true;

    while(1)
    {
        if(btn_2.is_pressed) //Getting Out
        {
            if(log_enabled) Serial.print(" ----- Back to Menu ! \n ");

            rgb_leds_off();//Redundant but safe

            wait_for_btn_2_release();

            if(oled_enabled)oled_clear();             

            create_task_demo_menu();


            task_demo_2_imu_pos_i2c_release();
            vTaskDelete(NULL);//Delete itself
            //Nothing Should Run Here
        }

        if (btn_1.is_pressed) //Next Pattern was selected
        {
            rgb_leds_off();//Redundant but safe
            wait_for_btn_1_release();
            first_loop = true;
        }

        if(btn_1.number_of_push == 0 ) //Simple IMU
        {
            if(first_loop)
            {
                if(log_enabled) Serial.print("\n-- IMU Pos Across Y (Roll)--");
                rgb_led_blink_once(2,'b',100,100);
                first_loop = false;
            }
            
            if(!btn_1.is_pressed && !btn_2.is_pressed)
            {
                rgb_leds_demo_imu_pos(2,led_brightness,'y',max_pos);
                wait(10);                
            }    
        }
        
        else if(btn_1.number_of_push == 1 ) //IMU with kick
        {
            if(first_loop)
            {
                if(log_enabled) Serial.print("\n--IMU Pos Across X (Pitch)--");
                rgb_led_blink_once(2,'b',100,100);
                first_loop = false;
            }
            
            if(!btn_1.is_pressed && !btn_2.is_pressed)
            {
               rgb_leds_demo_imu_pos(2,led_brightness,'x',max_pos);
                wait(10);          
            }    
        }

        else if(btn_1.number_of_push == 2 )
        {
            if(first_loop)
            {
                if(log_enabled) Serial.print("\n-- IMU Pos Across Z (YAW)--");
                rgb_led_blink_once(2,'b',100,100);
                first_loop = false;
            }
            
            if(!btn_1.is_pressed && !btn_2.is_pressed)
            {
                rgb_leds_demo_imu_pos(2,led_brightness,'z',max_pos);
                wait(10);                  
            }    
            
        }

        else if(btn_1.number_of_push == 3 )
        {
            btn_1.number_of_push =0;
            //Back to X
        }
    }
    wait(10);

}



//DEMO 3 --- IMU Accelerations

TaskHandle_t task_demo_3_imu_acc_handle = NULL;

void create_task_demo_3_imu_acc() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n-- creating task_demo_3_acc_imu --");

    task_demo_3_imu_acc_i2c_declare();

    xTaskCreate
    (
        demo_3_imu_acc,           //Function Name
        "task_demo_3_acc_imu", //Logging Name
        4096,                //Stack Size
        NULL,                //Passing Parameters
        3,                   //Task Priority
        &task_demo_3_imu_acc_handle
    );   

    if(log_enabled) Serial.print("-- done --\n");
}

void task_demo_3_imu_acc_i2c_declare()
{
    if(log_enabled)Serial.print("\ntask_demo_3_imu_acc_i2c_declared\n");
    //This Taks will use the following I2C_Devs

    imu_needed++;
    rgb_needed++;
    //temp_needed++;
    //lux_needed++;
    //rtc_needed++;
    //fuelgauge_needed++;
    oled_needed++;
}

void task_demo_3_imu_acc_i2c_release()
{
    if(log_enabled)Serial.print("\ntask_demo_3_imu_acc_i2c_released\n");
    
    //This Taks will release the following I2C_Devs
    
    imu_needed--;
    rgb_needed--;
    //temp_needed--;
    //lux_needed--;
    //rtc_needed--;
    //fuelgauge_needed--;
    oled_needed--;
}

void demo_3_imu_acc(void * parameters)
{

    #define max_acc 5000
    #define led_brightness 100


    //TODO Define Max Parameters 

    btn_1.number_of_push=0;

    if(log_enabled) Serial.println("\n\n------Starting Demo 3 IMU ACC----\n");

    bool first_loop = true;

    while(1)
    {
        if(btn_2.is_pressed) //Getting Out
        {
            if(log_enabled) Serial.print(" ----- Back to Menu ! \n ");

            rgb_leds_off();//Redundant but safe

            //Wait before running the new task creation to avoid false triggers    
            wait_for_btn_2_release();   

            if(oled_enabled)oled_clear();             

            create_task_demo_menu();

            
            task_demo_3_imu_acc_i2c_release();
            vTaskDelete(NULL);//Delete itself
            //Nothing Should Run Here
        }

        if (btn_1.is_pressed) //Next Pattern was selected
        {
            rgb_leds_off();//Redundant but safe
            wait_for_btn_1_release();
            first_loop = true;
        }

        if(btn_1.number_of_push == 0 )
        {
            if(first_loop)
            {
                if(log_enabled) Serial.print("\n--IMU Acc Across X --");
                rgb_led_blink_once(2,'w',100,100);
                first_loop = false;
            }
            
            if(!btn_1.is_pressed && !btn_2.is_pressed)
            {
                rgb_leds_demo_imu_acc(2,true,led_brightness,'x',max_acc);
                wait(10);                
            }    
        }
        
        else if(btn_1.number_of_push == 1 )
        {
            if(first_loop)
            {
                if(log_enabled) Serial.print("\n-- IMU ACC Across Y --");
                rgb_led_blink_once(2,'w',100,100);
                first_loop = false;
            }
            
            if(!btn_1.is_pressed && !btn_2.is_pressed)
            {
                rgb_leds_demo_imu_acc(2,true,led_brightness,'y',max_acc);
                wait(10);          
            }    
        }

        else if(btn_1.number_of_push == 2 )
        {
            if(first_loop)
            {
                if(log_enabled) Serial.print("\n-- IMU Acc Across Z --");
                rgb_led_blink_once(2,'w',100,100);
                first_loop = false;
            }
            
            if(!btn_1.is_pressed && !btn_2.is_pressed)
            {
                rgb_leds_demo_imu_acc(2,true,led_brightness,'z',max_acc);
                wait(10);                  
            }    
            
        }

        else if(btn_1.number_of_push == 3 )
        {
            btn_1.number_of_push =0;
            //Back to X
        }
    }
    wait(10);
}

//DEMO 4 --- LOCK

//Key is RGB

TaskHandle_t task_demo_4_lock_handle = NULL;

void create_task_demo_4_lock() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n-- creating task_demo_4_lock --");

    task_demo_4_lock_i2c_declare();

    xTaskCreate
    (
        demo_4_lock,           //Function Name
        "task_demo_4_lock", //Logging Name
        4096,                //Stack Size
        NULL,                //Passing Parameters
        3,                   //Task Priority
        &task_demo_4_lock_handle
    );  

    if(log_enabled) Serial.print("-- done --\n");
}

void task_demo_4_lock_i2c_declare()
{
    if(log_enabled)Serial.print("\ntask_demo_4_lock_i2c_declared\n");
    //This Taks will use the following I2C_Devs

    //imu_needed++;
    rgb_needed++;
    //temp_needed++;
    //lux_needed++;
    //rtc_needed++;
    //fuelgauge_needed++;
    oled_needed++;
}

void task_demo_4_lock_i2c_release()
{
    if(log_enabled)Serial.print("\ntask_demo_4_lock_i2c_released\n");
    //This Taks will release the following I2C_Devs
    
    //imu_needed--;
    rgb_needed--;
    //temp_needed--;
    //lux_needed--;
    //rtc_needed--;
    //fuelgauge_needed--;
    oled_needed--;
}


void demo_4_lock(void * parameters)
{

    if(log_enabled) Serial.println("\n\n------Starting Demo 4 LOCK----\n");

    int current_led = 0;
    btn_1.number_of_push = 0;
    bool first_loop = true;
    char current_color = 'r';

    #define brightness 50
    #define blink_interval_ms 300

    //TODO Later find a nice array way to do this 
    // KEY For DEMO is RGB
    char key_0 = 'r'; 
    char key_1 = 'g';
    char key_2 = 'b';

    char selection_0 ;
    char selection_1 ;
    char selection_2 ;


    //0 proccess
    //1 wrong key
    //2 correct key
    //3 blocked (attempts finished)
    int lock_status = 0;

    int remaining_attempts = 5;

    bool correct_combination = false;

    if(log_enabled) Serial.print("\n\n------First LED---");

    while(1)
    {
        //RUN the OLED
        if(oled_enabled)oled_demo_lock(lock_status,remaining_attempts);   

        //Exit just if combination is succesful and BTN_2 Pressed   
        if(btn_2.is_pressed && correct_combination) 
        {
            if(log_enabled) Serial.print(" ----- Back to Menu ! \n ");

            rgb_leds_off();//Redundant but safe

            wait_for_btn_2_release();

            oled_clear();          

            create_task_demo_menu();

            task_demo_4_lock_i2c_release();
            vTaskDelete(NULL);//Delete itself
        }

        //Lock again if combination succesful and BTN_1 Pressed
        if (btn_1.is_pressed && correct_combination) //Next LED was selected
        {
            rgb_leds_off();//Redundant but safe
            wait_for_btn_1_release();

            if(log_enabled) Serial.print("\n------Restarting LOCKING---");
            lock_status = 0;
            first_loop = true;
            correct_combination = false;
            current_led = 0;
            btn_1.number_of_push = 0;

        }

        //Next LED was selected
        if (btn_1.is_pressed && !correct_combination) 
        {            
            wait_for_btn_1_release();
            first_loop = true;
            
            if (current_led > 2) current_led = 0;
        }

        else if(btn_2.is_pressed && !correct_combination) 
        {
            if (current_led == 0)
            {
                //Freeze the led color and jump to next
                rgb_led_on(current_led,current_color,brightness);
                //Save the Selection
                selection_0 = current_color;
                //Restart the Color Order
                btn_1.number_of_push =0; //Restarting on R
                first_loop = true; //Entering for First time on New LED
                //Jump to Next Led
                current_led ++;                
                if(log_enabled) Serial.print("\n------Second LED---");
            }
            else if (current_led ==1)
            {
               rgb_led_on(current_led,current_color,brightness);    
               selection_1 =  current_color; 
               btn_1.number_of_push =0;
               first_loop = true;
               current_led++; 
               if(log_enabled) Serial.print("\n------Third LED---");
            } 
            else if (current_led ==2)
            {
                //Freeze the led color and compare key
                rgb_led_on(current_led,current_color,brightness);
                wait(1000);// Wait to increase tension on the user XD     

                selection_2 = current_color;

                if (selection_0 == key_0 &&
                    selection_1 == key_1 &&
                    selection_2 == key_2) //Correct Key
                {
                    if(log_enabled) Serial.print("\n--Correct Key!--\n");
                    lock_status = 2;
                    rgb_leds_blink_once('g',brightness,blink_interval_ms); 
                    correct_combination = true; 
                }
                else //False Key
                {
                    //attempts_left--  Not implemented but good to remember 
                    if(log_enabled) Serial.print("\n--Incorrect Key!--\n");    
                    

                    lock_status = 1;
                    remaining_attempts--;
                    if(log_enabled)Serial.printf("--Remaining Attempts: %d\n", remaining_attempts); 
                    
                    rgb_leds_blink_once('r',brightness,blink_interval_ms);

                    current_led = 0;
                    btn_1.number_of_push = 0;
                    first_loop = true;
                }                 
            }
            wait_for_btn_2_release();          
        }


        if(btn_1.number_of_push > 2 )
        {
            btn_1.number_of_push =0;
            //Back to 0
        }

        if(btn_1.number_of_push == 0 && first_loop)//Red
        {
            if(log_enabled) Serial.print("-R-");
            current_color = 'r';

            first_loop = false;                  
        }
        
        else if(btn_1.number_of_push == 1 && first_loop)//Green
        {
            if(log_enabled) Serial.print("-G-");
            current_color = 'g';

            first_loop = false;
        }

        else if(btn_1.number_of_push == 2 && first_loop)//Blue
        {
            if(log_enabled) Serial.print("-B-");
            current_color = 'b';     
                
            first_loop = false;
        }

        if(!btn_1.is_pressed && !btn_2.is_pressed && !correct_combination)
        {             
            rgb_led_blink_once(current_led,current_color,brightness,blink_interval_ms); 
        } 

        else wait(10);//waiting after correct key (Restart or Exit) decision 
    
    }
    wait(10);

}


// BUZZER DEMO

//DEMO 5 --- BUZZER

TaskHandle_t task_demo_5_buzzer_handle = NULL;

void create_task_demo_5_buzzer() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n-- creating task_demo_5_buzzer --");

    task_demo_5_buzzer_i2c_declare();
    
    xTaskCreate
    (
        demo_5_buzzer,           //Function Name
        "task_demo_5_buzzer", //Logging Name
        4096,                //Stack Size
        NULL,                //Passing Parameters
        3,                   //Task Priority
        &task_demo_5_buzzer_handle
    );

    if(log_enabled) Serial.print("-- done --\n");
}

void task_demo_5_buzzer_i2c_declare()
{
    if(log_enabled)Serial.print("\ntask_demo_5_buzzer_i2c_declared\n");
    //This Taks will use the following I2C_Devs

    //imu_needed++;
    //rgb_needed++;
    //temp_needed++;
    //lux_needed++;
    //rtc_needed++;
    //fuelgauge_needed++;
    oled_needed++;
}

void task_demo_5_buzzer_i2c_release()
{
    if(log_enabled)Serial.print("\ntask_demo_5_buzzer_i2c_released\n");

    //This Taks will release the following I2C_Devs
    
    //imu_needed--;
    //rgb_needed--;
    //temp_needed--;
    //lux_needed--;
    //rtc_needed--;
    //fuelgauge_needed--;
    oled_needed--;
}

void demo_5_buzzer(void * parameters)
{
    int tones_total = 7; //Including Exit option 

    btn_1.number_of_push=0;
    btn_2.number_of_push=0;

    if(log_enabled) Serial.print("\n\n------Starting Demo 5 BUZZER----");

    bool first_loop = true;


    if(log_enabled) Serial.print("\n----Sending Buzzer Menu Order--");

    int cursor_pos = 50;

    if(oled_enabled)oled_template_buzzer(cursor_pos);  

    if(log_enabled) Serial.print("\n---LOOPING--");

    while(1)
    {
        if(btn_1.is_pressed)
        {
            if(btn_1.number_of_push > tones_total) // tones + exit 
            {
                //Back to the First Option
                btn_1.number_of_push=0;
                cursor_pos = 50;
            }
            else cursor_pos +=10;//Signalize Next Option

            if(btn_1.number_of_push == demos_total)
            {
                if(log_enabled) Serial.print("\n-- Selected EXIT --- ");
                rgb_leds_off();
            }
            else
            {
                if(log_enabled) Serial.printf("\n-- Selected Tone: %u --- ",btn_1.number_of_push);
                rgb_led_on(btn_1.number_of_push,'r',100);
            } 
                        
            oled_template_buzzer(cursor_pos);
 
            //TODO Uncomment this if there is more demos(and revisse previos if (modify "demos_total" statement )) 

            /*
            else if (btn_1.number_of_push>4 && btn_1.number_of_push<10)
            {
                rgb_led_on(btn_1.number_of_push-5,'g',100);
            }
  
            else if (btn_1.number_of_push>9 && btn_1.number_of_push<15)
            {
                rgb_led_on(btn_1.number_of_push-10,'b',100);
            }
            */
            wait_for_btn_1_release();

        }

        if(btn_2.is_pressed)
        {
            wait_for_btn_2_release(); 

            rgb_leds_off(); 

            //demo _1 lux
            //demo _2 lock 
            //demo _3 IMU with danger zones
            //demo _4 lock
            //demo _5 buzzer
            //demo _6 status
            //
            //option 7 is exit   

            if (btn_1.number_of_push < tones_total)
            {
                if(log_enabled) Serial.printf("--- Selected Tone %u---",btn_1.number_of_push);
                buzzer_tone(btn_1.number_of_push);
                btn_2.is_pressed = false; //Out to menu (tone is executed just once every click)
            }
            else //exit 
            {                
                create_task_devel_menu();

                task_demo_menu_i2c_release();
                vTaskDelete(NULL);//Delete Itself
                break;
            }     
        }
        wait(10); //to Not freak_out if skipping all ifs
    }      
}


//DEMO 6 --- STATUS ------------------------------------------------

TaskHandle_t task_demo_6_status_handle = NULL;

void create_task_demo_6_status() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n-- creating task_demo_6_status --");

    task_demo_6_status_i2c_declare();
    
    xTaskCreate
    (
        demo_6_status,           //Function Name
        "task_demo_6_status", //Logging Name
        4096,                //Stack Size
        NULL,                //Passing Parameters
        1,                   //Task Priority
        &task_demo_6_status_handle
    );

    if(log_enabled) Serial.print("-- done --\n");
}

void task_demo_6_status_i2c_declare()
{
    if(log_enabled)Serial.print("\ntask_demo_6_status_i2c_declared\n");
    //This Taks will use the following I2C_Devs

    //imu_needed++;
    //rgb_needed++;
    temp_needed++;
    lux_needed++;
    rtc_needed++;
    fuel_gauge_needed++;
    oled_needed++;
}

void task_demo_6_status_i2c_release()
{
    if(log_enabled)Serial.print("\ntask_demo_6_status_i2c_released\n");

    //This Taks will release the following I2C_Devs
    
    //imu_needed--;
    //rgb_needed--;
    temp_needed--;
    lux_needed--;
    rtc_needed--;
    fuel_gauge_needed--;
    oled_needed--;
}

void demo_6_status(void * parameters)
{
    //List of screens
    #define battery 0
    #define imu 1

    btn_1.number_of_push=0;

    if(log_enabled) Serial.print("\n\n------Starting Demo 6 STATUS----");

    bool first_loop = true;

    while(1)
    {
        if(btn_2.is_pressed) //Getting Out
        {
            if(log_enabled) Serial.print(" ----- Back to Menu ! \n ");

            
            wait_for_btn_2_release();

            //rgb_leds_off();//Might be redundant 

            create_task_demo_menu();

            task_demo_6_status_i2c_release();
            vTaskDelete(NULL);//Delete itself
        }

        if (btn_1.is_pressed) //Next SCREEN was selected
        {
            //Just 2 screens at the moment , change his if nr. of screens increase
            if(btn_1.number_of_push > 1) btn_1.number_of_push = 0;    

            wait_for_btn_1_release();
            first_loop = true;
        }

        if(first_loop)
        {
            if(log_enabled) Serial.printf("\n--SHOWING STATUS SCREEN NR : %d--", btn_1.number_of_push);
            
            //Manually handling the imu trigger if the IMU SCREEN IS SELECTED
            if(btn_1.number_of_push == imu && imu_needed == 0) imu_needed++;
            else if(btn_1.number_of_push != imu && imu_needed > 0) imu_needed--;
            
            first_loop = false;
        }

        //Go to the proper screen status

        if(!btn_1.is_pressed && !btn_2.is_pressed)
        {
            oled_demo_status(btn_1.number_of_push); //SCREEN NR  
                
            wait(100); //Refresh rate                 
        }   

        else wait(10);       
    }
    wait(10);
}






