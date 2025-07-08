

// Define the modem type before including TinyGsmClient.h
#define TINY_GSM_MODEM_SIM7080
#define TINY_GSM_RX_BUFFER 4096
#define TINY_GSM_TX_BUFFER 4096
// Define the serial console for debug prints, if needed
//#define TINY_GSM_DEBUG Serial
//#define SerialMon Serial

#define TINY_GSM_USE_GPRS true   //check later
#define TINY_GSM_USE_WIFI false //todo check later for compatibility issues with wifi

#define GSM_PIN ""

const char carrier_apn[] = "TM";
const char gprsUser[] = "";
const char gprsPass[] = "";


#include <vars.h> 

#include <TinyGsmClient.h>
#include <HardwareSerial.h>
#include <FirebaseClient.h>

#include <tools.h>
#include <lte.h>
#include <gpio_exp.h>
#include <tools.h>
#include <i2c.h>
#include <rtc.h>

#include <firebase.h>

// Define the RX and TX pins for the SIM7080G
#define esp_pin_simcom_rx 34 // ESP32 GPIO14 connected to SIM7080G TX
#define esp_pin_simcom_tx 14 // ESP32 GPIO34 connected to SIM7080G RX

// Define the Serial2 port for communication with the SIM7080G
// Create HardwareSerial instance for the modem
HardwareSerial simcom_serial(2);

TinyGsm modem(simcom_serial);

TinyGsmClient lte_client(modem,0); //For Firebase over LTE

//GSMNetwork gsm_network(&modem, GSM_PIN, carrier_apn, gprsUser, gprsPass);

bool level = false;

int lte_signal_quality = 0;

int lte_status = lte_nvs_info_not_found; //default but must check on init

// Number of retries before toggling the lte_key

int lte_retries_before_key_turn_default = 2;
int lte_retries_before_key_turn = lte_retries_before_key_turn_default; 


// Server details to test TCP/SSL
const char server[]   = "vsh.pp.ua";
const char resource[] = "/TinyGSM/logo.txt";


//---------FROM THE LYLYYGO EXAMPLE-------------------
const char *register_info[] = 
{
  "Not registered, MT is not currently searching an operator to register to. The GPRS service is disabled, the UE is allowed to attach for GPRS if requested by the user.",
  "Registered, home network.",
  "Not registered, but MT is currently trying to attach or searching an operator to register to. The GPRS service is enabled, but an allowable PLMN is currently not available. The UE will start a GPRS attach as soon as an allowable PLMN is available.",
  "Registration denied, the GPRS service is disabled, the UE is not allowed to attach for GPRS if it is requested by the user.",
  "Unknown.",
  "Registered, roaming.",
};


enum 
{
  MODEM_CATM = 1,
  MODEM_NB_IOT,
  MODEM_CATM_NBIOT,
};

void getPsmTimer();
//------------------------------------------

bool simcom_initialized = false;
bool simcom_gps_initialized = false;
bool simcom_lte_initialized = false;

double simcom_gps_latitude , simcom_gps_longitude = 0 ;
double simcom_gps_speed_kmh, simcom_gps_speed_mph = 0 ;
double simcom_gps_heading, simcom_gps_altitude = 0 ;
int simcom_gps_sat_count, simcom_gps_hdop = 0 ;

TaskHandle_t task_simcom_gps_handle;

bool simcom_gps_locked = 0 ;

int simcom_gps_retry_nr = 0 ;
int simcom_gps_poll_nr = 0 ;

bool simcom_gps_is_on = false ;

bool simcom_gps_data_logging_needs_refresh = false;

//From Backend
int simcom_gps_refresh_seconds_default = 0 ;
int  simcom_gps_refresh_seconds = 0 ;
bool simcom_gps_enabled = 0 ;
bool simcom_gps_upload = false ;
bool simcom_task_gps_active = false ;



//LTE data
const char* apn = "TM"; 


//GENERAL LTE FUNCTIONALITIES-------------------------------------------------------------------


bool simcom_lte_activate_pdp()
{
  Serial.println("Activating PDP context...");
  simcom_serial.println("AT+CGACT=1,1");
  wait(500);

  String response = "";
  while (simcom_serial.available()) 
  {
    char c = simcom_serial.read();
    response += c;
  }

  //Serial.println("PDP Context Activation Response:");
  //Serial.println(response);

  if (response.indexOf("OK") == -1) 
  {
    Serial.println("Failed to activate PDP context.");
    return false;
  }

  Serial.println("PDP context activated successfully.");
  return true;

}

bool simcom_lte_check_gprs_connected()
{
  if (modem.isGprsConnected()) 
  {
    Serial.println("GPRS is connected");
    return true;
  } 
  else 
  {
    Serial.println("GPRS is not connected");
    return false;
  }
}

// Helper function to determine if DST is in effect in Germany
bool isDST_Germany(int year, int month, int day, int hour) 
{
    if (month < 3 || month > 10) return false;
    if (month > 3 && month < 10) return true;

    int lastSunday;
    struct tm t = {0};
    t.tm_year = year - 1900;
    t.tm_mon = month - 1;
    t.tm_mday = 31;
    mktime(&t);
    int wday = t.tm_wday;
    lastSunday = 31 - wday;

    if (month == 3) {
        if (day > lastSunday) return true;
        if (day < lastSunday) return false;
        return hour >= 2;
    } else if (month == 10) {
        if (day > lastSunday) return false;
        if (day < lastSunday) return true;
        return hour < 3;
    }
    return false;
}


