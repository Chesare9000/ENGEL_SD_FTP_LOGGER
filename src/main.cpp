
////// TO UPLOAD FILES TO THE FIREBASE STORAGE /LOG ---------------------------------------------------------------------------------------------------------------


// Taken from https://randomnerdtutorials.com/esp32-cam-save-picture-firebase-storage/
// And https://registry.platformio.org/libraries/mobizt/FirebaseClient/examples/CloudStorage/Upload/Upload.ino

//This will make a completely standalone firebase connection 

//This will work just on WIFI at the moment , later update to lte as well

//#define ENABLE_USER_AUTH
//#define ENABLE_STORAGE
//#define ENABLE_FS

#include <FirebaseClient.h>
//#include <set>
//#include <ExampleFunctions.h> // Provides the functions used in the examples.

#include <wifi.h>
#include <sd.h>
#include <storage_via_wifi.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <tools.h>
#include <vars.h>
#include <oled.h>
#include <buzzer.h>
#include <rtc.h>

#define firebase_user "cesar.gc@outlook.com"
#define firebase_pass "firebase"   
#define firebase_api_key "AIzaSyDPxQ-3--VDEGU37vpG7FdTiweSYohak68"
#define firebase_url "https://engel-dev-61ef3-default-rtdb.europe-west1.firebasedatabase.app/"
#define firebase_url_for_lte "engel-dev-61ef3-default-rtdb.europe-west1.firebasedatabase.app"


//#define WIFI_SSID "Not_Your_Hotspot"
//#define WIFI_PASSWORD "wifirocks"

//#define WIFI_SSID "cesar"
//#define WIFI_PASSWORD "cesar1234"


#define API_KEY "AIzaSyDPxQ-3--VDEGU37vpG7FdTiweSYohak68"
#define USER_EMAIL "cesar.gc@outlook.com"
#define USER_PASSWORD "firebase"

// Define the Firebase storage bucket ID e.g bucket-name.appspot.com */
#define STORAGE_BUCKET_ID "engel-dev-61ef3.firebasestorage.app"

// Photo path in filesystem and photo path in Firebase bucket
//#define SD_LOG_FILE_PATH "/logs/2025-07-14.txt"
//#define BUCKET_PHOTO_PATH "/logs/15840448/2025-07-14.txt"

//const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----X8d45ef470eccb5b9e0823187d87656d00793bf43-----END PRIVATE KEY-----\n";

// User functions
void processData2(AsyncResult &aResult);
void file_operation_callback2(File &file, const char *filename, file_operating_mode mode);

//bool upload_completed = false;

// ServiceAuth is required for Google Cloud Functions functions.
//ServiceAuth sa_auth(USER_EMAIL, "engel-dev-61ef3", PRIVATE_KEY, 3000 /* expire period in seconds (<= 3600) */);

//FileConfig media_file(SD_LOG_FILE_PATH, file_operation_callback2); // Can be set later with media_file.setFile("/image.png", file_operation_callback2);

File myFile;


int total_files_on_sd = 0;

// Authentication
UserAuth user_auth2(API_KEY, USER_EMAIL, USER_PASSWORD, 3000 /* expire period in seconds (<3600) */);

//Firebase Components 
FirebaseApp app2;

WiFiClientSecure ssl_client2;

using AsyncClient = AsyncClientClass;

AsyncClient aClient2(ssl_client2);

//CloudStorage cstorage;

Storage storage;

bool taskComplete = false;

bool firebase_file_initialized = false;

//AsyncResult cloudStorageResult;
AsyncResult storageResult;

void uploadLogsFromSD();

void test_uploadLogsFromSD();

void run_storage_via_wifi();

bool firebase_file_init(char* ssid , char* password);

//Terminating firebase upon request
void firebase_file_deinit();

void processData2(AsyncResult &aResult);


void processData2(AsyncResult &aResult)
{
    // Exits when no result available when calling from the loop.
    if (!aResult.isResult())
        return;

    if (aResult.isEvent())
    {
        Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.appEvent().message().c_str(), aResult.appEvent().code());
    }

    if (aResult.isDebug())
    {
        Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
    }

    if (aResult.isError())
    {
        Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
    }

    if (aResult.downloadProgress())
    {
        Firebase.printf("Downloaded, task: %s, %d%s (%d of %d)\n", aResult.uid().c_str(), aResult.downloadInfo().progress, "%", aResult.downloadInfo().downloaded, aResult.downloadInfo().total);
        if (aResult.downloadInfo().total == aResult.downloadInfo().downloaded)
        {
            Firebase.printf("Download task: %s, complete!✅️\n", aResult.uid().c_str());
        }
    }

    if (aResult.uploadProgress())
    {
        Firebase.printf("Uploaded, task: %s, %d%s (%d of %d)\n", aResult.uid().c_str(), aResult.uploadInfo().progress, "%", aResult.uploadInfo().uploaded, aResult.uploadInfo().total);
        if (aResult.uploadInfo().total == aResult.uploadInfo().uploaded)
        {
            Firebase.printf("Upload task: %s, complete!✅️\n", aResult.uid().c_str());
            Serial.print("Download URL: ");
            Serial.println(aResult.uploadInfo().downloadUrl);
        }
    }
}

String get_current_date_str()
{
    char date_buf[16];
    snprintf(date_buf, sizeof(date_buf), "%04d-%02d-%02d", year, month, day);
    return String(date_buf);
}

bool firebase_file_init(char* ssid , char* password)
{
  Serial.println("\n---Initializing Firebase---\n");    
  wait(100);

  //UBaseType_t stackRemaining = uxTaskGetStackHighWaterMark(NULL);
  //Serial.printf("📊 Task remaining stack: %u bytes\n", stackRemaining);
  
  if (WiFi.status() != WL_CONNECTED)
  {
    
    /*
    Serial.print("ERROR : WIFI NOT CONNECTED , EXITING");
    
    if(task_ftp_wifi_running)
    {
      buzzer_error();
      oled_logger_error_wifi();
      wait(1000);
      task_ftp_wifi_running = false;
    }
    
    firebase_file_initialized = false;

    return false;
    
    */
    
    Serial.print("\n---CONNECTING TO WIFI---");

    if(!wifi_connect_to(ssid,password))
    {
        Serial.print("ERROR : WIFI NOT CONNECTED , EXITING");
        firebase_file_initialized = false;
        wait(1000);
        return false;
    }

    
    
  } 
  else //WIFI is connected
  {
      Serial.print("\n---WIFI ALREADY CONNECTED --- CONTINUING---\n ");
  }

  rtc_update();

  if (year < 2020) 
  {
    Serial.println("RTC not valid!");
    return false;
  }
  else Serial.println("\n---RTC is valid!");

  Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

  
  ssl_client2.setInsecure();
  ssl_client2.setTimeout(5000);
  ssl_client2.setHandshakeTimeout(5);

  Serial.printf("aClient2 ptr: %p\n", (void*)&aClient2);
  Serial.printf("app2 ptr: %p\n", (void*)&app2);
  Serial.printf("user_auth2 ptr: %p\n", (void*)&user_auth2);
  Serial.printf("processData2 ptr: %p\n", (void*)processData2);
  
  if (!&aClient2 || !&app2 || !&user_auth2 || !processData2) {
    Serial.println("❌ Null pointer detected before initializeApp!");
    return false;
  }
  Serial.printf("Free heap before Firebase init: %u\n", ESP.getFreeHeap());
  
  if (!SD.begin()) 
  {
    Serial.println("SD card not initialized! Aborting Firebase init.");
    return false;
  }
  else Serial.println("\n---SD OK");

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    while (time(nullptr) < 1700000000) { // ~Jan 2024
    Serial.println("Waiting for time sync...");
    delay(1000);
    }

    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
Serial.printf("Min heap: %d bytes\n", ESP.getMinFreeHeap());
  
  wait(1000);
  
  Serial.println("Initializing app2...");
  initializeApp(aClient2, app2, getAuth(user_auth2), processData2, "authTask");

  //initializeApp(aClient2, app2, getAuth(user_auth2), 10000, processData2); // 10s
  
  wait(1000);

  Serial.println("\n---Setting Storage");

  wait(1000);

  app2.getApp<Storage>(storage);

  //app2.getApp<CloudStorage>(cstorage);
  // Or intialize the app and wait.
  //initializeApp(aClient2, app2, getAuth(user_auth2), 10 * 1000 /* 10s timeout */ , processData2);


  Serial.println("\n--Storage ready---");

  total_files_on_sd = print_sd_log_folder_content();

  // Ensure the chip ID is read if it hasn't been updated
  if (!esp_id) 
  {
      get_esp_id();
      wait(100);
  }    

  firebase_file_initialized = true;
  return true;
}

