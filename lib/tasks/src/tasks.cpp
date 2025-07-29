
#include <Arduino.h>

#include <vars.h>
#include <tools.h>
#include <ble_demo_fw.h>
#include <buzzer.h> 
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
#include <wifi.h> //local
#include <wifi_demo.h>
#include <charger.h>
#include <fuel_gauge.h>
#include <mqtt.h>
#include <firebase.h>
#include <gps.h>
#include <can.h>
#include <tasks.h>
#include <firebase.h>
#include <sd.h>


//TODO MOVE LATER ALL THIS TO BLE
//Part for the BLE


#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;

BLECharacteristic* pchar_heartbeat = NULL;
BLECharacteristic* pchar_temp_val = NULL;
BLECharacteristic* pchar_lux_val = NULL;

BLECharacteristic* pchar_rgb_pwm = NULL;

BLEDescriptor *pDescr_heartbeat;
BLE2902 *pBLE2902_heartbeat;

BLEDescriptor *pDescr_temp_val;
BLE2902 *pBLE2902_temp_val;

BLEDescriptor *pDescr_lux_val;
BLE2902 *pBLE2902_lux_val;

//RGB dont need descriptor

bool deviceConnected = false;
bool oldDeviceConnected = false;


bool ble_initialized = false;
bool connection_logged = false;

bool new_ble_rgb_pwm_value = false;

bool ble_oled_refresh = false;

//TODO Later set this as config parameter , e.g. max_brightness_pwm 

int ble_rgb_pwm =0;
uint32_t ble_beat = 0;
int ble_oled_status = 0;


bool task_gps_active = false;
bool task_can_active = false;



//For Parking_alarm
int parking_alarm_warning_time_ms = 5000; 
int parking_alarm_snooze_time_ms = 10000;



//GPS not found retry 
int gps_not_found_retry_seconds = 10; 

bool imu_test_was_running = false;

bool task_mubea_running = false;


int black_box_oled_refresh_time = 10; 

bool button_bypassed_by_terminal = true;


// For more UUIDs:
// https://www.uuidgenerator.net

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"

#define uuid_heartbeat "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define uuid_rgb_pwm "8bea0da8-9cea-4fbd-889b-136ce1ce8e75"
#define uuid_temp_val "b424195c-0700-4493-9212-afa7b72aa149"
#define uuid_lux_val "e743febd-26da-41a7-88fc-519374ab1c53"
//#define uuid_battery "92148185-45f2-4453-9bab-58988c9705e6"






bool waiting_for_oled = false;


//Callbacks for Server Status
class MyServerCallbacks: public BLEServerCallbacks 
{
    void onConnect(BLEServer* pServer) 
    {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) 
    {
      deviceConnected = false;
    }
};

//Calback for the characteristics that receive parameters
class CharacteristicsCallBack: public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pchar_rgb_pwm) override
    {

        //Demo for parsing into different datatypes

        std::string pchar_rgb_pwm_value_stdstr = pchar_rgb_pwm->getValue();
        String pchar_rgb_pwm_value_string = String(pchar_rgb_pwm_value_stdstr.c_str());
        int pchar_rgb_pwm_value_int = pchar_rgb_pwm_value_string.toInt();
        //if(log_enabled)Serial.print("\n rgb_pwm: "+String(pchar_rgb_pwm_value_int));

        ble_rgb_pwm = pchar_rgb_pwm_value_int;
        new_ble_rgb_pwm_value = true; //send order to refresh
    }
};

//---- END OF MIGRATION PROPOSAL----------------------



//Test
TaskHandle_t ble_normal_handle = NULL;


//Here the List of Generic Taks
//Specific will be implemented on their respective src

//Rules
//All FReeRTOS tasks mus be infinite (while(1){}) 
//or delete themselves with vTaskDelete(NULL);//Delete itself 

// Refresh Sensors ------------------------------------

//TODO Later make flags to update if needed e.g. if(imu_needed) etc...



//Developer Menu ----------------------------------

TaskHandle_t devel_menu_handle = NULL;

void create_task_devel_menu() //once created it will automatically run
{
    if(log_enabled)Serial.print("\n-- Creating developer menu --");

    xTaskCreate
    (
        devel_menu,           //Function Name (must be inf.loop)
        "task_devel_menu", //Logging Name (Just for ID)
        4096,                //Stack Size 
        NULL,                //Passing Parameters
        10,                   //Task Priority
        &devel_menu_handle
    );

    wait(100);

    task_devel_menu_i2c_declare();    

    wait(100);
    
    if(log_enabled)Serial.print("-- done --\n");
}

void task_devel_menu_i2c_declare()
{
    if(log_enabled)Serial.print("\ntask_devel_menu_i2c_declared\n");
    //This Taks will use the following I2C_Devs
    //imu_needed++;
    //rgb_needed++;
    //temp_needed++;
    //lux_needed++;
    //rtc_needed++;
    //fuelgauge_needed++;
    oled_needed++;

    //DEFINING THIS TASK TIME CRITICAL TO REFRESH THE OLED ASAP
    //time_critical_tasks_running++;
}

void task_devel_menu_i2c_release()
{
    if(log_enabled)Serial.print("\ntask_devel_menu_i2c_released\n");
    //This Taks will release the following I2C_Devs

    //imu_needed--;
    //rgb_needed--;
    //temp_needed--;
    //lux_needed--;
    //rtc_needed--;
    //fuelgauge_needed--;
    oled_needed--;

    //RELEASING THE TIME_CRITICAL FLAG TO ALLOW FOR NON-CRITICAL REFRESH
    //time_critical_tasks_running--;
}

void devel_menu(void * parameters)
{
    //Relation Cursor->Mode
    #define demo   50
    #define first  60
    #define park   70 
    #define ride   80 
    #define ble    90 
    #define wifi  100 
    #define mubea  110
    #define exit  120  

    if(log_enabled) Serial.println("\n\n----Starting Devel Menu ---\n");

    int cursor_position = 50 ;
    bool first_loop = true;

    btn_1_interr_enable_on_press();
    btn_2_interr_enable_on_press();

    while(1)
    {
        //Selecting once something was chosen    
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

                case demo:  
                {
                    Serial.print("DEMO\n"); 
                    //create_task_sensors_refresh();
                    create_task_demo_menu(); 
                    break; 
                }

             
             
               //TODO complete all this

                case first: { Serial.print("\n ---first");  break; }
                case park:  { Serial.print("\n ---park");  break; }
                
                case ride:  
                { 
                    Serial.print("RIDE\n"); 
                    //5V_TEST AT THE MOMENT

                    Serial.print("\n ---TESTING LEDS"); 
                    oled_clear();
                    oled_refresh();

                    reg_5v_enable();
                    
                    lvl_shftr_disable();
                    rgb_leds_off();

                    break; 
                
                }
                
                case ble:   
                
                { 
                    Serial.print("BLE\n");   
                    //Standard Behaviour with predef.vars.(MOCKUP)
                    //TODO UNCOMMENT THIS ON NEW ESP32
                    //create_task_ble_normal();

                    //Destroying i2c_manager (should be the only one running regarding fw)
                    vTaskDelete(i2c_manager_handle);
                    
                    create_task_ble_app_demo();
                    
                    break; 
                
                }
                                    
                
                case wifi:  
                
                {       
                    Serial.print("WIFI\n");  
                    //create_task_wifi_demo();

                    //Depending on the default go to

                    switch(backend_config)
                    {
                        //case backend_config_mqtt     : create_task_mqtt();     break;
                        //case backend_config_firebase : create_task_firebase(); break;

                        default : Serial.print("\n---ERROR ON BACKEND_CONFIG : METHOD NOT IMPLEMENTED!---\n");break; 
                    }

                    break; 
                }
                
                case mubea:  
                { 
                    Serial.print("MUBEA \n");  
                    wait(100);
                    create_task_mubea();
                    break; 
                }

                case exit:
                {
                    Serial.print("EXIT\n"); 

                    //TODO measure the consumption in this processes
                    ////there is something weird with the cons.of 5v boost

                    
                    //set an alarm for 1 min as test
                    //TODO : make a nice menu to ask before sleeping

                    //if(log_enabled)Serial.println("\n-- Setting recurrent ALARM FOR 1 min");
                    //set_recurrent_alarm(0,0,1,0);
                    
                    
                    
                    if (lvl_shftr_enabled) lvl_shftr_disable();

                    if(reg_5v_enabled) reg_5v_disable();
                    

                    Serial.println("Sleeping in 1 secs");

                    wait(1000);

                    Serial.print("\n BYE....\n");

                    wait(100);

                    //TODO IMPLEMENT ALL THIS LATER
                    ff1_q_low();

                    Serial.printf("\n ERROR -> YOU SHOULD NEVER SEE THIS (3V3_REG IS LOW) "); break;

                    break; 
                }
                
                default: {Serial.printf("\n ERROR -> VAL: ",cursor_position); break; }
            }

            task_devel_menu_i2c_release();
            vTaskDelete(NULL);//Delete itself (also will reset vars.)
            
        }

        //Select Next Option
        if (btn_1.is_pressed) //Next Pattern was selected
        {
            if(cursor_position>=120) cursor_position = 50;
            else cursor_position+=10;
            
            first_loop = true;
            wait_for_btn_1_release();
        }

        if(first_loop) //Refreshing Serial & OLED
        {
            if(log_enabled)
            {
                Serial.print("\nCurrent Selection -> "); 
                switch(cursor_position)
                {
                    case demo:  { Serial.print(" demo");  break; }
                    case first: { Serial.print(" first");  break; }
                    case park:  { Serial.print(" park");  break; }
                    case ride:  { Serial.print(" ride"); break; }
                    case ble:   { Serial.print(" ble");   break; }
                    case wifi:  { Serial.print(" wifi");  break; }
                    case mubea:  { Serial.print(" mubea");  break; }
                    case exit:  { Serial.print(" exit");  break; }
                    
                    default: {Serial.printf("\n ERROR -> VAL: ",cursor_position); break; }
                }

                
            }
           
            if(oled_enabled) oled_template_mode_devel(cursor_position);

            first_loop = false;
        }
        else wait(10);
    }
}