bool simcom_lte_sync_time() 
{
  Serial.println("\n--- Syncing Time via SIM7080G NTP (AT+CNTP) ---");

  if (!modem.isGprsConnected()) 
  {
    Serial.println("GPRS not connected. Cannot sync time.");
    return false;
  }

  const char* ntpServer = "pool.ntp.org";

  // Set NTP server and timezone offset (0 = UTC)
  modem.sendAT("+CNTP=\"", ntpServer, "\",0");
  if (modem.waitResponse(10000L) != 1) 
  {
    Serial.println("Failed to configure CNTP");
    return false;
  }

  // Start NTP sync
  modem.sendAT("+CNTP");
  if (modem.waitResponse(10000L) != 1) 
  {
    Serial.println("NTP sync command failed");
    return false;
  }

  wait(1000); // Wait for time to update

  // Request current clock from modem
  modem.sendAT("+CCLK?");
  String timeStr;
  if (modem.waitResponse(1000L, timeStr) == 1) 
  {
    int idx = timeStr.indexOf("+CCLK: ");
    if (idx != -1) 
    {
      String date_and_time = timeStr.substring(idx + 8, idx + 25); // "yy/MM/dd,HH:mm:ss"
      int year   = 2000 + date_and_time.substring(0,2).toInt();
      int month  = date_and_time.substring(3,5).toInt();
      int day    = date_and_time.substring(6,8).toInt();
      int hour   = date_and_time.substring(9,11).toInt();
      int minute = date_and_time.substring(12,14).toInt();
      int second = date_and_time.substring(15,17).toInt();

      bool isDST = isDST_Germany(year, month, day, hour);
      int offset = isDST ? 2 : 1;
      hour += offset;
      if (hour >= 24) {
        hour -= 24;
        day += 1; // Simple increment, does not handle month/year rollover
      }

      char localTime[32];
      snprintf(localTime, sizeof(localTime), "%04d/%02d/%02d,%02d:%02d:%02d (UTC+%d)", 
        year, month, day, hour, minute, second, offset);

      Serial.print("Time successfully synced (Germany): ");
      Serial.println(localTime);
      
      return true;
    }
  }
  Serial.println("Failed to retrieve time after sync");
  return false;
}

bool simcom_lte_sync_time_and_set_esp32() 
{
  Serial.println("\n--- Syncing Time via SIM7080G NTP (AT+CNTP) ---");

  if (!modem.isGprsConnected()) 
  {
    Serial.println("GPRS not connected. Cannot sync time.");
    return false;
  }

  const char* ntpServer = "pool.ntp.org";

  // Set NTP server and timezone offset (0 = UTC)
  modem.sendAT("+CNTP=\"", ntpServer, "\",0");
  if (modem.waitResponse(10000L) != 1) 
  {
    Serial.println("Failed to configure CNTP");
    return false;
  }

  // Start NTP sync
  modem.sendAT("+CNTP");
  if (modem.waitResponse(10000L) != 1)
  {
    Serial.println("NTP sync command failed");
    return false;
  }

  wait(1000); // Wait for time to update

  // Request current clock from modem
  modem.sendAT("+CCLK?");
  String timeStr;
  if (modem.waitResponse(1000L, timeStr) == 1) 
  {
    int idx = timeStr.indexOf("+CCLK: ");
    if (idx != -1) 
    {
      String date_and_time = timeStr.substring(idx + 8, idx + 25); // "yy/MM/dd,HH:mm:ss"
      int year   = 2000 + date_and_time.substring(0,2).toInt();
      int month  = date_and_time.substring(3,5).toInt();
      int day    = date_and_time.substring(6,8).toInt();
      int hour   = date_and_time.substring(9,11).toInt();
      int minute = date_and_time.substring(12,14).toInt();
      int second = date_and_time.substring(15,17).toInt();

      bool isDST = isDST_Germany(year, month, day, hour);
      int offset = isDST ? 2 : 1;
      hour += offset;
      if (hour >= 24) {
        hour -= 24;
        day += 1; // Simple increment, does not handle month/year rollover
      }

      struct tm t;
      t.tm_year = year - 1900;
      t.tm_mon  = month - 1;
      t.tm_mday = day;
      t.tm_hour = hour;
      t.tm_min  = minute;
      t.tm_sec  = second;
      t.tm_isdst = isDST ? 1 : 0;

      time_t epoch = mktime(&t);
      struct timeval now = { .tv_sec = epoch, .tv_usec = 0 };
      settimeofday(&now, NULL);

      Serial.printf("ESP32 time set (Germany): %s", asctime(&t));
      return true;
    }
  }
  Serial.println("Failed to retrieve time after sync");
  return false;
}

//Managed dy GPIO EXP so this is just a nameholder
void simcom_turn_key()
{
  if(i2c_manager_running)
	{
		Serial.println("\n--Stopping I2C Manager before starting LTE");
		i2c_manager_running = false;
		wait(1000);
    gpio_exp_p11_simcom_turn_key();
		wait(1000);
		Serial.println("\n---Restarting I2C Manager before starting LTE");
		create_task_i2c_manager();
	} 
	else 
  {
    gpio_exp_p11_simcom_turn_key();
    wait(1000);
  }
  
  
}

bool simcom_init() 
{
  Serial.println("\n\n---Initializing SIM7080G...");

  //Always turn key upon init either to reset the modem or to turn it on
  //if(lte_status != lte_on_and_working) simcom_turn_key(); // Toggle the key to turn on or reset the modem
  
  //TODO check if we can improve the initial time skipping some tests or the turnkey

  simcom_turn_key(); // Toggle the key to turn on or reset the modem


  // Retry logic for modem initialization
  int retry_count = 0;
  
  while (retry_count < lte_retries_before_key_turn) 
  {
    Serial.printf("\n---Establishing Serial Communication with the modem, attempt nr: %d", retry_count + 1 );
    // Initialize Serial2 for SIM7080G communication
    simcom_serial.begin(115200, SERIAL_8N1, esp_pin_simcom_rx, esp_pin_simcom_tx);
    wait(500); // Allow some time for the serial connection to stabilize

    // Check if the modem responds to AT commands
    if (modem.testAT()) 
    {
      Serial.println("\n---Modem initialized successfully!");
      retry_count =0;
      return true;
    } 
    else 
    {
      Serial.println("\n---Failed to communicate with the modem. Turning Key...");
      simcom_serial.end(); // Close the serial connection
      retry_count++;
      simcom_turn_key(); // Toggle the key to reboot the modem
      //wait(1000); // Wait before retrying
    }
  }

  Serial.printf("\n---AT command test failed after %d retries!",lte_retries_before_key_turn);
  return false;    
}

