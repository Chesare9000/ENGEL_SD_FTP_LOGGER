




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
#include <i2c.h>
#include <wifi_demo.h>
#include <charger.h>
#include <fuel_gauge.h>
#include <buzzer.h>

#include <ble_demo_fw.h>
#include <listener.h>
#include <ble.h>

// BLE UUID
std::string uuid = "0000fef3-0000-1000-8000-00805f9b34fb";
// reference to ble service
BLE::BLEngelService *engelService = new BLE::BLEngelService();
BLE::Listener *listener;

std::map<std::string, std::string> cast_reference;
std::map<std::string, int*> int_map;
std::map<std::string, bool*> bool_map;
std::map<std::string, float*> float_map;
std::map<std::string, uint32_t*> uint32_map;
std::map<std::string, std::string*> string_map;
std::map<std::string, char*> char_map;

//Initializing Variables for callback 
//Callback must raise the callback_received flag;

char ble_led_color = 'r';
int ble_led_mode = 0;
int ble_led_brightness = 0;

/*
0 -> Off
1 -> Single Beep
2 -> Double Beep
3 -> Alarm
*/
int ble_buzzer_mode = 0 ; // long press

/*
0 -> Off
1 -> Silent Alarm
2 -> Loud Alarm
*/
int ble_alarm_mode = 0;

bool callback_received = false;


//creating local vars to compare existing status

char ble_led_color_current = 'r';
int ble_led_mode_current = 0;
int ble_led_brightness_current = 0;
bool running_task_ble_led_mode = false;



int ble_buzzer_mode_current = 0 ;
bool running_task_ble_buzzer = false;


int ble_alarm_mode_current = 0;
bool ble_alarm_mode_changed = true;   
bool running_task_ble_alarm = false;


//BLE_APP DEMO

//For stopping the tasks from other fns.
TaskHandle_t task_ble_app_demo_handle = NULL;

void create_task_ble_app_demo() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n-- creating task_ble_app_demo --");

    xTaskCreate
    (
        ble_app_demo,           //Function Name
        "ble_app_demo",    //Logging Name
        4096,                //Stack Size
        NULL,                //Passing Parameters
        2,                   //Task Priority
        &task_ble_app_demo_handle
    );

    if(log_enabled) Serial.print("-- done --\n");

}

void connectVariables()
{
  /// To connect variables with BLE service

  cast_reference = 
  {
	//BLE Set
	{"ble_led_brightness", "int"},
    {"ble_led_mode", "int"},
    {"ble_buzzer_mode", "int"},
    {"ble_alarm_mode", "int"},

    //BLE Receive
    {"imu_yaw", "float"},
	{"imu_pitch", "float"},
	{"imu_roll", "float"},
    {"bat_percent", "int"},
    {"board_temp", "int"},
    {"ble_led_color", "char"}
  };

  int_map = 
  {
    //BLE Set
    {"ble_led_brightness", &ble_led_brightness},
    {"ble_led_mode", &ble_led_mode},
    {"ble_buzzer_mode", &ble_buzzer_mode},
    {"ble_alarm_mode", &ble_alarm_mode},

    //BLE Receive
    {"bat_percent", &bat_percent},
    {"board_temp", &board_temp}
  };


  bool_map = 
  {
  };



  float_map = 
  {
    {"imu_yaw", &imu_yaw},
    {"imu_pitch", &imu_pitch},
    {"imu_roll", &imu_roll}
  };

  char_map = 
  {
    {"ble_led_color", &ble_led_color}
  };


// can we change const char* to std::string?
string_map = 
  {
    //{"ssid", &ssid}, // won't work because it's const char* and not std::string
    //{"password", &password} // won't work because it's const char* and not std::string
  };
}

void setup_ble()
{
	engelService = new BLE::BLEngelService();

	// Begin the BLE service
	Serial.println("Starting BLE Service");

	engelService->m_cast_reference = &cast_reference;
	engelService->m_int_map = &int_map;
	engelService->m_float_map = &float_map;
	engelService->m_bool_map = &bool_map;
	engelService->m_string_map = &string_map;
    engelService->m_char_map = &char_map;

	listener = new BLE::Listener(*engelService);

	engelService->begin();

}


