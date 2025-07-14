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



#define firebase_user "cesar.gc@outlook.com"
#define firebase_pass "firebase"   
#define firebase_api_key "AIzaSyDPxQ-3--VDEGU37vpG7FdTiweSYohak68"
#define firebase_url "https://engel-dev-61ef3-default-rtdb.europe-west1.firebasedatabase.app/"
#define firebase_url_for_lte "engel-dev-61ef3-default-rtdb.europe-west1.firebasedatabase.app"


#define WIFI_SSID "Not_Your_Hotspot"
#define WIFI_PASSWORD "wifirocks"

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

// ServiceAuth is required for Google Cloud Functions functions.
//ServiceAuth sa_auth(USER_EMAIL, "engel-dev-61ef3", PRIVATE_KEY, 3000 /* expire period in seconds (<= 3600) */);

//FileConfig media_file(SD_LOG_FILE_PATH, file_operation_callback2); // Can be set later with media_file.setFile("/image.png", file_operation_callback2);

File myFile;


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
    ssl_client2.setTimeout(1000);
    ssl_client2.setHandshakeTimeout(5);
    
    //set_ssl_client_insecure_and_buffer(ssl_client2);

    // Assign the valid time only required for authentication process with ServiceAuth and CustomAuth.
    //app2.setTime(get_ntp_time());

    Serial.println("Initializing app2...");
    initializeApp(aClient2, app2, getAuth(user_auth2), processData2, "authTask");
    // initializeApp(aClient, app, getAuth(user_auth), auth_debug_print, "authTask");

    //app2.getApp<CloudStorage>(cstorage);
    // Or intialize the app and wait.
    // initializeApp(aClient, app, getAuth(user_auth), 120 * 1000, auth_debug_print);

    app2.getApp<Storage>(storage);

    print_sd_log_folder_content();

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

    // For async call with AsyncResult.
    //processData2(cloudStorageResult);

    // For async call with AsyncResult.
    processData2(storageResult);
}

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
            Firebase.printf("Download task: %s, complete!\n", aResult.uid().c_str());
        }
    }

    if (aResult.uploadProgress())
    {
        Firebase.printf("Uploaded, task: %s, %d%s (%d of %d)\n", aResult.uid().c_str(), aResult.uploadInfo().progress, "%", aResult.uploadInfo().uploaded, aResult.uploadInfo().total);
        if (aResult.uploadInfo().total == aResult.uploadInfo().uploaded)
        {
            Firebase.printf("Upload task: %s, complete!\n", aResult.uid().c_str());
            Serial.print("Download URL: ");
            Serial.println(aResult.uploadInfo().downloadUrl);
        }
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


void uploadLogsFromSD() 
{     
    /*
    GoogleCloudStorage::UploadOptions options;
    options.mime = "text/plain";
    options.uploadType = GoogleCloudStorage::upload_type_resumable;
    // options.uploadType = GoogleCloudStorage::upload_type_simple;
    */


  File dir = SD.open("/logs");
  File entry = dir.openNextFile();
  while (entry) 
  {
    if (!entry.isDirectory() && String(entry.name()).endsWith(".txt")) 
    {
        String filePath = String("/logs/"); 
        filePath += entry.name();

        String firebasePath = String("/logs/");
        firebasePath += String(esp_id);
        firebasePath += String("/");
        firebasePath += entry.name();

        FileConfig logFile(filePath.c_str(), file_operation_callback2);

        //logFile.setFile(filePath.c_str(),file_operation_callback2);
        
        Serial.print("\nUploading file: ");Serial.print(filePath);
        Serial.print("\nFirebase Path: ");Serial.print(firebasePath);Serial.println("\n");
        Serial.printf("File Size: %d\n", entry.size());

        //cstorage.upload(aClient2,GoogleCloudStorage::Parent(STORAGE_BUCKET_ID, firebasePath.c_str()),getFile(media_file),"text/plain",processData2,"⬆️  logUpload");
        
        //Was using this one before
        //storage.upload(aClient2, FirebaseStorage::Parent(STORAGE_BUCKET_ID, firebasePath.c_str()), getFile(logFile), "text/plain", processData2, "⬆️  uploadTask");


        // Sync call which waits until the operation complete.
        bool status = storage.upload(aClient2, FirebaseStorage::Parent(STORAGE_BUCKET_ID, firebasePath.c_str()), getFile(logFile), "text/plain");
        if (status)Serial.println("Upload task(await), complete!");
        else Firebase.printf("Error, msg: %s, code: %d\n", aClient2.lastError().message().c_str(), aClient2.lastError().code());

        wait(5000);
        /*
        
        Serial.println("Uploading file...");

        // Async call with callback function.
        //cstorage.upload(aClient2, GoogleCloudStorage::Parent(STORAGE_BUCKET_ID, "media.mp4"), getFile(media_file), options, processData, "⬆️  uploadTask");

        // Async call with AsyncResult for returning result.
        //cstorage.upload(aClient2, GoogleCloudStorage::Parent(STORAGE_BUCKET_ID, "media.mp4"), getFile(media_file), options, cloudStorageResult);

        // Sync call which waits until the operation complete.
        
        bool status = cstorage.upload(aClient2, GoogleCloudStorage::Parent(STORAGE_BUCKET_ID, "media.mp4"), getFile(media_file), options);
        if (status)
            Serial.println("🔼 Upload task(await), complete!✅️");
        else
            Firebase.printf("Error, msg: %s, code: %d\n", aClient2.lastError().message().c_str(), aClient2.lastError().code());

        

        For the Normal Storage from the example 

        Serial.println("Uploading file...");

        // Async call with callback function.
        storage.upload(aClient, FirebaseStorage::Parent(STORAGE_BUCKET_ID, "media.mp4"), getFile(media_file), "video/mp4", processData, "⬆️  uploadTask");

        // Async call with AsyncResult for returning result.
        storage.upload(aClient, FirebaseStorage::Parent(STORAGE_BUCKET_ID, "media.mp4"), getFile(media_file), "video/mp4", storageResult);

        // Sync call which waits until the operation complete.
        bool status = storage.upload(aClient, FirebaseStorage::Parent(STORAGE_BUCKET_ID, "media.mp4"), getFile(media_file), "video/mp4");
            if (status)
            Serial.println("🔼 Upload task(await), complete!✅️");
        else
            Firebase.printf("Error, msg: %s, code: %d\n", aClient.lastError().message().c_str(), aClient.lastError().code());


        */
    }
    entry = dir.openNextFile();
  }
}