void simcom_send_AT_command(const String& atCommand) 
{
  Serial.println("\n--- Sending AT Command ---");
  Serial.println("Command: " + atCommand);

  // Send the AT command to the SIMCom module
  simcom_serial.println(atCommand);
  wait(500); // Wait for the response

  // Read and print the response
  String response = "";
  unsigned long startTime = millis();
  const unsigned long timeout = 5000; // Timeout for response (5 seconds)

  while (millis() - startTime < timeout) 
  {
      while (simcom_serial.available()) 
      {
          char c = simcom_serial.read();
          response += c;
          wait(1);
      }

      // Break the loop if the response contains "OK" or "ERROR"
      if (response.indexOf("OK") != -1 || response.indexOf("ERROR") != -1) 
      {
          break;
      }
  }

  if (response.length() > 0) 
  {
    // Print the response
    Serial.println("\n--- Response ---");
    Serial.println(response);
  } 
  else 
  {
    Serial.println("No response received (timeout).");
  }

  // Handle the response (example switch-case logic)
  if (response.indexOf("OK") != -1) 
  {
    Serial.println("Command executed successfully.");
  } 
  else if (response.indexOf("ERROR") != -1) 
  {
    Serial.println("Command execution failed.");
  } 
  else 
  {
    Serial.printf("\n---Unexpected response received: %s ",response.c_str());
  }
}

//To turn ON just call lte_init() ,  the turn_key is handled there

bool simcom_turn_off() 
{
  Serial.println("Powering off the modem...");

  simcom_turn_key(); // Toggle the key to power off the modem
  wait(1000);

  int retry_count = 0;
      const int max_retries = 10; // Adjust as needed

      while (retry_count < max_retries)
      {
          Serial.printf("\n---Testing modem communication, attempt nr: %d", retry_count + 1);

          if (!modem.testAT())
          {
              Serial.println("\n---Modem is OFF (no communication)!");
              return true;
          }
          else
          {
              Serial.println("\n---Modem still ON, turning key to power off...");
              simcom_turn_key(); // Toggle the key to power off the modem
              wait(2000); // Wait for the modem to power down
              retry_count++;
          }
      }

      Serial.println("\n---Failed to turn off modem after max retries.");
      return false;
}



//FOR THE GPS-----------------------------------------------------------------------------------

//+CGNSINF: <GNSS run status>,<Fix status>,<UTC date & time>,<Latitude>,<Longitude>,
//<MSL altitude>,<Speed>,<Course>,<HDOP>,<PDOP>,<VDOP>,<Reserved1>,
//<Reserved2>,<Reserved3>,<GNSS satellites in use>,<GNSS satellites in view>,<C/N0 max>,<HPA>,<VPA>

//FOR THE GPS encoding
/*
| Field Index | Description                    | Example Value        |
|-------------|--------------------------------|----------------------|
| 0           | GNSS run status                | `1`                  |
| 1           | Fix status                     | `1`                  |
| 2           | UTC date & time                | `20250507152749.000` |
| 3           | Latitude                       | `52.509932`          |
| 4           | Longitude                      | `13.416903`          |
| 5           | MSL altitude (meters)          | `56.334`             |
| 6           | Speed (km/h)                   | `0.00`               |
| 7           | Course (degrees)               | `1.1`                |
| 8           | HDOP                           | `1.5`                |
| 14          | GNSS satellites in use         | `6`                  |
*/


bool simcom_gps_init()
{
  Serial.println("\n---Initializing GPS...");

  // Check if the modem is initialized
  if (!simcom_initialized) 
  {
      Serial.println("Modem not initialized. Cannot initialize GPS.");
      simcom_initialized = simcom_init();
      wait(1000);

      if(simcom_initialized) Serial.println("Modem initialized successfully!");
      else //here a counter for 10 seconds if not exit and return
      {
        Serial.println("Failed to initialize modem. Retrying...");
        wait(1000);
        simcom_initialized = simcom_init();
        wait(1000);

        if(!simcom_initialized)
        {
          Serial.println("Failed to initialize modem after retry. Exiting...");
          simcom_gps_initialized = false;
          return false;
        }
        else Serial.println("Modem initialized successfully!");  
      }
  }

  //If simcom_gps is initialized

  // Send AT command to enable GPS
  simcom_serial.println("AT+CGNSPWR=1");
  wait(500); // Wait for the response

  // Read the response
  String response = "";
  while (simcom_serial.available()) 
  {
      char c = simcom_serial.read();
      response += c;
  }

  // Check if the response contains "OK"
  if (response.indexOf("OK") != -1) 
  {
      Serial.println("GPS initialized successfully!");
      simcom_gps_initialized = true;
      return true;
  } 
  else 
  {
      Serial.println("Failed to initialize GPS.");
      simcom_gps_initialized = false;
      return false;
  }

}