void ble_app_demo(void * parameters)
{
    Serial.println("\n\n-Emulating I2c Manager Init");

    if (!i2c_initialized) i2c_init();  //Just Double check

    connectVariables();
    
    //BLE INIT
    setup_ble();

    //Hardcoding IMU VARS TO FORCE INIT
    imu_needed = true;
    imu_running = true;

    //lux init
    lux_running = true ;
    int count= 0 ;
    int refresh_cycles = 10 ;

    //Emulating the OLED on status mode
    //List of screens
    #define battery 0
    #define imu 1

    bool first_loop = true;

    btn_1_interr_enable_on_press();
    btn_2_interr_enable_on_press();

    btn_1.number_of_push=0;

    ble_alarm_mode = 2;
    callback_received = true;


    rgb_leds_enable();

    //Start the BLE_LED_MODE task
    create_task_ble_led_mode();

    //All tasks are infinite loops
    while(1)
    {


        //VARS REFRESHED BY CALLBACK

        //VARS REFRESHED INTERNALLY    

        //runs every time
        imu_run();

        //if(callback_received) //refresh vars
        //{
            if (ble_led_color != ble_led_color_current)
            {
                //Change LEDs color

                /*
                char ble_led_color
                r Red
                g Green
                b Blue
                y Yellow
                p Purple
                w White
                o Orange
                */

                if(debug_mode)Serial.println("\n--BLE_LED_COLOR changed");

                rgb_leds_on(ble_led_color,ble_led_brightness);     

                ble_led_color_current = ble_led_color;
            }
            
            if(ble_led_brightness != ble_led_brightness_current)
            {
                //change LEDs brightness (0 to 255)
                //Same fn. but changing the other parameter

                if(debug_mode)Serial.println("\n--BLE_LED_BRIGHTNESS changed");

                rgb_leds_on(ble_led_color,ble_led_brightness);

                ble_led_brightness_current = ble_led_brightness;

            }
            
            //BLE_LED_MODE running on its own task
    
            
            //if change detected from the previous cycle
            if(ble_buzzer_mode != ble_buzzer_mode_current)
            {
                /*
                0 Off
                1 Single Beep - Lock On on Loud Mode
                2 Double Beep - Lock Off off on Loud Mode
                3 Alarm
                */ 

                if(ble_buzzer_mode ==1) //Single Beep
                {
                    if(debug_mode)Serial.println("\n--Playing One Beep");

                    buzzer_one_tone(2000,300,300,1);

                    //resseting mode to bring back the off state
                    ble_buzzer_mode = 0;
                }

                if(ble_buzzer_mode ==2) //Double Beep
                {
                    if(debug_mode)Serial.println("\n--Playing Two Beeps");

                    buzzer_one_tone(1000,300,300,2);

                    //resseting mode to bring back the off state
                    ble_buzzer_mode = 0;

                }

                //If we have a continuous mode (like buzzer =3 ) 
                //we have to start a task
                if(ble_buzzer_mode == 3)//Alarm
                {
                    if(debug_mode)Serial.println("\n--Buzzer in Alarm Mode");
                    
                    //Will destroy itself when ble_buzzer_mode != 3

                    if(!running_task_ble_buzzer)create_task_ble_buzzer();
                }

                //If We need more looping modes we need to handle them properly on the task
                //for example with local variables inside the loop
                
                ble_buzzer_mode_current = ble_buzzer_mode;

            }

            if(ble_alarm_mode != ble_alarm_mode_current)
            {
                //Task will Delete Itself
                if(ble_alarm_mode==0) Serial.println("\n-Alarm Disabled-");

                else
                {
                    ble_alarm_mode_changed = true;   
                    
                    if(!running_task_ble_alarm)
                    {
                        Serial.printf("\n Creating Alarm Task for mode : %d",ble_alarm_mode);    
                        create_task_ble_alarm();
                    }                  

                }
                   

                /*
                Change to Alarm Mode will override all previous ble vars)

                0 disable alarm
                1 silent alarm enabled
                2 loud alarm enabled

                Talk to see if we implement a locked demo mode in where the device will
                beep and notify the app if moved (loud mode) 
                or will just send notification (silent mode)
                */
               
                
                ble_alarm_mode_current = ble_alarm_mode;
            }

            //ONCE THIS IS FINISHED UPDATE THE OLED SCREEN NR3
      
            //clean the flag  
            //callback_received = false ;  
        //}
        
        //refresh every 10 iterations
        if(count > refresh_cycles)
        {
            lux_val = lux_get();

            non_critical_refresh(0);

            oled_demo_status(btn_1.number_of_push); //SCREEN NR 
            oled_refresh();

            count = 0;
        }
        else count++;       


        //FOR OLED
        if (btn_1.is_pressed) //Next SCREEN was selected
        {
            //Just 2 screens at the moment , change his if nr. of screens increase
            if(btn_1.number_of_push > 2) btn_1.number_of_push = 0;    

            wait_for_btn_1_release();
            if(log_enabled) Serial.printf("\n--SHOWING STATUS SCREEN NR : %d--", btn_1.number_of_push);

            oled_clear();
            oled_demo_status(btn_1.number_of_push); //SCREEN NR 
            oled_refresh();
        }

        if(btn_2.is_pressed) //Getting Out
        {          
            btn_1_interr_disable();
            btn_2_interr_disable();
          
            wait_for_btn_2_release();

           
            vTaskDelete(task_ble_led_mode_handle);
            running_task_ble_led_mode = false;
            rgb_leds_off();         
            
            //Reenabling the i2c manager
            create_task_i2c_manager();

            if(log_enabled) Serial.print(" ----- Back to Menu ! \n ");
            create_task_devel_menu();

            vTaskDelete(NULL);//Delete itself
        
        }
    }


}


