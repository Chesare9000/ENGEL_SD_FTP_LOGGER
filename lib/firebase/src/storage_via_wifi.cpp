//TODO , later save file size to better update last chunk to preserve all data , but this is for a later more specific update



////// TO UPLOAD FILES TO THE FIREBASE STORAGE /LOG ---------------------------------------------------------------------------------------------------------------


// Taken from https://randomnerdtutorials.com/esp32-cam-save-picture-firebase-storage/
// And https://registry.platformio.org/libraries/mobizt/FirebaseClient/examples/CloudStorage/Upload/Upload.ino

//This will make a completely standalone firebase connection 

//This will work just on WIFI at the moment , later update to lte as well

#define ENABLE_USER_AUTH
#define ENABLE_STORAGE
#define ENABLE_FS

#include <FirebaseClient.h>
#include <set>
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


//TODO if i add logger it crashes , check if stak is not enough or what happen
//as this is detainig me frrom onvert the storage upload to a task 
//#include <logger.h>



#define firebase_user "cesar.gc@outlook.com"
#define firebase_pass "firebase"   
#define firebase_api_key "AIzaSyDPxQ-3--VDEGU37vpG7FdTiweSYohak68"
#define firebase_url "https://engel-dev-61ef3-default-rtdb.europe-west1.firebasedatabase.app/"
#define firebase_url_for_lte "engel-dev-61ef3-default-rtdb.europe-west1.firebasedatabase.app"


//#define WIFI_SSID "Not_Your_Hotspot"
//#define WIFI_PASSWORD "wifirocks"

//#define WIFI_SSID "wifi"
//#define WIFI_PASSWORD "wifi1234"


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

bool firebase_file_initialized = false;

bool delete_invalid_files_on_sd = false;

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



void firebase_file_deinit()
{
  Serial.println("Deinitializing Firebase...");

  //storage.reset();  // Optional: clear Storage object
  storage.resetApp();
  // Close the SSL client completely
  ssl_client2.stop();

  // Manually reset the internal AsyncClient state
  // Mark global flags
  firebase_file_initialized = false;

  // Reset the Firebase App's token and state manually
  app2.getApp<Storage>(storage);  // optional cleanup
  //app2.stop();
  Serial.println("[Firebase] Deinit done.");
}

