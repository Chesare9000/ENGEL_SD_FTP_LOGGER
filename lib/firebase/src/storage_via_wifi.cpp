////// TO UPLOAD FILES TO THE FIREBASE STORAGE /LOG ---------------------------------------------------------------------------------------------------------------


// Taken from https://randomnerdtutorials.com/esp32-cam-save-picture-firebase-storage/
// And https://registry.platformio.org/libraries/mobizt/FirebaseClient/examples/CloudStorage/Upload/Upload.ino

//This will make a completely standalone firebase connection 

//This will work just on WIFI at the moment , later update to lte as well

//#define ENABLE_USER_AUTH
//#define ENABLE_STORAGE
//#define ENABLE_FS

#define ENABLE_SERVICE_AUTH
#define ENABLE_CLOUD_STORAGE
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
#define FIREBASE_PROJECT_ID "engel-dev-61ef3"  

//#define WIFI_SSID "Not_Your_Hotspot"
//#define WIFI_PASSWORD "wifirocks"

#define WIFI_SSID "cesar"
#define WIFI_PASSWORD "cesar1234"


//#define API_KEY firebase_api_key
//#define USER_PASSWORD firebase_pass

// Define the Firebase storage bucket ID e.g bucket-name.appspot.com */
#define STORAGE_BUCKET_ID "engel-dev-61ef3.firebasestorage.app"

#define FIREBASE_CLIENT_EMAIL "firebase-adminsdk-zoh3a@engel-dev-61ef3.iam.gserviceaccount.com"
// Photo path in filesystem and photo path in Firebase bucket
//#define SD_LOG_FILE_PATH "/logs/2025-07-14.txt"
//#define BUCKET_PHOTO_PATH "/logs/15840448/2025-07-14.txt"

const char PRIVATE_KEY[] PROGMEM = 	"-----BEGIN PRIVATE KEY-----\nMIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCosJBMaCBaaigY\neWd1qZCz8nf9Vm8oqSOosIZBJH5z+qv8hvQT/MuRdc5MAAL9zSTjZ4fGfzgkwooL\nWVINGtcnFMt9hUuxRevy+4q1c3AgmyIv+8E0iLe/6oxPZnmIXqwAGieVRgWpiHyE\nRgTH424hDls1lkxFTxqVK/JU+6f1iGXi9M3N7uJKjAkDmoV7Sg3mfbvUbLL01F6M\nLKm/BA6q4EqQBzePVYR6tWgiLHTXXFcLEE6f0U9+UcDorEadUJQzRSpod57am03d\nvxXGGxbkxxskqRd8dKZ7yyHCMQnKNERjWNr+e+y8Qb6zhMA9CWrqEBWpHm8PzAxb\nE3Lftcr1AgMBAAECggEAM3a6bzUNOchNNzpCoBj9Nojwpm9qNkNzx5EjRFh347ZE\nJiSd7kMfb9868uDGCNw/NsltGNfTLxPSvaegarMXlKq5ci/qacjlNRoctULkoG3z\nviPWS8qyFHDgIZEn3rzTSKyyCs50B8bGBiy+ZKU0Ag25Md4KotKBT6U6p0andTaK\nv4dMYeivRGFaM+WQ4FzGV51scbpuWZg5WHZ7HDZlN3uSaGT5DciRgfudG24I53Gy\nB6GR2RRAo5OYmX9lms+uKay/6Lqxn4MFR0nWC1qRxnV+JvuWqbflI2//8iz60tUY\nnfkMXyVr+r4odNpeI+ZUOKPguUv8cb/MkhAa0OvmeQKBgQDZsEghMAeJrmtMc6YE\nleaxKq/FbIS+3ef8SmKwzQxCpBPKVCPgXu6MW4tTfq9rKXX8TcX7cgHFJ2hNP2jR\n6/xyz0+paqionxfjl+cc2RiU2Xaix5+UF38cyYr96MsW2Ftdx5Pl8OsQpEpv2RFs\n03DjbVb3MBFLvEKsYHPH7HyizwKBgQDGYLDgBYY6C9ikuy8O8Vn0h+0m5F+xqbMC\ntLfsq5QqMARg1t+o/vVogb1WITUgjbqZABSOMkPhM/RVMxwVLGH4wmNxZy0kc6Mb\neVoegK5y+UUx6R1QAjVxlRgsoWHj48ooaBJdZtzMNvmmCNdPpQ0EXQuO7T/4+X80\nXvIqGs22+wKBgQCJmsANkoHBS2ryGcyg62y3IyeW4WEMEdO6C/9UiVktqvADnqpL\nA1dBjACHV/ZlBAFb6oGyzz9FNALfiaylvcmHfXp6ujxA0shUCsqB1s2OEeTHQu6h\nuzSTqubcV9JA76TZo/XejSraCzAugYVdSE78xDoL6OuV9zwiIJovj1K9ywKBgAd5\nOxClhKIJMjc6ihJRC2eH+2o8rlI+J21Rq6Ax8poIRxHy5mgyesJeKOjmxV6dmEsN\nUrjssdv5Hhpbm5I8otBBWoe6MFwwMxPk5X1Csc/JDk9MDfumqabGzCtaRRrVyRbu\nnMqCBo13AL69lIb+m9fvPXE8BO33UFCDxzHI4rkjAoGAN5+uiqv8rRSsSzcm//9K\n3wrZd+gzEqU4JDTJvQ15he08b6D0QcQxYosHfXVZIQWwIY0boO2Zf3nRbq+2PS8T\nJjH2GWuCtLAlDBqQOD56pS67HaDdgy+tWxZsEvwlmBN10vtHC3S8Byf+F64KcQVI\nXxXNb6oqI34id10kx7Znskk=\n-----END PRIVATE KEY-----\n";

