//THIS IS NOT IMPLEMENTED ON THE MAIN KERNEL AND JUST USE AS DEMO FOR WEBSOCKET
//PIECECS OF THESE SNIPPET HAVE BEEN REMADE ON THE WIFI.cpp TO HOLD THE WIFI IMPLEMENTATION
//THIS CAN BE SAFELY REMOVED AND WONT AFFECT THE MAIN WIFI FUNCTIONALITY






// ---------------------------------------------------------------------------------------
//
// Code for a webserver on the ESP32 to control LEDs (device used for tests: ESP32-WROOM-32D).
// The code allows user to switch between three LEDs and set the intensity of the LED selected
//
// For installation, the following libraries need to be installed:
// * Websockets by Markus Sattler (can be tricky to find -> search for "Arduino Websockets"
// * ArduinoJson by Benoit Blanchon
//
// Written by mo thunderz (last update: 19.11.2021)
//
// ---------------------------------------------------------------------------------------
#include <Arduino.h>

#include <WiFi.h>                                     // needed to connect to WiFi
#include <WebServer.h>                                // needed to create a simple webserver (make sure tools -> board is set to ESP32, otherwise you will get a "WebServer.h: No such file or directory" error)
#include <WebSocketsServer.h>                         // needed for instant communication between client and server through Websockets
#include <ArduinoJson.h>                              // needed for JSON encapsulation (send multiple variables with one string)

#include <tools.h>           
#include <vars.h>
#include <interrupts.h>
#include <rgb.h>
#include <oled.h>
#include <gpio_exp.h>
#include <tasks.h>

#include "wifi_demo.h"
               
// ssid and password of Wifi connection:
const char* demo_ssid = "Not_Your_Hotspot";
const char* demo_password = "wifirocks";

// The String below "webpage" contains the complete HTML code that is sent to the client whenever someone connects to the webserver
// NOTE 27.08.2022: I updated in the webpage "slider.addEventListener('click', slider_changed);" to "slider.addEventListener('change', slider_changed);" -> the "change" did not work on my phone.
String webpage = "<!DOCTYPE html><html><head><title>Page Title</title></head><body style='background-color: #EEEEEE;'><span style='color: #003366;'><h1>ENGEL WIFI TEST</h1><form> <p>ALARM STATUS</p> <div> <input type='radio' id='alarm_on' name='operation_mode'> <label for='alarm_on'>ON</label> <input type='radio' id='alarm_off' name='operation_mode'> <label for='alarm_off'>OFF</label> </div></form><form> <p>LED Color:</p> <div> <input type='radio' id='led_red' name='operation_mode'> <label for='led_red'>RED</label> <input type='radio' id='led_green' name='operation_mode'> <label for='led_green'>GREEN</label> <input type='radio' id='led_blue' name='operation_mode'> <label for='led_blue'>BLUE</label> </div></form><br>Set RGB PWM: <br><input type='range' min='0' max='255' value='0' class='slider' id='id_wifi_rgb_pwm'>Value: <span id='id_wifi_rgb_pwm_value'>-</span><br></span></body><script>document.getElementById('led_red').addEventListener('click', led_changed);document.getElementById('led_green').addEventListener('click', led_changed);document.getElementById('led_blue').addEventListener('click', led_changed);document.getElementById('alarm_on').addEventListener('click', alarm_changed);document.getElementById('alarm_off').addEventListener('click', alarm_changed);var slider = document.getElementById('id_wifi_rgb_pwm'); slider.addEventListener('change', slider_changed);var output = document.getElementById('id_wifi_rgb_pwm_value');var Socket;function init() { Socket = new WebSocket('ws://' + window.location.hostname + ':81/'); Socket.onmessage = function(event) { processCommand(event); };}function led_changed() { var wifi_rgb_led_selected = 0; if(document.getElementById('led_green').checked == true) { wifi_rgb_led_selected = 1; } else if(document.getElementById('led_blue').checked == true) { wifi_rgb_led_selected = 2; } console.log(wifi_rgb_led_selected); var msg = { type: 'wifi_rgb_led', value: wifi_rgb_led_selected}; Socket.send(JSON.stringify(msg)); }function alarm_changed() { var wifi_alarm_status = 0; if(document.getElementById('alarm_on').checked == true) { wifi_alarm_status = 1; } console.log(wifi_alarm_status); var msg = { type: 'wifi_alarm_status', value: wifi_alarm_status}; Socket.send(JSON.stringify(msg)); }function slider_changed() { var wifi_rgb_pwm = slider.value; console.log(wifi_rgb_pwm); var msg = { type: 'wifi_rgb_pwm', value: wifi_rgb_pwm}; Socket.send(JSON.stringify(msg)); }function processCommand(event) { var obj = JSON.parse(event.data); var type = obj.type; if (type.localeCompare(\"wifi_rgb_pwm\") == 0) { var wifi_rgb_pwm = parseInt(obj.value); console.log(wifi_rgb_pwm); slider.value = wifi_rgb_pwm; output.innerHTML = wifi_rgb_pwm; } else if(type.localeCompare(\"wifi_rgb_led\") == 0) { var wifi_rgb_led_selected = parseInt(obj.value); console.log(wifi_rgb_led_selected); if(wifi_rgb_led_selected == 0) { document.getElementById('led_red').checked = true; } else if (wifi_rgb_led_selected == 1) { document.getElementById('led_green').checked = true; } else if (wifi_rgb_led_selected == 2) { document.getElementById('led_blue').checked = true; } } else if(type.localeCompare(\"wifi_alarm_status\") == 0) { var wifi_alarm_status = parseInt(obj.value); console.log(wifi_alarm_status); if(wifi_alarm_status == 0) { document.getElementById('alarm_off').checked = true; } else if (wifi_alarm_status == 1) { document.getElementById('alarm_on').checked = true; } } }window.onload = function(event) { init();}</script></html>";