bool firebase_file_init()
{
    if (WiFi.status() != WL_CONNECTED)
    {
      
        if(!wifi_connect_to(logger_wifi_ssid,logger_wifi_password))
        {
            Serial.print("ERROR : WIFI COULDNT CONNECT,EXITING");
            oled_logger_wifi_failed(); if(!i2c_manager_running)oled_refresh();
           
            buzzer_error();
            firebase_file_initialized = false;
            ftp_wifi_running = false;
            return false;
        }
      /*
      Serial.print("ERROR : WIFI NOT CONNECTED , EXITING");
      oled_logger_wifi_failed();
      buzzer_error();
      firebase_file_initialized = false;
      ftp_wifi_running = false;
      wait(1000);
      return false;
      */
    } 

    oled_ftp_wifi_connecting_to_database(); if(!i2c_manager_running)oled_refresh();


    Serial.println("\n---Initializing Firebase---\n");    
    
    Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

    // Configure SSL client
    ssl_client2.setInsecure();
    //ssl_client2.setTimeout(5000);
    //ssl_client2.setHandshakeTimeout(5);
    
    //set_ssl_client_insecure_and_buffer(ssl_client2);

    // Assign the valid time only required for authentication process with ServiceAuth and CustomAuth.
    //app2.setTime(get_ntp_time());

    Serial.println("Initializing app2...");
    //initializeApp(aClient2, app2, getAuth(user_auth2), processData2, "authTask");

    //app2.getApp<CloudStorage>(cstorage);
    // Or intialize the app and wait.
    initializeApp(aClient2, app2, getAuth(user_auth2), 120 * 1000);

    app2.getApp<Storage>(storage);

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

void run_storage_via_wifi()
{
    // To maintain the authentication process.
    app2.loop();

    if (app2.ready() && !taskComplete) 
    {
        taskComplete = true;
        wait(10);
        processAndUploadLogs(delete_invalid_files_on_sd);
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
        oled_sd_error();if(!i2c_manager_running)oled_refresh();
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




String get_current_date_str()
{
    char date_buf[16];
    snprintf(date_buf, sizeof(date_buf), "%04d-%02d-%02d", year, month, day);
    wait(100);
    return String(date_buf);
}




bool getExistingFilesFromStorage(const String &subFolderPath, std::set<String> &existingFiles) {
    Serial.println("[STEP 1] Requesting file list from Firebase Storage...");
    Serial.flush();
    wait(100);
    oled_ftp_wifi_connecting_to_database();
    if(!i2c_manager_running) oled_refresh();

    String result = storage.list(aClient2, FirebaseStorage::Parent(STORAGE_BUCKET_ID));
    wait(200);

    if (aClient2.lastError().code() != 0) {
        Serial.printf("❌ [ERROR] Could not list storage files. Msg: %s | Code: %d\n",
                      aClient2.lastError().message().c_str(),
                      aClient2.lastError().code());
        buzzer_alarm();
        oled_ftp_wifi_error("LIST");
        if(!i2c_manager_running) oled_refresh();
        wait(500);
        return false;
    }

    Serial.printf("[INFO] Received listing: %d bytes\n", result.length());
    Serial.printf("[INFO] Filtering entries under subfolder: '%s'\n", subFolderPath.c_str());
    Serial.flush();
    wait(200);

    int idx = 0;
    size_t countAll = 0;
    size_t countMatched = 0;

    while (true) {
        idx = result.indexOf("\"name\":", idx);
        if (idx == -1) break;
        countAll++;

        idx = result.indexOf("\"", idx + 7);
        int endIdx = result.indexOf("\"", idx + 1);
        if (endIdx == -1) break;

        String fullPath = result.substring(idx + 1, endIdx);

        if (fullPath.startsWith(subFolderPath)) {
            String filename = fullPath.substring(fullPath.lastIndexOf("/") + 1);

            if (filename.endsWith(".tx")) {
                Serial.printf("[WARN] Firebase bug fix: '%s' → '%s'\n",
                              filename.c_str(), (filename + "t").c_str());
                filename += "t";
            }

            existingFiles.insert(filename);
            countMatched++;
        }

        idx = endIdx + 1;
        wait(10);
    }

    Serial.printf("✅ [DONE] Found %d files total, %d match '%s'\n",
                  countAll, countMatched, subFolderPath.c_str());
    Serial.flush();
    wait(300);

    return true;
}


bool uploadChunkToFirebase(const String &localPath, const String &remoteFolder) {
    Serial.printf("[STEP] Preparing to upload chunk: %s\n", localPath.c_str());
    Serial.flush();
    wait(100);

    String fileName = localPath.substring(localPath.lastIndexOf("/") + 1);
    oled_print_uploader_status("Uploading Chunk", fileName, "");
    if (!i2c_manager_running) oled_refresh();

    if (!SD.exists(localPath)) {
        Serial.printf("❌ [ERROR] Chunk not found on SD: %s\n", localPath.c_str());
        buzzer_error();
        oled_print_uploader_status("Error: Missing", fileName, "SD not found");
        if (!i2c_manager_running) oled_refresh();
        wait(200);
        return false;
    }

    File sdFile = SD.open(localPath, FILE_READ);
    if (!sdFile) {
        Serial.printf("❌ [ERROR] Cannot open file for upload: %s\n", localPath.c_str());
        buzzer_error();
        oled_print_uploader_status("Error: SD Open", fileName, "");
        if (!i2c_manager_running) oled_refresh();
        wait(200);
        return false;
    }

    String fullRemotePath = remoteFolder + "/" + fileName;
    Serial.printf("[INFO] Upload target: Firebase → %s\n", fullRemotePath.c_str());
    Serial.flush();
    wait(200);

    FileConfig chunkFile(localPath.c_str(), file_operation_callback2);
    wait(100);

    Serial.println("[STEP] Initiating synchronous upload...");
    Serial.flush();
    wait(100);

    bool status = storage.upload(
        aClient2,
        FirebaseStorage::Parent(STORAGE_BUCKET_ID, fullRemotePath.c_str()),
        getFile(chunkFile),
        "text/plain"
    );

    sdFile.close();
    wait(150);

    if (!status || aClient2.lastError().code() != 0) {
        Serial.printf("❌ [FAIL] Upload failed: %s | Code: %d\n",
                      aClient2.lastError().message().c_str(),
                      aClient2.lastError().code());
        buzzer_alarm();
        oled_print_uploader_status("Upload Failed", fileName,
                                   "Code: " + String(aClient2.lastError().code()));
        if (!i2c_manager_running) oled_refresh();
        wait(300);
        return false;
    }

    Serial.printf("✅ [SUCCESS] Uploaded: %s\n", fileName.c_str());
    buzzer_success();
    oled_print_uploader_status("Upload Done", fileName, "");
    if (!i2c_manager_running) oled_refresh();
    wait(200);

    // Delete chunk after upload
    if (localPath.indexOf("_part") != -1) {
        if (SD.remove(localPath)) {
            Serial.printf("[CLEANUP] Deleted uploaded chunk: %s\n", localPath.c_str());
        } else {
            Serial.printf("[WARN] Could not delete chunk: %s\n", localPath.c_str());
        }
        Serial.flush();
        wait(200);
    }

    return true;
}



// === New: Clean Logs Folder Function ===
void cleanLogsFolder(bool deleteInvalid) {
    File dir = SD.open("/logs");
    if (dir && dir.isDirectory()) {
        int removedCount = 0;
        int ignoredCount = 0;

        while (true) {
            File entry = dir.openNextFile();
            if (!entry) break;

            if (!entry.isDirectory()) {
                String name = entry.name();
                bool mustDelete = false;
                bool isPartFile = (name.indexOf("_part") != -1);

                // Check valid format: YYYY-MM-DD.txt
                bool validFormat =
                    name.startsWith("202") &&
                    name.endsWith(".txt") &&
                    name.length() == 14;

                if (isPartFile) {
                    // Always delete chunk leftovers
                    mustDelete = true;
                } else if (!validFormat) {
                    if (deleteInvalid) {
                        mustDelete = true;  // Delete invalid non-part files
                    } else {
                        Serial.printf("⚠️ [IGNORE] Invalid log file left untouched: %s\n", name.c_str());
                        ignoredCount++;
                    }
                }

                if (mustDelete) {
                    String fullPath = "/logs/" + name;
                    if (SD.remove(fullPath)) {
                        Serial.printf("🧹 Removed file: %s\n", fullPath.c_str());
                        removedCount++;
                        buzzer_heartbeat_tick();
                    } else {
                        Serial.printf("[WARN] Failed to delete: %s\n", fullPath.c_str());
                    }
                    wait(50);
                }
            }
            entry.close();
        }

        dir.close();
        Serial.printf("[INFO] Total files removed: %d | Ignored: %d\n", removedCount, ignoredCount);
    } else {
        Serial.println("[WARN] '/logs' directory not found.");
        buzzer_warning();
        oled_logger_error_on_sd();
        if (!i2c_manager_running) oled_refresh();
    }
}

// === processAndUploadLogs Updated ===
void processAndUploadLogs(bool deleteInvalidFiles = false) 
{
    Serial.println("=== [START] processAndUploadLogs ===");
    Serial.flush();
    buzzer_attention();

    oled_print_uploader_status("Starting FTP", "", "");
    if (!i2c_manager_running) oled_refresh();
    wait(200);

    String todayStr = get_current_date_str();
    Serial.printf("[INFO] Current date: %s\n", todayStr.c_str());
    wait(200);

    String remoteFolder = "logs/" + String(esp_id);
    Serial.printf("[INFO] Remote folder path: %s\n", remoteFolder.c_str());
    wait(200);

    // STEP 1: Clean leftover chunks + invalid files
    Serial.println("[STEP 1] Cleaning leftover and invalid files...");
    buzzer_status_ping();
    oled_print_uploader_status("Cleaning SD Logs", "", "");
    if (!i2c_manager_running) oled_refresh();
    wait(200);

    {
        File dir = SD.open("/logs");
        if (dir && dir.isDirectory()) {
            int removedCount = 0;
            int ignoredCount = 0;
            while (true) {
                File entry = dir.openNextFile();
                if (!entry) break;

                if (!entry.isDirectory()) {
                    String name = entry.name();
                    bool removeThis = false;
                    bool isLeftoverChunk = (name.indexOf("_part") != -1);

                    // Determine if file is invalid format
                    bool invalidFormat = 
                        !name.endsWith(".txt") ||
                        name.length() != 14 ||
                        !name.startsWith("202");

                    if (isLeftoverChunk) {
                        removeThis = true; // always remove leftover chunks
                    } else if (invalidFormat) {
                        if (deleteInvalidFiles) {
                            removeThis = true;
                        } else {
                            removeThis = false; // ignore if flag is false
                        }
                    }

                    if (removeThis) {
                        String fullPath = "/logs/" + name;
                        if (SD.remove(fullPath)) {
                            Serial.printf("🧹 Removed invalid file: %s\n", fullPath.c_str());
                            removedCount++;
                            buzzer_heartbeat_tick();
                        } else {
                            Serial.printf("[WARN] Failed to delete: %s\n", fullPath.c_str());
                        }
                        wait(50);
                    } else if (invalidFormat) {
                        Serial.printf("⚠️ [IGNORE] Invalid log file left untouched: %s\n", name.c_str());
                        ignoredCount++;
                    }
                }
                entry.close();
            }
            dir.close();
            Serial.printf("[INFO] Total invalid/leftover files removed: %d | Ignored: %d\n", removedCount, ignoredCount);
        } else {
            Serial.println("[WARN] '/logs' directory not found.");
            buzzer_warning();
            oled_logger_error_on_sd();
            if (!i2c_manager_running) oled_refresh();
        }
        wait(200);
    }

    // STEP 2: Fetch remote files
    Serial.println("[STEP 2] Fetching remote storage list...");
    buzzer_sonar_ping();
    oled_print_uploader_status("Fetching Remote", "", "");
    if (!i2c_manager_running) oled_refresh();
    wait(200);

    std::set<String> existingRemoteFiles;
    if (!getExistingFilesFromStorage(remoteFolder, existingRemoteFiles)) {
        Serial.println("❌ [ERROR] Could not fetch remote list. Restarting...");
        buzzer_alarm();
        oled_print_uploader_status("Fetch Failed", "", "");
        if (!i2c_manager_running) oled_refresh();
        wait(2000);
        ESP.restart();
    }

    Serial.printf("[INFO] Remote files count: %d\n", existingRemoteFiles.size());
    wait(300);

    // STEP 3: Process SD logs
    Serial.println("[STEP 3] Checking SD log files...");
    buzzer_status_ping();
    wait(200);

    File dir = SD.open("/logs");
    if (!dir || !dir.isDirectory()) {
        Serial.println("❌ [ERROR] /logs directory missing");
        buzzer_alarm();
        oled_logger_error_on_sd();
        if (!i2c_manager_running) oled_refresh();
        wait(2000);
        ESP.restart();
    }

    int processedFiles = 0;
    while (true) {
        File fileEntry = dir.openNextFile();
        if (!fileEntry) break;

        if (!fileEntry.isDirectory()) {
            String fileName = fileEntry.name();

            // Accept only valid YYYY-MM-DD.txt files
            bool validFormat =
                fileName.startsWith("202") &&
                fileName.endsWith(".txt") &&
                fileName.length() == 14;

            if (!validFormat) {
                Serial.printf("⚠️ [SKIP] Ignoring non-log file: %s\n", fileName.c_str());
                fileEntry.close();
                continue;  // Skip tmp.txt, firebase_latest.txt, etc.
            }

            processedFiles++;
            String filePath = "/logs/" + fileName;
            bool isToday = (fileName.substring(0, 10) == todayStr);
            size_t maxPartSize = 1024 * 1024 * 3;
            size_t totalSize = fileEntry.size();

            Serial.printf("\n[FILE] #%d: %s (%.2f MB) | Today: %s\n",
                          processedFiles,
                          fileName.c_str(),
                          totalSize / (1024.0 * 1024.0),
                          isToday ? "YES" : "NO");
            wait(300);

            fileEntry.close();
            wait(100);

            if (totalSize <= maxPartSize) {
                if (existingRemoteFiles.find(fileName) != existingRemoteFiles.end() && !isToday) {
                    Serial.printf("⏩ [SKIP] %s already exists on Firebase.\n", fileName.c_str());
                    buzzer_notification();
                    oled_print_uploader_status("Skipping File", fileName, "Exists");
                    if (!i2c_manager_running) oled_refresh();
                    wait(200);
                    continue;
                }

                Serial.printf("[ACTION] Uploading small file: %s\n", fileName.c_str());
                buzzer_attention();
                oled_print_uploader_status("Uploading File", fileName, "");
                if (!i2c_manager_running) oled_refresh();
                wait(200);

                if (!uploadChunkToFirebase(filePath, remoteFolder)) {
                    Serial.printf("🚨 [ERROR] Direct upload failed: %s\n", fileName.c_str());
                    buzzer_alarm();
                    oled_print_uploader_status("Upload Failed", fileName, "");
                    if (!i2c_manager_running) oled_refresh();
                    wait(2000);
                    ESP.restart();
                }

                Serial.printf("[OK] Small file uploaded: %s\n", fileName.c_str());
                buzzer_success();
                oled_print_uploader_status("Upload Done", fileName, "");
                if (!i2c_manager_running) oled_refresh();
                wait(300);
                continue;
            }

            size_t totalParts = (totalSize + maxPartSize - 1) / maxPartSize;
            size_t remoteCount = 0;

            String baseName = fileName.substring(0, fileName.lastIndexOf("."));
            for (const auto &r : existingRemoteFiles) {
                if (r.startsWith(baseName + "_part")) remoteCount++;
            }

            Serial.printf("[INFO] Remote parts: %d | Expected: %d\n", remoteCount, totalParts);
            wait(300);

            if (remoteCount == totalParts && !isToday) {
                Serial.printf("⏩ [SKIP] Large file fully uploaded: %s\n", fileName.c_str());
                buzzer_notification();
                oled_print_uploader_status("Skipping File", fileName, "Complete");
                if (!i2c_manager_running) oled_refresh();
                wait(200);
                continue;
            }

            Serial.printf("[ACTION] Splitting file: %s → %d parts\n", fileName.c_str(), totalParts);
            buzzer_attention();
            oled_print_uploader_status("Splitting File", fileName, String(totalParts) + " parts");
            if (!i2c_manager_running) oled_refresh();
            wait(500);

            File mainFile = SD.open(filePath, FILE_READ);
            if (!mainFile) {
                Serial.printf("❌ [ERROR] Cannot open large file: %s\n", filePath.c_str());
                buzzer_alarm();
                oled_logger_error_on_sd();
                if (!i2c_manager_running) oled_refresh();
                wait(2000);
                ESP.restart();
            }

            size_t partIndex = 1;
            while (mainFile.available()) {
                String partName = baseName + "_part" + String(partIndex) +
                                  "_of_" + String(totalParts) + ".txt";
                String tempPath = "/logs/tmp.txt";

                Serial.printf("[CHUNK] Creating %s\n", partName.c_str());
                oled_print_uploader_status("Creating Chunk", partName, "");
                if (!i2c_manager_running) oled_refresh();
                wait(200);

                File chunk = SD.open(tempPath, FILE_WRITE);
                if (!chunk) {
                    Serial.println("❌ [ERROR] Cannot create chunk file.");
                    buzzer_alarm();
                    oled_logger_error_on_sd();
                    if (!i2c_manager_running) oled_refresh();
                    mainFile.close();
                    wait(2000);
                    ESP.restart();
                }

                size_t written = 0;
                while (mainFile.available() && written < maxPartSize) {
                    String line = mainFile.readStringUntil('\n');
                    chunk.println(line);
                    written += line.length() + 1;
                }
                chunk.close();
                wait(100);

                String finalPath = "/logs/" + partName;
                if (SD.exists(finalPath)) SD.remove(finalPath);
                wait(100);
                SD.rename(tempPath, finalPath);
                wait(150);

                Serial.printf("[CHUNK] Saved: %s (%.2f KB)\n", partName.c_str(), written / 1000.0);
                buzzer_status_ping();
                oled_print_uploader_status("Uploading Chunk", partName,
                                           String(partIndex) + "/" + String(totalParts));
                if (!i2c_manager_running) oled_refresh();
                wait(300);

                if (!uploadChunkToFirebase(finalPath, remoteFolder)) {
                    Serial.printf("🚨 [ERROR] Upload failed: %s\n", finalPath.c_str());
                    buzzer_alarm();
                    oled_print_uploader_status("Upload Failed", partName, "");
                    if (!i2c_manager_running) oled_refresh();
                    wait(2000);
                    ESP.restart();
                }

                Serial.printf("[OK] Uploaded chunk %d/%d: %s\n", partIndex, totalParts, partName.c_str());
                buzzer_success();
                wait(300);

                partIndex++;
            }

            mainFile.close();
            Serial.printf("[DONE] Large file uploaded: %s\n", fileName.c_str());
            buzzer_success();
            oled_print_uploader_status("Upload Done", fileName, "");
            if (!i2c_manager_running) oled_refresh();
            wait(300);
        }
    }

    dir.close();

    Serial.printf("🟢 [COMPLETE] Uploaded %d files successfully.\n", processedFiles);
    buzzer_success();
    oled_print_uploader_status("All Done", "", "Restarting ESP");
    if (!i2c_manager_running) oled_refresh();
    wait(5000);

    Serial.println("[ACTION] Restarting ESP...");
    buzzer_shutdown();
    ESP.restart();
}