void firebase_file_deinit()
{
  //Serial.println("🧹 Cleaning up Firebase state before exiting ...");
  // Important: clear pending operations and free heap-allocated data

 // Manually destroy the FirebaseApp by reinitializing in-place
  //app2.~FirebaseApp();
  //new (&app2) FirebaseApp();

  // Manually destroy the AsyncClient
  //aClient2.~AsyncClientClass();
  //new (&aClient2) AsyncClientClass(ssl_client2);

  // Stop SSL session if active
  //ssl_client2.stop();

  // Optional: clear taskComplete or other flags
  taskComplete = false;
  firebase_file_initialized = false;
}


void run_storage_via_wifi()
{
  if(task_ftp_wifi_running)
  {
    // To maintain the authentication process.
    app2.loop();

    if (app2.ready() && !taskComplete) 
    {
        taskComplete = true;
        uploadLogsFromSD();
    }
    else wait(10);

    // For async call with AsyncResult.
    processData2(storageResult);

  }
}


void file_operation_callback2(File &file, const char *filename, file_operating_mode mode) 
{
  switch (mode) 
  {
    case file_mode_open_read:
      myFile = SD.open(filename, FILE_READ);
      if (!myFile || !myFile.available()) 
      {
        Serial.println("[ERROR] Failed to open file for reading");
      }
      break;
    case file_mode_open_write:
      myFile = SD.open(filename, FILE_WRITE);
      break;
    case file_mode_open_append:
      myFile = SD.open(filename, FILE_APPEND);
      break;
    case file_mode_remove:
      SD.remove(filename);
      break;
    default:
      break;
  }
  file = myFile;
  file.seek(0); // Ensure pointer is at start
}


bool getExistingFilesFromStorage(const String &subFolderPath, std::set<String> &existingFiles)
{
    Serial.println("🗂 Getting full root list from Firebase Storage...");
    String result = storage.list(aClient2, FirebaseStorage::Parent(STORAGE_BUCKET_ID));

    if (aClient2.lastError().code() != 0)
    {
        Serial.printf("❌ Failed to list. Msg: %s, Code: %d\n", aClient2.lastError().message().c_str(), aClient2.lastError().code());
        return false;
    }

    Serial.printf("🔍 Filtering files under: %s\n", subFolderPath.c_str());

    // Parse line-by-line and filter by subfolder
    int idx = 0;
    while (true)
    {
        idx = result.indexOf("\"name\":", idx);
        if (idx == -1)
            break;

        idx = result.indexOf("\"", idx + 7); // first quote after "name":
        int endIdx = result.indexOf("\"", idx + 1);
        if (endIdx == -1)
            break;

        String fullPath = result.substring(idx + 1, endIdx); // e.g., logs/7801764/2025-07-04_part1.tx

        // Only consider files inside the subFolderPath
        if (fullPath.startsWith(subFolderPath))
        {
            String filename = fullPath.substring(fullPath.lastIndexOf("/") + 1);

            // ✅ Fix Firebase bug: .tx → .txt
            if (filename.endsWith(".tx")) 
            {
                filename += "t";
                //Serial.printf("⚠️ Firebase bug fix applied: renamed '%s' to '%s'\n", 
                //              fullPath.substring(fullPath.lastIndexOf("/") + 1).c_str(), 
                //              filename.c_str());
            }
            existingFiles.insert(filename);
        }

        idx = endIdx + 1;
    }

    Serial.printf("✅ Found %d existing files under '%s'\n", existingFiles.size(), subFolderPath.c_str());
    return true;
}



void uploadLogsFromSD() 
{
  String todayStr = get_current_date_str();

  File dir = SD.open("/logs");
  if (!dir) 
  {
    Serial.println("Failed to open /logs directory");
    buzzer_error();
    wait(2000);
    Serial.println("----- Restarting ESP due to upload error");
    wait(2000); 
    //Upon Faiure will just restart everything to avoid being stuck
    ESP.restart();
    return;
  }

  std::set<String> existingRemoteFiles;
  String remoteFolder = "logs/" + String(esp_id);

  if (!getExistingFilesFromStorage(remoteFolder, existingRemoteFiles))
  {
      Serial.println("⚠️ Skipping remote check due to error...");
      wait(2000);
      Serial.println("----- Restarting ESP due to upload error");
      wait(2000); 
      //Upon Faiure will just restart everything to avoid being stuck
      ESP.restart();
  }

  File entry = dir.openNextFile();
  int fileCounter = 0;

  buzzer_notification();

  while (entry) 
  {
    if (!entry.isDirectory() && String(entry.name()).endsWith(".txt")) 
    {
      String filePath = String("/logs/") + entry.name();
      std::vector<String> partFiles;

      String mimeType = "text/plain";

      // ✅ Determine if this is today's file BEFORE chunking
      String fileDate = entry.name();
      fileDate = fileDate.substring(0, 10); // YYYY-MM-DD
      bool isToday = (fileDate == todayStr);

      splitFileIntoChunksIfNeeded(filePath, partFiles, 2100000, existingRemoteFiles);

      buzzer_quick_alert();

      for (size_t i = 0; i < partFiles.size(); i++) 
      {
        String partPath = partFiles[i];
        String partFileName = partPath.substring(partPath.lastIndexOf("/") + 1);

        bool isTodayFile = false;

        // Check if this chunk belongs to today's file
        if (partFileName.indexOf(get_current_date_str()) != -1)
        {
            isTodayFile = true;
            Serial.printf("📅 Detected today's file: %s → will force upload.\n", partFileName.c_str());
        }

        if (existingRemoteFiles.find(partFileName) != existingRemoteFiles.end() && !isTodayFile)
        {
            Serial.printf("✅ File already exists — skipping upload: %s\n", partFileName.c_str());
            continue;
        }
        else
        {
          if(!isTodayFile)Serial.printf("\n---File: %s not found on Storage , uploading ---\n", partFileName.c_str());
        }

        String firebasePath = String("/logs/") + esp_id + "/" + partPath.substring(partPath.lastIndexOf("/") + 1);

        FileConfig logFile(partPath.c_str(), file_operation_callback2);
        bool uploadSuccess = false;

        int attempt = 0;

        while (attempt < 5 && !uploadSuccess) 
        {
          attempt++;
          if (attempt > 1) Serial.printf("\nRetrying upload attempt %d for %s\n", attempt, partPath.c_str());

          File freshFile = SD.open(partPath.c_str(), FILE_READ);
          if (!freshFile) 
          {
            Serial.printf("❌ Failed to reopen %s\n", partPath.c_str());
            buzzer_error();
            break;
          }
          freshFile.close(); // just to check availability

          oled_logger_uploading(i + 1, partFiles.size());

          //oled_logger_uploading(fileCounter + 1, total_files_on_sd);

          Serial.printf("Uploading file: %s\nFirebase Path: %s\n", partPath.c_str(), firebasePath.c_str());

          bool status = storage.upload(aClient2, FirebaseStorage::Parent(STORAGE_BUCKET_ID, firebasePath.c_str()), getFile(logFile), mimeType);
          
          if (status) 
          {
            if (aClient2.lastError().code() == 0) 
            {
                Serial.println("✅ Upload complete!");
                uploadSuccess = true;

                buzzer_ok();

                // Only delete if Firebase confirms full size upload and has a _part in it
                if (partPath.indexOf("_part") != -1) 
                {
                    SD.remove(partPath.c_str());
                    Serial.printf("🧹 Deleted chunk file: %s\n", partPath.c_str());
                }
            } 
            else 
            {
                Serial.printf("⚠️ Upload reported success but had error: %s\n", aClient2.lastError().message().c_str());
               
                buzzer_attention();

                wait(5000);

                if(attempt > 3)
                {
                  buzzer_error();
                  
                  // Only delete if contains a _part on it
                  if (partPath.indexOf("_part") != -1) 
                  {
                      SD.remove(partPath.c_str());
                      Serial.printf("🧹 Deleted chunk file: %s\n", partPath.c_str());
                  }
                  
                  wait(2000);
                  Serial.println("----- Restarting ESP due to upload error");
                  wait(2000); 
                  //Upon Faiure will just restart everything to avoid being stuck
                  ESP.restart();
                }
            }
          } 
          else 
          {
            Firebase.printf("❌ Upload failed. Msg: %s, Code: %d\n", aClient2.lastError().message().c_str(), aClient2.lastError().code());
            buzzer_attention();  

            wait(5000);

            if(attempt > 3 )
            {
              buzzer_error();

              // Only delete if Firebase confirms full size upload
              if (partPath.indexOf("_part") != -1) 
              {
                  SD.remove(partPath.c_str());
                  Serial.printf("🧹 Deleted chunk file: %s\n", partPath.c_str());
              }

              wait(2000);
              Serial.println("----- Restarting ESP due to upload error");
              wait(2000); 
              //Upon Faiure will just restart everything to avoid being stuck
              ESP.restart();

            }
          }
        }

        if (!uploadSuccess) 
        {
          //Wil never arrive here in principle , TODO maybe later improve error handling
          Serial.printf("🚫 Failed to upload %s after retries.\n", partPath.c_str());
          buzzer_error();
        }

        //Continuing even after fail 
        //TODO maybe just continue if succeeded?
        fileCounter++;
      }
    }
    entry = dir.openNextFile();
  }

  dir.close();
  Serial.printf("🟢 Finished uploading all log files.\n");
  buzzer_success();
  oled_ftp_wifi_success();
  wait(5000);
  Serial.printf("\nReturning to Menu\n");
  wait(3000);
  task_ftp_wifi_running = false;
}