void simcom_gps_update() 
{
  if(!simcom_initialized)
  {
    Serial.println("\nModem not initialized. Cannot update GPS.");
    simcom_initialized = simcom_init();

    if(simcom_initialized)
    {
      Serial.println("\nModem initialized successfully!");
      
    }
    else //here a counter for 10 seconds if not exit and return
    {
      Serial.println("\nFailed to initialize modem. Retrying...");
      wait(1000);
      simcom_initialized = simcom_init();

      wait(1000);

      if(!simcom_initialized)
      {
        Serial.println("\nFailed to initialize modem after retry. Exiting...");
        return;
      }
      else Serial.println("\nModem initialized successfully!");
    }

    

  }

  if (!simcom_gps_initialized)
  {
    Serial.println("\nGPS not initialized. Cannot update GPS data.");
    simcom_gps_initialized = simcom_gps_init();

    wait(5000);

    if(simcom_gps_initialized)
    {
      Serial.println("\nGPS initialized successfully!");
      
    }
    else //here a counter for 10 seconds if not exit and return
    {
      Serial.println("\nFailed to initialize GPS. Retrying...");
      wait(1000);
      simcom_gps_initialized = simcom_gps_init();

      wait(1000);

      if(!simcom_gps_initialized)
      {
        Serial.println("\nFailed to initialize GPS after retry. Exiting...");
        return;
      }
      else Serial.println("\nGPS initialized successfully!");
    }
  }

  //If al is already Initialized

  //Serial.println("Requesting GPS data from SIM7080G...");

  // Send the AT+CGNSINF command to get GPS information
  simcom_serial.println("AT+CGNSINF");
  wait(500); // Wait for the response

  // Read the response
  String response = "";
  while (simcom_serial.available()) 
  {
      char c = simcom_serial.read();
      response += c;
  }

  String simcom_data = "";

  // Debug: Print the raw response
  //Serial.println("Raw GPS Response:");
  //Serial.println(response);

  // Check if the response contains valid GPS data
  if (response.indexOf("+CGNSINF:") != -1) 
  {
    // Extract the GPS data from the response
    int startIndex = response.indexOf(":") + 1;
    if (startIndex > 0 && startIndex < response.length()) 
    {
        simcom_data = response.substring(startIndex, response.length());
        simcom_data.trim();
    } 
    else 
    {
        Serial.println("Invalid startIndex for GPS data extraction.");
        return;
    }

    // Debug: Print the extracted GPS data
    //Serial.println("Extracted GPS Data:");
    //Serial.println(simcom_data);

    // Split the GPS data into fields
    int fieldIndex = 0;
    String fields[20]; // Array to hold the fields
    while (simcom_data.length() > 0) 
    {
        int commaIndex = simcom_data.indexOf(",");
        if (commaIndex == -1) 
        {
            fields[fieldIndex++] = simcom_data;
            break;
        } 
        else 
        {
            fields[fieldIndex++] = simcom_data.substring(0, commaIndex);
            simcom_data = simcom_data.substring(commaIndex + 1);
        }
    }

    // Parse the fields and update global variables
    simcom_gps_locked    = (fields[1] == "1"); // Field 1: Fix status (1 = valid fix)
    simcom_gps_latitude  =  fields[3].toFloat(); // Field 3: Latitude
    simcom_gps_longitude =  fields[4].toFloat(); // Field 4: Longitude
    simcom_gps_altitude  =  fields[5].toFloat(); // Field 5: Altitude
    simcom_gps_speed_kmh =  fields[6].toFloat(); // Field 6: Speed (km/h)
    simcom_gps_heading   =  fields[7].toFloat(); // Field 7: Course (heading)
    simcom_gps_hdop      =  fields[8].toFloat(); // Field 8: HDOP
    simcom_gps_sat_count =  fields[14].toInt(); // Field 14: Number of satellites

    // Debug: Print the updated GPS data
    Serial.println("\n------------------------------");
    Serial.println("\nUpdated GPS Data:");
    Serial.printf("Latitude: %f\n", simcom_gps_latitude);
    Serial.printf("Longitude: %f\n", simcom_gps_longitude);
    Serial.printf("Speed (km/h): %f\n", simcom_gps_speed_kmh);
    Serial.printf("Heading: %f\n", simcom_gps_heading);
    Serial.printf("Altitude: %f\n", simcom_gps_altitude);
    Serial.printf("Satellites: %d\n", simcom_gps_sat_count);
    Serial.printf("HDOP: %f\n", simcom_gps_hdop);
    Serial.printf("GPS Locked: %s\n", simcom_gps_locked ? "Yes" : "No");
    Serial.println("------------------------------");
  } 
  else Serial.println("Failed to retrieve GPS data.");
}



//FOR THE LTE--------------------------------------------------------------------------------------


void simcom_lte_get_signal_quality()
{
  Serial.println("\n---Getting LTE Signal Quality...");
  lte_signal_quality = modem.getSignalQuality();
  Serial.printf("---Signal quality: %d <- ",lte_signal_quality);

  String qualityDesc;

  switch (lte_signal_quality) 
  {
      case 0:
          qualityDesc = "(Very poor or no signal, ≤ -113 dBm)";
          break;
      case 1:
          qualityDesc = "(Very poor, -111 dBm)";
          break;
      case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
          qualityDesc = "(Poor, -109 to -91 dBm)";
          break;
      case 10: case 11: case 12: case 13: case 14:
          qualityDesc = "(Fair, -89 to -81 dBm)";
          break;
      case 15: case 16: case 17: case 18: case 19:
          qualityDesc = "(Good, -79 to -71 dBm)";
          break;
      case 20: case 21: case 22: case 23: case 24:
          qualityDesc = "(Very good, -69 to -57 dBm)";
          break;
      case 25: case 26: case 27: case 28: case 29: case 30:
          qualityDesc = "(Excellent, -55 to -43 dBm)";
          break;
      case 31:
          qualityDesc = "(Maximum, ≥ -41 dBm)";
          break;
      case 99:
          qualityDesc = "(Unknown/Not detectable)";
          break;
      default:
          qualityDesc = "(Invalid/Out of range)";
          break;
  }

  Serial.println(qualityDesc);

}


