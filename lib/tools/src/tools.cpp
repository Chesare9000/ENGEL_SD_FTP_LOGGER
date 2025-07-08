
//Generic Deps.
#include <Arduino.h>

//Custom Deps.
#include <vars.h>
#include <tools.h>
#include <serial.h>
#include <oled.h>
#include <i2c.h>
#include <rgb.h>
#include <gpio_exp.h>
#include <temp.h>
#include <lux.h>
#include <imu.h>
#include <rtc.h>
#include <interrupts.h>
#include <demos.h>
#include <tasks.h>
#include <charger.h>
#include <fuel_gauge.h>
#include <nvs.h>
#include <sd.h>
#include <time.h>
#include <tools.h>

//TODO in Train , save the shutdown_reason to nvs and firebase



#define mov_switch_check_max_iterations 3
int mov_switch_check_counter = 0;

uint32_t esp_id = 0;

String hw_variant_string = "";
String backend_led_color_string = "red"; //default

int minutes_without_moving = 0; //Used to count the minutes without movement

//minutes without moving before sleep
int mwm_before_sleep_default = 30;
int mwm_before_sleep = mwm_before_sleep_default;

int minute_counter = 0;

int ms_to_s = 1000;

int one_minute_in_ms = 60 * ms_to_s ; //A minute in milliseconds

//TODO MAKE THIS A BOOL and exit with a test resut
void init_all()
{
  //Absolute first fn. in case we change i2c or serial pins
  get_hw_version(); 

  //Warning! Order Matters !
  if(log_enabled)serial_init();

  nvs_retrieve_all(nvs_log_mode_verbose);

  get_esp_id(); //TODO: check if NVS handshake with ESP_ID

  i2c_init(); 

  //TODO Throw a flag here if something is missing and 
  //after retry with same mistake exit with init_all with error; 
  if(log_enabled) i2c_scanner_with_names();

  gpio_exp_init();

  get_hw_variant();

  get_hw_version(); //Re-Loop just to print

  update_hw_variant_string();

  get_debug_mode();

  //FIRST HW_VARIANT AND HW_VERSION, THEN DEFINE GPIOS

  gpios_init();

  interrupts_init();

  lux_init();

  rtc_init();
  
  if(oled_enabled)oled_init(); 

  rgb_leds_init();
  
  fuel_gauge_init();

  temp_init();


  built_in_led_blink_once(500); //To indicate End of Setup

  global_vars_mutex_init();

  sd_init(); //This Updates the SD Values as well

  //TODO think if initialize all others as well? IMU , TEMP , RTC ,etc.

}

