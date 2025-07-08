#include "ble.h"
#include <ArduinoJson.h>
#include <thread>


BLE::BLEngelService::BLEngelService(std::string pserviceUUID, std::string pcharacteristicUUID) : 
  mServiceUUID(pserviceUUID),
  mCharacteristicUUID(pcharacteristicUUID),
  mScanForBeaconCallback(new BLE::EngelScanForBeaconCallback()){

  mBLEScan = BLEDevice::getScan(); //create new scan
  mBLEScan->setAdvertisedDeviceCallbacks(mScanForBeaconCallback);
  mBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  mBLEScan->setInterval(100);
  mBLEScan->setWindow(99); // less or equal setInterval value

}

void BLE::BLEngelService::begin() {
  Serial.println("Initializing BLE Device...");
  BLEDevice::init("Engel");
  Serial.println("Starting BLE work!");

  Serial.println("Creating BLE Server...");
  mServer = BLEDevice::createServer();
  mServer->setCallbacks(new BLE::EngelServerCallbacks(
    std::bind(&BLE::BLEngelService::onDeviceConnected, this),
    std::bind(&BLE::BLEngelService::onDeviceDisconnected, this)));
  Serial.println("BLE Server created!");

  Serial.println("Creating BLE Service...");
  mService = mServer->createService(mServiceUUID);
  Serial.println("Service defined!");

  Serial.println("Creating BLE Characteristic...");
  mChangesCharacteristic = mService->createCharacteristic(
                      mCharacteristicUUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  mChangesCharacteristic->setCallbacks(new BLE::EngelCharacteristicCallbacks(std::bind(&BLE::BLEngelService::onWriteReceived, this, std::placeholders::_1)));

  mDeviceInfoCharacteristic = mService->createCharacteristic(
                      "2A29",
                      BLECharacteristic::PROPERTY_READ
                    );
  
  std::vector<std::string> deviceInfo = {"bat_percent", "board_temp"};
  mDeviceInfoCharacteristic->setValue(createJSON(deviceInfo));

  Serial.println("Characteristic defined!");

  Serial.println("Starting BLE Service...");
  mService->start();
  Serial.println("Service started!");

  Serial.println("Setting up BLE Advertising...");
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(mServiceUUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  startAdvertising();
  Serial.println("Advertising started!");

  Serial.println("Characteristic defined! Now you can read it in your phone! Let's go!!!");
}


void BLE::BLEngelService::startAdvertising() {
  BLEDevice::startAdvertising();
}

// Task function
void initialNotification(void* pvParameters) {
    auto var_list = {"int", "bool", "float", "string"};
    // Cast the parameter to the correct type
    BLE::BLEngelService* service = static_cast<BLE::BLEngelService*>(pvParameters);

    // Delay for 5 seconds
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Execute the loop
    for (auto && var : var_list) {
      vTaskDelay(pdMS_TO_TICKS(500));
        service->sendAll(var);
    }

    // Delete the task when done
    vTaskDelete(NULL);
}

void BLE::BLEngelService::onDeviceConnected() {
  Serial.println("Device connected");
    deviceConnected = true;

  // Create the task and pass the service instance as a parameter
    xTaskCreate(
        initialNotification,      // Task function
        "Task",            // Name of the task
        8192,              // Stack size (in words, not bytes)
        this,              // Task input parameter
        1,                 // Priority of the task
        NULL               // Task handle
    );

}

void BLE::BLEngelService::onDeviceDisconnected() {
  Serial.println("Disconnected a Device!");
  deviceConnected = false;
  startAdvertising();
}

void BLE::BLEngelService::sendUpdate(bool notify) {
  if(deviceConnected){
    std::string jsonText = createJSON(changedKeys);
    //Serial.println("Sending update to client");
    //Serial.println(String(jsonText.c_str()));
    mChangesCharacteristic->setValue(jsonText);

    // check if we have manufacturer device updates
    std::vector<std::string> deviceInfo = {"bat_percent", "board_temp"};
    mDeviceInfoCharacteristic->setValue(createJSON(deviceInfo));

    if(notify){
      notifyClient();
    }
  }
}

void BLE::BLEngelService::onWriteReceived(String value) {
    JsonDocument doc;
    deserializeJson(doc, std::string(value.c_str()));

    // print the content of the JSON
    serializeJsonPretty(doc, Serial);

    // loop through all the keys
    for(auto && key : *m_cast_reference){
      if(doc.containsKey(key.first)){
        // this locks the mutex, so no other thread can access the shared resources in this scope
        std::lock_guard<std::mutex> lock(var_mutex);

        // here we update the values in the shared resources
        // possible types are : int, bool, double, float, string, char, uint32_t, uint8_t, uint16_t, uint64_t

        if(std::find(changedKeys.begin(), changedKeys.end(), key.first) == changedKeys.end()){
            changedKeys.push_back(key.first);
          }

        

        if(key.second == "int"){
          Serial.println(("int was set " + key.first + " : " + std::to_string(doc[key.first].as<int>())).c_str());
          *(*m_int_map)[key.first] = doc[key.first].as<int>();
          
        } else if(key.second == "float"){
          Serial.println(("float was set " + key.first + " : " + std::to_string(doc[key.first].as<float>())).c_str());
          *(*m_float_map)[key.first] = doc[key.first].as<float>();
        }
        else if(key.second == "bool"){
          Serial.println(("bool was set " + key.first + " : " + std::to_string(doc[key.first].as<bool>())).c_str());
          Serial.println("previous value: " + String(std::to_string(*(*m_bool_map)[key.first]).c_str()));
          *(m_bool_map->at(key.first)) = doc[key.first].as<bool>();
          Serial.println("new value: " + String(std::to_string(*(*m_bool_map)[key.first]).c_str()));
        }
        else if(key.second == "string"){
          //Serial.println(("string was set " + key.first + " : " + doc[key.first].as<std::string>()).c_str());
          //*(*m_string_map)[key.first] = doc[key.first].as<std::string>();
        }
        else if(key.second == "char"){
          Serial.println(("char was set " + key.first + " : " + std::to_string(doc[key.first].as<int>())).c_str());
          *(*m_char_map)[key.first] = static_cast<char>(doc[key.first].as<int>());
        }
        else{
          Serial.println("Unknown type");
          Serial.println(key.second.c_str());
        }
      }
    }

    // update the characteristic with the new values
    mChangesCharacteristic->setValue(createJSON(changedKeys));
}
    
std::string BLE::BLEngelService::createJSON(std::vector<std::string> &keys) {
  // will return a JSON string with all keys passed
  JsonDocument doc;

  //Serial.println("Creating JSON");
  /*for(auto && key : *m_cast_reference){
    Serial.println((key.first + " : " + key.second).c_str());

  }*/

  for( auto && key : keys){

    //Serial.println(key.c_str());
    //Serial.println((*m_cast_reference)[key].c_str());
    
    if((*m_cast_reference)[key] == "int"){
      /*Serial.println("int");
      for(auto && key : *m_int_map){
        Serial.println(key.first.c_str());
      }*/
      doc[key] = *(*m_int_map)[key];
    } 
    
    if((*m_cast_reference)[key] == "float"){
      doc[key] = *(*m_float_map)[key]; 
    } 
    
    if((*m_cast_reference)[key] == "bool"){
      doc[key] = *m_bool_map->at(key);
    } 
    
    if((*m_cast_reference)[key] == "string"){
      doc[key] = *m_string_map->at(key);
    }

    if((*m_cast_reference)[key] == "char"){
        doc[key] = static_cast<int>(*m_char_map->at(key));
    }

  }

  std::string output;
  serializeJson(doc, output);
  return output;

}

void BLE::BLEngelService::notifyClient() {
  if(deviceConnected){
    mChangesCharacteristic->notify();
  }
}

void BLE::BLEngelService::lookForBeacon(String UUID, std::function<void(bool)> callback, int minSignal){
  mScanForBeaconCallback->UUID = std::string(UUID.c_str());
  mScanForBeaconCallback->callback = callback;
  BLEScanResults foundDevices = mBLEScan->start(2, false); // scan for 2 seconds
  Serial.println("Scan done!");
  mBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
}

 void BLE::BLEngelService::isDeviceUnlocked(std::function<void(bool)> callback){
  //std::map<std::string, std::shared_ptr<std::string>> *m_string_map;
  auto it = m_string_map->find("lockKey");
  if(it == m_string_map->end()){
    //throw std::runtime_error("No target key UUID set, but trying to scan for beacon");
    callback(false);
  }

  auto uuid = it->second;

  // scan for beacons
  lookForBeacon(String(uuid->c_str()), callback, -50);

 }

 void BLE::BLEngelService::sendAll(std::string mapType){

  if(!deviceConnected){
    return;
  }

  if(mapType == "int"){
    for(auto && key : *m_int_map){
      changedKeys.push_back(key.first);
    }
  } else if(mapType == "float"){
    for(auto && key : *m_float_map){
      changedKeys.push_back(key.first);
    }
  } else if(mapType == "bool"){
    for(auto && key : *m_bool_map){
      changedKeys.push_back(key.first);
    }
  } else if(mapType == "string"){
    for(auto && key : *m_string_map){
      changedKeys.push_back(key.first);
    }
  }
  else if(mapType == "char"){
    for(auto && key : *m_char_map){
      changedKeys.push_back(key.first);
    }
  }
  

  sendUpdate(true);
  changedKeys.clear();

 }