/*
// Generic Deps.
#include <Arduino.h>

// Custom Deps.
#include <vars.h>
#include <tools.h>
#include <serial.h>
#include <fuel_gauge.h>
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
#include <nvs.h>
#include <firebase.h>
//#include <wifi_demo.h>
//#include <ble.h>
//#include <listener.h>
//#include <mqtt.h>


//Needed by Firebase 
//(will not compile othersiwe)
#include <LittleFS.h>
#include <SD.h>
#include <WiFi.h>
#include <Update.h>

#include <lte.h>

#include <gps.h>

#include <logger.h>

#include <storage_via_wifi.h>

//#include "esp_task_wdt.h"



// Previously declared on Vars.h
bool i2c_initialized = false;
bool serial_initialized = false;
bool gpios_initialized = false;
bool interrupts_initialized = false;
bool lux_initialized = false;
bool oled_initialized = false;
bool rtc_initialized = false;

bool lora_enabled = false;

bool log_enabled = true;
bool oled_enabled = true;

bool rgb_leds_initialized = false;
int rgb_leds_brightness = 100; //max 255

int board_temp = 0;
float board_temp_max_threshold = 50.0;
float board_temp_min_threshold = 40.0;

int lux_val = 0;
int lux_low_threshold = 100;
int lux_int_persistance_counts = 4;

bool reg_5v_enabled = false;

bool lvl_shftr_enabled = false;

bool ff2_q_status = false;

int demos_total = 7; // including exit option

bool movement_detected = false;
bool moving = false;

button btn_1 = {esp_btn_1_pin, 0, false};
button btn_2 = {esp_btn_2_pin, 0, false};

bool demo_is_ongoing = false;

int boot_mode = 0;
int current_mode = 0;

// TODO if works change all initialization vars here to their respective .h or .cpp

// int imu_mode=0;

// GPIO Pins (Just Init with -1)
// To be set after get_hw_version();
// NEW
int esp_int_gpio_exp_pin = -1;
int esp_reg_5v_en_pin = -1;
// OLD
int esp_buzzer_pin = -1;
int esp_lora_vcc_en_pin = -1;
int esp_nrf_reset_pin = -1;
int esp_lora_reset_pin = -1;
int esp_int_charger_pin = -1;

// I2c Bus Vars
int imu_needed = 0;
int rgb_needed = 0;
int temp_needed = 0;
int lux_needed = 0;
int rtc_needed = 0;
int fuel_gauge_needed = 0;
int oled_needed = 0;

bool rgb_bypassed = false;

bool imu_running = false;
bool rgb_running = false;
bool temp_running = false;
bool lux_running = false;
bool rtc_running = false;
// bool fuelgauge_running  = false;
bool oled_running = false;


int oled_token = oled_free;




// BLE UUID
/*std::string uuid = "0000fef3-0000-1000-8000-00805f9b34fb";
// reference to ble service
BLE::BLEngelService *engelService = new BLE::BLEngelService();
BLE::Listener *listener;

std::map<std::string, std::string> cast_reference;
std::map<std::string, int*> int_map;
std::map<std::string, bool*> bool_map;
std::map<std::string, float*> float_map;
std::map<std::string, uint32_t*> uint32_map;
std::map<std::string, std::string*> string_map;*/

/*

int non_critical_task_refresh_ms = 1000;

bool fuel_gauge_initialized = false;

// Charger Vars
bool charger_initialized = false;
bool charging = false;		// Based on Charging status Pin
bool charger_pgood = false; // Pin to indicate usb connected with 5V

int bat_percent = -1;
float bat_voltage = -1;
float bat_c_rate = -1;

float low_bat_threshold = 3.3; // in Volts

// GPIO_EXP Expected Inputs States-------------

bool int_usb = true;
bool int_charger = true;
bool int_rtc = false;
bool int_low_bat = true;
bool int_temp = true;
bool int_lux = true;

// GPIO EXP Derived Flags----------------------
bool debug_mode = false;
bool usb_connected = false;
// bool charging = false; Aready Declared but keep here for reference
bool rtc_alarm = false;
bool low_bat = false;
bool overheat = false;
bool dark = false;

bool gpio_exp_changed = false;

// Defining HW VARIANT through HW_ID resistors
bool hw_id_0 = false;
bool hw_id_1 = false;
bool hw_id_2 = false;
bool hw_id_3 = false;

// default is devel
int hw_variant = hw_variant_devel;

// Newest version by default
// Downgrade if necessary on get_hw_version()
int hw_version = 5;

bool test_is_charging = false;

int time_critical_tasks_running = 0;


//MQTT related
int mqtt_cycle_nr =0;

int current_internet_connection = -1 ; //Not connected

//Handle by NVS later
int backend_config = backend_config_firebase;  

void setup()
{
	init_all();

	create_task_i2c_manager(); //always running and shall not be kiled

	//TODO
	//check_wake_up_reason();

	//Detect and Hold the loop if action is needed
	//check_alarms();


    //by some weird reason we have to start this before renunning the task TODO fix this but ftm keep it
    //if(!firebase_file_initialized) firebase_file_init(logger_wifi_ssid,logger_wifi_password);
    //oled_needed++;

    if (boot_mode == 0) // DEVELOPER MODE
	{
        wait(1000);
		//create_task_mubea();
        //change to firebase when ready

        if(!task_logger_sd_ftp_running)
        {
            Serial.print("--\nRunning task_logger_sd_ftp Initialization Order"); 
          
            oled_task_logger_sd_ftp_running();

           // wait(100);

            //Start the Firebase to upload the values
            create_task_logger_sd_ftp();
            //wait(1000);
        }  
	}
   

	// will set up and start the ble service
	// variables should be connected before this
	// setup_ble();

    // Increase the watchdog timeout to 10 seconds
    //esp_task_wdt_init(10, true);
    //esp_task_wdt_add(NULL); // Add the current task to the watchdog

    

}

void loop()
{
    wait(10);	
    /*
    if(firebase_file_initialized) 
    {
        run_storage_via_wifi();
    }

    else
    {
        Serial.print("ERROR , Firebase File not INIT!");
        wait(1000);
        firebase_file_init(logger_wifi_ssid,logger_wifi_password);
    }
    */

