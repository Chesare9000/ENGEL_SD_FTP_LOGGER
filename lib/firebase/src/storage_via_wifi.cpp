////// TO UPLOAD FILES TO THE FIREBASE STORAGE /LOG ---------------------------------------------------------------------------------------------------------------


// Taken from https://randomnerdtutorials.com/esp32-cam-save-picture-firebase-storage/
// And https://registry.platformio.org/libraries/mobizt/FirebaseClient/examples/CloudStorage/Upload/Upload.ino

//This will make a completely standalone firebase connection 

//This will work just on WIFI at the moment , later update to lte as well

#define ENABLE_USER_AUTH
#define ENABLE_STORAGE
#define ENABLE_FS

#include <FirebaseClient.h>
//#include <ExampleFunctions.h> // Provides the functions used in the examples.

#include <wifi.h>
#include <sd.h>
#include <storage_via_wifi.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <tools.h>
#include <vars.h>
#include <oled.h>



#define firebase_user "cesar.gc@outlook.com"
#define firebase_pass "firebase"   
#define firebase_api_key "AIzaSyDPxQ-3--VDEGU37vpG7FdTiweSYohak68"
#define firebase_url "https://engel-dev-61ef3-default-rtdb.europe-west1.firebasedatabase.app/"
#define firebase_url_for_lte "engel-dev-61ef3-default-rtdb.europe-west1.firebasedatabase.app"


//#define WIFI_SSID "Not_Your_Hotspot"
//#define WIFI_PASSWORD "wifirocks"

#define WIFI_SSID "cesar"
#define WIFI_PASSWORD "cesar1234"


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

// Firebase components
FirebaseApp app2;

WiFiClientSecure ssl_client2;
//SSL_CLIENT ssl_client2;

using AsyncClient = AsyncClientClass;
AsyncClient aClient2(ssl_client2);

//CloudStorage cstorage;

Storage storage;

bool taskComplete = false;

bool firebasse_file_initialized = false;

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


bool firebase_file_init()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        if(!wifi_connect_to(WIFI_SSID,WIFI_PASSWORD))
        {
            Serial.print("ERROR : WIFI NOT CONNECTED , EXITING");
            firebasse_file_initialized = false;
            return false;;
        }
    } 

    
    
    Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

    // Configure SSL client
    ssl_client2.setInsecure();
    ssl_client2.setTimeout(5000);
    ssl_client2.setHandshakeTimeout(5);
    
    //set_ssl_client_insecure_and_buffer(ssl_client2);

    // Assign the valid time only required for authentication process with ServiceAuth and CustomAuth.
    //app2.setTime(get_ntp_time());

    Serial.println("Initializing app2...");
    initializeApp(aClient2, app2, getAuth(user_auth2), processData2, "authTask");

    //app2.getApp<CloudStorage>(cstorage);
    // Or intialize the app and wait.
    // initializeApp(aClient, app, getAuth(user_auth), 120 * 1000, auth_debug_print);

    app2.getApp<Storage>(storage);

    total_files_on_sd = print_sd_log_folder_content();

    // Ensure the chip ID is read if it hasn't been updated
    if (!esp_id) 
    {
        get_esp_id();
        wait(100);
    }    

    firebasse_file_initialized = true;
    return true;

}

void run_storage_via_wifi()
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
    //processData2(cloudStorageResult);

    // For async call with AsyncResult.
    //processData2(storageResult);
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



void uploadLogsFromSD() 
{
  File dir = SD.open("/logs");
  if (!dir) 
  {
    Serial.println("Failed to open /logs directory");
    return;
  }

  File entry = dir.openNextFile();
  int fileCounter = 0;

  while (entry) 
  {
    if (!entry.isDirectory() && String(entry.name()).endsWith(".txt")) 
    {
      String filePath = String("/logs/") + entry.name();
      std::vector<String> partFiles;

      String mimeType = "text/plain";

      splitFileIntoChunksIfNeeded(filePath, partFiles);

      for (size_t i = 0; i < partFiles.size(); i++) 
      {
        String partPath = partFiles[i];
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
            break;
          }
          freshFile.close(); // just to check availability

          oled_logger_uploading(fileCounter + 1, total_files_on_sd);

          Serial.printf("\nUploading file: %s\nFirebase Path: %s\n", partPath.c_str(), firebasePath.c_str());

          bool status = storage.upload(aClient2, FirebaseStorage::Parent(STORAGE_BUCKET_ID, firebasePath.c_str()), getFile(logFile), mimeType);
          
          if (status) 
          {
            if (aClient2.lastError().code() == 0) 
            {
                Serial.println("✅ Upload complete!");
                uploadSuccess = true;

                // Only delete if Firebase confirms full size upload
                if (partPath.indexOf("_part") != -1) 
                {
                    SD.remove(partPath.c_str());
                    Serial.printf("🧹 Deleted chunk file: %s\n", partPath.c_str());
                }
            } 
            else 
            {
                Serial.printf("⚠️ Upload reported success but had error: %s\n", aClient2.lastError().message().c_str());
            }
          } 
          else 
          {
            Firebase.printf("❌ Upload failed. Msg: %s, Code: %d\n", aClient2.lastError().message().c_str(), aClient2.lastError().code());
          }
        }

        if (!uploadSuccess) 
        {
          Serial.printf("🚫 Failed to upload %s after retries.\n", partPath.c_str());
        }

        fileCounter++;
      }
    }

    entry = dir.openNextFile();
  }

  dir.close();
  Serial.printf("🟢 Finished uploading all log files.\n");

}