void gpios_init() // Setup and Init all ESP_GPIO pins
{

  //CONSTANT GPIOS defined on vars.h

  //GPIOs based on hw_version----------------------------

  //TO DO , BRING BACK IN HW THE LORA RESET TO 14

  //TO DO ATTENTION !! DELETE THIS LATER , IS JUST FOR TESTING V4 BUG ON LORA
  hw_version = 5;

  if(hw_version > 3) //NEW_CONFIG
  {
    if(log_enabled) Serial.println("\n--- Using New Config (>V3) for GPIOs ---");
    esp_buzzer_pin = 33 ;
    esp_int_gpio_exp_pin = 39 ;//Added on V4
    esp_reg_5v_en_pin = 32 ;   // Moved from EXP_P04 on >V4 
    
    //lora_reset was removed on V4 but brought back on V5
    //to be compatible with dependency requirements

    if(hw_version > 4 )
    {
      esp_lora_reset_pin = 14 ;  //<- Brought back on V5
    }
  }
  else //OLD_CONFIG
  {
    if(log_enabled) Serial.println("\n--- Using Old Config (<=V3) for GPIOs ---");
    esp_buzzer_pin = 15 ;      //<- Moved to 33 on V4
    esp_lora_vcc_en_pin = 32 ; //<- Moved to FF2 on V4
    esp_nrf_reset_pin = 33 ;   //<- Moved to EXP_P13 on V4
    esp_lora_reset_pin = 14 ;  //<- Moved to EXP_P11 on V4 and brought back on V5
    esp_int_charger_pin = 36 ; //<- Moved to EXP_P05 on V4
  }

  //CONFIGURING GPIOS --------------------------------------------

  //Vars just for internal debugging
  int counter = 0;
  bool print_log = true;

  if(!gpios_initialized)
  {
    if(print_log) Serial.printf("\nInit. esp_btn_1_pin to %d",esp_btn_1_pin);
    pinMode(esp_btn_1_pin, INPUT);
    if(print_log) Serial.printf("\nInit. esp_btn_2_pin to %d",esp_btn_2_pin);
    pinMode(esp_btn_2_pin, INPUT);
    if(print_log) Serial.printf("\nInit. esp_imu_int_pin to %d",esp_imu_int_pin);
    pinMode(esp_imu_int_pin, INPUT);
    if(print_log) Serial.printf("\nInit. esp_lora_DIO_0_pin to %d",esp_lora_DIO_0_pin);
    pinMode(esp_lora_DIO_0_pin, INPUT);
    if(print_log) Serial.printf("\nInit. esp_mov_switch_pin to %d",esp_mov_switch_pin);  
    pinMode(esp_mov_switch_pin, INPUT);
    if(print_log) Serial.printf("\nInit. hw_version_pin to %d",hw_version_pin); 
    pinMode(hw_version_pin, INPUT);
    

    //INITIALIZING BUZZER (TODO maybe set it as FN.)
    if(print_log) Serial.printf("\nInit. esp_buzzer_pin to %d",esp_buzzer_pin);
    pinMode(esp_buzzer_pin, OUTPUT );
    //digitalWrite(esp_buzzer_pin, LOW); 
    
    const int buzzer_channel = 0;
    const int buzzer_resolution = 8; // 8-bit PWM

    if(print_log) Serial.printf(" <- Attaching esp_buzzer_pin to channel %d",buzzer_channel);
    //TODO chek if necessary : to init the ledcWritetone
    ledcSetup(buzzer_channel, 1000, buzzer_resolution); // Initial frequency 1000 Hz
    ledcAttachPin(esp_buzzer_pin,buzzer_channel);// buzzer_gpio_pin, channel


    if(print_log) Serial.printf("\nInit. esp_rgb_data_pin to %d",esp_rgb_data_pin);
    pinMode(esp_rgb_data_pin, OUTPUT );
    digitalWrite(esp_rgb_data_pin, LOW);   
    
    if(print_log) Serial.printf("\nInit. esp_built_in_led_pin to %d",esp_built_in_led_pin);
    pinMode(esp_built_in_led_pin, OUTPUT );
    digitalWrite(esp_built_in_led_pin, LOW); 

    //CONFIG AND INITIAL STATE ON ESP_V_* GPIO PINS HANDLED BY SPI DEFAULTS
  
    if (hw_version > 3) //current_config
    {
      if(print_log) Serial.printf("\nInit. V4+ PIN esp_reg_5v_en_pin to %d",esp_reg_5v_en_pin);
      pinMode(esp_reg_5v_en_pin, OUTPUT ); //<- Moved from EXP_P04 on >V4
      digitalWrite(esp_reg_5v_en_pin, LOW);  

      if(print_log) Serial.printf("\nInit. V4+ PIN esp_int_gpio_exp_pin to %d",esp_int_gpio_exp_pin);
      pinMode(esp_int_gpio_exp_pin, INPUT ); //<- Added on V4
    }
      
    else //old config
    {
      if(print_log) Serial.printf("\nInit. OLD_GPIO esp_lora_vcc_en_pin to %d",esp_lora_vcc_en_pin);
      pinMode(esp_lora_vcc_en_pin, OUTPUT );//<- Moved to FF2 on V4
      digitalWrite(esp_lora_vcc_en_pin, LOW); 

      if(print_log) Serial.printf("\nInit. OLD_GPIO esp_nrf_reset_pin to %d",esp_nrf_reset_pin);
      pinMode(esp_nrf_reset_pin, OUTPUT ); //<- Moved to EXP_P13 on V4
      digitalWrite(esp_nrf_reset_pin, LOW); 

      if(print_log) Serial.printf("\nInit. OLD_GPIO esp_lora_reset_pin to %d",esp_lora_reset_pin);
      pinMode(esp_lora_reset_pin , OUTPUT ); //<- Moved to EXP_P11 on V4
      digitalWrite(esp_lora_reset_pin, LOW); 

      if(print_log) Serial.printf("\nInit. OLD_GPIO esp_int_charger_pin to %d",esp_int_charger_pin);
      pinMode(esp_int_charger_pin, INPUT); //<- Moved to EXP_P05 on V4

    }
    //pinMode(lvl_shifter_en, OUTPUT );
    //digitalWrite(lvl_shifter_en, LOW); 
    
    gpios_initialized =  true;
    if(log_enabled)Serial.println("\n--- GPIOS Initialized ---- ");

  }
  else
  {
    if(log_enabled)Serial.println("\n--ERROR ON INIT_GPIOS() -> GPIOS already Initialized! ---\n");
  }
}