/*
}*/

//TODO
//MAKE TIMERS FOR INDIVIDUAL FIREBASE TIMERS


//HW Check charger and correct working

//HW add battery for RTC and change RTC if not compatible with battery

//Check charging circuit and confirm it works ok

//MAKE DEMOS FOR LEDS ON IMUS
//GYRO
//ACC

//TODO OLED to String
//TODO also display on other axis
/*
0 off
1 info on firebase
2 GPS
3 CAN
4 IMU_POS_X
5 IMU ACC_X
6 overall status (default)
7 ota
*/












/*

----TODO

Kill al task once we enter OTA , blackbox and all 
POWER SAVING MODE including turning off all and testing (make light sleep , deep sleep and kill versions)
CHECK SD OVERWRITE
save all max retries and delays for input and output as variables on the firebase 
Send an output notification for power saving and go to sleep just if ack is received , otherwise ater couple of retries to indicatte sleep while offline
Send to Firebase the heartbeat , mwr , bb_id every certain time or n/a if deactivated power saving mode , last known time , specially before starting bblackbox, 
confirm blackbox is having the correct data to link time even if its interrupted
Correct BlackBox data and time

make the firebase work with wifi and once is not found then swap to lte 


*/


//ADD TIMER FOR HEART BEAT < HEARTBEAT NR AND LAST HEARTBEAT DATE AND TIME
//AND WAKE UP IN MOTION

/*

OLED
-1 OFF
 1 OLED_BLACK_BOX_INFO
 2 GPS
 3 CAN
 4 IMU_POS_X
 5 IMU_ACC_X


USING NOW (RESISTORS SOLDERED AND READY)

ADDED Two NEW Soft UARTS taken from the LORA Pins
BY ADDING THE SD CARD NOW WE ARE SHARING PINS

ESP_V_MISO now is (GPIO19) WHITE <- SD_V_MISO
ESP_V_MOSI now is (GPIO23) GREEN <- SD_V_MOSI
ESP_V_CS  (GPIO5 outputs PWM signal at boot,strapping pin) WHITE/BLUE <- SD_V_CS/SS    
ESP_V_CLK (GPIO18) ORANGE <- SD_V_CLK

GPIO12 / PURPLE is now CAN_TX (change later this one as is problematic on boot)
GPIO36 / ORANGE is CAN_RX 

GPIO14 LORA_RESET (PWM on BOOT) GREEN -> esp_pin_lte_tx connected to SIMCOM_IC_RX on IC
GPIO34 LORA_DIO_0_AL_3v3 (JUST INPUT)YELLOW <- esp_pin_lte_rx connected to SIMCOM_IC_TX on IC

ESP_RX2 (GPIO16) GREY <- is esp_pin_gps_rx connected to GPS_TX on IC
ESP_TX2 (GPIO17) WHITE/GREEN <- is esp_pin_gps_tx connected to GPS_RX on IC	

// PORT EXPANDER
//was GPIO14 <- FONA_RESET
//was GPIO16 <- FONA_KEY / SIMCOM_KEY


WE Will SET The Three available UARTS

UART0 - Serial Monitor (USB)
UART1 - GPS (LC29H)
UART2 - SIMCOM (LTE+GPS)





TODO CHANGE IN HW

We need a bigger ESP32 with More GPIOs or maybe a big i2c extender , wil see 



*/



//CHECK WHY FIRESTORE GPS AND CAN ARE NOT BEING UPDATED
//











//If on MUBEA GPS we push btn2 while on lamp then we will start for not your hotspot , else motionlab



