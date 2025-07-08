//This implementation is purely to get connected to internet via wifi
//Stuff related to logic between lte , mqtt , ble etc is handled in other files.


#include <Arduino.h>
#include <WiFi.h> //ESP32

#include <tools.h>           
#include <vars.h>
#include <interrupts.h>
#include <rgb.h>
#include <oled.h>
#include <gpio_exp.h>
#include <tasks.h>
#include <wifi.h> //local one
#include <mqtt.h>
#include <nvs.h>

//VARS
//Initial state before NVS init
bool wifi_has_credentials = false;

// SSID and password of Wifi connection
//Coming frrom NVS or First Time Use
/*
String wifi_ssid = "";
String wifi_pass = "";
*/
//TODO , replace below with above

String wifi_ssid_motionlab = "Motionlab-Member";
String wifi_pass_motionlab = "always.in.motion";

String wifi_ssid_hq = "Not_Your_Hotspot";
String wifi_pass_hq = "wifirocks";

String wifi_ssid_default = wifi_ssid_hq ;
String wifi_pass_default = wifi_pass_hq;

String wifi_ssid = wifi_ssid_default;
String wifi_pass = wifi_pass_default;

//KW
//wifi_ssid = "WLAN1-RB6E06";
//wifi_pass = "b8Fq2r8f8hYQ8qgR";     


enum wifi_status
{
  wifi_status_off,
  wifi_status_on_idle,
  wifi_status_searching,
  wifi_status_connecting,
  wifi_status_connected
};

int wifi_status = wifi_status_off;

bool wifi_connected = false;


bool wifi_connect() 
{
  int counter = 0;

  //wifi_status = wifi_status_off;

  if(oled_enabled)oled_wifi();

  WiFi.begin(wifi_ssid, wifi_pass);
  wifi_status = wifi_status_connecting;
  if(oled_enabled)oled_wifi();
  // print SSID to the serial interface for debugging  
  if(log_enabled)
  {
    Serial.print("Connecting to WiFi with SSID: ");
    Serial.print(wifi_ssid.c_str());
  } 
  
  while (WiFi.status() != WL_CONNECTED) // wait until WiFi is connected
  {             
    wait(1000);
    Serial.print("*");
    counter++;
    if(counter >10)
    {
        if(log_enabled)Serial.println("ERROR while trying to connect to WIFI");
        wifi_status = wifi_status_on_idle;
        return false;
    }
  }

  wifi_status = wifi_status_connected;

  Serial.print("\n Connected to : ");
  Serial.print(wifi_ssid.c_str());
  Serial.print(", with IP Address: ");
  Serial.println(WiFi.localIP());                     // show IP address that the ESP32 has received from router
  
  if(oled_enabled)oled_wifi();
  wifi_connected = true;
  return true;
}

bool wifi_disconnect()
{
  if (WiFi.status() == WL_CONNECTED) 
  {
    Serial.println("Disconnecting from WiFi...");
    
    // Disconnect from WiFi
    WiFi.disconnect(true); // Pass 'true' to clear saved credentials
    delay(100); // Allow time for disconnection
    
    // Turn off WiFi module
    WiFi.mode(WIFI_OFF);
    delay(100); // Allow time for the module to turn off
    
    Serial.println("WiFi disconnected and OFF.");
    wifi_status = wifi_status_on_idle;
    return true;
  } 
  else 
  {
    Serial.println("ERROR : WiFi is not connected.");
    return false;
  }

  
}

void wifi_off()
{
  WiFi.mode( WIFI_MODE_NULL );
  if(log_enabled)Serial.print("\nWIFI OFF\n");
  wifi_connected = false;
  wifi_status = wifi_status_off;
}

                    