bool simcom_lte_init()
{
  Serial.println("\n---Initializing SIM7080G for CAT-M1...");

  String result ;

  //Checking SIM
  if (modem.getSimStatus() != SIM_READY) 
  {
      Serial.print("\n---SIM Card is not inserted!!!---");
      return false ;
  } 
  else Serial.print("\n---SIM Card Detected---");

  Serial.println("\n---Testing the TinyGSM config for LTE Cat M...");
  
  // Disable RF
  modem.sendAT("+CFUN=0");
  if (modem.waitResponse(20000UL) != 1)
  {
    Serial.println("Disable RF Failed!");
    return false;
  }

  modem.setNetworkMode(2);    // use automatic

  modem.setPreferredMode(MODEM_CATM); // Set the preferred mode to LTE Cat-M

  uint8_t pre = modem.getPreferredMode();

  uint8_t mode = modem.getNetworkMode();

  Serial.printf("\n--- get_network_mode:%u det_preferred_mode:%u\n ---", mode, pre);


  // Set the APN manually. Some operators need to set APN first when registering the network.
  modem.sendAT("+CGDCONT=1,\"IP\",\"", apn, "\"");
  if (modem.waitResponse() != 1) 
  {
    Serial.println("\n---Set operators apn Failed!");
    return false;
  }
  else Serial.println("\n---Set operators apn Succeed!"); 

  //!! Set the APN manually. Some operators need to set APN first when registering the network.
  modem.sendAT("+CNCFG=0,1,\"", apn, "\"");
  if (modem.waitResponse() != 1) 
  {
    Serial.println("Config apn Failed!");
    return false;
  }
  else Serial.println("Config apn Succeed!");

  // Enable RF
  modem.sendAT("+CFUN=1");
  if (modem.waitResponse(20000UL) != 1) 
  {
      Serial.println("Enable RF Failed!");
      return false;
  }
  Serial.println("Enable RF Succeed");


  Serial.print("\n---Waiting for Network Registration---");
  //Wait for the network registration to succeed
  SIM70xxRegStatus s;
  do 
  {
      s = modem.getRegistrationStatus();
      if (s != REG_OK_HOME && s != REG_OK_ROAMING) 
      {
          Serial.print(".");
          wait(500);
      }

  } while (s != REG_OK_HOME && s != REG_OK_ROAMING) ;

  Serial.println();
  Serial.print("---Network register info:");
  Serial.println(register_info[s]);


  // Activate network bearer, APN can not be configured by default,
  // if the SIM card is locked, please configure the correct APN and user password, use the gprsConnect() method
  modem.sendAT("+CNACT=0,1");
  if (modem.waitResponse() != 1) 
  {
    Serial.println("Activate network bearer Failed!");
    return false;
  }
  else Serial.println("Activate network bearer Succeed!");

  // if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
  //     return ;
  // }

  bool res = modem.isGprsConnected();
  Serial.print("---GPRS status:");
  Serial.println(res ? "connected" : "not connected");

  String ccid = modem.getSimCCID();
  Serial.print("---CCID:");
  Serial.println(ccid);

  String imei = modem.getIMEI();
  Serial.print("---IMEI:");
  Serial.println(imei);

  String imsi = modem.getIMSI();
  Serial.print("---IMSI:");
  Serial.println(imsi);

  String cop = modem.getOperator();
  Serial.print("---Operator:");
  Serial.println(cop);

  IPAddress local = modem.localIP();
  Serial.print("---Local IP:");
  Serial.println(local);

  simcom_lte_get_signal_quality();

  if (!simcom_lte_sync_time_and_set_esp32())
  {
    Serial.println("Failed to sync NTP time - certificate validation may fail");
    return false;
  }

  else 
  {
    Serial.print("\n---NTP time synced successfully!");
    Serial.print("\n---Syncing RTC as well...");
    sync_rtc_from_esp32_time();  // Sync to MCP7940
  }

  //Comment this if further Testing is implemented
  return true;

  /*
  Serial.println("\n--- Testing DNS Resolution ---");
  simcom_send_AT_command("AT+CDNSGIP=\"engel-dev-61ef3-default-rtdb.europe-west1.firebasedatabase.app\"");

  Serial.println("\n--- Testing TCP Connection ---");
  simcom_send_AT_command("AT+CIPSTART=\"TCP\",\"34.107.226.223\",\"80\"");

  Serial.println("\n--- Testing SSL/TLS Connection ---");
  simcom_send_AT_command("AT+CIPSSL=1");
  simcom_send_AT_command("AT+CIPSTART=\"TCP\",\"34.107.226.223\",\"443\"");
  */

  

  //TESTS
  //Send HTTP request

  /*
 
  TinyGsmClient client(modem, 0);
  const int     port = 80;

  Serial.println("\nSending HTTP request...");

  Serial.printf("---Connecting to", server);
  if (!client.connect(server, port)) 
  {
    Serial.println("... failed");
    return false;
  } 
  else 
  {
    Serial.println("... success");

    // Make a HTTP GET request:
    client.print(String("GET ") + resource + " HTTP/1.0\r\n");
    client.print(String("Host: ") + server + "\r\n");
    client.print("Connection: close\r\n\r\n");

    // Wait for data to arrive
    uint32_t start = millis();

    while (client.connected()  && 
           !client.available() && 
           millis() - start < 30000L) 
    {
      wait(100);
    };

    // Read data
    start = millis();
    char logo[640] = {'\0',};
    int read_chars = 0;

    while (client.connected() && millis() - start < 10000L) 
    {
      while (client.available()) 
      {
          if (read_chars < sizeof(logo) - 2) { // leave space for null terminator
              logo[read_chars]     = client.read();
              logo[read_chars + 1] = '\0';
              read_chars++;
          } else {
              // Discard extra data to avoid overflow
              client.read();
          }
          start = millis();
          wait(1);
      }
    }

    Serial.println(logo);
    Serial.print("#####  RECEIVED:");
    Serial.print(strlen(logo));
    Serial.println("CHARACTERS");
    client.stop();
  }
  

  //Send HTTPS request
  
  Serial.println("\nSending HTTPS request");

  TinyGsmClientSecure secureClient(modem, 1);
  const int           securePort = 443;

  Serial.printf("Connecting securely to", server);
  if (!secureClient.connect(server, securePort)) 
  {
    Serial.println("... failed");
    return false;
  } 
  else //Connection Succesful
  {
    Serial.println("... success");

    // Make a HTTP GET request:
    secureClient.print(String("GET ") + resource + " HTTP/1.0\r\n");
    secureClient.print(String("Host: ") + server + "\r\n");
    secureClient.print("Connection: close\r\n\r\n");

    // Wait for data to arrive
    uint32_t startS = millis();

    while (secureClient.connected()  && 
           !secureClient.available() && 
           millis() - startS < 30000L ) 
    {
        wait(100);
    };

    // Read data
    startS = millis();
    char logoS[640] = {'\0', };
    int read_charsS = 0;

    while (secureClient.connected() && millis() - startS < 10000L) 
    {
      while (secureClient.available()) 
      {
          if (read_charsS < sizeof(logoS) - 2) 
          {
              logoS[read_charsS] = secureClient.read();
              logoS[read_charsS + 1] = '\0';
              read_charsS++;
          } 
          else 
          {
              secureClient.read();
          }
          startS = millis();
          wait(1);
      }
    }

    Serial.println(logoS);
    Serial.print("---RECEIVED: ");
    Serial.print(int(strlen(logoS)));
    Serial.println(" CHARACTERS");
    secureClient.stop();

    
  }

  */
    
  //---END OF TESTS

  //remember to return true at some point if tests are omitted;
  
}