/*

//Inputs
        if (aResult.uid() == "firebase_get_leds_on")
        {
            // Convert the payload to a boolean 
            if(payload == "true") backend_led_status = true;
            else if(payload == "false") backend_led_status = false;
    
            else
            {
                Serial.print("\n---ERROR ON firebase_get_leds_on: not a bool. Overwriting default value.\n");
                backend_led_status = false; // Default value TODO make a set of default values
                firebase_get_leds_on_needs_overwrite = true;
            }

            if(last_known_led_status != backend_led_status)
            {
                Firebase.printf("\n---Firebase value for backend_led_status: %s\n", backend_led_status ? "true" : "false");

                if(backend_led_status)
                {
                    Serial.println("\n---Turning ON LEDs via firebase\n!");
                    rgb_leds_on(backend_led_color,backend_led_brightness);
                }  
                else
                {
                    Serial.println("\n---Turning OFF LEDs via firebase\n!");
                    rgb_leds_off();
                }
                last_known_led_status = backend_led_status;
            }            
        }

        else if (aResult.uid() == "firebase_get_leds_brightness")
        {            
            backend_led_brightness = payload.toInt();

            if (backend_led_brightness != last_known_led_brightness)
            {
                if(backend_led_brightness > max_brightness) 
                {
                    backend_led_brightness = max_brightness;
                    firebase_get_leds_brightness_needs_overwrite = true;
                }

                else if (backend_led_brightness < min_brightness)
                {
                    backend_led_brightness = min_brightness;
                    firebase_get_leds_brightness_needs_overwrite = true;
                }

                else 
                {  //If the value is between the min and max brightness
                    backend_led_brightness = backend_led_brightness;    
                }
                
                if(firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.printf("\n---leds_brightness changed to : %d via firebase\n",backend_led_brightness);
                }

                //Executing the Order if leds are enabled

                if(backend_led_status)rgb_leds_on(backend_led_color,backend_led_brightness);
                
                else 
                {   
                    if(log_enabled)Serial.printf("\n---ERROR --- LEDS are OFF (backend_leds_status:%d) \n",backend_led_status);
                }

                last_known_led_brightness = backend_led_brightness;
            } 
          
        }

        else if(aResult.uid() == "firebase_get_leds_color")
        {
            //Fitering the String as it sometimes contain the quotes within the String and this is wrong 
            if (payload.startsWith("\"") && payload.endsWith("\"")) {payload = payload.substring(1, payload.length() - 1);}

            if(payload != last_know_led_color_string)
            {
                if(firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.printf("\n---leds_color changed to : %s via firebase\n",payload);
                }

                if     ( payload == "red")   {backend_led_color  = 'r';}
                else if( payload == "green") {backend_led_color  = 'g';}
                else if( payload == "blue")  {backend_led_color  = 'b';}
                else if( payload == "white") {backend_led_color  = 'w';}

                else 
                {
                    Serial.print("\n---Color Not Recognized,defaulting to red"); 
                    backend_led_color = 'r';
                    backend_led_color_string = "red";
                    firebase_get_leds_color_needs_overwrite = true;
                }                                   
                
                //Executing Order if LEDS are enable

                if(backend_led_status)rgb_leds_on(backend_led_color,backend_led_brightness);
                else { if(log_enabled)Serial.printf("\n ERROR --- LEDS are OFF (backend_leds_status:%d) \n",backend_led_status);}

                last_know_led_color_string = payload;

            }

        }







        else if (aResult.uid() == "firebase_get_park_alarm_on")
        {
            // Convert the payload to a boolean 
            if(payload == "true") backend_parking_alarm_state = true;
            else if(payload == "false") backend_parking_alarm_state = false;
           
            else
            {
                Serial.print("\n---ERROR ON firebase_get_park_alarm_on: not a bool. Overwriting default value.\n");
                backend_parking_alarm_state = false; // Default value TODO make a set of default values
                firebase_get_park_alarm_on_needs_overwrite = true;
            }

            if(last_known_backend_parking_alarm_state != backend_parking_alarm_state)
            {
                if(firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.printf("\n---park_alarm_state changed to : %s via firebase\n", backend_parking_alarm_state ? "true" : "false");
                }  

                if(backend_parking_alarm_state)
                {
                    //We can activate the alarm JUST if we are in parking mode
                    
                    if (main_status)
                    {
                        if(moving)
                        {
                            Serial.print("\n---ERROR : Cannot Turn on the parking_alarm_while riding! ");
                            Serial.print("\n---Waiting to see if we stop moving soon , otherwise throw an error");

                            //TOD15O OVERRIDE INPUT TO SHOW THAT THIS IS NOT ALLOWED ATM

                            wait(5000);
                            
                            if(!moving )//TODO add the imu_checks here as well
                            {
                                main_status = false; 
                            }
                        }

                        else //Forcing change from riding to parked upon request and without having to wait still
                        {
                            Serial.print("\n---Main Status changed to parked upon request");
                            main_status = false; 

                            oled_token = oled_taken;
                            //if(log_enabled)Serial.println("\n---Sending to OLED---> "); 
                            wait(100); 
                            oled_main_status_changed_to_parking();
                            wait(1000);
                            oled_token = oled_free;

                            //TODO later turn off this flag also after waiting for some time
                            task_imu_status = task_imu_not_needed;//turn off the imu and reset all flags
                            wait(500);
                        }               
                    }
                    else
                    {
                        Serial.println("\n---Turning ON ALARM via firebase\n");

                        buzzer_one_tone(1000,500,1,1);
                        //Here Start the alarm with the given mode
                        create_task_parking_alarm();
                        //Give chance for the task to be created and run before continuing
                        wait(2000);
                    }
                } 
                else
                {
                    Serial.println("\n---Turning OFF ALARM_ON flag on Firebase");
                    wait(100);
                    //The Alarm will handle all the turning off by itself
                }

                last_known_backend_parking_alarm_state = backend_parking_alarm_state;
            }            
        }       
        
        else if (aResult.uid() == "firebase_get_park_alarm_mode")
        {
            // Convert the payload to a boolean 
            if(payload == "true") backend_parking_alarm_mode = true;
            else if(payload == "false") backend_parking_alarm_mode = false;
            else
            {
                Serial.print("\n---ERROR ON firebase_get_park_alarm_mode: not a bool. Overwriting default value.\n");
                backend_parking_alarm_mode = false; // Default value TODO make a set of default values
                firebase_get_park_alarm_mode_needs_overwrite = true;
            }

            if(last_known_park_alarm_mode != backend_parking_alarm_mode)
            {
                //Firebase.printf("---Firebase value for leds_on: %s\n", leds_on ? "true" : "false");

                if(firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.printf("\n---park_alarm_mode changed to : %s " ,  backend_parking_alarm_mode ? "true" : "false");
                }       

                //Displaying Change as there is no command to execute here
                if(backend_parking_alarm_mode)
                {
                    if(log_enabled)Serial.println("\n---Setting Parking Alarm to LOUD Mode via firebase\n!");
                }
                else
                {
                    if(log_enabled) Serial.println("\n---Setting Parking Alarm to SILENT Mode via firebase\n!"); 
                } 

                
                last_known_park_alarm_mode = backend_parking_alarm_mode;
            }            
        }

        else if (aResult.uid() == "firebase_get_park_alarm_snooze")
        {
            // Convert the payload to a boolean 
            if(payload == "true") backend_parking_alarm_snooze = true;
            else if(payload == "false") backend_parking_alarm_snooze = false;
            else
            {
                Serial.print("\n--- ERROR ON firebase_get_park_alarm_snooze: not a bool. Overwriting default value.\n");
                backend_parking_alarm_snooze = false; // Default value TODO make a set of default values
                firebase_get_park_alarm_snooze_needs_overwrite = true;
            }   

            if(last_known_park_alarm_snooze != backend_parking_alarm_snooze)
            {
                if(firebase_log_mode > firebase_log_mode_silent)
                {
                    Firebase.printf("\n---Firebase value for backend_parking_alarm_snooze changed to : %s\n", backend_parking_alarm_snooze ? "true" : "false");
                }       

                if(backend_parking_alarm_snooze)
                {
                    if(log_enabled)Serial.printf("\n---Park_Alarm Snoozed for %d s ", backend_parking_alarm_snooze_time_ms/1000);
                }
                else
                {
                    if(log_enabled) Serial.println("\n---park_alarm_snooze resetted!"); 
                } 
                
                last_known_park_alarm_snooze = backend_parking_alarm_snooze;
                //TODO MAybe reset this after the timer or something like that so we allowed another snooze
            }            
        }

        else if (aResult.uid() == "firebase_get_park_alarm_dismissed")
        {
            // Convert the payload to a boolean 
            if(payload == "true") backend_parking_alarm_dismissed = true;
            else if(payload == "false") backend_parking_alarm_dismissed = false;
            else
            {
                Serial.print("\n---ERROR ON firebase_get_park_alarm_dismissed: not a bool. Overwriting default value.\n");
                backend_parking_alarm_dismissed = false; // Default value TODO make a set of default values
                firebase_get_park_alarm_dismissed_needs_overwrite = true;
            }

            if(last_known_park_alarm_dismissed != backend_parking_alarm_dismissed)
            {
                if(firebase_log_mode > firebase_log_mode_silent)
                {
                    Firebase.printf("\n---Firebase value for backend_parking_alarm_dismissed changed to : %s\n", backend_parking_alarm_dismissed ? "true" : "false");
                }       

                if(backend_parking_alarm_dismissed)
                {
                    if(log_enabled)Serial.print("\n---Park_Alarm Dismissed : Attention , Function not yet implemented !!! ");
                }
                else
                {
                    if(log_enabled) Serial.println("\n---backend_parking_alarm_dismissed = false: Attention , Function not yet implemented !!! "); 
                } 
                
                last_known_park_alarm_dismissed = backend_parking_alarm_dismissed;
                //TODO Maybe reset this after the timer or something like that so we allowed another snooze

                /*
                PREVIOUS IMPLEMENTATION

                //DISMISSED
                if(!Firebase.RTDB.readStream(&fbdo_in_accident_dismissed))
                {
                    if(firebase_log_mode > firebase_log_mode_silent)
                    {
                        Serial.printf("ERROR on accident_dismissed CallBack: %s " ,fbdo_in_accident_dismissed.errorReason().c_str());
                    }
                }

                if(fbdo_in_accident_dismissed.streamAvailable())
                {
                    //Refreshing the value
                    if(fbdo_in_accident_dismissed.dataType() == "boolean")
                    {
                        //just dismissing if the flag is inactive , and the accident is detected or confirmed

                        if(!accident_dismissed && ( accident_detected || accident_confirmed ))
                        {
                            accident_dismissed = fbdo_in_accident_dismissed.boolData();
                        
                            if(firebase_log_mode > firebase_log_mode_silent)
                            {
                                Serial.printf("\n---accident_dismissed changed to : %d (Type : %s) via firebase\n", fbdo_in_accident_dismissed.boolData(),
                                                                                                fbdo_in_accident_dismissed.dataType());
                            }      

                            //Executing the change
                            accident_detected  = false;
                            accident_confirmed = false;

                            //Resetting the flag
                            accident_dismissed = false;
                        
                            //need an override to inputs
                            //firebase_inputs_need_manual_refresh <- Deprecated = true;
                            
                            if(firebase_log_mode > firebase_log_mode_silent)
                            {
                                Serial.printf("\n---accident_dismissed forcing an input manual refresh on firebase ..");
                            }  

                            //Raise the flag to update Firestore as well    
                            firestore_accident_dismissed_needs_update = true;  
                            firestore_needs_update = true;
                        } 

                        //If is dismissed without an accident_mode detected then is forced to reset
                        else if (fbdo_in_accident_dismissed.boolData())
                        {
                        if(log_enabled)
                        {
                            Serial.print("\nError on Accident_Dismiss!:");                
                        }

                        if(!accident_detected && !accident_confirmed )
                        {
                            if(log_enabled)
                            {
                                Serial.print("\n 'accident mode' not triggered!. Resetting flag!");                
                            }

                            //Resetting Flag    
                            accident_dismissed = false;
                            //firebase_inputs_need_manual_refresh <- Deprecated = true;

                            if(firebase_log_mode > firebase_log_mode_silent)
                            {
                                Serial.printf("\n---accident_dismissed forcing an input manual refresh on firebase ..");
                            } 
                        }

                        else 
                        {
                            if(log_enabled)
                            {
                                Serial.print("\n Unknown!- Check further!");                
                            }
                        }               

                        }

                    }
                }
                //If the stream is unavailable we do nothing because that means that the value has not changed
                
                */    