//End of Developer Menu ----------------------------------



TaskHandle_t first_time_config_handle = NULL;

void create_task_first_time_config() //once created it will automatically run
{
    if(log_enabled)Serial.print("\n-- Creating first time config --");

    xTaskCreate
    (
        first_time_config,           //Function Name (must be inf.loop)
        "first_time_config", //Logging Name (Just for ID)
        4096,                //Stack Size 
        NULL,                //Passing Parameters
        2,                   //Task Priority
        &first_time_config_handle
    );

    if(log_enabled)Serial.print("-- done --\n");
}

void first_time_config(void * parameters)
{
    /*
    STEPS FOR SETUP
    
    1-BLE connect (Blue Glow)
    3-WIFI Connect given the ccredentials (Green if OK , RED if not)
    4-Decide lock ccomb, imu active and light intensity, etc
    4-Save on NVS
    */










}

//TODO LATER MOVE ALL THIS TO BLE


void ble_init() 
{
  std::string ble_name = "ENGEL_V4.0"; 

  if(log_enabled)
  { 
    Serial.print("\nInitializing BLE with name -> ");
    Serial.print(String(ble_name.c_str()));
    Serial.println();
  }

  // Create the BLE Device
  BLEDevice::init(ble_name);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  //Defeine the Properties of the Characteristics  

  //HeartBeat
  pchar_heartbeat = pService->createCharacteristic
                    (
                      uuid_heartbeat,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );               

  //TEMP
  pchar_temp_val = pService->createCharacteristic
                    (
                      uuid_temp_val,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );   


  //LUX  
  pchar_lux_val = pService->createCharacteristic
                    (
                      uuid_lux_val,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );   


  // Listening Characteristics 
  pchar_rgb_pwm = pService->createCharacteristic
                    (
                     uuid_rgb_pwm,
                     BLECharacteristic::PROPERTY_READ | 
                     BLECharacteristic::PROPERTY_WRITE
                    );     

  //BLE Descriptors

  //For Heartbeat
  pDescr_heartbeat = new BLEDescriptor((uint16_t)0x2901);
  pDescr_heartbeat->setValue("HEARTBEAT");
  pchar_heartbeat->addDescriptor(pDescr_heartbeat);
  pBLE2902_heartbeat = new BLE2902();
  pBLE2902_heartbeat->setNotifications(true);
  pchar_heartbeat->addDescriptor(pBLE2902_heartbeat);

  //For Temp      
  pDescr_temp_val = new BLEDescriptor((uint16_t)0x2901);
  pDescr_temp_val->setValue("TEMP_PCB_in_C");
  pchar_temp_val->addDescriptor(pDescr_temp_val);
  pBLE2902_temp_val = new BLE2902();
  pBLE2902_temp_val->setNotifications(true);
  pchar_temp_val->addDescriptor(pBLE2902_temp_val);

  //For Lux      
  pDescr_lux_val = new BLEDescriptor((uint16_t)0x2901);
  pDescr_lux_val->setValue("LUX_VAL");
  pchar_lux_val->addDescriptor(pDescr_lux_val);
  pBLE2902_lux_val = new BLE2902();
  pBLE2902_lux_val->setNotifications(true);
  pchar_lux_val->addDescriptor(pBLE2902_lux_val);

  //For the Listening RGB_PWM  
  pchar_rgb_pwm->addDescriptor(new BLE2902());
  pchar_rgb_pwm->setCallbacks(new CharacteristicsCallBack());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  ble_initialized = true;

  
  Serial.print("\n---BLE INITIALIZED!\n");
  Serial.print("\n---WAITING FOR CONNECTION---\n");
}

void ble_stop_advertising()
{
    BLEDevice::stopAdvertising();//stop advertising  
    //BLEDevice::deinit(true);    
    ble_initialized= false;
    if(log_enabled)Serial.print("\n---BLE Stopped Advertising---\n");
}
 

//Used on previous fn so moved to .H
//TaskHandle_t ble_normal_handle = NULL;

void create_task_ble_normal() //once created it will automatically run
{
    if(log_enabled)Serial.print("\n-- Creating BLE normal  --");

    task_ble_normal_i2c_declare();

    xTaskCreate
    (
        ble_normal,           //Function Name (must be inf.loop)
        "ble_normal", //Logging Name (Just for ID)
        4096,                //Stack Size 
        NULL,                //Passing Parameters
        5,                   //Task Priority
        &ble_normal_handle
    );

    if(log_enabled)Serial.print("-- done --\n");
    
}

void task_ble_normal_i2c_declare()
{
    if(log_enabled)Serial.print("\ntask_ble_normal_i2c_declared\n");
    //This Taks will use the following I2C_Devs
    //imu_needed++;
    rgb_needed++;
    temp_needed++;
    lux_needed++;
    //rtc_needed++;
    //fuelgauge_needed++;
    oled_needed++;
}

void task_ble_normal_i2c_release()
{
    if(log_enabled)Serial.print("\ntask_ble_normal_i2c_released\n");
    //This Taks will release the following I2C_Devs
    
    //imu_needed--;
    rgb_needed--;
    temp_needed--;
    lux_needed--;
    //rtc_needed--;
    //fuelgauge_needed--;
    oled_needed--;
}

