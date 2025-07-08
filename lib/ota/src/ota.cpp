#include <Arduino.h>
#include <vars.h>
#include <tools.h>
#include <ota.h>
#include <buzzer.h>
#include <oled.h>
#include <wifi.h>


bool ota_enabled_default = false;

bool ota_enabled = ota_enabled_default;

bool ota_running = false;

bool task_ota_active = false;

int ota_waiting_for_wifi_timeout = 10000; // 60 seconds timeout

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>


#include <ElegantOTA.h>

//const char* ota_ssid = "Not_Your_Hotspot";
//const char* ota_password = "wifirocks";

const char* ota_ssid = "ota";
const char* ota_password = "password";

WebServer ota_server(80);

unsigned long ota_progress_millis = 0;


void onOTAStart() 
{
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) 
{
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) 
{
  // Log when OTA has finished
  if (success) 
  {
    Serial.println("OTA update finished successfully!");
    buzzer_success();

  } 
  else 
  {
    Serial.println("There was an error during OTA update!");
    buzzer_error();
  }
  // <Add your own code here>
}

void ota_setup(void) 
{
  Serial.print("\nRUNNING OTA Setup...");  

  oled_ota();
  oled_refresh();


  if (WiFi.status() == WL_CONNECTED) wifi_off(); 

  // Ensure WiFi is off before starting OTA
  Serial.println("\nTurning OFF WiFi before OTA setup...");
  WiFi.disconnect(true); // Disconnect from any previous WiFi connection
  WiFi.mode(WIFI_OFF); // Set WiFi mode to OFF
  wait(1000);

  
  Serial.print("\nConnecting to OTA WIFI");  

  //TODO add version and compare , if the version is inferior then do if not the overwrite
  
  //Modified WIFI routing , unique to OTA
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ota_ssid, ota_password);
  Serial.println("");

  buzzer_startup();

  //TODO later add more interesting things to the OLED_OTA
  oled_ota();
  oled_refresh();

  unsigned long waiting_for_wifi = millis();

  // Wait for connection and if not then restart everything and try again
  while (WiFi.status() != WL_CONNECTED) 
  {
    wait(1000);
    Serial.print(".");

    if (millis() > waiting_for_wifi + ota_waiting_for_wifi_timeout ) // 60 seconds timeout
    {
      Serial.printf("\n--- Failed to connect to WiFi within %d seconds, restarting ...", int(ota_waiting_for_wifi_timeout / 1000));
      //todo later save on nvs the cause of restart as ota_wifi_waiting_timeout
      ESP.restart(); // Restart the ESP if it takes too long to connect
    }
    //TODO if waiting for too much just restart the ESP and try again
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ota_ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

   //TODO later add more interesting things to the OLED_OTA
  oled_ota();
  oled_refresh();

  wait(1000);
  

  ota_server.on("/", []() 
  {
    ota_server.send(200, "text/plain", "---ENGEL_SYSTEMS OTA Handler---");
  });

  ElegantOTA.begin(&ota_server);    // Start ElegantOTA
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  ota_server.begin();
  Serial.println("HTTP server started");

  buzzer_ok();
  
  //TODO deploy ota without the need to be in the same netwqork but now via internet
}

void ota_loop(void) 
{
  ota_server.handleClient();
  ElegantOTA.loop();
}


//OTA Task---------------------------------------------------------------------------------------------------- 

TaskHandle_t task_ota_handle = NULL;

void create_task_ota() //once created it will automatically run
{
    if (task_ota_active) 
    {
        if (log_enabled) Serial.println("\n-- OTA Task already active, skipping creation --");
        return; // Exit if the task is already active
    }

    oled_ota();
    oled_refresh();
    

    if(log_enabled) Serial.print("\n--waiting or VTaskDelete on Firebase--");
    //killing all other task in this delay
    wait(5000);

    if(log_enabled) Serial.print("\n--creating task_ota--");

    task_ota_i2c_declare();
    wait(100);

    xTaskCreate
    (
        task_ota,   //Function Name (must be a while(1))
        "task_ota", //Logging Name
        32768,           //Stack Size
        NULL,            //Passing Parameters
        10,              //Task Priority
        &task_ota_handle
    );   

    task_ota_active = true;

    if(log_enabled && task_ota_active) Serial.print("Task_ota_active --\n");
}

void task_ota_i2c_declare()
{
    if(log_enabled)Serial.print("\ntask_ota_i2c_declared\n");
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

void task_ota_i2c_release()
{
    if(log_enabled)Serial.print("\ntask_ota_i2c_released\n");
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

void task_ota(void * parameters)
{
    ota_setup();

    Serial.println("\n--- After 10 mins of inactivity the device will be resetted ---");
    unsigned long watchdog = millis(); // Reset the inactivity timer to help as watchdog

    //every 10 secs print the WiFi State

    int wifi_check_interval = 30000;
    unsigned long last_wifi_check = millis();

    //in case the i2c_manager is still there 
    i2c_manager_running = false;

    while(1)
    {
        ota_loop();
        
        //AN OTA can never take that much otherwise something went wrong
        if(millis() > watchdog + (15000*60)) //15 mins tolerance
        {
            Serial.println("\n--- OTA Task failed or inactive for too long, restarting ESP ---");
            buzzer_error();
            //buzzer_one_tone(100, 100, 1, 1);
            wait(1000);
            ESP.restart(); // Restart the ESP
        }

        //If al is good then show nothing
        if(millis() - last_wifi_check > wifi_check_interval)
        {
            last_wifi_check = millis();
           
            //Serial.printf("WiFi status: %d\n", WiFi.status());
            //Making an if else that will explain WiFi.status() instead of just a number
            if (WiFi.status() == WL_CONNECTED) 
            {
              //If al is good then show nothing
              //Serial.println("WiFi is connected.");
            } 
            else if (WiFi.status() == WL_DISCONNECTED) 
            {
                Serial.println("WiFi is disconnected.");
            } 
            else if (WiFi.status() == WL_CONNECT_FAILED) 
            {      
                Serial.println("WiFi connection failed.");
            } 
            else if (WiFi.status() == WL_NO_SSID_AVAIL) 
            {
                Serial.println("No SSID available.");
            } 
            else if (WiFi.status() == WL_IDLE_STATUS) 
            {
                Serial.println("WiFi is idle.");
            } 
            else 
            {
                Serial.println("Unknown WiFi status.");
            }
            //Serial.printf("Heap: %d, PSRAM: %d\n", ESP.getFreeHeap(), ESP.getFreePsram());
        }
    }
}