/*
            }            
        }

        else if (aResult.uid() == "firebase_get_gps_enabled")
        {
            // Convert the payload to a boolean 
            if(payload == "true") gps_enabled = true;
            else if(payload == "false") gps_enabled = false;
            else
            {
                Serial.print("\n---ERROR ON firebase_get_gps_enabled: not a bool . Overwriting default value.\n");
                gps_enabled = false; // Default value TODO make a set of default values
                firebase_get_gps_enabled_needs_overwrite = true;
            }

            if(last_known_gps_enabled != gps_enabled)
            {
                //Firebase.printf("---Firebase value for leds_on: %s\n", leds_on ? "true" : "false");

                if(firebase_log_mode > firebase_log_mode_silent)
                {
                    Firebase.printf("\n---Firebase value for gps_enabled changed to : %s\n", gps_enabled ? "true" : "false");
                }
                
                //HERE Execute any needed change

                if(gps_enabled)
                {
                    if(!task_gps_active)
                    {
                        Serial.print("\n---Turning ON GPS via firebase\n!");
                        //Turn it ON outside the callback
                    }
                } 
                else
                {
                    if(task_gps_active)
                    {
                        task_gps_active = false;
                    }
                }
                               
                last_known_gps_enabled = gps_enabled;
            }            
        }

        else if (aResult.uid() == "firebase_get_gps_upload")
        {
            // Convert the payload to a boolean 
            if(payload == "true") gps_upload = true;
            else if(payload == "false") gps_upload = false;
            else 
            {
                Serial.print("\n---ERROR ON firebase_get_gps_upload: not a bool ");
                gps_upload = false; // Default value TODO make a set of default values
                firebase_get_gps_upload_needs_overwrite = true;
            }

            if(last_known_gps_upload != gps_upload)
            {
               
                if(firebase_log_mode > firebase_log_mode_silent)
                {
                    Firebase.printf("\n---Firebase value for gps_upload changed to : %s\n", gps_upload ? "true" : "false");
                }  
                
                if(gps_upload && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---GPS Data Will be uploaded to Firebase ");
                }
                
                if(!gps_upload && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---GPS Data Will NOT be uploaded to Firebase ");
                }     
                
                last_known_gps_upload = gps_upload;
            }            
        }

        else if (aResult.uid() == "firebase_get_gps_refresh_seconds")
        {            
            if(payload.toInt() > 0) gps_refresh_seconds = payload.toInt();
            else 
            {
                Serial.printf("\n---ERROR on gps_refresh_seconds : Value < 0 , defaulting to : %d\n", gps_refresh_seconds_default);
                gps_refresh_seconds = gps_refresh_seconds_default ;//default on error
                firebase_get_gps_refresh_seconds_needs_overwrite = true;
            }

            if(last_known_gps_refresh_seconds != gps_refresh_seconds)
            {
                //Firebase.printf("---Firebase value for leds_on: %s\n", leds_on ? "true" : "false");

                if(firebase_log_mode > firebase_log_mode_silent)
                {
                    Firebase.printf("\n---Firebase value for gps_refresh_seconds changed to : %d\n", gps_refresh_seconds);
                }                                      
                last_known_gps_refresh_seconds = gps_refresh_seconds;
            }            
        }

        else if (aResult.uid() == "firebase_get_can_enabled")
        {
            // Convert the payload to a boolean 
            if(payload == "true") can_enabled = true;
            else if(payload == "false") can_enabled = false;
            else
            {
                Serial.print("\n---ERROR ON firebase_get_can_enabled: not a bool , overwriting default value.\n");
                can_enabled = false; // Default value TODO make a set of default values
                firebase_get_can_enabled_needs_overwrite = true;
            }

            if(last_known_can_enabled != can_enabled)
            {
                //Firebase.printf("---Firebase value for leds_on: %s\n", leds_on ? "true" : "false");

                if(firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.printf("\n---Firebase value for can_enabled changed to : %s\n", can_enabled ? "true" : "false");
                }
                
                //HERE Execute any needed change

                if(can_enabled)
                {
                    if(!task_can_active)
                    {
                        Serial.print("\n---Turning ON CAN via firebase\n!");
                        //Turn it ON outside the callback
                    }
                } 
                else
                {
                    if(task_can_active)
                    {
                        task_can_active = false;
                    }
                }
                               
                last_known_can_enabled = can_enabled;
            }            
        }

        else if (aResult.uid() == "firebase_get_can_upload")
        {
            // Convert the payload to a boolean 
            if(payload == "true") can_upload = true;
            else if(payload == "false") can_upload = false;
            else
            {
                Serial.print("\n---ERROR ON firebase_get_can_upload: not a bool , overwriting default value.\n");
                can_upload = false; // Default value TODO make a set of default values
                firebase_get_can_upload_needs_overwrite = true;
            }

            if(last_known_can_upload != can_upload)
            {
               
                if(firebase_log_mode > firebase_log_mode_silent)
                {
                    Firebase.printf("\n---Firebase value for can_upload changed to : %s\n", can_upload ? "true" : "false");
                }  
                
                if(can_upload && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---CAN Data Will be uploaded to Firebase ");
                }
                
                if(!can_upload && firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.print("\n---CAN Data Will NOT be uploaded to Firebase ");
                }     
                
                last_known_can_upload = can_upload;
            }            
        }

        else if (aResult.uid() == "firebase_get_can_refresh_seconds")
        {
            
            if(payload.toInt() > 0) can_refresh_seconds = payload.toInt();
            else 
            {
                Serial.printf("\n---ERROR on can_refresh_seconds : Value < 0 , defaulting to : %d\n", can_refresh_seconds_default);
                can_refresh_seconds = can_refresh_seconds_default ;//default on error
                firebase_get_can_refresh_seconds_needs_overwrite = true;    
            }

            if(last_known_can_refresh_seconds != can_refresh_seconds)
            {
                //Firebase.printf("---Firebase value for leds_on: %s\n", leds_on ? "true" : "false");

                if(firebase_log_mode > firebase_log_mode_silent)
                {
                    Firebase.printf("\n---Firebase value for can_refresh_seconds changed to : %d\n", can_refresh_seconds);
                }                                      
                last_known_can_refresh_seconds = can_refresh_seconds;
            }            
        }

        else if (aResult.uid() == "firebase_get_black_box_enabled")
        {
            // Convert the payload to a boolean 
            if(payload == "true") black_box_enabled = true;
            else if(payload == "false") black_box_enabled = false;
            else
            {
                Serial.print("\n---ERROR ON firebase_get_black_box_enabled: not a bool , overwriting default value.\n");
                black_box_enabled = false; // Default value TODO make a set of default values
                firebase_get_black_box_enabled_needs_overwrite = true;
            }

            if(last_known_black_box_enabled != black_box_enabled)
            {
                //Firebase.printf("---Firebase value for leds_on: %s\n", leds_on ? "true" : "false");

                if(firebase_log_mode > firebase_log_mode_silent)
                {
                    Serial.printf("\n---Firebase value for blak_box_enabled changed to : %s\n", black_box_enabled ? "true" : "false");
                }
                
                //Start The Task Outside the Callback

                if(black_box_enabled) //TURN ON
                {
                    if(!task_sd_active)
                    {
                        Serial.println("\n---Turning ON BLACK BOX via firebase\n!");
                    }
                } 
                else //TURN OFF THE TASK
                {
                    if(task_sd_active)
                    {
                        task_sd_active = false;
                    }
                }
                               
                last_known_black_box_enabled = black_box_enabled;
            }  
        }

        else if (aResult.uid() == "firebase_get_black_box_refresh_seconds")
        {            
            if(payload.toInt() > 0) black_box_logging_interval_milliseconds = payload.toInt();
            else 
            {
                Serial.printf("\n---ERROR on black_box_refresh_seconds : Value < 0 , defaulting to : %d\n", black_box_logging_interval_milliseconds_default);
                black_box_logging_interval_milliseconds = black_box_logging_interval_milliseconds_default ;//default on error
                firebase_get_black_box_refresh_seconds_needs_overwrite = true;
            }

            if(last_known_black_box_logging_interval_milliseconds != black_box_logging_interval_milliseconds)
            {
                //Firebase.printf("---Firebase value for leds_on: %s\n", leds_on ? "true" : "false");

                if(firebase_log_mode > firebase_log_mode_silent)
                {
                    Firebase.printf("\n---Firebase value for black_box_refresh_seconds changed to : %d\n", black_box_logging_interval_milliseconds);
                }                                      
                last_known_black_box_logging_interval_milliseconds = black_box_logging_interval_milliseconds;
            }            
        }

        else if (aResult.uid() == "firebase_get_oled_dev_screen_nr")
        {                       
            if((payload.toInt() > 0 && payload.toInt() < 6) || payload.toInt() == -1) 
            {
                oled_dev_screen_nr = payload.toInt();
            }          
            
            else 
            {
                Serial.printf("\n---ERROR on oled_dev_screen_nr : Unacceptabble Value , defaulting to : %d\n", oled_dev_screen_nr_default);
                oled_dev_screen_nr = oled_dev_screen_nr_default ;//default on error
                firebase_get_oled_dev_screen_nr_needs_overwrite = true;
            }

            if(last_known_oled_dev_screen_nr != oled_dev_screen_nr)
            {              
                
                if(firebase_log_mode > firebase_log_mode_silent)
                {
                    Firebase.printf("\n---Firebase value for oled_dev_screen_nr changed to : %d\n", oled_dev_screen_nr);
                }             
                
                //Turns OLED off
                if(payload.toInt() == -1) oled_needs_clear = true; 

                last_known_oled_dev_screen_nr = oled_dev_screen_nr;
            }            
        }

        else if (aResult.uid() == "firebase_get_hw_version")
        {
            if( payload.toInt() == hw_version)
            {
                if(log_enabled)Serial.printf("\n---hw_version : %d already synced with firebase\n", hw_version);
                hw_version_synced = true;
            }
            else
            {
                Serial.printf("\n---ERROR on hw_version, overwriting to version : %d\n", hw_version);
                firebase_get_hw_version_needs_overwrite = true;
            }
        }

        else if (aResult.uid() == "firebase_get_hw_variant")
        {
            //Fitering the String as it sometimes contain the quotes within the String and this is wrong 
            if (payload.startsWith("\"") && payload.endsWith("\"")) {payload = payload.substring(1, payload.length() - 1);}

            if(payload == hw_variant_string)
            {
                if(log_enabled)Serial.printf("\n---hw_variant : %s already synced with firebase\n", hw_variant_string.c_str());
                hw_variant_synced = true;
            }
            else
            {
                Serial.printf("\n---ERROR on hw_variant, overwriting to version : %s\n", hw_variant_string.c_str());
                firebase_get_hw_variant_needs_overwrite = true;
            }
        }
		*/