//Movement Switch

//This is a simple fn to trigger is the mov sensor is active
//ON V4 the mov sens is not correctly wired as is NC and will always trigger the or gate
//ON V5 it was corrected so this fn. must be deprecated and a new fn that includes the correct
///configuration must be implemented 
//

//The idea is that the mov_switch will be LOW and when there is movement it will be high on the OR GATE
//BUT the ESP32 wil have inverted logic , meaning movement on LOW and still on HIGH

//TO DO : CHANGE THIS ON V5 that will be ACTIVE HIGH
void check_movement() //This is just for the ESP Pin attached to the Movement Switch 
{
  if(movement_detected && moving)//Movement triggered
  {
    if(log_enabled)Serial.print("\n---Moving !");
    //disable the interruption to avoid spamming
    mov_switch_interrupt_disable();
    movement_detected = false;    
  }

  if(!movement_detected && moving) //MOVEMENT CHECK
  {
    //Serial.printf("\nSWITCH STATE: %d ",digitalRead(esp_mov_switch_pin));

    if(digitalRead(esp_mov_switch_pin)) //not moving
    {
      mov_switch_check_counter++; 
    }
    else mov_switch_check_counter = 0; //still moving
    
    //to confirm that we are not moving we first need 10 continuous highs

    if (mov_switch_check_counter > mov_switch_check_max_iterations )
    {
      moving = false;
      if(log_enabled)Serial.print(" ---Movement Stopped---");
      mov_switch_check_counter = 0;
      //reenable interruption to get triggers again
      mov_switch_interrupt_enable();      
    } 
  }
  //If no movement detected and not moving do nothing
}

//Replace Delay statement with this little tool
void wait(int time_in_ms)
{
  vTaskDelay( (time_in_ms) / portTICK_PERIOD_MS );
} 

//Little function (not a task)
void built_in_led_blink_once(int time_in_ms) //time in milliseconds of the blink
{
  digitalWrite(esp_built_in_led_pin, HIGH);   // turn the LED on (HIGH is the voltage level)
  wait(time_in_ms);
  digitalWrite(esp_built_in_led_pin, LOW);   // turn the LED on (HIGH is the voltage level)  
}
void wait_for_btn_1_release()
{
  while(btn_1.is_pressed) //wait for releases
  {
    if(!digitalRead(btn_1.pin)) //Button Released
    {
        wait(50); //debouncer
        btn_1.is_pressed = false;
    }
    else wait(10);          
  }
}