// User functions
void processData2(AsyncResult &aResult);
void file_operation_callback2(File &file, const char *filename, file_operating_mode mode);

//bool upload_completed = false;

// ServiceAuth is required for Google Cloud Functions functions.
ServiceAuth sa_auth2(FIREBASE_CLIENT_EMAIL, FIREBASE_PROJECT_ID , PRIVATE_KEY, 3000 /* expire period in seconds (<= 3600) */);

//FileConfig media_file(SD_LOG_FILE_PATH, file_operation_callback2); // Can be set later with media_file.setFile("/image.png", file_operation_callback2);

File myFile;


int total_files_on_sd = 0;

// Authentication
//UserAuth user_auth2(API_KEY, USER_EMAIL, USER_PASSWORD, 3000 /* expire period in seconds (<3600) */);

// Firebase components
FirebaseApp app2;

WiFiClientSecure ssl_client2;
//SSL_CLIENT ssl_client2;

using AsyncClient = AsyncClientClass;
AsyncClient aClient2(ssl_client2);

CloudStorage cstorage;

//Storage storage;

bool taskComplete = false;

bool firebasse_file_initialized = false;

AsyncResult cloudStorageResult;
//AsyncResult storageResult;


// Function to get NTP server time.
uint32_t get_ntp_time()
{
    uint32_t ts = 0;
    Serial.print("Getting time from NTP server... ");

    int max_try = 10, retry = 0;
    while (time(nullptr) < FIREBASE_DEFAULT_TS && retry < max_try)
    {
        configTime(3 * 3600, 0, "pool.ntp.org");
        unsigned long m = millis();
        while (time(nullptr) < FIREBASE_DEFAULT_TS && millis() - m < 10 * 1000)
        {
            delay(100);
            ts = time(nullptr);
        }
        Serial.print(ts == 0 ? " failed, retry... " : "");
        retry++;
    }
    ts = time(nullptr);

    Serial.println(ts > 0 ? "success" : "failed");
    return ts;
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
    app2.setTime(get_ntp_time());

    Serial.println("Initializing app2...");
    initializeApp(aClient2, app2, getAuth(sa_auth2), processData2, "authTask");
    //initializeApp(aClient2, app2, getAuth(user_auth2), processData2, "authTask");

    app2.getApp<CloudStorage>(cstorage);
    // Or intialize the app and wait.
    // initializeApp(aClient, app, getAuth(user_auth), 120 * 1000, auth_debug_print);

    //app2.getApp<Storage>(storage);

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
  GoogleCloudStorage::UploadOptions options;
  options.mime = "text/plain";
  //options.uploadType = GoogleCloudStorage::upload_type_resumable;
  options.uploadType = GoogleCloudStorage::upload_type_simple;  
        
  File dir = SD.open("/logs");
  if(!dir)
  {
    Serial.println("Failed to open /logs directory");
    return;
  }

  int files_counter = 0 ;

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

        // Retry logic
        int attempt = 0;
        bool uploadSuccess = false;

        while (attempt < 5 && !uploadSuccess) 
        {           
            attempt++;
            if(attempt > 1 ) Serial.printf("\nRetrying upload , attempt nr. %d --------------------------------\n", attempt);

            Serial.printf("\nUploading file nr. %d : ", files_counter + 1 ); Serial.print(filePath);
            Serial.print("\nFirebase Path: "); Serial.print(firebasePath);

            if(entry.size() > 1000000)Serial.printf("\nFile Size: %d MB \n", int(entry.size()/1000000));//MBytes
            else if(entry.size() > 1000)   Serial.printf("\nFile Size: %d KB \n", int(entry.size()/1000));   //KBytes
            else Serial.printf("\nFile Size: %d Bytes\n", entry.size()); //Bytes

            // REOPEN THE FILE FRESH FOR EACH ATTEMPT , so the file pointer is reset to the beginning.
            File fresh_file = SD.open(filePath.c_str(), FILE_READ);
            if (!fresh_file) 
            {
                Serial.println("\n---Error: Failed to reopen file for upload.");
                break; // Give up on this file
            }
            

            FileConfig logFile(filePath.c_str(), file_operation_callback2);

            oled_logger_uploading(files_counter + 1 , total_files_on_sd );

            // Sync call which waits until the operation complete.
            bool status = cstorage.upload(aClient2, GoogleCloudStorage::Parent(STORAGE_BUCKET_ID,firebasePath.c_str()), getFile(logFile), options);
            if (status)
            {
                Serial.println("\n---- Upload task complete!");
                uploadSuccess = true;
                wait(100);
            }
            else
            {
                Firebase.printf("\nError, msg: %s, code: %d\n", aClient2.lastError().message().c_str(), aClient2.lastError().code());  
                //storageResult.clear();
                //aClient2.stopAsync();    
                wait(100);
            }
            
            
            /*
            // Sync call which waits until the operation complete.
            bool status = storage.upload(aClient2, FirebaseStorage::Parent(STORAGE_BUCKET_ID, firebasePath.c_str()), getFile(logFile), "text/plain");
            if (status)
            {
                Serial.println("\n---- Upload task complete!");
                uploadSuccess = true;
                wait(100);
            }
            else
            {
                Firebase.printf("\nError, msg: %s, code: %d\n", aClient2.lastError().message().c_str(), aClient2.lastError().code());  
                storageResult.clear();
                aClient2.stopAsync();    
                wait(100);
            }

            */
           
        }

        if (!uploadSuccess) 
        {
            Serial.println("\nERROR : Max retry attempts reached. Moving to next file.");            
        }

        entry.close();
    }
    entry = dir.openNextFile();
    files_counter++;
  }
  dir.close();

  Serial.printf("\n ------Batch Upload Finished, Files uploaded : %d ----------------",files_counter);
}


/*
void uploadLogsFromSD() 
{
  GoogleCloudStorage::UploadOptions options;
  options.mime = "text/plain";
  options.uploadType = GoogleCloudStorage::upload_type_resumable;

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

          bool status = cstorage.upload(aClient2, GoogleCloudStorage::Parent(STORAGE_BUCKET_ID, firebasePath.c_str()), getFile(logFile), options);
          if (status) 
          {
            Serial.println("✅ Upload complete!");
            uploadSuccess = true;

            // If it's a chunk (not original), delete after upload
            if (partPath.indexOf("_part") != -1) 
            {
              SD.remove(partPath.c_str());
              Serial.printf("🧹 Deleted chunk file: %s\n", partPath.c_str());
            }

            wait(100);
          } 
          else 
          {
            Firebase.printf("❌ Upload failed. Msg: %s, Code: %d\n", aClient2.lastError().message().c_str(), aClient2.lastError().code());
            wait(100);
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



*/