/**
 * ABOUT:
 *
 * The example to upload object (file) to Storage bucket.
 *
 * This example uses the UserAuth class for authentication.
 * See examples/App/AppInitialization for more authentication examples.
 *
 * The complete usage guidelines, please read README.md or visit https://github.com/mobizt/FirebaseClient
 *
 * SYNTAX:
 *
 * 1.------------------------
 *
 * FileConfig::FileConfig(<file_name>, <file_callback>);
 *
 * <file_name> - The filename included path of file that will be used.
 * <file_callback> - The callback function that provides file operation.
 *
 * The file_callback function parameters included the File reference returned from file operation, filename for file operation and file_operating_mode.
 * The file_operating_mode included file_mode_open_read, file_mode_open_write, file_mode_open_append and file_mode_open_remove.
 *
 * The file name can be a name of source (input) and target (output) file that used in upload and download.
 *
 * 2.------------------------
 *
 * Storage::upload(<AsyncClient>, <FirebaseStorage::Parent>, <file_config_data>, <MIME>, <AsyncResultCallback>, <uid>);
 *
 * <AsyncClient> - The async client.
 * <FirebaseStorage::Parent> - The FirebaseStorage::Parent object included Storage bucket Id and object in its constructor.
 * <file_config_data> - The filesystem data (file_config_data) obtained from FileConfig class object.
 * <MIME> - The MIME type of file to be upload.
 * <AsyncResultCallback> - The async result callback (AsyncResultCallback).
 * <uid> - The user specified UID of async result (optional).
 *
 * The bucketid is the Storage bucket Id of object to upload.
 * The object is the object to be stored in the Storage bucket.
 */