//led mode handler
//this will automatically update the led pattern after the loop is finished to not 

//For Handling the LEDS
TaskHandle_t task_ble_led_mode_handle = NULL ;

void create_task_ble_led_mode() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n-- creating task_ble_led_mode --");

    xTaskCreate
    (
        ble_led_mode_task,   //Function Name
        "ble_led_mode",      //Logging Name
        4096,                //Stack Size
        NULL,                //Passing Parameters
        2,                   //Task Priority
        &task_ble_led_mode_handle
    );

    running_task_ble_led_mode = true;

    if(log_enabled) Serial.print("-- done --\n");

}

void ble_led_mode_task(void * parameters)
{    
    //Local Parameters, if needed can be exposed as externs
    int blink_interval = 1000; 
    
    int min_brightness = 0;
    int fading_interval =10;

    rgb_leds_enable();

    //First Entry
    if(ble_led_mode_current == 0)
    {
        //Solid Light
        rgb_leds_on(ble_led_color,ble_led_brightness);
    }  

    //All tasks are infinite loops
    while(1)
    {
        /*
        ble_led_mode
        0 solid
        1 blinking 1 second
        2 fading
        3 pong
        */

        //the ble_led_mode  changed
        if(ble_led_mode != ble_led_mode_current)
        {
            if(ble_led_mode >=0 && ble_led_mode <= 3)
            {
                if(debug_mode) Serial.printf("\n--BLE_LED MODE changed to %d\n",ble_led_mode); 
            }   
            else
            {
                if(debug_mode) Serial.printf("\n--ERROR on BLE_LED MODE:%d, NO SUCH OPTION\n",ble_led_mode); 
            }
            
            //Change LED mode

            ble_led_mode_current = ble_led_mode;

            //only mode that need to be executed just once
            if(ble_led_mode_current == 0)
            {
                //Solid Light
                rgb_leds_on(ble_led_color,ble_led_brightness);
            }            
        }

        if(ble_led_mode == 1) //Blinking 1 Second interval
        {
            rgb_leds_blink_once(ble_led_color, ble_led_brightness,blink_interval);            
        }

        else if(ble_led_mode == 2) //Fading
        {
            
            rgb_leds_fade_once(2,ble_led_color,min_brightness,ble_led_brightness,fading_interval);            
        }

        else if(ble_led_mode == 3)//Pong
        {
            for(int i=0 ; i<5 ;i++)
            {
                if(ble_led_mode != ble_led_mode_current)break;
                rgb_leds_off();
                rgb_led_on(i,ble_led_color,ble_led_brightness);
                wait(100);    
            }
            for(int i=4 ; i>-1 ;i--)
            {
                if(ble_led_mode != ble_led_mode_current)break;
                rgb_leds_off();
                rgb_led_on(i,ble_led_color,ble_led_brightness);
                wait(100);    
            }
        }

        //TODO MAYBE LATER ONE MANAGED BY LUX    

        else wait(100);
    }

}



//For Handling the BUZZER
TaskHandle_t task_ble_buzzer_handle = NULL ;

void create_task_ble_buzzer() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n-- creating ble_buzzer_task --");

    xTaskCreate
    (
        ble_buzzer_task,   //Function Name
        "ble_buzzer_task",      //Logging Name
        4096,                //Stack Size
        NULL,                //Passing Parameters
        2,                   //Task Priority
        &task_ble_buzzer_handle
    );

    running_task_ble_buzzer = true;

    if(log_enabled) Serial.print("-- done --\n");

}

void ble_buzzer_task(void * parameters)
{
    if(log_enabled) 
    {
        Serial.print("\n-- STARTING BLE_BUZZER_TASK --\n");
        Serial.printf("\n--BLE_BUZZER_MODE : %d --\n",ble_buzzer_mode);
    }

    while(1)
    {
        //Right now is just enabled for the alarm (ble_buzzer_mode == 3
        if(ble_buzzer_mode == 3)
        {
            rise_fall(1);
        }

        else //ble_buzzer_mode changed and the task is not necessary anymore
        {
            Serial.printf("\n--BLE_BUZZER_MODE : %d --\n",ble_buzzer_mode);
            Serial.print("\n-- DELETING BLE_BUZZER_TASK");
            wait(200);    
            running_task_ble_buzzer = false;
            vTaskDelete(NULL);
        }
    }    

}