// global variables of the LED selected and the intensity of that LED
int wifi_rgb_led = 0;
int wifi_rgb_pwm = 0;
int wifi_alarm_status = 0;
int wifi_clients =0;


//BRING BACK THIS IF NEEDED but rename it , as is now used on WIFI.cpp
bool wifi_initialized = false; //Depreccated , WIFI don't need init

bool new_wifi_info = false;

//0 Not Init (OFF)
//1 Initializing (WIFI+Sockets+Json)
//2 Initialized
//3 Searching (and printing) Networks
//4 Connecting
//5 Connected 
//6 Transmitting (no client)
//7 Transmitting + Receiving (client connected)
//->Disconnected , back to 0

//Bring_back if needed but rename it because is used on wifi.cpp
//int wifi_status = 0;





// The JSON library uses static memory, so this will need to be allocated:
// -> in the video I used global variables for "doc_tx" and "doc_rx", however, I now changed this in the code to local variables instead "doc" -> Arduino documentation recomends to use local containers instead of global to prevent data corruption

// Initialization of webserver and websocket
WebServer server(80);                                 // the server uses port 80 (standard port for websites
WebSocketsServer webSocket = WebSocketsServer(81);    // the websocket uses port 81 (standard port for websockets

//included also on .h , UNCOMMENT IF NEEDED
void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length);

void wifi_demo_init() 
{

  wifi_status = 1;
  if(oled_enabled)oled_wifi_demo();

  WiFi.begin(demo_ssid, demo_password);                         // start WiFi interface
  wifi_status = 2;
  Serial.println("Connecting to WiFi with demo_ssid: " + String(demo_ssid));     // print demo_ssid to the serial interface for debugging
  
  wifi_status = 4;
  while (WiFi.status() != WL_CONNECTED) // wait until WiFi is connected
  {             
    wait(1000);
    Serial.print(".");

  }

  wifi_status = 5;
  Serial.print("Connected to network with IP address: ");
  Serial.println(WiFi.localIP());                     // show IP address that the ESP32 has received from router
  
  server.on("/", []() {                               // define here wat the webserver needs to do
    server.send(200, "text/html", webpage);           //    -> it needs to send out the HTML string "webpage" to the client
  });
  server.begin();                                     // start server
  
  webSocket.begin();                                  // start websocket
  webSocket.onEvent(webSocketEvent);                  // define a callback function -> what does the ESP32 need to do when an event from the websocket is received? -> run function "webSocketEvent()"

  if(oled_enabled)oled_wifi_demo();
  wifi_initialized = true;

}