void ble_normal(void * parameters)
{
    //Here a mockup with the Android app from MIT
    //This is thew mold of the intention for the BLE

    //OUT with BTN2
    btn_2_interr_enable_on_press();


    if(oled_enabled) 
    {
        ble_oled_status = 0;//Init and Waiting for BLE CONNECTION
        refresh_oled_ble_normal();
    }

    const u_int8_t ble_tx_interval = 1000;
    unsigned long ble_tx_interval_counter = millis();
    
    while(1)
    {
        if(btn_2.is_pressed) //Gettinng Out
        {
            btn_2_interr_disable();

            if(log_enabled) Serial.print(" ----- Back to Menu ! \n ");   

            ble_stop_advertising();
            
            if(rgb_bypassed) rgb_bypassed=false;
            wait_for_btn_2_release(); //will reset the btn.pressed flag also

            oled_clear();

            create_task_devel_menu();

            task_ble_normal_i2c_release();
            vTaskDelete(NULL);//Delete itself
        }

        if(!ble_initialized) ble_init();

        else 
        {   // notify changed value
            if (deviceConnected) 
            {
                //Connected and Transmitting
                if(oled_enabled && ble_oled_status ==0 || ble_oled_status ==3)
                {
                    ble_oled_status=1;
                    refresh_oled_ble_normal();                        
                } 

                if(!connection_logged)
                {
                    if(log_enabled)Serial.print("\n----Connected to BLE---\n");
                    connection_logged = true;
                } 

                //Send ble data every ble_tx_interval ms
                if(millis() > ble_tx_interval_counter + ble_tx_interval )
                {
                    if(log_enabled)Serial.print("\nBLE -> TX : ");    

                    pchar_heartbeat->setValue(ble_beat);
                    pchar_heartbeat->notify();
                    if(log_enabled)Serial.printf("ble_beat:%d | ",ble_beat); 
                    ble_beat++;

                    pchar_temp_val->setValue(board_temp);
                    pchar_temp_val->notify();
                    if(log_enabled)Serial.printf("Temp:%d | ",board_temp);

                    pchar_lux_val->setValue(lux_val);
                    pchar_lux_val->notify();
                    if(log_enabled)Serial.printf("Lux:%d | ",lux_val);

                    ble_tx_interval_counter = millis();
                }

                //Refresh the status of the RX vars. if new info is RX
                if(new_ble_rgb_pwm_value)
                {
                    //2 -> Connected and Transceiving 
                    if(oled_enabled && ble_oled_status ==1) ble_oled_status=2;

                    if(log_enabled)Serial.print("\n\t\t\t\t\t\tRX : ");

                    if(log_enabled)Serial.printf("%d | ",ble_rgb_pwm);

                    //Reacting to RGB PWM Order from App  
                    if (ble_rgb_pwm > 0 )
                    { 

                        if(rgb_bypassed) 
                        {
                            rgb_bypassed = false;
                            rgb_running = true;
                            rgb_leds_enable();
                            wait(500);
                            Serial.print(" LEDS ON |");
                        }
                        
                        if(log_enabled)Serial.print("LEDS: ");
                        
                        if(log_enabled)Serial.printf("%d | \n",ble_rgb_pwm);

                        rgb_leds_on('r',ble_rgb_pwm);
                    }
                    else
                    {
                        //if its the only resource using the rgb_leds
                        if(rgb_needed == 1 && !rgb_bypassed)
                        {
                            rgb_bypassed = true;
                            rgb_leds_disable();
                            if(log_enabled)Serial.print("LEDS:OFF");
                        }
                    }
                    new_ble_rgb_pwm_value = false;
                }

                if(oled_enabled)refresh_oled_ble_normal();  
            }
            // disconnecting
            if (!deviceConnected && oldDeviceConnected) 
            {
                //3 -> Disconnected and Re-Advertising
                if(oled_enabled) 
                {
                    ble_oled_status = 3;
                    refresh_oled_ble_normal();
                }  

                if(log_enabled)Serial.println("Disconnected -> Advertising Again");
                wait(1000); // give the bluetooth stack the chance to get things ready

                pServer->startAdvertising(); // restart advertising
                oldDeviceConnected = deviceConnected;
                connection_logged = false; //Prepare to log connection

            }
            // connecting
            if (deviceConnected && !oldDeviceConnected) 
            {
                // do stuff here on connecting
                oldDeviceConnected = deviceConnected;
            }          
        }
        wait(10);
    }
}

//ADD DECRIPTORS FOR SLIDER

TaskHandle_t ble_first_time_config_handle = NULL;

void create_task_ble_first_time_config() //once created it will automatically run
{
    if(log_enabled)Serial.print("\n-- Creating BLE first time config --");

    xTaskCreate
    (
        ble_first_time_config,           //Function Name (must be inf.loop)
        "ble_first_time_config", //Logging Name (Just for ID)
        4096,                //Stack Size 
        NULL,                //Passing Parameters
        1,                   //Task Priority
        &ble_first_time_config_handle
    );

    if(log_enabled)Serial.print("-- done --\n");
}

void ble_first_time_config(void * parameters)
{
    

    

}

void non_critical_refresh(int print) // 0:none , 1:necessary , 2:verbose 
{
    //wait(non_critical_task_refresh_ms);

    //if(log_enabled) Serial.print("\n---Updating NON-Critical Parameters ---\n");
    
    fuel_gauge_update(print);// 0:none , 1:necessary , 2:verbose 

    temp_update(print);// 0:none , 1:necessary , 2:verbose 

    //if return  true when something changed
    get_gpio_inputs_status(print);// 0:none , 1:necessary , 2:verbose 
    //get_gpio_inputs_status(2);

    check_movement();

    if(gpio_exp_changed)
    {
        if( print > 1 ) Serial.println(" <-- GPIO_EXP Change ACK.\n");
        //TODO This is just a dummy , implement the correct later. 
        gpio_exp_changed = false;
    }

    if(log_enabled && print>1 )Serial.println();//To separate every iteration on verbose
    //if(log_enabled && non_critical_task_refresh_ms>1000 ) Serial.print(".");//To confirm Slow refresh mode
}
/*

TaskHandle_t non_critical_task_handle = NULL;

void create_task_non_critical_task() //once created it will automatically run
{
    if(log_enabled)Serial.print("\n-- Creating Non_Critical_Task --");

    xTaskCreate
    (
        non_critical_task,           //Function Name (must be inf.loop)
        "non_critical_task", //Logging Name (Just for ID)
        4096,                //Stack Size 
        NULL,                //Passing Parameters
        1,                   //Task Priority
        &non_critical_task_handle
    );

    if(log_enabled)Serial.print("-- done --\n");
}

void non_critical_task(void * parameters)
{
    //Every 10 seconds loop through this and refresh parameters
    while(1)
    {
        wait(non_critical_task_refresh_ms);

        if(log_enabled) Serial.print("\n---Updating NON-Critical Parameters ---\n");
        
        fuel_gauge_update(true);//bool to print

        temp_update(true);//bool to print

        //if return  true when something changed
        get_gpio_inputs_status(1);
        //get_gpio_inputs_status(2);

        if(gpio_exp_changed)
        {
            //TODO This is just a dummy , implement the correct later. 
            Serial.println("\n---- GPIO_EXP Change ACK.\n");
            gpio_exp_changed = false;
        }
        
        //Bat charging?

        if(oled_enabled) Serial.println();
    }

}
*/


//This is the ALARM while Parking
//Can be used on mqtt_mode, firebase_mode and firestore_mode 
TaskHandle_t task_parking_alarm_handle = NULL;

void create_task_parking_alarm() //once created it will automatically run
{
    if(log_enabled)Serial.print("\n-- Creating Parking Alarm  --");

    wait(100);

    task_parking_alarm_i2c_declare();

    //in case there is a double tap of the alarm it wont panic 
    wait(100);

    xTaskCreate
    (
        task_parking_alarm,  //Function Name (must be inf.loop)
        "parking_alarm_task",//Logging Name (Just for ID)
        10000,                //Stack Size 
        NULL,                //Passing Parameters
        2,                   //Task Priority (higjer than normal backend cycle so can override publishing)
        &task_parking_alarm_handle
    );
    
    wait(100);

    if(log_enabled)Serial.print("-- done --\n");
    
}

void task_parking_alarm_i2c_declare()
{
    wait(100);
    if(log_enabled)Serial.print("\n--parking_alarm_i2c_declared\n");

    wait(100);
    //This Taks will use the following I2C_Devs
    //imu_needed++;
    rgb_needed++;
    //temp_needed++;
    //lux_needed++;
    //rtc_needed++;
    //fuelgauge_needed++;
    oled_needed++;
}

void task_parking_alarm_i2c_release()
{
    wait(100);
    if(log_enabled)Serial.print("\n--parking_alarm_i2c_released\n");
    wait(100);

    //This Taks will release the following I2C_Devs
    
    //imu_needed--;
    rgb_needed--;
    //temp_needed--;
    //lux_needed--;
    //rtc_needed--;
    //fuelgauge_needed--;
    oled_needed--;
}