//Minimal for when the LTE was already succesfu at least once


//TODO continue developing this one 

bool simcom_lte_init_minimal()
{
  Serial.println("\n---Initializing SIM7080G for CAT-M1 (Minimal)...");

  String result ;

  /*
  modem.setNetworkMode(2);    // use automatic

  modem.setPreferredMode(MODEM_CATM); // Set the preferred mode to LTE Cat-M

  // Set the APN manually. Some operators need to set APN first when registering the network.
  modem.sendAT("+CGDCONT=1,\"IP\",\"", apn, "\"");
  if (modem.waitResponse() != 1) 
  {
    Serial.println("\n---Set operators apn Failed!");
    return false;
  }
  else Serial.println("\n---Set operators apn Succeed!"); 

  //!! Set the APN manually. Some operators need to set APN first when registering the network.
  modem.sendAT("+CNCFG=0,1,\"", apn, "\"");
  if (modem.waitResponse() != 1) 
  {
    Serial.println("Config apn Failed!");
    return false;
  }
  else Serial.println("Config apn Succeed!");

  // Enable RF
  modem.sendAT("+CFUN=1");
  if (modem.waitResponse(20000UL) != 1) 
  {
      Serial.println("Enable RF Failed!");
      return false;
  }
  Serial.println("Enable RF Succeed");
  */

  Serial.print("\n---Waiting for Network Registration---");
  //Wait for the network registration to succeed
  SIM70xxRegStatus s;
  do 
  {
      s = modem.getRegistrationStatus();
      if (s != REG_OK_HOME && s != REG_OK_ROAMING) 
      {
          Serial.print(".");
          wait(500);
      }

  } while (s != REG_OK_HOME && s != REG_OK_ROAMING) ;

  Serial.println();
  Serial.print("---Network register info:");
  Serial.println(register_info[s]);

  /*
  // Activate network bearer, APN can not be configured by default,
  // if the SIM card is locked, please configure the correct APN and user password, use the gprsConnect() method
  modem.sendAT("+CNACT=0,1");
  if (modem.waitResponse() != 1) 
  {
    Serial.println("Activate network bearer Failed!");
    return false;
  }
  else Serial.println("Activate network bearer Succeed!");

  // if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
  //     return ;
  // }
 
  bool res = modem.isGprsConnected();
  Serial.print("---GPRS status:");
  Serial.println(res ? "connected" : "not connected");

  String ccid = modem.getSimCCID();
  Serial.print("---CCID:");
  Serial.println(ccid);

  String imei = modem.getIMEI();
  Serial.print("---IMEI:");
  Serial.println(imei);

  String imsi = modem.getIMSI();
  Serial.print("---IMSI:");
  Serial.println(imsi);

  String cop = modem.getOperator();
  Serial.print("---Operator:");
  Serial.println(cop);

  IPAddress local = modem.localIP();
  Serial.print("---Local IP:");
  Serial.println(local);
  */
  
  lte_signal_quality = modem.getSignalQuality();
  Serial.printf("---Signal quality: %d <- ",lte_signal_quality);

  String qualityDesc;

  switch (lte_signal_quality) 
  {
      case 0:
          qualityDesc = "(Very poor or no signal, ≤ -113 dBm)";
          break;
      case 1:
          qualityDesc = "(Very poor, -111 dBm)";
          break;
      case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
          qualityDesc = "(Poor, -109 to -91 dBm)";
          break;
      case 10: case 11: case 12: case 13: case 14:
          qualityDesc = "(Fair, -89 to -81 dBm)";
          break;
      case 15: case 16: case 17: case 18: case 19:
          qualityDesc = "(Good, -79 to -71 dBm)";
          break;
      case 20: case 21: case 22: case 23: case 24:
          qualityDesc = "(Very good, -69 to -57 dBm)";
          break;
      case 25: case 26: case 27: case 28: case 29: case 30:
          qualityDesc = "(Excellent, -55 to -43 dBm)";
          break;
      case 31:
          qualityDesc = "(Maximum, ≥ -41 dBm)";
          break;
      case 99:
          qualityDesc = "(Unknown/Not detectable)";
          break;
      default:
          qualityDesc = "(Invalid/Out of range)";
          break;
  }
  
  Serial.println(qualityDesc);

  if (!simcom_lte_sync_time_and_set_esp32())
  {
    Serial.println("Failed to sync NTP time - certificate validation may fail");
    return false;
  }

  else 
  {
    Serial.print("\n---NTP time synced successfully!");
    Serial.print("\n---Syncing RTC as well...");
    sync_rtc_from_esp32_time();  // Sync to MCP7940
  }

  //Comment this if further Testing is implemented
  return true;

  /*
  Serial.println("\n--- Testing DNS Resolution ---");
  simcom_send_AT_command("AT+CDNSGIP=\"engel-dev-61ef3-default-rtdb.europe-west1.firebasedatabase.app\"");

  Serial.println("\n--- Testing TCP Connection ---");
  simcom_send_AT_command("AT+CIPSTART=\"TCP\",\"34.107.226.223\",\"80\"");

  Serial.println("\n--- Testing SSL/TLS Connection ---");
  simcom_send_AT_command("AT+CIPSSL=1");
  simcom_send_AT_command("AT+CIPSTART=\"TCP\",\"34.107.226.223\",\"443\"");
  */

  

  //TESTS
  //Send HTTP request

  /*
 
  TinyGsmClient client(modem, 0);
  const int     port = 80;

  Serial.println("\nSending HTTP request...");

  Serial.printf("---Connecting to", server);
  if (!client.connect(server, port)) 
  {
    Serial.println("... failed");
    return false;
  } 
  else 
  {
    Serial.println("... success");

    // Make a HTTP GET request:
    client.print(String("GET ") + resource + " HTTP/1.0\r\n");
    client.print(String("Host: ") + server + "\r\n");
    client.print("Connection: close\r\n\r\n");

    // Wait for data to arrive
    uint32_t start = millis();

    while (client.connected()  && 
           !client.available() && 
           millis() - start < 30000L) 
    {
      wait(100);
    };

    // Read data
    start = millis();
    char logo[640] = {'\0',};
    int read_chars = 0;

    while (client.connected() && millis() - start < 10000L) 
    {
      while (client.available()) 
      {
          if (read_chars < sizeof(logo) - 2) { // leave space for null terminator
              logo[read_chars]     = client.read();
              logo[read_chars + 1] = '\0';
              read_chars++;
          } else {
              // Discard extra data to avoid overflow
              client.read();
          }
          start = millis();
          wait(1);
      }
    }

    Serial.println(logo);
    Serial.print("#####  RECEIVED:");
    Serial.print(strlen(logo));
    Serial.println("CHARACTERS");
    client.stop();
  }
  

  //Send HTTPS request
  
  Serial.println("\nSending HTTPS request");

  TinyGsmClientSecure secureClient(modem, 1);
  const int           securePort = 443;

  Serial.printf("Connecting securely to", server);
  if (!secureClient.connect(server, securePort)) 
  {
    Serial.println("... failed");
    return false;
  } 
  else //Connection Succesful
  {
    Serial.println("... success");

    // Make a HTTP GET request:
    secureClient.print(String("GET ") + resource + " HTTP/1.0\r\n");
    secureClient.print(String("Host: ") + server + "\r\n");
    secureClient.print("Connection: close\r\n\r\n");

    // Wait for data to arrive
    uint32_t startS = millis();

    while (secureClient.connected()  && 
           !secureClient.available() && 
           millis() - startS < 30000L ) 
    {
        wait(100);
    };

    // Read data
    startS = millis();
    char logoS[640] = {'\0', };
    int read_charsS = 0;

    while (secureClient.connected() && millis() - startS < 10000L) 
    {
      while (secureClient.available()) 
      {
          if (read_charsS < sizeof(logoS) - 2) 
          {
              logoS[read_charsS] = secureClient.read();
              logoS[read_charsS + 1] = '\0';
              read_charsS++;
          } 
          else 
          {
              secureClient.read();
          }
          startS = millis();
          wait(1);
      }
    }

    Serial.println(logoS);
    Serial.print("---RECEIVED: ");
    Serial.print(int(strlen(logoS)));
    Serial.println(" CHARACTERS");
    secureClient.stop();

    
  }

  */
    
  //---END OF TESTS

  //remember to return true at some point if tests are omitted;
  
}
