void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length)// the parameters of this callback function are always the same -> num: id of the client who send the event, type: type of message, payload: actual data sent and length: length of payload
{  
  switch (type)  // switch on the type of information sent
  {                                    
    case WStype_DISCONNECTED: // if a client is disconnected, then type == WStype_DISCONNECTED
      
      Serial.println("Client " + String(num) + " disconnected");
      
      wifi_clients--;
      if(log_enabled)Serial.printf("\n\nToltal Clients: %d \n",wifi_clients);
      
      if(wifi_clients==0)wifi_status = 6;
      
      if(oled_enabled)oled_wifi_demo();

    break;

    case WStype_CONNECTED:// if a client is connected, then type == WStype_CONNECTED
      
      Serial.println("Client " + String(num) + " connected");
      
      // send wifi_rgb_pwm and LED_select to clients -> as optimization step one could send it just to the new client "num", but for simplicity I left that out here
      sendJson("wifi_rgb_pwm", String(wifi_rgb_pwm));
      sendJson("wifi_rgb_led", String(wifi_rgb_led));
      sendJson("wifi_alarm_status", String(wifi_alarm_status));
      
      wifi_clients++;
      if(log_enabled)Serial.printf("\n\nTotal Clients: %d \n",wifi_clients);       
      
      wifi_status = 7;
      if(oled_enabled)oled_wifi_demo();
    
    break;

    case WStype_TEXT: // if a client has sent data, then type == WStype_TEXT
    
      // try to decipher the JSON string received
      StaticJsonDocument<200> doc; // create JSON container 
      DeserializationError error = deserializeJson(doc, payload);
      
      if(error) 
      {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
          return;
      }
      
      else 
      {
        // JSON string was received correctly, so information can be retrieved:
        const char* l_type = doc["type"];
        const int l_value = doc["value"];
        Serial.println("Type: " + String(l_type));
        Serial.println("Value: " + String(l_value));

        // if wifi_rgb_pwm value is received -> update and write to LED
        if(String(l_type) == "wifi_rgb_pwm") 
        {
            wifi_rgb_pwm = int(l_value);
            sendJson("wifi_rgb_pwm", String(l_value));

            Serial.print("PWM Slider ->");
            Serial.print(wifi_rgb_pwm);
            Serial.println();
        }

        //Select RGB Color
        if(String(l_type) == "wifi_rgb_led") 
        {
            wifi_rgb_led = int(l_value);
            sendJson("wifi_rgb_led", String(l_value));

            Serial.print("Selected LED Color -> ");

            switch(wifi_rgb_led)
            {
              case 0 : Serial.print(" RED"); break;
              case 1 : Serial.print(" GREEN"); break;
              case 2 : Serial.print(" BLUE"); break;
            }

            Serial.println();
            
            //here the color selected program

        }
        if(String(l_type) == "wifi_alarm_status") 
        {
            wifi_alarm_status = int(l_value);
            sendJson("wifi_alarm_status", String(l_value));
            
            //here the color selected program
            if(wifi_alarm_status)Serial.print("---Alarm ON!---\n");
            else Serial.print("---Alarm OFF!---\n"); 
        }

        if(!new_wifi_info) new_wifi_info = true;
      }

    Serial.println("");
    break; 
  }
}

// Simple function to send information to the web clients
void sendJson(String l_type, String l_value) 
{
    String jsonString = "";                           // create a JSON string for sending data to the client
    StaticJsonDocument<200> doc;                      // create JSON container
    JsonObject object = doc.to<JsonObject>();         // create a JSON Object
    object["type"] = l_type;                          // write data into the JSON object -> I used "type" to identify if wifi_rgb_led or wifi_rgb_pwm is sent and "value" for the actual value
    object["value"] = l_value;
    serializeJson(doc, jsonString);                // convert JSON object to string
    webSocket.broadcastTXT(jsonString);               // send JSON string to all clients
}




//WIFI_DEMO TASK ----------

TaskHandle_t task_wifi_demo_handle = NULL;

void create_task_wifi_demo() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n-- creating task_wifi_demo --");

    task_wifi_demo_i2c_declare();

    xTaskCreate
    (
        wifi_demo,           //Function Name (must be a while(1))
        "task_wifi_demo", //Logging Name
        4096,                //Stack Size
        NULL,                //Passing Parameters
        8,                   //Task Priority
        &task_wifi_demo_handle
    );   

    if(log_enabled) Serial.print("-- done --\n");
}