/*
#include <Arduino.h>
#include <FirebaseClient.h>
#include "ExampleFunctions.h" // Provides the functions used in the examples.
#include <tools.h>

#define WIFI_SSID "Not_Your_Hotspot"
#define WIFI_PASSWORD "wifirocks"

#define API_KEY "AIzaSyDPxQ-3--VDEGU37vpG7FdTiweSYohak68"
#define USER_EMAIL "cesar.gc@outlook.com"
#define USER_PASSWORD "firebase"   

// Define the Firebase storage bucket ID e.g bucket-name.appspot.com */
/*
#define STORAGE_BUCKET_ID "engel-dev-61ef3.firebasestorage.app"

void processData(AsyncResult &aResult);

FileConfig media_file("/media.mp4", file_operation_callback); // Can be set later with media_file.setFile("/media.mp4", file_operation_callback);

UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD, 3000 /* expire period in seconds (<3600) *//*);

FirebaseApp app;

SSL_CLIENT ssl_client;

// This uses built-in core WiFi/Ethernet for network connection.
// See examples/App/NetworkInterfaces for more network examples.
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);

Storage storage;

bool taskComplete = false;

AsyncResult storageResult;

*/

// Generic Deps.
#include <Arduino.h>

// Custom Deps.
#include <vars.h>
#include <tools.h>
#include <serial.h>
#include <fuel_gauge.h>
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
#include <nvs.h>
#include <firebase.h>
//#include <wifi_demo.h>
//#include <ble.h>
//#include <listener.h>
//#include <mqtt.h>


//Needed by Firebase 
//(will not compile othersiwe)
#include <LittleFS.h>
#include <SD.h>
#include <WiFi.h>
#include <Update.h>

#include <lte.h>

#include <gps.h>

#include <logger.h>

#include <storage_via_wifi.h>

//#include "esp_task_wdt.h"



// Previously declared on Vars.h
bool i2c_initialized = false;
bool serial_initialized = false;
bool gpios_initialized = false;
bool interrupts_initialized = false;
bool lux_initialized = false;
bool oled_initialized = false;
bool rtc_initialized = false;

bool lora_enabled = false;

bool log_enabled = true;
bool oled_enabled = true;

bool rgb_leds_initialized = false;
int rgb_leds_brightness = 100; //max 255

int board_temp = 0;
float board_temp_max_threshold = 50.0;
float board_temp_min_threshold = 40.0;

int lux_val = 0;
int lux_low_threshold = 100;
int lux_int_persistance_counts = 4;

bool reg_5v_enabled = false;

bool lvl_shftr_enabled = false;

bool ff2_q_status = false;

int demos_total = 7; // including exit option

bool movement_detected = false;
bool moving = false;

button btn_1 = {esp_btn_1_pin, 0, false};
button btn_2 = {esp_btn_2_pin, 0, false};

bool demo_is_ongoing = false;

int boot_mode = 0;
int current_mode = 0;

// TODO if works change all initialization vars here to their respective .h or .cpp

// int imu_mode=0;

// GPIO Pins (Just Init with -1)
// To be set after get_hw_version();
// NEW
int esp_int_gpio_exp_pin = -1;
int esp_reg_5v_en_pin = -1;
// OLD
int esp_buzzer_pin = -1;
int esp_lora_vcc_en_pin = -1;
int esp_nrf_reset_pin = -1;
int esp_lora_reset_pin = -1;
int esp_int_charger_pin = -1;

// I2c Bus Vars
int imu_needed = 0;
int rgb_needed = 0;
int temp_needed = 0;
int lux_needed = 0;
int rtc_needed = 0;
int fuel_gauge_needed = 0;
int oled_needed = 0;

bool rgb_bypassed = false;

bool imu_running = false;
bool rgb_running = false;
bool temp_running = false;
bool lux_running = false;
bool rtc_running = false;
// bool fuelgauge_running  = false;
bool oled_running = false;


int oled_token = oled_free;




// BLE UUID
/*std::string uuid = "0000fef3-0000-1000-8000-00805f9b34fb";
// reference to ble service
BLE::BLEngelService *engelService = new BLE::BLEngelService();
BLE::Listener *listener;

std::map<std::string, std::string> cast_reference;
std::map<std::string, int*> int_map;
std::map<std::string, bool*> bool_map;
std::map<std::string, float*> float_map;
std::map<std::string, uint32_t*> uint32_map;
std::map<std::string, std::string*> string_map;*/



int non_critical_task_refresh_ms = 1000;

bool fuel_gauge_initialized = false;

// Charger Vars
bool charger_initialized = false;
bool charging = false;		// Based on Charging status Pin
bool charger_pgood = false; // Pin to indicate usb connected with 5V

int bat_percent = -1;
float bat_voltage = -1;
float bat_c_rate = -1;

float low_bat_threshold = 3.3; // in Volts

// GPIO_EXP Expected Inputs States-------------

bool int_usb = true;
bool int_charger = true;
bool int_rtc = false;
bool int_low_bat = true;
bool int_temp = true;
bool int_lux = true;

// GPIO EXP Derived Flags----------------------
bool debug_mode = false;
bool usb_connected = false;
// bool charging = false; Aready Declared but keep here for reference
bool rtc_alarm = false;
bool low_bat = false;
bool overheat = false;
bool dark = false;

bool gpio_exp_changed = false;

// Defining HW VARIANT through HW_ID resistors
bool hw_id_0 = false;
bool hw_id_1 = false;
bool hw_id_2 = false;
bool hw_id_3 = false;

// default is devel
int hw_variant = hw_variant_devel;

// Newest version by default
// Downgrade if necessary on get_hw_version()
int hw_version = 5;

bool test_is_charging = false;

int time_critical_tasks_running = 0;


//MQTT related
int mqtt_cycle_nr =0;

int current_internet_connection = -1 ; //Not connected

//Handle by NVS later
int backend_config = backend_config_firebase;  









bool task_ftp_wifi_running_first_loop = true;


void setup()
{
    init_all();

    oled_task_logger_sd_ftp_running();

    //create_task_i2c_manager(); //always running and shall not be kiled

    // wait(100);

    //Start the Firebase to upload the values
    //create_task_logger_sd_ftp();
    //wait(1000);

    task_ftp_wifi_running = true;
    

    wait(100);

}





void loop()
{
    //Uploading Files to Storage
    //Temporal : All this must rerplaced by the task () while the stack overflow is fixed
    if(task_ftp_wifi_running)
    {
        if(task_ftp_wifi_running_first_loop)
        {
            oled_starting_ftp_via_wifi();

            if(task_logger_sd_ftp_running)
            {
                task_logger_sd_ftp_running = false;
            }
            
            task_ftp_wifi_running_first_loop = false;
        }


        if(firebase_file_initialized && WiFi.status() == WL_CONNECTED) 
        {
            run_storage_via_wifi();
        }

        else
        {
            if(!firebase_file_initialized)
            {
                Serial.println("\n---Firebase need Initialization ---\n");  

                if(!firebase_file_init(logger_wifi_ssid , logger_wifi_password))
                {
                    //Serial.print("ERROR ON firebase_file_init() , retrrying in 5 seconds");
                    //wait(5000);
                    
                    Serial.print("ERROR ON firebase_file_init()");
                    task_ftp_wifi_running = false;
                    wait(100);
                }
            }

            //Already Initialized but got disconnected
            if(firebase_file_initialized && WiFi.status() != WL_CONNECTED)
            {
                Serial.print("\n---RECONNECTING TO WIFI---");

                if(!wifi_connect_to(logger_wifi_ssid,logger_wifi_password))
                {
                    Serial.print("ERROR : WIFI NOT CONNECTED , EXITING");
                    firebase_file_initialized = false;
                    //Turning Off the task upon wifi disconnecction (just 1 persistence loop)
                    task_ftp_wifi_running = false;
                    wait(1000);
                }
            } 
        }  

        if (!task_ftp_wifi_running && !task_logger_sd_ftp_running)
        {
            //Managed by task_ftp_wifi

            if(log_enabled) Serial.print(" \n---returning to menu!---");
                    
            create_task_logger_sd_ftp();

            //Resetting Flags
            task_ftp_wifi_running_first_loop = true;
        }
    }
    
    else wait(10);
}