void simcom_lte_check_network_status() 
{
  Serial.println("\n---Checking network status...");

  // Get signal quality
  int signalQuality = modem.getSignalQuality();
  Serial.printf("Signal Quality: %d\n", signalQuality);

  // Check registration status
  int networkStatus = modem.getRegistrationStatus();
  if (networkStatus == 1 || networkStatus == 5) {
      Serial.println("Modem is registered on the network.");
  } else {
      Serial.println("Modem is not registered. Check SIM card or signal.");
  }
}

//Not used at the moment , not critical then
bool simcom_lte_connect(const char* apn) 
{
  Serial.println("Starting LTE Cat-M connection...");

  // Step 1: Initialize the modem
  if (!simcom_initialized) 
  {
      Serial.println("Modem not initialized. Initializing now...");
      if (!simcom_init()) 
      {
          Serial.println("Failed to initialize modem.");
          return false;
      }
  }

  //Step 1.5: Check SIM card status
  if (!simcom_lte_check_sim_card_status()) return false;
  wait(1000);

  // Step 1.8
  //Checking that the Module is correctly setup for cat m1 on europe
  //simcom_serial.println("AT+CBANDCFG=\"CAT-M\",1,3,5,8,20,28");
  Serial.println("Configuring SIM7080G for CAT-M1...");

  // Set LTE-M bands (Bands 20 and 8 for Germany)
  simcom_serial.println("AT+CBANDCFG=\"CAT-M\",20,8");
  wait(500);

  // Set network mode to LTE-M only
  simcom_serial.println("AT+CNMP=38");
  wait(500);

  // Set preferred mode to LTE-M
  simcom_serial.println("AT+CMNB=1");
  wait(500);

  // Step 2: Check signal strength
  simcom_lte_check_signal_strength();

  // Step 4: Set APN

  Serial.println("Setting APN...");
  simcom_serial.println("AT+CGDCONT=1,\"IP\",\"TM\"");
  //String apn_command = "AT+CGDCONT=1,\"IP\",\"" + String(apn) + "\"";
  //simcom_serial.println(apn_command);
  wait(500);

  String response = "";
  while (simcom_serial.available()) 
  {
      char c = simcom_serial.read();
      response += c;
  }

  //Serial.println("APN Response:");
  //Serial.println(response);

  if (response.indexOf("OK") == -1) 
  {
      Serial.println("Failed to set APN.");
      return false;
  }

  Serial.println("APN set successfully.");

  
  // Step 5: Activate PDP context
  Serial.println("Activating PDP context...");
  simcom_serial.println("AT+CGACT=1,1");
  wait(500);

  response = "";
  while (simcom_serial.available()) 
  {
      char c = simcom_serial.read();
      response += c;
  }

  //Serial.println("PDP Context Activation Response:");
  //Serial.println(response);

  if (response.indexOf("OK") == -1) 
  {
      Serial.println("Failed to activate PDP context.");
      return false;
  }

  Serial.println("PDP context activated successfully.");
  
  // Step 3: Wait for network registration

  if (!simcom_lte_wait_for_network_registration(60000)) 
  { // Wait up to 60 seconds
      Serial.println("Failed to register on the network.");
      return false;
  }

  //Rechecking Signal strength
  simcom_lte_check_signal_strength() ;

  // Step 6: Check IP address
  Serial.println("Checking IP address...");
  simcom_serial.println("AT+CGPADDR=1");
  wait(500);

  response = "";
  while (simcom_serial.available()) 
  {
      char c = simcom_serial.read();
      response += c;
  }

  Serial.println("IP Address Response:");
  Serial.println(response);

  if (response.indexOf("+CGPADDR:") == -1) 
  {
      Serial.println("Failed to obtain IP address.");
      return false;
  }

  Serial.println("IP address obtained successfully.");
  return true;

}