void task_parking_alarm(void * parameters)
{
    wait(100);
    //OUT with BTN1
    btn_1_interr_enable_on_press();

    bool first_loop = true;

    wait(100);

    //TODO MAKE THE oled_enabled statements on all codes
    if(oled_enabled)
    {
        //holding the token to advertise
        oled_token = oled_taken ;
        wait(100);
        if(log_enabled)Serial.println("\n---Sending to OLED---> "); 
        wait(100); 
        oled_parking_alarm_disclaimer();
        wait(3000);
        oled_token = oled_free ;
    }
    
    //0 first loop
    //1 notification
    //2 permanent
    int alarm_stage = 0;

    if(log_enabled)Serial.print("\n--- Park_Alarm Task Running ! ---");
    wait(100);

    while(1)
    {
        if(btn_1.is_pressed || !backend_parking_alarm_state) //Dismissing and Destroying Alarm
        { 

            buzzer_one_tone(1000,500,200,2);

            rgb_leds_blink_once('g',255,100);

            btn_1_interr_disable();
            
            wait(100);
            
            if(log_enabled)
            {
                Serial.print("\n ----- Alarm Dismissed ");//Sanity check with double if

                //Remote Dismiss (backend_parking_alarm_state was set to off while alarm is triggered)                                    
                if     (backend_config == backend_config_mqtt     && !backend_parking_alarm_state) Serial.print("via MQTT !     \n ");
                else if(backend_config == backend_config_firebase && !backend_parking_alarm_state) Serial.print("via Firebase ! \n ");

                if(btn_1.is_pressed) //Manual Dismiss
                {
                    Serial.print("via BTN_1 ! \n ");

                    //For FirebaseWe need to refresh the inputs manually to force an update on the DB
                    if(backend_config == backend_config_firebase)
                    {
                        //Already Deletings the flags here as we need to update asap
                        backend_parking_alarm_movement_detected= false;
                        backend_parking_alarm_triggered= false;
                        backend_parking_alarm_state = false;
                        //firebase_inputs_need_manual_refresh = true;
                        if(log_enabled)
                        {
                            Serial.printf("\n---alarm_dismissed forcing an input manual refresh on firebase ..");
                        } 
                    } 
                }       
            }
             
            oled_token = oled_taken;
            wait(100);
            if(log_enabled)Serial.println("\n---Sending to OLED---> "); 
            wait(100); 
            oled_parking_alarm_dismissed();   
            wait(2000);
            oled_token = oled_free;      

            //Returning LEDS to the previous state
            if(backend_led_status) rgb_leds_on(backend_led_color,backend_led_brightness);
            
            //WAITING FOR FIRESTORE TO FINISH IF NEEDED
            wait(3000);
                 
            wait_for_btn_1_release(); //will reset the btn.pressed flag also (that why it's after the log) 
            //force the update here because the release() will clear the btn_1.pressed flag 
            //on mqtt will bring problems if we don't redeclare here this flag
            backend_parking_alarm_state = false;
            backend_parking_alarm_movement_detected = false;
            backend_parking_alarm_triggered= false;
            wait(100);

            //Destroying the Task and releasing flag
            task_parking_alarm_i2c_release();
            wait(100);

            if(log_enabled)Serial.println("\n---ALARM TASK DESTROYED\n");  
            wait(1000);

            vTaskDelete(task_parking_alarm_handle);//Delete itself
            
        }

        else //Normal Operation 
        {   
            //TODO MAKE THE FIRST NOTIFICATION (movement_detected)

            //First Stage - Warning    
            if (movement_detected && alarm_stage == 0)
            {
                //not giving back the token until alarm is dismissed
                oled_token = oled_taken;
                wait(100);

                if(log_enabled)Serial.println("\n---Sending to OLED---> "); 
                wait(100); 

                oled_parking_alarm_movement_detected();
                wait(100);

                backend_parking_alarm_movement_detected = true;
                wait(100);

                rgb_leds_blink_once('r',255,500);

                buzzer_one_tone(1000,1000,10,1);

                //force immediate status update on MQTT
                switch(backend_config)
                {
                    case backend_config_mqtt:
                    {
                        //mqtt_publish(emergency,print_all);
                        break; 
                    } 
                    case backend_config_firebase:
                    {
                        //Dont send from here , it will be sent on the firebase kernel once the flag is raised  
                        //firebase_update_engel_main_output(firebase_out_park_alarm_movement_detected,firebase_log_mode_verbose);
                        break;
                    }
                }           
                
                alarm_stage = 1 ;

                //Wait here an compare after certain time if it is still moving
                wait(parking_alarm_warning_time_ms);  

                if(moving)
                {
                    alarm_stage = 2;
                    
                    backend_parking_alarm_triggered = true;
                    first_loop = true;
                    if(log_enabled)Serial.println("\n---MOVEMENT STILL DETECTED , ESCALATING---\n");      
                }
                else
                {
                    if(log_enabled)Serial.println("\n---Dismissing Warning and returning to normal guard---\n");  
                        
                    if(oled_enabled)
                    {
                        if(log_enabled)Serial.println("\n---Sending to OLED---> "); 
                        wait(100); 
                        oled_token = oled_taken;
                        oled_parking_alarm_disclaimer();
                        wait(1000);
                        if(log_enabled)Serial.println(" < --- Sent to OLED---");  
                        oled_token = oled_free; 
                    } 
                    
                    backend_parking_alarm_movement_detected = false;
                    
                    alarm_stage = 0;

                    switch(backend_config)
                    {
                        case backend_config_mqtt:
                        {
                            //mqtt_publish(emergency,print_all);
                            break; 
                        } 
                        case backend_config_firebase:
                        {
                            //Dont send from here , it will be sent on the firebase kernel once the flag is raised  
                            //firebase_update_engel_main_output(firebase_out_park_alarm_movement_detected,firebase_log_mode_verbose);
                            break;
                        }
                    }                                      
                }    
            }
            //Permanent Alarm
            else if (alarm_stage == 2)
            {
                if(log_enabled & first_loop)
                {
                    if(backend_parking_alarm_mode) Serial.print("\n\n--- LOUD ");
                    else                           Serial.print("\n\n--- SILENT ");
                    Serial.print("ALARM TRIGGERED! - BTN_1 press or Backend order to dismiss---\n\n");
                    
                    if(oled_enabled)
                    {
                        oled_token = oled_taken;
                        if(log_enabled)Serial.println("\n---Sending to OLED---> "); 
                        wait(100); 
                        oled_parking_alarm_triggered();

                    }

                    switch(backend_config)
                    {
                        case backend_config_mqtt:
                        {
                            //mqtt_publish(emergency,print_all);
                            break; 
                        } 
                        case backend_config_firebase:
                        {
                            //Dont send from here , it will be sent on the firebase kernel once the flag is raised  
                            //firebase_update_engel_main_output(firebase_out_park_alarm_movement_detected,firebase_log_mode_verbose);
                            break;
                        }
                    }
                    
                    first_loop = false;

                    wait(100);

                }

                if(backend_parking_alarm_mode)//FOR LOUD ALARM
                {
                    rgb_leds_blink_once('r',255,200);

                    rgb_leds_on('r',255);
                    
                    buzzer_one_tone(2730,200,0,1);
                    buzzer_one_tone(2000,200,0,1);

                    if(backend_parking_alarm_snooze) //old value is parking_alarm_snooze
                    {
                        if(log_enabled) Serial.printf("\n--- Parking Alarm Snoozed for %d seconds ---",parking_alarm_snooze_time_ms/1000);
                        rgb_leds_off();
                        buzzer_one_tone(2730,200,100,1);//little delay
                        buzzer_one_tone(2730,200,0,1);

                        oled_token = oled_taken;
                        if(log_enabled)Serial.println("\n---Sending to OLED---> "); 
                        wait(100); 
                        oled_parking_alarm_snoozed();

                        //Stuck here as the alarm is not allowed to do anything until the snooze is finished

                        unsigned long snooze_finished = millis() + parking_alarm_snooze_time_ms ;

                        while(millis() < snooze_finished )
                        {
                            wait(100);
                        }
                        
                        if(log_enabled) Serial.print("\n--- Parking Alarm Reactivated after Snooze --- ");
                        oled_token = oled_free; 

                        //Resetting Normal Alarm Parameters
                        alarm_stage = 0;
                        backend_parking_alarm_movement_detected = false;
                        backend_parking_alarm_triggered = false;

                        //Deactivating Snooze and forcing Input override on Database
                        //old one parking_alarm_snooze = false;
                        backend_parking_alarm_snooze = false;
                        //firebase_inputs_need_manual_refresh = true;  
                        if(log_enabled)
                        {
                            Serial.printf("\n---parking_alarm forcing an input manual refresh on firebase ..");
                        } 
                        buzzer_one_tone(100,100,1,1);
                    }
                }                
                
            }
            else wait(100);
        }
    }
}