void task_wifi_demo_i2c_declare()
{
    if(log_enabled)Serial.print("\ntask_wifi_demo_i2c_declared\n");
    //This Taks will use the following I2C_Devs

    //imu_needed++;
    rgb_needed++;
    //temp_needed++;
    //lux_needed++;
    //rtc_needed++;
    //fuelgauge_needed++;
    oled_needed++;
}

void task_wifi_demo_i2c_release()
{
    if(log_enabled)Serial.print("\ntask_wifi_demo_i2c_released\n");
    
    //This Taks will release the following I2C_Devs
    
    //imu_needed--;
    rgb_needed--;
    //temp_needed--;
    //lux_needed--;
    //rtc_needed--;
    //fuelgauge_needed--;
    oled_needed--;
}

void wifi_demo(void * parameters)
{
  //OUT with BTN2
  btn_2_interr_enable_on_press();

  while(1)
  {
    if(btn_2.is_pressed) //Gettinng Out
    {
      btn_2_interr_disable();
    
      if(log_enabled) Serial.print(" ----- Back to Menu ! \n ");   

      wifi_demo_disconnect();
      wifi_demo_off();
      
      
      if(rgb_bypassed) rgb_bypassed=false;
      
      wait_for_btn_2_release(); //will reset the btn.pressed flag also

      oled_clear();

      create_task_devel_menu();

      task_wifi_demo_i2c_release();
      vTaskDelete(NULL);//Delete itself
    }
    
    else if (!wifi_initialized) 
    {
      if(log_enabled)Serial.print("\n---Initializing WIFI---\n");
      wifi_demo_init();
      //Initialized, Connected and Transmitting (waiting for client)
      wifi_status = 6;
      if(oled_enabled) oled_wifi_demo();
    }

    else
    {
      server.handleClient(); // Needed for the webserver to handle all clients
      webSocket.loop();     // Update function for the webSockets 

      //Refresh the status of the RX vars. just if new RX info is avaiable
      //TODO RIGHT NOW CHANGING VARS AND MANAGING HERE
      //THINK IF ITS BETTER TO HANDLE ALL ON THE CALLBACKS
      if(new_wifi_info)
      {
          //Check first for alarm

          if(wifi_alarm_status)
          {
            if(rgb_bypassed) //if off by slider
            {
                rgb_bypassed = false;
                rgb_running = true;
                rgb_leds_enable();
                wait(500);
                Serial.print(" LEDS ON |");
            }
          }

          //Reacting to RGB PWM Order from App  
          else if (wifi_rgb_pwm > 0 )
          { 
              if(rgb_bypassed) 
              {
                  rgb_bypassed = false;
                  rgb_running = true;
                  rgb_leds_enable();
                  wait(500);
                  Serial.print(" LEDS ON |");
              }
              
              if(log_enabled)Serial.print("LEDS: ");
              
              if(log_enabled)Serial.printf("%d | \n",wifi_rgb_pwm);

              if(wifi_rgb_led == 0) rgb_leds_on('r',wifi_rgb_pwm);
              else if(wifi_rgb_led == 1) rgb_leds_on('g',wifi_rgb_pwm);
              else if(wifi_rgb_led == 2) rgb_leds_on('b',wifi_rgb_pwm);
          }
          else //ALARM_OFF AND slider on 0
          {
              //if its the only resource using the wifi_rgb_leds
              if(rgb_needed == 1 && !rgb_bypassed)
              {
                  rgb_bypassed = true;
                  rgb_leds_disable();
                  if(log_enabled)Serial.print("LEDS:OFF");
              }
          }
          //Refresh WIFI and exit
          if(oled_enabled) oled_wifi_demo();
          new_wifi_info = false;
          
      }//End of new info

      //If the alarm is still active then keep looping on blink
      if (wifi_alarm_status) 
      {
        rgb_leds_blink_once('r',255,500);
        wait(500);
      }

    }

  }
  
}

void wifi_demo_disconnect()
{
  WiFi.disconnect();
  if(log_enabled)Serial.print("\nWIFI DISCONNECTED\n");
  wifi_clients =0;
  wifi_status = 0;
}

void wifi_demo_off()
{
  WiFi.mode( WIFI_MODE_NULL );
  if(log_enabled)Serial.print("\nWIFI OFF\n");
  wifi_initialized = false;
  wifi_clients =0;
  wifi_status = 0;
}




