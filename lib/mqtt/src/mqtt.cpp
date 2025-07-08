//In this implementation we will find first the pure functions 
//to get MQTT running and later the logic to get the info out and how 
//often depending on the type of connection, bein WiFi or LTE

#include <PubSubClient.h>
#include <WiFi.h>

#include <wifi.h>
#include <tools.h>
#include <mqtt.h>
#include <vars.h>
#include <rgb.h>
#include <tasks.h>
#include <interrupts.h>
#include <oled.h>
#include <nvs.h>

/*
//TODO change this to NVS
const char* mqtt_broker_domain = "192.168.178.162";
uint16_t    mqtt_broker_port   = 1883;

const char* mqtt_broker_client_id   = "engel";
const char* mqtt_broker_client_pass = "engel";

bool mqtt_initialized = false;
bool mqtt_connected = false;

bool backend_updated = false;

//MQTT EMulating BLE Structure for same vars
//Initializing Variables for callback 
//Callback must raise the callback_received flag;

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


//Coming from Wifi lib and 
//Needed by MQTT client constructor
WiFiClient wifi_client;

PubSubClient mqtt_client(wifi_client);




//ESP will publish the topics:
//(cannot be changed from the outside)

//SENSORS 
const char* mqtt_out_lux  = "/engel_main/out/sensors/lux";
const char* mqtt_out_temp = "/engel_main/out/sensors/temp";
const char* mqtt_out_soc  = "/engel_main/out/sensors/soc";

//STATUS
const char* mqtt_out_charging = "/engel_main/out/status/charging";
const char* mqtt_out_usb_in   = "/engel_main/out/status/usb_in";
const char* mqtt_out_moving   = "/engel_main/out/status/moving";
const char* mqtt_out_low_bat  = "/engel_main/out/status/low_bat";
const char* mqtt_out_cycle_nr = "/engel_main/out/status/cycle_nr";

//EMERGENCY (CRITICAL SIGNALS)
const char* mqtt_out_park_alarm_movement_detected  = "/engel_main/out/emergency/park_alarm/movement_detected";
const char* mqtt_out_park_alarm_triggered  = "/engel_main/out/emergency/park_alarm/triggered";



//DASHBOARD REFRESHING (COPY FROM SUBS)
const char* mqtt_out_leds_on          = "/engel_main/out/actions/leds/on";
const char* mqtt_out_leds_brightness  = "/engel_main/out/actions/leds/brightness";
const char* mqtt_out_leds_color       = "/engel_main/out/actions/leds/color";
const char* mqtt_out_leds_pattern     = "/engel_main/out/actions/leds/pattern";

const char* mqtt_out_park_alarm_on     = "/engel_main/out/actions/park_alarm/on";
const char* mqtt_out_park_alarm_mode   = "/engel_main/out/actions/park_alarm/mode";





//ESP will subscribe to topics:
//(Changed from outside)

const char* mqtt_in_leds_on          = "/engel_main/in/actions/leds/on";
const char* mqtt_in_leds_brightness  = "/engel_main/in/actions/leds/brightness";
const char* mqtt_in_leds_color       = "/engel_main/in/actions/leds/color";
const char* mqtt_in_leds_pattern     = "/engel_main/in/actions/leds/pattern";

const char* mqtt_in_park_alarm_on     = "/engel_main/in/actions/park_alarm/on";
const char* mqtt_in_park_alarm_mode   = "/engel_main/in/actions/park_alarm/mode";






//MQTT specific functions

void mqtt_init()
{
    Serial.println("\n----Initializing MQTT!");
    mqtt_client.setServer(mqtt_broker_domain,mqtt_broker_port);
    mqtt_client.setCallback(subscriber_callback);
    mqtt_initialized = true;

    //Setting Defaults
    backend_parking_alarm_state = false; //OFF
    // 0-> silent , 1 -> loud 
    backend_parking_alarm_mode = false;
    backend_parking_alarm_movement_detected = false;
    backend_parking_alarm_triggered = false;
    
}

void mqtt_connect()
{
    while(!mqtt_initialized)
    {
        mqtt_init();
    }
    
    while(!mqtt_client.connected())
    {
        Serial.println( "\nConnecting To ");
        Serial.print(mqtt_broker_domain);
        if(mqtt_client.connect("engel_test",
                          mqtt_broker_client_id,
                          mqtt_broker_client_pass))
        {
            Serial.println("\n --- Connected !"); 
            mqtt_connected = true;           
        }
        else
        {
            Serial.println("\n --- Retrying Connection in 5 secs !");
            wait(5000);   
        }
    }
}

bool mqtt_still_connected()
{
    if  (mqtt_client.connected()) return true;
    
    else 
    { 
        mqtt_connected = false;
        return false;
    }
}

void mqtt_client_loop()
{
    mqtt_client.loop();
}


void mqtt_publish(out_topic_list topic_to_publish,topic_print_info selection)
{
     
    //Serial.printf("\r--MQTT_TX_ID: %d--",mqtt_cycle_nr);
    //publish the mqtt_cycle_nr
    char cycle_nr_string[50];
    snprintf(cycle_nr_string,75,"%ld",mqtt_cycle_nr);    
    mqtt_client.publish(mqtt_out_cycle_nr,cycle_nr_string);      

    switch(topic_to_publish)
    {
        //all sensor topics
        case sensors:
        {
            if(log_enabled && (selection == print_topic || selection == print_all))
            {Serial.print("\n---Publishing Sensors: ");}
            
            //lux
            if(log_enabled && selection == print_all) Serial.printf("|LUX:%d",lux_val);
            
            char lux_string[50];
            snprintf(lux_string,75,"%ld",lux_val);    
            mqtt_client.publish(mqtt_out_lux,lux_string); 

            //temp
            if(log_enabled && selection == print_all) Serial.printf("|TEMP:%d",board_temp);
            
            char temp_string[50];
            snprintf(temp_string,75,"%ld",board_temp);    
            mqtt_client.publish(mqtt_out_temp,temp_string);

            //soc
            if(log_enabled && selection == print_all) Serial.printf("|SOC:%d",bat_percent);
            
            char soc_string[50];
            snprintf(soc_string,75,"%ld",bat_percent);    
            mqtt_client.publish(mqtt_out_soc,soc_string);         
        }
        break;

        case status:
        {
            if(log_enabled && (selection == print_topic || selection == print_all)) 
            {Serial.print("\n\n---Publishing Status: ");}

            //charging
            if(log_enabled && selection == print_all) Serial.printf("|CHARGING:%d",charging);

            char charging_string[50];
            snprintf(charging_string,75,"%ld",charging);    
            mqtt_client.publish(mqtt_out_charging,charging_string);         

            //usb_in
            if(log_enabled && selection == print_all) Serial.printf("|USB_IN:%d",usb_connected);
            
            char usb_in_string[50];
            snprintf(usb_in_string,75,"%ld",usb_connected);    
            mqtt_client.publish(mqtt_out_usb_in,usb_in_string);

            //moving
            if(log_enabled && selection == print_all) Serial.printf("|MOVING:%d",moving);

            char moving_string[50];
            snprintf(moving_string,75,"%ld",moving);    
            mqtt_client.publish(mqtt_out_moving,moving_string);

            //low_bat
            if(log_enabled && selection == print_all) Serial.printf("|LOW_BAT:%d",low_bat);

            char low_bat_string[50];
            snprintf(low_bat_string,75,"%ld",low_bat);    
            mqtt_client.publish(mqtt_out_low_bat,low_bat_string);

            
            //TO DO Add later a publish to all the subs to confirm change within the esp
        }
        break;


        //either crash or alarms (Passed without delay)
        case emergency:
        {
            if(log_enabled && (selection == print_topic || selection == print_all)) 
            {Serial.print("\n\n---Publishing Emergency: ");}

            //PARK_ALARM_MOVEMENT_DETECTED
            if(log_enabled && selection == print_all) Serial.printf("|PARK_ALARM_MOVEMENT_DETECTED:%d",backend_parking_alarm_movement_detected);

            char mqtt_parking_alarm_movement_detected_string[50];
            snprintf(mqtt_parking_alarm_movement_detected_string,75,"%ld",backend_parking_alarm_movement_detected);   

            mqtt_client.publish(mqtt_out_park_alarm_movement_detected,mqtt_parking_alarm_movement_detected_string);

            //PARK_ALARM_TRIGGERED
            if(log_enabled && selection == print_all) Serial.printf("|PARK_ALARM_TRIGGERED:%d",backend_parking_alarm_triggered);

            char mqtt_parking_alarm_triggered_string[50];
            snprintf(mqtt_parking_alarm_triggered_string,75,"%ld",backend_parking_alarm_triggered);   

            mqtt_client.publish(mqtt_out_park_alarm_triggered,mqtt_parking_alarm_triggered_string);

        }
        break;

        //Alarm Related(non-critical, just update dashboard)
        case alarm_refresh:
        {

            if(log_enabled && (selection == print_topic || selection == print_all)) 
            {Serial.print("\n\n---Publishing Alarm_Info: ");}

            //Alarm Status
            if(log_enabled && selection == print_all) Serial.printf("|P_ALARM_ON:%d",backend_parking_alarm_state);

            char mqtt_park_alarm_state_string[50];
            snprintf(mqtt_park_alarm_state_string,75,"%ld",backend_parking_alarm_state);    
            mqtt_client.publish(mqtt_out_park_alarm_on,mqtt_park_alarm_state_string);         

            //Alarm_Mode
            if(log_enabled && selection == print_all) Serial.printf("|P_ALARM_MODE:%d",backend_parking_alarm_mode);

            char mqtt_park_alarm_mode_string[50];
            snprintf(mqtt_park_alarm_mode_string,75,"%ld",backend_parking_alarm_mode);    
            mqtt_client.publish(mqtt_out_park_alarm_mode,mqtt_park_alarm_mode_string); 

            //Alarm_Triggered
            if(log_enabled && selection == print_all) Serial.printf("|P_ALARM_TRIGGER:%d",backend_parking_alarm_triggered);

            char mqtt_parking_alarm_triggered_string[50];
            snprintf(mqtt_parking_alarm_triggered_string,75,"%ld",backend_parking_alarm_triggered);    
            mqtt_client.publish(mqtt_out_park_alarm_triggered,mqtt_parking_alarm_triggered_string);

        }
        break;

        //Actions Related(non-critical, just update dashboard)
        case actions_refresh:
        {

            if(log_enabled && (selection == print_topic || selection == print_all)) 
            {Serial.print("\n\n---Publishing Actions_Info: ");}

            //LEDS_ON
            if(log_enabled && selection == print_all) Serial.printf("|LEDS_ON:%d",backend_led_status);

            char mqtt_led_status_current_string[50];
            snprintf(mqtt_led_status_current_string,75,"%ld",backend_led_status);    
            mqtt_client.publish(mqtt_out_leds_on,mqtt_led_status_current_string);         

            //LEDS_Brightness
            if(log_enabled && selection == print_all) Serial.printf("|LEDS_BRIGHTNESS:%d",backend_led_brightness);

            char mqtt_led_brightness_current_string[50];
            snprintf(mqtt_led_brightness_current_string,75,"%ld",backend_led_brightness);    
            mqtt_client.publish(mqtt_out_leds_brightness,mqtt_led_brightness_current_string); 

            //LED_COLOR
            if(log_enabled && selection == print_all) Serial.printf("|LEDS_COLOR:%c",backend_led_color);

            char mqtt_led_color_current_string[50];
            snprintf(mqtt_led_color_current_string,75,"%c",backend_led_color);    
            mqtt_client.publish(mqtt_out_leds_color,mqtt_led_color_current_string);

            //LED_PATTERN NOT USED AT THE MOMENT (TO DO)
        }
        break;

    }
}


void mqtt_subscribe(in_topic_list topic_to_subscribe)
{
    if(log_enabled) Serial.print("Subscribing to ");

    if(topic_to_subscribe == leds || topic_to_subscribe == all)
    {
        mqtt_client.subscribe(mqtt_in_leds_on);
        mqtt_client.subscribe(mqtt_in_leds_brightness);
        mqtt_client.subscribe(mqtt_in_leds_color);
        mqtt_client.subscribe(mqtt_in_leds_pattern);        
    }

    if(topic_to_subscribe == park_alarm || topic_to_subscribe == all)
    {
        mqtt_client.subscribe(mqtt_in_park_alarm_on);
        mqtt_client.subscribe(mqtt_in_park_alarm_mode);     
    }
}



void subscriber_callback(char* topic , byte* payload , unsigned int length )
{
    Serial.print("\n\n---MQTT Subscriber Message received!");
    Serial.print("\nTopic : ");
    Serial.print(topic);
    Serial.print("\nValue: ");

    for(int i=0;i<length;i++)
    {
        Serial.print((char) payload[i]);
    }

    Serial.println();

    //Refreshing parameters (wil override any existing setting)
    //TODO : Store it later in anothe variable to set permissions 
    
    //if the arrays are identical strmp == 0;
    if(!strcmp(topic,mqtt_in_leds_on))
    {
        if((char)payload[0] == '0')
        {
            if(backend_led_status)
            {
              Serial.println("\n--- Turning OFF LEDs via MQTT!");
              rgb_leds_off();
              backend_led_status = false;
            }
            else
            {
                if(log_enabled)Serial.print("\nERROR --- LEDS already OFF!");
            }  

            //Get out of the function to save time
            return;          
        }
        else if((char)payload[0] == '1')
        {
            if(!backend_led_status)
            {
              Serial.println("\n--- Turning on LEDs via MQTT!");
              rgb_leds_on(backend_led_color,backend_led_brightness);
              backend_led_status = true;
            }    

            else
            {
              if(log_enabled)Serial.print("\nERROR ---LEDS already ON");  
            }
            
            //Get out of the function to save time
            return; 
        }
        else
        {
            if(log_enabled)Serial.print("\nERROR ON (MQTT_LEDS_ON) Subscriber parameter!");
        }      
    }

    //if the arrays are identical strmp == 0;
    else if(!strcmp(topic,mqtt_in_leds_brightness))
    {
        payload[length] = '\0';
        int value = atoi((char*)payload);

        if      (value > led_brightness_max) value = led_brightness_max;
        else if (value < led_brightness_min) value = led_brightness_min;

        if(log_enabled)Serial.printf("\n---Setting Brightness to :%d via MQTT\n", value);

        backend_led_brightness = value;

        if(backend_led_status)rgb_leds_on(backend_led_color,backend_led_brightness);

        else { if(log_enabled)Serial.print("\n ERROR --- LEDS are OFF\n");}

    }

    else if(!strcmp(topic,mqtt_in_leds_color))
    {
        //expecting a char included on the color function  
        char color = (char)payload[0];

        if(log_enabled)
        {
            Serial.print("\n---Setting Color to : ");
            Serial.print(color);
            Serial.print(" via MQTT ");
            Serial.println();
        }
      
        backend_led_color = color;

        if(backend_led_status)rgb_leds_on(backend_led_color,backend_led_brightness);

        else { if(log_enabled)Serial.print("\n ERROR --- LEDS are OFF\n");}
    }

    else if(!strcmp(topic,mqtt_in_park_alarm_on))
    {
        if((char)payload[0] == '0')
        {
            if(backend_parking_alarm_state)
            {
              Serial.println("\n--- Turning OFF Alarm via MQTT!");
              backend_parking_alarm_state = false;//This trigger will stop and kill the alarm task
            }
            else
            {
                if(log_enabled)Serial.print("\nERROR --- ALARM already OFF!");
            }  

            //Get out of the function to save time
            return;          
        }
        else if((char)payload[0] == '1')
        {
            if(!backend_parking_alarm_state)
            {
              Serial.println("\n--- Turning on ALARM via MQTT!");
                            
              //Here Start the alarm with the given mode

              create_task_parking_alarm();

              backend_parking_alarm_state = true;

            }    

            else
            {
              if(log_enabled)Serial.print("\nERROR ---ALARM already ON");  
            }
            
            //Get out of the function to save time
            return; 
        }
        else
        {
            if(log_enabled)Serial.print("\nERROR ON (MQTT_PARK_ALARM_ON) Subscriber parameter!");
        }      
    }

    else if(!strcmp(topic,mqtt_in_park_alarm_mode))
    {
        if((char)payload[0] == '0')
        {
            if(log_enabled) Serial.println("\n--- Setting Parking Alarm to Silent Mode via MQTT!");
            backend_parking_alarm_mode = false;
            //Get out of the function to save time
            return;          
        }
        else if((char)payload[0] == '1')
        {
            if(log_enabled) Serial.println("\n--- Setting Parking Alarm to Loud Mode via MQTT!");
            backend_parking_alarm_mode = true;
            //Get out of the function to save time
            return;  
        }
        else
        {
            if(log_enabled)Serial.print("\nERROR ON (MQTT_PARK_ALARM_MODE) Subscriber parameter!");
        }      
    }

    //make alarm , check switch between ble and wifi and ota 
    
    backend_updated = true;
}


//Network implementation depending on the connection type

//Vars


//0 not connected
//1 connected via wifi
//2 connected via lte
int connected_to_internet = 0;




//MQTT Task 
//Main Task to keep MQTT alive

TaskHandle_t task_mqtt_handle = NULL;

void create_task_mqtt() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n-- creating task_mqtt --");
    

    task_mqtt_i2c_declare();

    xTaskCreate
    (
        task_mqtt,           //Function Name (must be a while(1))
        "task_mqtt", //Logging Name
        4096,                //Stack Size
        NULL,                //Passing Parameters
        8,                   //Task Priority
        &task_mqtt_handle
    );   

    if(log_enabled) Serial.print("-- done --\n");
}

void task_mqtt_i2c_declare()
{
    

    if(log_enabled)Serial.print("\ntask_mqtt_i2c_declared\n");
    //This Taks will use the following I2C_Devs

    //imu_needed++;
    rgb_needed++;
    temp_needed++;
    lux_needed++;
    //rtc_needed++;
    //fuelgauge_needed++;
    oled_needed++;
}

void task_mqtt_i2c_release()
{
    if(log_enabled)Serial.print("\ntask_mqtt_i2c_released\n");
    
    //This Taks will release the following I2C_Devs
    
    //imu_needed--;
    rgb_needed--;
    temp_needed--;
    lux_needed--;
    //rtc_needed--;
    //fuelgauge_needed--;
    oled_needed--;
}



void task_mqtt(void * parameters)
{

  if(log_enabled)Serial.println("Running mqtt_task ");
  wait(100);

    
  //OUT with BTN2
  btn_2_interr_enable_on_press();
  wait(100);

  int interval = 1000;

  unsigned long counter = 0;


  while(1)
  {
    if(btn_2.is_pressed) //Gettinng Out
    {
      btn_2_interr_disable();
    
      if(log_enabled) Serial.print(" ----- Back to Menu ! \n ");   

      bool result = wifi_disconnect();
      wifi_off();

      //Clearing Flags
      wifi_connected   = false;
      mqtt_initialized = false;
      mqtt_connected   = false;
           
      wait_for_btn_2_release(); //will reset the btn.pressed flag also

      oled_clear();
      wait(100);

      create_task_devel_menu();
      wait(100);

      task_mqtt_i2c_release();
      vTaskDelete(NULL);//Delete itself
    }
    
    else if (!wifi_connected) 
    {
      
      if(log_enabled)Serial.print("\n---Connecting WIFI---\n");
      
      if(wifi_has_credentials) wifi_connect();

      wait(100);
      
    }
    
    //If never connected or connection broken
    else if(wifi_connected && !mqtt_connected)
    {
      if(log_enabled)Serial.print("\n---Connecting to MQTT ---\n");
      
      mqtt_connect();      
      wait(100);

      mqtt_subscribe(all);
      wait(100);

    }

    //All successfull
    else if (mqtt_connected)
    {
        
      if(mqtt_still_connected())//Will handle the flag 'mqtt_connected' so no else needed
      {

        if(log_enabled)Serial.println("\n--- Connected to MQTT via WIFI");

        mqtt_cycle_nr =0;

        while(mqtt_still_connected()) //Normal Loop
        {
          mqtt_client_loop();

          wait(100);

          //back to menu
          if (btn_2.is_pressed) 
          {
            if(backend_parking_alarm_triggered)
            {
              Serial.println("Cannot Exit until Alarm is dismissed!");
              wait_for_btn_2_release();                            
            }

            else break; //Getting out of while and exiting to menu later

          }
          
          //Publish Loop
          //loop every wait milliseconds
          if ( millis() > counter + interval )
          {  
            //wont send oled messages if the 
            //alarm is holding the token            
            
            if(oled_enabled && oled_token == 0) oled_mqtt(mqtt_cycle_nr);

            //Cannot publish on other tasks, just one at the time
            mqtt_publish(sensors,silent);
            wait(100);
            mqtt_publish(status,silent);
            wait(100);
            mqtt_publish(alarm_refresh,silent);
            wait(100);
            mqtt_publish(actions_refresh,silent);
            wait(100);
            mqtt_publish(emergency,silent);
            wait(100);        
            //todo actions_to refresh dashboard

            mqtt_cycle_nr++;
            counter = millis();
          }

          if(backend_updated)
          {
            if(log_enabled) Serial.print("MQTT UPDATED!");
            
            built_in_led_blink_once(500);

            wait(100);

            backend_updated = false;
          }
          
          wait(100);
        }
        
      }
    }

    else wait(100); //IDLE if not in loop

  }

}



*/