//TASK GPS----------------------------------------------------

TaskHandle_t task_gps_handle = NULL;

void create_task_gps() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n-- creating task_gps --");

    task_gps_i2c_declare();

    xTaskCreate
    (
        task_gps,           //Function Name
        "task_gps",    //Logging Name
        4096,                //Stack Size
        NULL,                //Passing Parameters
        4,                   //Task Priority
        &task_gps_handle
    );

    task_gps_active = true;

    //Reseting after reinit of the task
    gps_initialized = false;

    if(log_enabled) Serial.print("-- done --\n");

}

void task_gps_i2c_declare()
{
    if(log_enabled)Serial.print("\ntask_gps_i2c_declared\n");
    //This Taks will use the following I2C_Devs

    //imu_needed++;
    //rgb_needed++;
    //temp_needed++;
    //lux_needed++;
    //rtc_needed++;
    //fuelgauge_needed++;
    oled_needed++;

    //DEFINING THIS TASK TIME CRITICAL TO REFRESH THE OLED ASAP
    //time_critical_tasks_running++;

}

void task_gps_i2c_release()
{
    if(log_enabled)Serial.print("\ngps_i2c_released\n");
    //This Taks will release the following I2C_Devs
    
    //imu_needed--;
    //rgb_needed--;
    //temp_needed--;
    //lux_needed--;
    //rtc_needed--;
    //fuelgauge_needed--;
    oled_needed--;

    //RELEASING THE TIME_CRITICAL FLAG TO ALLOW FOR NON-CRITICAL REFRESH
    //time_critical_tasks_running--;
}


void task_gps(void * parameters)
{ 
    if(log_enabled) Serial.print("\n\n------Starting Task GPS----");

    unsigned long timer = millis();

    //Show initially GPS

    //add later the poll nr and retry nr

    gps_initialized = gps_init();
  
    while(1)
    {
        if(gps_initialized)
        {
            timer = millis();

            //Serial.print("Polling GPS....");
            gps_poll();
     
            while ( millis() < timer  +  (gps_refresh_seconds*1000))
            {
                if(!task_gps_active)
                {
                    Serial.print("---\n task_gps terminated ----");
                    vTaskDelete(NULL); 
                }
                else wait(100);
            }
        }
        else //GPS Not Found
        {
            Serial.print("---\n GPS Module Not Found, retrying ----");
            gps_initialized = gps_init();

            timer = millis();

            while ( millis() < timer + (gps_not_found_retry_seconds*1000))
            {
                if(!task_gps_active)
                {
                    Serial.print("---\n task_gps terminated ----");
                    vTaskDelete(NULL); 
                }
                else wait(10);
            }
        } 
    }    
}

//TASK CAN-----------------------------------------------------

TaskHandle_t task_can_handle = NULL;

void create_task_can() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n-- creating task_can --");

    task_can_i2c_declare();

    xTaskCreate
    (
        task_can,            //Function Name
        "task_can",          //Logging Name
        4096,                //Stack Size
        NULL,                //Passing Parameters
        2,                  //Task Priority
        &task_can_handle
    );

    task_can_active = true;

    if(log_enabled) Serial.print("-- done --\n");

}

void task_can_i2c_declare()
{
    if(log_enabled)Serial.print("\ntask_can_i2c_declared\n");
    //This Taks will use the following I2C_Devs

    //imu_needed++;
    //rgb_needed++;
    //temp_needed++;
    //lux_needed++;
    //rtc_needed++;
    //fuelgauge_needed++;
    oled_needed++;

    //DEFINING THIS TASK TIME CRITICAL TO REFRESH THE OLED ASAP
    //time_critical_tasks_running++;

}

void task_can_i2c_release()
{
    if(log_enabled)Serial.print("\ncan_i2c_released\n");
    //This Taks will release the following I2C_Devs
    
    //imu_needed--;
    //rgb_needed--;
    //temp_needed--;
    //lux_needed--;
    //rtc_needed--;
    //fuelgauge_needed--;
    oled_needed--;

    //RELEASING THE TIME_CRITICAL FLAG TO ALLOW FOR NON-CRITICAL REFRESH
    //time_critical_tasks_running--;
}

bool can_activation_message_ack_received = false; 
//Depending on the vehicle the can activation wil be or not needed , todo later convert this into a macro with ifdef
bool can_activation_needed = false;

void task_can(void * parameters)
{ 
    if(log_enabled) Serial.print("\n\n------Starting Task Can----\n\n");
    //TODO later expand thi to macro
    if(vehicle_id == mubea_heavy_cargo) can_activation_needed = true;

    can_initialized = can_init(can_log_mode_moderate);
    

    unsigned long can_init_timer = millis();
    int can_init_retries = 0;

    //TODO , check if this is just a bypass or in reality no acknowledge message is expected from the vehicle
    bool can_activation_message_ack_bypassed = false;

    //This musst always suceed before going to the next step
    while(!can_initialized )
    {
        if(millis() > can_init_timer + 5000)
        {
            Serial.println("\n---Retrying Internal CAN Transceiver Initialization (HW)---\n");
            
            can_initialized = can_init(can_log_mode_moderate);
            
            if(!can_initialized)
            {
                can_init_retries++;
                can_init_timer = millis();
            }
            else
            {
                can_init_retries = 0;
                break;   
            } 

            if(can_init_retries > 4)
            {
                Serial.printf("CAN Transceiver Initialization (HW) failed after %d retries", can_init_retries + 1);

                //TODO here upload the faiure to firebase to force the task_can disabling                 

                Serial.printf("\n---Destroying Task CAN upon error");

                wait(1000);

                task_can_active = false;

                vTaskDelete(NULL);                
            }
        }
        else wait(100);
    }
    //If a started correctly we continue here    

    // This activates the CAN bus communication if an order to enable it is needed
    if (can_initialized && can_activation_needed)
    {
        can_activation_message_ack_received = can_send_activation_message(); 

        if(can_activation_message_ack_received)
        {
            if(log_enabled) Serial.print("\nCAN Activation Message Received , proceeding ! ----\n");
        }
        else
        {
            if(log_enabled) Serial.print("\nCAN Activation Message NOT Received , proceeding to see if we get CAN info anyways ! ----\n");
        }
    }

    unsigned long retry_timer = millis();
    int retry_counter = 0;
      
    while(1)
    {
        //Will retry up to 5 times every 5 seconds and then stop all retries and continue the loop to just listen 
        if(!can_activation_message_ack_received && millis() > retry_timer + 5000 && !can_activation_message_ack_bypassed)
        {
            if(log_enabled) Serial.printf("\n\n------ Resending CAN Activation Message , retry nr: %d ----\n\n", retry_counter + 1 );
            
            can_activation_message_ack_received = can_send_activation_message(); 

            if(can_activation_message_ack_received)
            {
                if(log_enabled) Serial.print("\n---CAN Activation Message Received , proceeding ! ----\n");
            }
            else
            {
                if(retry_counter < 5)
                {
                    if(log_enabled) Serial.print("\n---CAN Activation Message NOT Received , proceeding to see if we get CAN info anyways ! ----\n");
                    retry_counter++;
                    retry_timer = millis();
                }

                else
                {
                   if(log_enabled)
                   {
                        Serial.print("\n--- CAN Activation Message NOT Received after severall retries ----\n");
                        Serial.print("\n--- We will not interrupt the task but will send error 103 to Database ----\n");

                        //TODO here send to firebase the error codee

                        //errors_detected++;
                        //can_error_detected++;
                        //can_eror_code = 103;

                        can_activation_message_ack_bypassed = true;
                   } 
                }
            }            
        }        

        can_poll(can_log_mode_moderate);
        
        if(!task_can_active)
        {
            Serial.print("\n\n---task_can terminated ----\n\n");

            //always init while starting the task so not needed but good practice
            can_initialized = false;

            wait(100);

            vTaskDelete(NULL); 
        }
        else wait(10);
    }        
}

//TASK MUBEA-----------------------------------------------------

TaskHandle_t task_mubea_handle = NULL;

