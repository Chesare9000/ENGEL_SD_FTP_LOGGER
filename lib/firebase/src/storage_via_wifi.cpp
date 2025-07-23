////// TO UPLOAD FILES TO THE FIREBASE STORAGE /LOG ---------------------------------------------------------------------------------------------------------------


// Taken from https://randomnerdtutorials.com/esp32-cam-save-picture-firebase-storage/
// And https://registry.platformio.org/libraries/mobizt/FirebaseClient/examples/CloudStorage/Upload/Upload.ino

//This will make a completely standalone firebase connection 

//This will work just on WIFI at the moment , later update to lte as well

#define ENABLE_USER_AUTH
#define ENABLE_STORAGE
#define ENABLE_FS

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


#define API_KEY firebase_api_key
#define USER_EMAIL firebase_user
#define USER_PASSWORD firebase_pass

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
    
    /*
    
    Serial.print("\n---CONNECTING TO WIFI---");

    if(!wifi_connect_to(ssid,password))
    {
        Serial.print("ERROR : WIFI NOT CONNECTED , EXITING");
        firebase_file_initialized = false;
        wait(1000);
        return false;
    }

    */
    
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

  Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

  // Add these checks:
  if (!ssl_client2) 
  {
    Serial.println("❌ SSL Client not initialized");
    return false;
  }
  
  ssl_client2.setInsecure();
  ssl_client2.setTimeout(5000);
  ssl_client2.setHandshakeTimeout(5);

  //Serial.printf("aClient2 ptr: %p\n", (void*)&aClient2);
  //Serial.printf("app2 ptr: %p\n", (void*)&app2);
  //Serial.printf("user_auth2 ptr: %p\n", (void*)&user_auth2);
  //Serial.printf("processData2 ptr: %p\n", (void*)processData2);
  
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
  
  wait(1000);
  
  Serial.println("Initializing app2...");
  //initializeApp(aClient2, app2, getAuth(user_auth2), processData2, "authTask");

  initializeApp(aClient2, app2, getAuth(user_auth2), 10000, processData2); // 10s

  Serial.println("app2 initialized...");
  wait(1000);
  
  if (!app2.ready()) 
  {
      Serial.println("❌ Firebase not ready. Aborting...");
      return false;
  }
  else  Serial.println("\n--Firebase ready---");

  wait(1000);

  //app2.getApp<CloudStorage>(cstorage);
  // Or intialize the app and wait.
  //initializeApp(aClient2, app2, getAuth(user_auth2), 10 * 1000 /* 10s timeout */ , processData2);

  if (app2.ready()) 
  {
    app2.getApp<Storage>(storage);
  } 
  else 
  {
    Serial.println("❌ App not ready, skipping storage init.");
    return false;
  }

  wait(1000);

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