//If LTE cannot connect we will run the troubleshooter
void simcom_lte_check_signal_strength() 
{
  Serial.println("Checking signal strength...");
  simcom_serial.println("AT+CSQ");
  wait(500);

  String response = "";
  while (simcom_serial.available()) 
  {
      char c = simcom_serial.read();
      response += c;
  }

  //Serial.println("Signal Strength Response:");
  //Serial.println(response);

  // Parse the signal strength
  int rssiStart = response.indexOf(":") + 2;
  int rssiEnd = response.indexOf(",", rssiStart);
  if (rssiStart > 0 && rssiEnd > rssiStart) 
  {
      String rssiStr = response.substring(rssiStart, rssiEnd);
      int rssi = rssiStr.toInt();
      if (rssi == 99) 
      {
          Serial.println("Signal strength: Not detectable.");
      } 
      else 
      {
          Serial.printf("Signal strength (RSSI): %d dBm\n", -113 + 2 * rssi);
      }
  } 
  else 
  {
      Serial.println("Failed to parse signal strength.");
  }
}

bool simcom_lte_wait_for_network_registration(unsigned long timeoutMs) 
{
  Serial.println("Waiting for network registration...");
  unsigned long startTime = millis();

  while (millis() - startTime < timeoutMs) 
  {
      simcom_serial.println("AT+CEREG?");
      wait(1000);

      String response = "";
      while (simcom_serial.available()) 
      {
          char c = simcom_serial.read();
          response += c;
      }

      //Serial.println("Network Registration Response:");
      //Serial.println(response);

      if (response.indexOf(",1") != -1 || response.indexOf(",5") != -1) 
      {
          Serial.println("Modem is registered on the network.");
          return true;
      }

      Serial.println("Modem is not registered. Retrying...");
      wait(2000); // Wait before retrying
  }

  Serial.println("Failed to register on the network within the timeout period.");
  return false;
}

bool simcom_lte_check_sim_card_status() 
{
  Serial.println("Checking SIM card status...");
  simcom_serial.println("AT+CPIN?");
  wait(500);

  String response = "";
  while (simcom_serial.available()) 
  {
      char c = simcom_serial.read();
      response += c;
  }

  //Serial.println("SIM Card Status Response:");
  //Serial.println(response);

  if (response.indexOf("READY") != -1) 
  {
      Serial.println("SIM card is ready.");
      return true;
  } 
  else 
  {
      Serial.println("SIM card is not ready. Check if a PIN is required.");
      return false;
  }
}

//Maybe reuse or fragment this later
void simcom_lte_test_ping() 
{

}

/*

TESTS

ON the setup

simcom_initialized = simcom_init();

	if(simcom_initialized)
	{
		if (simcom_lte_init())
		{
			Serial.println("LTE Cat-M connection established successfully!");
			simcom_lte_initialized = true;
		} 
		else 
		{
			Serial.println("Failed to intialized simcom lte cat-m connection.");
			simcom_lte_initialized = false;
		}
	}
    
 ON THE LOOP
 
	// Update GPS data
	simcom_gps_update();

	// Update OLED display with the latest GPS data
	if (gps_locked) 
	{
		oled_gps_good_data_template();
	} 
	else 
	{
		oled_gps_waiting_data();
	}

	// Refresh the OLED display
	oled_refresh();

	wait(10000); // Wait for 1 second before the next update
	
*/