void create_task_mubea() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n-- creating task_mubea --");

    task_mubea_i2c_declare();

    xTaskCreate
    (
        task_mubea,           //Function Name
        "task_mubea",    //Logging Name
        4096,                //Stack Size
        NULL,                //Passing Parameters
        4,                   //Task Priority
        &task_mubea_handle
    );

    if(log_enabled) Serial.print("-- done --\n");

}

void task_mubea_i2c_declare()
{
    if(log_enabled)Serial.print("\ntask_mubea_i2c_declared\n");
    //This Taks will use the following I2C_Devs

    //imu_needed++;
    //rgb_needed++;
    //temp_needed++;
    //lux_needed++;
    //rtc_needed++;
    //fuelgauge_needed++;
    oled_needed++;

    //DEFINING THIS TASK TIME CRITICAL TO REFRESH THE OLED ASAP
    //time_critical_tasks_running++;

}

void task_mubea_i2c_release()
{
    if(log_enabled)Serial.print("\ntask_mubea_i2c_released\n");
    //This Taks will release the following I2C_Devs
    
    //imu_needed--;
    //rgb_needed--;
    //temp_needed--;
    //lux_needed--;
    //rtc_needed--;
    //fuelgauge_needed--;
    oled_needed--;

    //RELEASING THE TIME_CRITICAL FLAG TO ALLOW FOR NON-CRITICAL REFRESH
    //time_critical_tasks_running--;
}