//For Handling the ALARM
TaskHandle_t task_ble_alarm_handle = NULL ;

void create_task_ble_alarm() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n-- creating ble_alarm_task --");

    xTaskCreate
    (
        ble_alarm_task,   //Function Name
        "ble_alarm_task",      //Logging Name
        4096,                //Stack Size
        NULL,                //Passing Parameters
        2,                   //Task Priority
        &task_ble_alarm_handle
    );

    running_task_ble_alarm = true;

    if(log_enabled) Serial.print("-- done --\n");



}

void ble_alarm_task(void * parameters)
{
    if(log_enabled) 
    {
        Serial.print("\n-- STARTING BLE_ALARM_TASK --\n");
        Serial.printf("\n--BLE_ALARM_MODE : %d --\n",ble_alarm_mode);
    }



    //0 not triggered
    //1 first warning
    //2 Alarm Triggered & Notifiation Sent
    //3 Infinite loop until response (button or app)
    int ble_alarm_status = 0;


    while(1)
    {
        /*
        Change to Alarm Mode will override all previous ble vars)

        0 disable alarm
        1 silent alarm enabled
        2 loud alarm enabled
        */

        if(ble_alarm_mode == 0) //Alarm Deactivated
        {
            Serial.printf("\n--BLE_ALARM_MODE : %d --\n",ble_alarm_mode);
            Serial.print("\n-- DELETING BLE_ALARM_TASK");

            ble_alarm_mode_changed =false;
            running_task_ble_alarm = false;
            
            wait(200);  
            running_task_ble_alarm = false;  
            vTaskDelete(NULL);

        }
        
        else if(ble_alarm_mode ==1)  //Silent Alarm
        {
            if(ble_alarm_mode_changed)
            {
                Serial.print("\n--Silent Alarm Enabled--\n");
                ble_alarm_mode_changed = false;

                //Resetting Flags
                ble_alarm_status = 0;
            }

            if (ble_alarm_status == 0)
            {
                if(moving)
                {
                    Serial.println("Moving ! -- First Warning");
                    wait(5000);
                    if(moving)ble_alarm_status++; //If still moving then we escalate , if not we reset
                }
            }

            if(ble_alarm_status == 1)
            {
                Serial.println("Silent Alarm Triggered");
                Serial.println("Notifying via App");
                Serial.println("Waiting for Button or App Disable");
                ble_alarm_status++;
            }

            if(ble_alarm_status == 2)
            {
                if(btn_1.is_pressed || ble_alarm_mode == 0 )
                {
                    Serial.println("Silent Alarm Dismissed");
                    ble_alarm_mode = 0; //In case of button dismiss
                    //No need to reset local vars as the mode 0 will destroy the task
                }
            }
        }

        else if (ble_alarm_mode == 2) //Loud Alarm
        {
            if(ble_alarm_mode_changed)
            {
                Serial.print("\n--Loud Alarm Enabled--\n");
                ble_alarm_mode_changed = false;

                //Resetting Flags
                ble_alarm_status = 0;
            }

            if (ble_alarm_status == 0)
            {
                if(moving)
                {
                    Serial.println("Moving ! -- First Warning");
                    //buzzer_one_tone(1000,200,0,1);
                    rgb_leds_blink_once('r',200,5000); //this will already wait internally so we can compare after 5 secs
                    //wait(5000);
                    if(moving)ble_alarm_status++; //If still moving then we escalate , if not we reset
                }
            }

            if(ble_alarm_status == 1)
            {
                Serial.println("Loud Alarm Triggered");
                Serial.println("Ringing and Notifying via App");
                Serial.println("Waiting for Button or App Disable");
                
                ble_buzzer_mode = 3;

                if(!running_task_ble_buzzer)create_task_ble_buzzer();

                ble_led_mode = 1;
                ble_led_brightness = 200;

                ble_alarm_status++;
            }

            if(ble_alarm_status == 2)
            {
                //All looping , waiting for response here 
                if(btn_1.is_pressed || ble_alarm_mode == 0 )
                {
                    Serial.println("Loud Alarm Dismissed");



                    ble_alarm_mode = 0; //In case of button dismiss

                    //turning off leds
                    ble_led_mode = 0;
                    ble_led_brightness = 0;
                    
                    //This will reset the var and destroy the task    
                    ble_buzzer_mode  = 0; 

                    //buzzer_one_tone(500,200,100,3);

                    //No need to reset local vars as the mode 0 will destroy the task
                }
            }

        }


        else //ERROR
        {
            if(ble_alarm_mode_changed)
            {
                Serial.print("\n--ERROR ble_alarm_mode NOT RECOGNIZED : ESCAPING TASK--\n");

                ble_alarm_mode = 0;
            }
        }

        wait(10);
    }    

}