void wait_for_btn_2_release()
{
  while(btn_2.is_pressed) //wait for releases
  {
    if(!digitalRead(btn_2.pin)) //Button Released
    {
        wait(50); //debouncer
        btn_2.is_pressed = false;
    }
    else wait(10);          
  }
}

void get_hw_version()
{
  float adc_val = 0.0f;
  float volts = 0.0f;
  float volts_via_millivolts = 0.0f;

  adc_val = analogRead(hw_version_pin); // 12bits ADC (0 – 4095) 
  volts = map(adc_val,0.0f,4095.0f,0.0f,3300.0f)/1000.0f; // Converting to (0 - 3.3) Volts 

  volts_via_millivolts = analogReadMilliVolts(hw_version_pin) / 1000.0f;
  
  if(log_enabled)
  {
    Serial.print("\n--- Getting HW Version---");
    Serial.printf("\nADC Count (12bits) : %f ",adc_val); //(0 – 4095)
    Serial.printf("\nVoltage Measured: %.2f V ",volts); //(0 – 3.3)
    Serial.printf("\nTest for Millivolts measurement: %.2f",volts_via_millivolts);
  }

  //To Identify the Version we use a voltage divider
  //R1 set the Version while R2 is fixed on 100k

  //ESP32 ADC is non linear, so we need big tolerances:

  //OLD versions of this board wont have this functionality

  //V4 -> R1 = 100k : Ideally 1.65V on hw_version_pin   
  if(volts >= 1.4 && volts < 2)
  {
    hw_version = 4;
  }
  //V5 -> R1 = 200k : Ideally 1.1V on hw_version_pin   
  else if(volts >= 0.9 && volts < 1.4 )
  {
    hw_version = 5;
  }
  //(OLD CONFIG DEFAULT) NO RESISTANCE DETECTED
  else if (volts >= 2) 
  {
    if(log_enabled) 
    {
      hw_version = 3;
      Serial.print("\n\n--- RESISTANCE NOT DETECTED ---");
      Serial.print("\n\n--- DEFAULTING TO V3 (OLD) ---");
    }
  }
  //LOW VOLTAGE : ERROR (FALSE OR DEFECTIVE RESISTOR)
  else
  {
    if(log_enabled) 
    {
      hw_version = -1;
      Serial.print("\n\n--- ERROR : VOLTAGE TOO LOW ---");
      Serial.print("\n\n--- CHECK HW RESISTORS ---");
      //TODO : MAKE A BOOL TO STOP ALL PROCESSES AND IDLE
      //IF AN ERROR LIKE THIS IS FOUND  
    }
  }

  if(log_enabled) //PIN not available or not found
  {
    if(log_enabled) Serial.printf("\n\n--- HW VERSION : %d ---", hw_version);
  }
}


//This will extract the unique id for this esp32 
void get_esp_id()
{ 
  for (int i = 0; i < 17; i = i + 8) 
  {
    esp_id |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }

  Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("This chip has %d cores\n", ESP.getChipCores());
  //Later Enclose this
  Serial.print("Chip ID: ");
  Serial.println(esp_id);
}




void update_hw_variant_string()
{
  //converting to string for better readability but in kernel managed as enum
  //This var will be visible outside the fn as is used for firestore later on
  
  switch (hw_variant)
  {
      case hw_variant_lite   : hw_variant_string = "lite";    break;
      case hw_variant_lora   : hw_variant_string = "lora";    break;
      case hw_variant_lte_gps: hw_variant_string = "lte+gps"; break;
      case hw_variant_pro    : hw_variant_string = "pro";     break;
      case hw_variant_devel  : hw_variant_string = "devel";   break;

      default                : hw_variant_string = "N/A";     break;
  }
}