void task_mubea(void * parameters)
{ 
    
    if(log_enabled) Serial.print("\n\n------Starting Task Mubea----");
    
    btn_1.number_of_push=0;
    btn_2.number_of_push=0;

    btn_1_interr_enable_on_press();
    btn_2_interr_enable_on_press();

    bool can_disabled_first_loop = true;
    bool gps_disabled_first_loop = true;

    //CAN Memory-Holders    
    bool task_can_was_active = false;
    bool task_gps_was_active = false;

    unsigned long firebase_time_holder = millis() + 1000;

    
    
    //check here if can and gps are running
    //if(can_enabled && task_can_handle ==NULL)create_task_can();
    //if(gps_enabled && task_gps_handle ==NULL)create_task_gps();


    Serial.print("Starting Mubea_Task");

    Serial.print("\n--- DEFAULT : GPS OLED MODE ---");   
    Serial.print("\n--- PRESS BTN_2 FOR ENABLING FIREBASE ---");   
    
    
    //Start the CAN and the GPS (NOW DONE ON CALLBACKS)
    //TO DO : DECIDE IF CHANGING TO PREVIOS MODE (UPPER ONE)
    //IN THE MEANTIME STARING ACN AND GPS MANUALLY

    if(can_enabled && !task_can_active) create_task_can();


    if(gps_enabled && !task_gps_active) create_task_gps();


    wait(100);

    //Always Starting with the GPS data so oled is setting the GPS template
    if(gps_initialized && gps_enabled) oled_gps_waiting_data();

    //Timer for interrupting waiting times
    unsigned long time_holder = millis();
    
    //After some time the Firebase will start uploading the data 

    //this is the oled_screen_counter for the imu_log
    int imu_log_screen_counter = 0 ;

    task_mubea_running = true;  
    
    while(1)
    {        
        if(task_mubea_running == false)
        {
            Serial.print("\n---Destroying Task MUBEA");
            vTaskDelete(NULL);
        } 

        // **Serial Input Handling**
        if (Serial.available() > 0) 
        {
            String input = Serial.readStringUntil('\n'); // Read input until newline
            int input_number = input.toInt(); // Convert input to integer

            if (input_number >= 0 && input_number <= 15) // Ensure valid range
            {
                btn_1.number_of_push = input_number; // Mimic button press
                Serial.printf("\n--- Button number set to: %d via Serial ---\n", btn_1.number_of_push);
                button_bypassed_by_terminal = true;
            }
            else
            {
                Serial.println("\n--- Invalid input. Enter a number between 0 and 15 ---");
            }
        }

        // 0 is looping on GPS 
        
        if(btn_1.number_of_push > 0) //DASHBOARD CHANGE
        {
            btn_1_interr_disable();

            if (btn_1.number_of_push == 1) // CHANGING TO GPS SPOOF
            {
                if(log_enabled) 
                {
                    Serial.print("\n\n--- CHANGING TO GPS SPOOFING ?---");  

                    oled_gps_spoof_question();
                }
                
                //oled_clear();

                //oled_needs_refresh = true;

                btn_1.number_of_push = 2;
            }

            //Do nothing on 2 as is looping on GPS_Spoofed

            if  (btn_1.number_of_push == 3) //Changing to CAN
            {
                if(log_enabled) 
                {
                    Serial.print("\n\n--- CHANGING TO CAN ---"); 

                    if(can_enabled)
                    {
                        //changing to smaller font as well   
                        oled_can_data_template(); 
                    }
                     
                    else
                    {
                        Serial.print("<--- CAN IS DISABLED , NOTHING WILL DISPLAY ---");  
                        oled_can_disabled();
                    }  
                }
                   
                //oled_clear();
                //oled_needs_refresh = true;  
                
                btn_1.number_of_push = 4;
            } 

            //Do nothing on 4 as is looping on CAN

            //We will make a special case to start the 
            //simulation , the graph or the blackbox on demand


            else if (btn_1.number_of_push == 5) // CHANGING TO IMU_LOG
            {
                if(log_enabled) 
                {
                    Serial.print("\n\n--- CHANGING TO IMU_LOG?");  

                    //changing to normal bigger font as well
                    oled_imu_log_question();                   
                }
                btn_1.number_of_push = 6;
            }

            //Do nothing on 6 as is looping on IMU_LOG

            else if (btn_1.number_of_push == 7) // CHANGING TO IMU_GRAPH
            {
                if(log_enabled) 
                {
                    Serial.print("\n\n--- CHANGING TO IMU_GRAPH?");  
                    //changing to normal bigger font as well
                    oled_imu_graph_question();  
                }               
                
                //Serial.print(" ... Ready ");

                btn_1.number_of_push = 8;
            }

            //Do nothing on 8 as is looping on IMU_GRAPH
            
            else if (btn_1.number_of_push == 9) // CHANGING TO IMU_SIMULATION
            {
                if(log_enabled) 
                {
                    Serial.print("\n\n--- CHANGING TO IMU_SIM?---");  
                    oled_imu_sim_question();
                    //In case the i2c_manager is not working 
                    //due to IMU_GRAPH previous activation 
                    if(imu_test_was_running)oled_refresh();
                }
                btn_1.number_of_push = 10;
            }

            //Do nothing on 10 as is looping on IMU_SIM

            else if (btn_1.number_of_push == 11) // CHANGING TO SD
            {                           
                if(log_enabled) 
                {
                    Serial.print("\n\n--- CHANGING TO SD_CARD_BLACK_BOX? ---");   
                    oled_black_box_question();
                    if(imu_test_was_running)oled_refresh();
                }
                btn_1.number_of_push = 12;
            }
            //Do nothing on 12 as is looping on SD_black_box

            else if (btn_1.number_of_push == 13) // CHANGING TO Firebase
            { 
                //TODO Decide about this one later
                //Killing SD TODO Check if we let it running or kill it
                task_sd_active = false; 

                if(log_enabled) 
                {
                    Serial.print("\n\n--- START FIREBASE? ---");   
                    oled_firebase_question();
                }

                btn_1.number_of_push = 14;
            }
            //Do nothing on 14 as is looping on Firebase

            else if (btn_1.number_of_push == 15) // CHANGING TO GPS
            { 
                if(log_enabled) 
                {
                    Serial.print("\n\n--- CHANGING TO GPS ---");   


                    if(gps_enabled)
                    {
                        oled_gps_bad_data_template();
                    }
                    else 
                    {
                        Serial.print("<--- GPS IS DISABLED , NOTHING WILL DISPLAY ---"); 
                        oled_gps_disabled(); 
                    }   

                    Serial.print("\n\n--- PRESS BTN_2 FOR ENABLING FIREBASE ---");   
                }
                
                //oled_clear();
                //oled_needs_refresh = true;

                //Resetting OLED flag for gps/can disable message
                can_disabled_first_loop = true;
                gps_disabled_first_loop = true;

                //TODO , think if we want to restart the whole ESP or restart the Firebase


                //TODO DELETE AFTER TEST
                task_firebase_active = false;

                //Returning to 0
                btn_1.number_of_push = 0;

               
            }

            wait_for_btn_1_release(); 

            btn_1_interr_enable_on_press();

            wait(10);
        }
        
        //LOGGER

        switch(btn_1.number_of_push)
        {
            case 0: //LOGGING GPS
            {
                if(btn_2.is_pressed) 
                {
                    Serial.print("\n--Waiting for BTN_2 to Be Released ---");
                }

                wait_for_btn_2_release();

                btn_2_interr_enable_on_press();
                
                
                if(gps_enabled) 
                { 
                    if(gps_locked)  //Good GPS Signal
                    {
                        //Here constantly polling
                        oled_gps_good_data_template();
                        if(gps_data_logging_needs_refresh)
                        {
                            Serial.println("\n--- GPS Data Changed ---");
                            gps_log_serial();
                            Serial.println("\n-----------------------");
                            //Whatever happens we here reset the flag
                            gps_data_logging_needs_refresh = false;
                        }
                    }
                    else 
                    {
                        //One time log
                        oled_gps_waiting_data(); 
                        if(gps_initialized) //Module Found but waiting for lock
                        {
                            if(gps_status != gps_status_on_not_fix)
                            {
                                oled_gps_bad_data_template();
                                Serial.println("\n ---Waiting for GPS 3D fix...\n ");
                                gps_status = gps_status_on_not_fix;        
                            }    
                            wait(100);                           
                        }

                        //One time log

                        else //Module not found
                        {
                            if(gps_status != gps_status_not_detected)
                            {
                                oled_gps_not_detected();
                                Serial.println("\n ---GPS Module not Detected...\n ");
                                gps_status = gps_status_not_detected;
                            }
                            wait(100);
                        }
                    }
                    
                    time_holder = millis();

                    wait(10);

                    while(1)
                    {
                        if( btn_1.number_of_push != 0                           || 
                            millis() > time_holder + (gps_refresh_seconds*1000) || 
                            !gps_enabled                                      /*||*/ 
                            /*btn_2.is_pressed TODO Bring Back this one later*/   ) 
                        {
                            break;
                        }
                        else wait(10);
                    }

                    //resetting flag
                    if(!gps_disabled_first_loop) gps_disabled_first_loop = true;
                }
                
                else //GPS IS DISABLED
                {
                    if(gps_disabled_first_loop)
                    {
                        gps_disabled_first_loop = false;
                        oled_gps_disabled();
                    }
                    wait(100);//To control loop speed
                }
            }
            break;

            case 2: //SPOOFING GPS 
            {
                //If there was no terminal remote order
                if(!button_bypassed_by_terminal)
                {
                    ///Here hang until btn_2 click 
                    //btn_1 click wil be handled by the interruption
                    while(!digitalRead(esp_btn_2_pin))
                    {
                        wait(10);
                        if(digitalRead(esp_btn_1_pin) || digitalRead(esp_btn_2_pin)) break;
                    }

                    if(digitalRead(esp_btn_1_pin))break;//Stop the cycle and get out


                    //If decided to run the spoof then continue here 

                }
                //Ending Bypassed mode
                else button_bypassed_by_terminal = false;
                

                if(gps_enabled)
                {
                    gps_spoof();
                    oled_gps_spoofed_data_template();
                    Serial.println("\n -- GPS SPOOFED Data -------- \n");
                    gps_log_serial();
                    Serial.println("\n ---------------------------- \n");

                    time_holder = millis();    

                    wait(10);

                    while(1)
                    {
                        if( btn_1.number_of_push != 2 || 
                            millis() > time_holder + (gps_refresh_seconds*1000) ||
                            !gps_enabled 
                            /*|| btn_2.is_pressed*/ ) 
                        {
                            break;
                        }
                        else wait(10);
                    }
                    //resetting flag
                    if(!gps_disabled_first_loop) gps_disabled_first_loop = true;

                }

                else //GPS IS DISABLED
                {
                    if(gps_disabled_first_loop)
                    {
                        Serial.print("<--- GPS IS DISABLED , NOTHING WILL DISPLAY ---"); 
                        oled_gps_disabled();
                        gps_disabled_first_loop = false;
                    }
                    wait(100);//To control loop speed
                }    
                
            }
            break;

            case 4: //LOGGING CAN
            {
                if(can_enabled)
                {
                    oled_can_data_template();
                    wait(100);
                    
                    //resetting flag
                    if(!can_disabled_first_loop) can_disabled_first_loop = true;
                }
                else
                {
                    if(can_disabled_first_loop)
                    {
                        can_disabled_first_loop = false;
                        oled_can_disabled();
                    }
                    wait(100);//To control loop speed
                }
                
            }
            break;

            case 6: //IMU_LOG
            {   
                
                //If there was no terminal remote order
                if(!button_bypassed_by_terminal)
                {
                    ///Here hang until btn_2 click 
                    //btn_1 click wil be handled by the interruption
                    while(!digitalRead(esp_btn_2_pin))
                    {
                        wait(10);
                        if(digitalRead(esp_btn_1_pin) || digitalRead(esp_btn_2_pin)) break;
                    }

                    if(digitalRead(esp_btn_1_pin))break;//Stop the cycle and get out

                    //If decided to run the log then continue here 
                }
                //Ending Bypassed mode
                else button_bypassed_by_terminal = false;


                Serial.print("--\nRunning IMU_LOG"); 
                oled_imu_log();

                //todo mayube in the new design create a separate i2c_channel for the imu so 
                //we do not have a problem reading it as fast as possible  
                
                recalibrate_imu_via_i2c_manager(imu_recalibrate_log_handler_moderate);
                
                while(!btn_1.is_pressed)
                {
                    wait(10);

                    if(imu_new_data)
                    {
                        switch(imu_log_screen_counter)
                        {
                            case 0: {oled_demo_imu_pos('x'); break;}
                            case 1: {oled_demo_imu_pos('y'); break;}
                            case 2: {oled_demo_imu_pos('z'); break;}

                            case 3: {oled_demo_imu_acc('x'); break;}
                            case 4: {oled_demo_imu_acc('y'); break;}
                            case 5: {oled_demo_imu_acc('z'); break;}
                        }  
                        imu_new_data = false;
                    }                                             

                    if(digitalRead(esp_btn_2_pin))
                    {
                        unsigned long int counter = millis();

                        while(digitalRead(esp_btn_2_pin))
                        {
                            if(millis() > counter + 5000)
                            {
                                recalibrate_imu_via_i2c_manager(imu_recalibrate_log_handler_moderate);

                                if(digitalRead(esp_btn_2_pin))
                                {
                                    Serial.print("-- Waiting for BTN_2 release --- ");
                                    while(digitalRead(esp_btn_2_pin))
                                    {
                                        wait(10);
                                    }
                                    Serial.print("< -- BTN_2 released! ");
                                    
                                }
                                break;
                            }   
                            else
                            {
                                //Normal OLED Screen Change 
                                wait(100);

                                if(!digitalRead(esp_btn_2_pin))
                                {
                                    if(imu_log_screen_counter == 5) imu_log_screen_counter = 0;
                                    else imu_log_screen_counter++;

                                    Serial.printf("--\nSwitching IMU_OLED_LOG_SCREEN nr: %d ",imu_log_screen_counter);                                         
                                }
                            } 
                        } 
                    }
                }                  
                
            }
            break;


            case 8: //IMU_GRAPH
            {

                //If there was no terminal remote order
                if(!button_bypassed_by_terminal)
                {
                    ///Here hang until btn_2 click 
                    //btn_1 click wil be handled by the interruption
                    while(!digitalRead(esp_btn_2_pin))
                    {
                        wait(10);
                        if(digitalRead(esp_btn_1_pin) || digitalRead(esp_btn_2_pin)) break;
                    }

                    if(digitalRead(esp_btn_1_pin))break;//Stop the cycle and get out

                    //If decided to run the graph then continue here 

                }
                //Ending Bypassed mode
                else button_bypassed_by_terminal = false;


                //to indicate that we killed the other tasks and we need to reenable them

                Serial.print("--\nRunning IMU_GRAPH"); 
                oled_imu_graph();

                imu_test_was_running = true;

                if(task_can_active)
                {
                    task_can_was_active = true;
                    //Destroying task_can;
                    can_enabled = false;
                    wait(100);
                } 
                
                if(task_gps_active)
                {
                    task_gps_was_active = true;
                    //Destroying task_gps;
                    gps_enabled = false;
                    wait(100);
                } 
                
                //Firebase will always be active at this point so just kill it
                if(task_firebase_active)
                {
                    task_firebase_active = false;
                    wait(100);
                } 

                //Destroying I2C Task Manager
                if(i2c_manager_running == true) 
                {
                    i2c_manager_running = false;
                    wait(1000);
                }

                Serial.print(" ... Ready ");

                ///Init IMU and set oled to permanent log
                if(!imu_initialized)imu_init();

                //All all is deativated we just loop here into the imu_sim
                oled_clear();
                oled_imu_graph();
                oled_refresh();

                while(!btn_1.is_pressed)
                {
                    imu_graph_demo();

                    if(digitalRead(esp_btn_2_pin))
                    {
                        Serial.print("\n -- Resetting / Recalibrating IMU.....");
                        imu_init();
                        Serial.print(" ... IMU Recalibrated!");

                        while(digitalRead(esp_btn_2_pin))
                        {
                            wait(100);
                        }
                    }
                } 
                                 
                
            }
            break;

            case 10: //IMU_SIM 
            {

                //If there was no terminal remote order
                if(!button_bypassed_by_terminal)
                {
                    ///Here hang until btn_2 click 
                    //btn_1 click wil be handled by the interruption
                    while(!digitalRead(esp_btn_2_pin))
                    {
                        wait(10);
                        if(digitalRead(esp_btn_1_pin) || digitalRead(esp_btn_2_pin)) break;
                    }

                    if(digitalRead(esp_btn_1_pin))break; //Stop the cycle and get out 


                    while (digitalRead(esp_btn_2_pin)) //wait for Release
                    {
                        wait(10);
                    }

                    //If decided to run the graph then continue here 
                }
                //Ending Bypassed mode
                else button_bypassed_by_terminal = false;



                
                Serial.print("--\nRunning IMU_SIM"); 
                oled_imu_sim();
                //to indicate that we kiled other tasks and we need to reenable them
                imu_test_was_running = true;

                if(task_can_active)
                {
                    task_can_was_active = true;
                    //Destroying task_can;
                    can_enabled = false;
                    wait(100);
                } 
                
                if(task_gps_active)
                {
                    task_gps_was_active = true;
                    //Destroying task_gps;
                    gps_enabled = false;
                    wait(100);
                } 
                                    
                //Firebase will always be active at this point so just kill it
                if(task_firebase_active)
                {
                    task_firebase_active = false;
                    wait(100);
                } 

                //Destroying I2C Task Manager
                if(i2c_manager_running == true) 
                {
                    i2c_manager_running = false;
                    wait(1000);
                }

                Serial.print(" ... Ready ");

                //Init IMU and set oled to permanent log
                if(!imu_initialized)imu_init();

                //All all is deativated we just loop here into the imu_sim
                oled_clear();
                oled_imu_sim();
                oled_refresh();

                while(!btn_1.is_pressed)
                {
                    imu_teapot_demo();
                    if(digitalRead(esp_btn_2_pin))
                    {
                        Serial.print("\n -- Resetting / Recalibrating IMU.....");
                        imu_init();
                        Serial.print(" ... Done ");

                        while(digitalRead(esp_btn_2_pin))
                        {
                            wait(100);
                        }
                    }
                }                  
                
            }
            break;

            case 12: //SD BLACK_BOX (using mutex)
            {                
                //manually add and disable the interruption
                btn_2_interr_enable_on_press();

                //At this point we have to bring back the others tasks 
                //even if we don't start the blacbox
                if(imu_test_was_running)
                {
                    //Reactivating I2C_Task_Manager
                    //TODO here put the guard for the TaskHandle check 
                    //or somehow check if its running before reinitializing 
                    create_task_i2c_manager();

                    if(task_can_was_active)
                    {
                        can_enabled = true;
                        create_task_can();
                        wait(100);
                    } 
                    
                    if(task_gps_was_active) 
                    {
                        gps_enabled = true;
                        create_task_gps();
                        wait(100);
                    }

                    imu_test_was_running = false;
                }


                //If there was no terminal remote order
                if(!button_bypassed_by_terminal)
                {
                    //Serial.print("\n WAITING FOR SD BLACK_BOX ANSWER ... ");

                    ///Here hang until btn_2 click 
                    //btn_1 click wil be handled by the interruption
                    while(!btn_2.is_pressed && !task_sd_active)
                    {
                        wait(10);
                        if(digitalRead(esp_btn_1_pin) || btn_2.is_pressed) break;
                    }

                    if(digitalRead(esp_btn_1_pin))
                    {
                        oled_clear();
                        break;//Stop the cycle and get out as the option changed
                    }

                }
                //Ending Bypassed mode
                else button_bypassed_by_terminal = false;

                if(black_box_enabled && !task_sd_active) 
                {     
                    oled_sd_running_mode_black_box();                   
                    create_task_sd();
                    wait(1000);
                }
                    
                //Here loop the oled every 5 seconds while running the task
                if(black_box_enabled && task_sd_active)
                {   
                    if(btn_2.is_pressed)
                    {
                        //TODO later add the separation between button and firebase
                        if(oled_dev_screen_nr > oled_dev_screen_nr_max ) oled_dev_screen_nr = 1;
                        else oled_dev_screen_nr++;

                        Serial.printf("\n\n--Switching to oled_dev_screen_nr nr: %d -- \n\n",oled_dev_screen_nr);
                        //guard already inside (preventing bouncing or looping if btn_2 is stil hold)

                        wait_for_btn_2_release();
                    }
                                        
                    //Here use the global_var_mutex as we are accesing the shared resources
                    if (xSemaphoreTake(global_vars_mutex, portMAX_DELAY) == pdTRUE)
                    {
                        oled_dev_info(oled_dev_screen_nr);

                        xSemaphoreGive(global_vars_mutex); // Release the mutex
                        //Guarantee passing the mutex and waiting for next free spot
                        wait(black_box_oled_refresh_time);
                    }
                }

                if(!black_box_enabled)
                {
                    Serial.print("\n--- BLACK_BOX NOT ENABLED via Firebase ---");
                    wait(1000);
                    Serial.print("\n--- This is a demo so we will spoof the activation here ---");
                    black_box_enabled = true;
                    button_bypassed_by_terminal = true;
                }
                wait(10);    
                
            }
            break;

            case 14: //FIREBASE_START?
            {
                //If there was no terminal remote order
                if(!button_bypassed_by_terminal)
                {
                    ///Here hang until btn_2 click 
                    //btn_1 click wil be handled by the interruption
                    while(!digitalRead(esp_btn_2_pin))
                    {
                        wait(10);
                        if(digitalRead(esp_btn_1_pin) || digitalRead(esp_btn_2_pin)) break;
                    }

                    if(digitalRead(esp_btn_1_pin))break; //Stop the cycle and get out 


                    while (digitalRead(esp_btn_2_pin)) //wait for Release
                    {
                        wait(10);
                    }

                    //If decided to run firebase continue here 
                }
                //Ending Bypassed mode
                else button_bypassed_by_terminal = false;
                

                while(!digitalRead(esp_btn_1_pin))
                {
                    if(!task_firebase_active)    
                    { 
                        Serial.print("--\nRunning Firebase Task Initialization Order"); 
                        oled_firebase_running();

                        wait(100);

                        //Start the Firebase to upload the values
                        create_task_firebase();
                        wait(1000);
                    }
                    wait(100); 
                    //Kill mubea on demand from within firebase
                    if(task_mubea_running == false)
                    {
                        Serial.print("\n---Destroying Task MUBEA---\n");
                        vTaskDelete(NULL);
                    }                                 
                }
            }
            break;
        }   
        wait(10); //To control loop speed
    }    
}

//MAKE THE ONES FOR ENABLING OR DISABLING WIFI , LTE , ETC

//FULL_START
    //RUNNING ALL IN AUTOMATIC
    //CONFIG_MENU 
    //BLBX:ON
    //LTE:ON
    //WIFI:ON
    //GPS:ON
    //CAN:ON
    //IMU:ON
//TEST_MENU
    //RUN PIECES
    //THE ONES SHOWN HERE 
