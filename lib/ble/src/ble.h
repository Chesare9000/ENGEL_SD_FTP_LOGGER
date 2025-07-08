#pragma once

#ifndef BLENGELSERVICE_H
#define BLENGELSERVICE_H

#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include <memory>
#include <mutex>


// See the following for generating UUIDs:
//https://www.uuidgenerator.net/

namespace BLE {

// this class is used to scan for a beacon with a specific UUID
class EngelScanForBeaconCallback : public BLEAdvertisedDeviceCallbacks
{  
  public:
  //this UUID must be set to the UUID of the beacon we are looking for
  std::string UUID = "00000000-0000-0000-0000-000000000000";
  std::function<void(bool)> callback;

  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.haveRSSI())
    {
      BLEUUID devUUID = advertisedDevice.getServiceUUID();
      int rssi = advertisedDevice.getRSSI();

      if(devUUID.toString() == UUID && rssi > -50){
        Serial.print("Found ServiceUUID: ");
        Serial.println(devUUID.toString().c_str());
        Serial.println("");
        callback(true);
        return;
      }
    }

    callback(false);
  }
};

class BLEngelService {
  public:
    BLEngelService(std::string serviceUUID = "8c595106-6a3c-4992-919f-a7e147897021", std::string characteristicUUID = "d4804a80-17cb-4ca2-a126-472550f8e54f");
    void begin();
    void startAdvertising();

    void onDeviceConnected();
    void onDeviceDisconnected();
    void onWriteReceived(String value);

    void isDeviceUnlocked(std::function<void(bool)> callback);

    // will scan for a beacon with the given UUID
    // will call the passed callback function when results are there
    void lookForBeacon(String UUID, std::function<void(bool)> callback, int minSignal = -50);

    // will reload the characteristics with the new values
    // notify will send the new values to the client
    void sendUpdate(bool notify = true);
    
    std::function<void(bool)> onDeviceConnectedCallback;
    std::function<void(bool)> onLockEventCallback;

    // set these so we can process the data
    std::map<std::string, int*> *m_int_map;
    std::map<std::string, float*> *m_float_map;
    std::map<std::string, bool*> *m_bool_map;
    std::map<std::string, std::string*> *m_string_map;
    std::map<std::string, char*> *m_char_map;
    std::map<std::string, uint32_t*> *m_uint32_t_map;
    std::map<std::string, uint8_t*> *m_uint8_t_map;
    std::map<std::string, uint16_t*> *m_uint16_t_map;
    std::map<std::string, uint64_t*> *m_uint64_t_map;

    // a bit complicated because we are using cpp standrad 11. With 17 we would make it easier
    std::map<std::string, std::string> *m_cast_reference = new std::map<std::string, std::string>();

    // this mutex needs to be same like in vars.h
    std::mutex var_mutex;

    // this needs to be defined because it can be read in the characteristic
    // #TODO: make this a map
    std::vector<std::string> changedKeys = { "bat_percent", "charging"};

    // sends all values in a map via BLE if there is a connection
    void sendAll(std::string mapType);

    //#TODO: permanently store the device name and only pair with that device
    
  private:
    std::string mServiceUUID;
    std::string mCharacteristicUUID;

    // this communicate changes via notify and take changes via BLE
    BLECharacteristic *mChangesCharacteristic;

    // this will display critical device info like battery state READ only
    BLECharacteristic *mDeviceInfoCharacteristic;
    
    BLEServer *mServer;
    BLEService *mService;
    std::string createJSON(std::vector<std::string> &keys);
    void notifyClient();

    bool deviceConnected = false;
    String configuredDevice = "";

    BLEScan *mBLEScan;
    BLE::EngelScanForBeaconCallback *mScanForBeaconCallback;
};

class EngelServerCallbacks: public BLEServerCallbacks {

    public:

    EngelServerCallbacks(std::function<void()> onDeviceConnected, std::function<void()> onDeviceDisconnected) {
      this->onDeviceConnected = onDeviceConnected;
      this->onDeviceDisconnected = onDeviceDisconnected;
    }

    std::function<void()> onDeviceConnected;
    std::function<void()> onDeviceDisconnected;

    void onConnect(BLEServer* pServer) {
        Serial.println("Connected a Device!");
        onDeviceConnected();
    };

    void onDisconnect(BLEServer* pServer) {
        Serial.println("Disconnected a Device!");
        onDeviceDisconnected();
    }
};

class EngelCharacteristicCallbacks: public BLECharacteristicCallbacks {

    public:

    EngelCharacteristicCallbacks(std::function<void(String)> onWrite) {
      this->onWriteReceived = onWrite;
    }

    std::function<void(String)> onWriteReceived;

    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
          String newvalue = String(value.c_str());
          onWriteReceived(newvalue);
        }
    }
};
}

#endif