void update_backend_led_color_string()
{
  //converting to string for better readability but in kernel managed as enum
  //This var will be visible outside the fn as is used for firestore later on
  
  switch (backend_led_color)
  {
      case 'r'   : backend_led_color_string = "red";   break;
      case 'g'   : backend_led_color_string = "green"; break;
      case 'b'   : backend_led_color_string = "blue";  break;
      case 'w'   : backend_led_color_string = "white"; break;
      
      default    : backend_led_color_string = "N/A";   break;
  }


}

//MUTEX TO PROTECT GLOBAL VARS 
SemaphoreHandle_t global_vars_mutex;

//Mutex to protect global vars
void global_vars_mutex_init()
{
  //Create a mutex to protect global variables
  if(log_enabled)Serial.println("\n--Creating a mutex to protect global variables--");

  global_vars_mutex = xSemaphoreCreateMutex();

  if (global_vars_mutex == NULL)
  {
    if(log_enabled)Serial.println("Failed to create global_vars_mutex!");
    while (1); // Halt execution if mutex creation fails
  }
  else 
  {
    if(log_enabled)Serial.println("\n--global_vars_mutex created!--");
  }
}

//minutes without reset
int mwr = 0;
int current_mwr = 0; 


//Quickly updates the minutes without reset
int get_mwr(int mwr_log_mode)
{
  if(millis() > current_mwr + one_minute_in_ms) // +1 minute
  {
    current_mwr = millis();
    mwr++;
    //mwr = int( millis() / one_minute_in_ms );

    if(log_enabled && mwr_log_mode > mwr_log_mode_silent)
    {
      Serial.printf("\n---minutes without reset: %d ---",mwr);
    }
  } 
  return mwr;
}


//Movement Monitor
//Will check the state of the flag on power saving and adjust accordingly

//TASK MOVEMENT_MONITOR-----------------------------------------------------

bool task_movement_monitor_active = false;

TaskHandle_t task_movement_monitor_handle = NULL;

void create_task_movement_monitor() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n-- creating task_movement_monitor --");

    task_movement_monitor_i2c_declare();

    xTaskCreate
    (
        task_movement_monitor,   //Function Name
        "task_movement_monitor", //Logging Name
        2048,                    //Stack Size
        NULL,                   //Passing Parameters
        2,                     //Task Priority
        &task_movement_monitor_handle
    );

    task_movement_monitor_active = true;

    if(log_enabled) Serial.print("-- done --\n");

}

void task_movement_monitor_i2c_declare()
{
    if(log_enabled)Serial.print("\n---task_movement_monitor_i2c_declared---\n");
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

void task_movement_monitor_i2c_release()
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



void movement_monitor_poll(int movement_monitor_log_mode)
{
  if ((movement_detected || moving) && minutes_without_moving > 0)
  {
    minutes_without_moving = 0;

    if(log_enabled && movement_monitor_log_mode > movement_monitor_log_mode_silent)
    {
      Serial.print("\n--- Resetting minutes without moving---");
    }
    minute_counter = millis();   
  }

  else if(!movement_detected && !moving && millis() > minute_counter + one_minute_in_ms )
  {
    minute_counter = millis(); 
    
    minutes_without_moving++;
    
    if(log_enabled && movement_monitor_log_mode > movement_monitor_log_mode_silent)
    {
      Serial.printf("\n---minutes without moving: %d ---",minutes_without_moving);
    }
  }
}

void task_movement_monitor(void * parameters)
{ 
    if(log_enabled) Serial.print("\n\n------Starting Task movement_monitor----");

    while(1)
    {
        if(!task_movement_monitor_active)
        {
            Serial.print("---\n task_movement_monitor manually terminated ----");
            vTaskDelete(NULL); 
        }

        else if(power_mode != power_mode_saving)
        {
          Serial.print("---\n task_movement_monitor terminated due to change on power_mode ----");
          vTaskDelete(NULL); 
        }

        else
        {
          movement_monitor_poll(movement_monitor_log_mode_moderate);
          wait(100);
        } 
    }        
}
