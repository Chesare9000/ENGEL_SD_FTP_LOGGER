
#include <Arduino.h>
#include <gps.h>
#include <oled.h>
#include <tools.h>
#include <vars.h>
#include <HardwareSerial.h>


//Here define the GPS model as we are using the same vars for all of them

//#define gps_model_fona_sim_808
#define gps_model_quectel_lc29h  


//Global Vars (Shared among all model implementations)

bool gps_initialized = false;
bool gps_locked = false;

bool gps_is_on = false;

//Managed on enum in gps.h
int gps_status = gps_status_not_detected;

bool gps_data_logging_needs_refresh = false;

double gps_latitude  = 0;
double gps_longitude = 0;
double gps_speed_kmh = 0;
double gps_speed_mph = 0; 
double gps_heading   = 0;
double gps_altitude  = 0;
int gps_sat_count    = 0;
int gps_hdop        = 0; //Horizontal Dilution of Precision

//REFRESHED UPON REST 
double old_gps_latitude  = 0;
double old_gps_longitude = 0;
double old_gps_speed_kmh = 0;
double old_gps_speed_mph = 0; 
double old_gps_heading   = 0;
double old_gps_altitude  = 0;
int    old_gps_sat_count = 0;
int    old_sat_count     = 0;



//STATIC THRESHOLD
/* CHANGE TO THIS ONE AFTER TEST
float logging_gps_lat_threshold = 0;
float logging_gps_lon_threshold = 0;
float logging_gps_kph_threshold = 10;
float logging_gps_mph_threshold = 10;
float logging_gps_hea_threshold = 10;
float logging_gps_alt_threshold = 10;
float logging_gps_sat_threshold = 5;
float logging_gps_hdop_threshold = 20;
*/

//MOVING THRESHOLD
float logging_gps_lat_threshold = 0;
float logging_gps_lon_threshold = 0;
float logging_gps_kph_threshold = 0;
float logging_gps_mph_threshold = 0;
float logging_gps_hea_threshold = 0;
float logging_gps_alt_threshold = 0;
float logging_gps_sat_threshold = 0;
float logging_gps_hdop_threshold = 0;



int gps_refresh_seconds_default = 10; //default value for gps refresh seconds
int gps_refresh_seconds = gps_refresh_seconds_default;

//TODO , maybe default false until getting confirmation from firebase?
bool gps_enabled = true;
bool gps_upload = true;

int gps_retry_nr = 0;
int gps_poll_nr = 0;


//Generic Functions

void gps_spoof()
{
    gps_latitude = 52.493814;
    gps_longitude = 13.44829;
    gps_speed_kmh = 1;
    gps_speed_mph = 1;
    gps_heading = 12;
    gps_altitude = 100;
}

void gps_log_serial()
{
    #ifdef gps_model_fona_sim_808

        Serial.print("GPS lat:");
        Serial.println(gps_latitude);
        Serial.print("GPS long:");
        Serial.println(gps_longitude);
        Serial.print("GPS speed KMH:");
        Serial.println(gps_speed_kmh);
        //Serial.print("GPS speed MPH:");
        //gps_speed_mph = gps_speed_kmh * 0.621371192;
        //Serial.println(gps_speed_mph);
        Serial.print("GPS heading:");
        Serial.println(gps_heading);
        Serial.print("GPS altitude:");
        Serial.println(gps_altitude);

    #elif defined(gps_model_quectel_lc29h)

        Serial.println();
        // Latitude and Longitude
        Serial.printf("Latitude:%f\n", gps_latitude);

        Serial.printf("Longitude:%f\n",gps_longitude);
        
        // Altitude
        Serial.print("Altitude (meters): ");
        Serial.println(gps_altitude);

        // Speed
        Serial.print("Speed (km/h): ");
        Serial.println(gps_speed_kmh);
        //Serial.print("Speed (mph): ");
        //Serial.println(gps_speed_mph);

        // Course (Heading)
        Serial.print("Course (degrees): ");
        Serial.println(gps_heading);

        // Number of Satellites
        Serial.print("Satellites: ");
        Serial.println(gps_sat_count);

        // HDOP (Horizontal Dilution of Precision)
        Serial.print("HDOP: ");
        Serial.println(gps_hdop);

    #endif
}


//Specific Functions and Initialization

#ifdef gps_model_fona_sim_808
//THIS IS FOR THE FONA MODULE-----------------------------------------------------------------

    #define ADAFRUIT_FONA_DEBUG false
    #include "Adafruit_FONA.h"

    #define FONA_RST 14
    #define FONA_KEY 16

    #define esp_pin_gps_tx 17 //CONNECTED TO FONA_RX
    #define esp_pin_gps_rx 34 //CONNECTED TO FONA_TX

    // this is a large buffer for replies
    char replybuffer[255];

    HardwareSerial *fonaSerial = &Serial1;
    Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

    


    bool gps_init()
    {    
        int retry_counts = 0;  
        Serial.print("\n--Initializing GPS_F module ...");
        Serial.printf("\nGPS_TX_PIN:%d , GPS_RX_PIN: %d ",esp_pin_gps_tx,esp_pin_gps_rx);

        //To make sure they are corretly configured //TODO move to gpio init later
        pinMode(esp_pin_gps_rx,INPUT);
        pinMode(esp_pin_gps_tx,OUTPUT);

        Serial.println();
        
        //in case the serial was already open
        //Serial.print("\n --- TURNING OFF GPS Serial --- ");
        //fonaSerial -> end();
        //Serial.print("--- Done --- ");
            
        //Resetting Serial communication for GPS
        wait(1000);
        fonaSerial -> end();    
        Serial.print("\n--Initializing GPS Serial ...");
        fonaSerial->begin(4800,SERIAL_8N1,esp_pin_gps_rx,esp_pin_gps_tx);
        Serial.print(" --- \n gps_serial_begin OK , Connecting to Module -- ");
        
        //First Try 
        while(!fona.begin(*fonaSerial)) 
        {
            Serial.printf ("\n GPS MODULE NOT RESPONDING , Retry Nr. %d ", retry_counts + 1 );
            oled_gps_not_detected();
            //wait(1000);
            retry_counts++;

            //Change this if more retry before key switch are desired
            if(retry_counts > -1)
            {
                Serial.print("\nTrying to turn on again the GPS_KEY");
                gps_turn_on();
                return false;
            }
        }

        //Module detected - Proceeding

        Serial.println(("\nGPS Module Detected..."));
        Serial.println(("\nEnabling GPS..."));

        fona.enableGPS(true); 

        Serial.println(("\nGPS is OK"));

        gps_initialized = true;

        Serial.print("\nGPS module initialized...");
        Serial.print("\nWaiting for GPS signal...");

        return true;
    } 

    void gps_poll()
    {
    //If btn_1_count is 0 we log GPS
    //If btn_1_count is 1 we log CAN   

    // if you ask for an altitude reading, getGPS will return false if there isn't a 3D fix
    gps_locked  = fona.getGPS(&gps_latitude, 
                                &gps_longitude, 
                                &gps_speed_kmh, 
                                &gps_heading, 
                                &gps_altitude);
    }


    

    //Wait until is on and confirm with true , otherwise return false
    bool gps_turn_on()
    {
        Serial.print("\n --- TURNING GPS_MODULE_KEY --- ");
        //defining again the key pin as output

        pinMode(FONA_KEY,OUTPUT);

        digitalWrite(FONA_KEY,HIGH);

        wait(2000);

        digitalWrite(FONA_KEY,LOW);
    
        wait(1000);

        //By keeping the key low we force gps_module activation

        if (gps_init()) 
        {
            gps_is_on = true;
            return true;
        }
        else
        {
            gps_is_on = false;
            return false;
        } 
    }

    void gps_reset()
    {
        //Will hard reset the GPS
        Serial.println("Resetting GPS_MODULE");
        pinMode(FONA_RST,OUTPUT);
        digitalWrite(FONA_RST,LOW);
        wait(500);
        digitalWrite(FONA_RST,HIGH);
    }
//--------------------------------------------------------------------------------------------------------------------------------------

#elif defined(gps_model_quectel_lc29h)
    
    //Here start the development for the LC29H from Quectel--------------------------------------------------------------------------------
    //Recognized as 0x50 but not using gps over i2c at the moment

    #include <Wire.h>
    #include <TinyGPS++.h>

    
    TinyGPSPlus gps;
    
    HardwareSerial gps_serial(1); // Use UART1

    #define esp_pin_gps_tx 17 //CONNECTED TO IC Q_RX 
    #define esp_pin_gps_rx 16 //CONNECTED TO IC Q_TX

    #define gps_serial_timeout 3000 //Timeout for the serial read

    bool gps_init()
    {
        //int retry_counts = 0; TODO , maybe ater implement retries  
        Serial.print("\n--Initializing GPS_Q module ...");
        Serial.printf("\nGPS_TX_PIN:%d , GPS_RX_PIN: %d ",esp_pin_gps_tx,esp_pin_gps_rx);

        //To make sure they are corretly configured //TODO move to gpio init later
        pinMode(esp_pin_gps_rx,INPUT);
        pinMode(esp_pin_gps_tx,OUTPUT);

        Serial.println();

        unsigned long start = millis();

        gps_serial.begin(115200, SERIAL_8N1, esp_pin_gps_rx, esp_pin_gps_tx);

        while (millis() - start < gps_serial_timeout) 
        {
            while (gps_serial.available() > 0) 
            {
              char c = gps_serial.read();

              if (gps.encode(c)) 
              {
                if (gps.time.isValid())//Checking if it outputs a time , even if its not correct 
                {
                    //Module detected - Proceeding
                    Serial.println(("\nGPS Module Detected..."));

                    gps_initialized = true;
                    gps_status = gps_status_on_not_fix;

                    Serial.print("\nGPS module initialized...");

                    return true; // Successfully parsed valid location data
                }
              }
              wait(1);
            }
            wait(1);
        }
        // Timed out without receiving valid data
        Serial.println(("\nERROR: GPS Module not Detected ..."));
        gps_status = gps_status_not_detected;
        return false;
    }

    void gps_poll() 
    {
        //Serial.print("\n---Requesting GPS.Data");

        unsigned long timer = millis();
        int gps_serial_read_timeout = 1000; // Timeout for reading GPS data in milliseconds

        // Read and parse NMEA data
        while (gps_serial.available() > 0) 
        {
            char c = gps_serial.read();
            //Serial.print(c); // Print raw NMEA data to the serial monitor
            gps.encode(c);   // Parse the NMEA data
            wait(1);

            if(millis()> timer + gps_serial_read_timeout) 
            {
                //Serial.println("\n---GPS.Data Timeout---");
                break; // Exit if timeout occurs
            }
        }

        //Serial.print("\n---GPS.Data Received..");
        //gps_log_serial();

        // Check if the GPS has a valid location
        if(gps.location.isValid()) gps_locked = true;
        else gps_locked = false;
        
        // Check for changes on the GPS parameters
        //log if sufficient change threshold was detected

        if(moving)
        {
            logging_gps_lat_threshold = 0.0001;
            logging_gps_lon_threshold = 0.0001;
            logging_gps_kph_threshold = 1;
            logging_gps_mph_threshold = 1;
            logging_gps_hea_threshold = 1;
            logging_gps_alt_threshold = 1;
            logging_gps_hdop_threshold = 10;
            logging_gps_sat_threshold = 2;
        }
        else
        {
            logging_gps_lat_threshold = 0.5;
            logging_gps_lon_threshold = 0.5;
            logging_gps_kph_threshold = 10;
            logging_gps_mph_threshold = 10;
            //we will not update heading as it changes a lot during cycles >300 per loop
            logging_gps_hea_threshold = 500;  
            logging_gps_alt_threshold = 10;
            logging_gps_hdop_threshold = 100;
            logging_gps_sat_threshold = 5;
        }
        

        if (gps_latitude != gps.location.lat()) 
        {            
            if (gps_latitude > gps.location.lat() + logging_gps_lat_threshold ||
                gps_latitude < gps.location.lat() - logging_gps_lat_threshold   ) 
            {
                gps_data_logging_needs_refresh = true;                
            }
            //Update variable even if its not logged
            gps_latitude = gps.location.lat();
        }

        if (gps_longitude != gps.location.lng()) 
        {            
            if (gps_longitude > gps.location.lng() + logging_gps_lon_threshold ||
                gps_longitude < gps.location.lng() - logging_gps_lon_threshold) 
            {
                gps_data_logging_needs_refresh = true;                
            }
            gps_longitude = gps.location.lng();
        }

        if (gps_speed_kmh != gps.speed.kmph()) 
        {            
            if (gps_speed_kmh > gps.speed.kmph() + logging_gps_kph_threshold ||
                gps_speed_kmh < gps.speed.kmph() - logging_gps_kph_threshold) 
            {
                gps_data_logging_needs_refresh = true;                
            }
            gps_speed_kmh = gps.speed.kmph();
            //gps_speed_mph = gps.speed.mph();
        }

        if (gps_heading != gps.course.deg()) 
        {            
            if (gps_heading > gps.course.deg() + logging_gps_hea_threshold ||
                gps_heading < gps.course.deg() - logging_gps_hea_threshold) 
            {
            gps_data_logging_needs_refresh = true;                
            }
            gps_heading = gps.course.deg();
        }

        if (gps_altitude != gps.altitude.meters()) 
        {            
            if (gps_altitude > gps.altitude.meters() + logging_gps_alt_threshold ||
                gps_altitude < gps.altitude.meters() - logging_gps_alt_threshold) 
            {
                gps_data_logging_needs_refresh = true;                
            }
            gps_altitude = gps.altitude.meters();
        }

        if (gps_sat_count != gps.satellites.value()) 
        {            
            if (gps_sat_count > gps.satellites.value() + logging_gps_sat_threshold ||
                gps_sat_count < gps.satellites.value() - logging_gps_sat_threshold) 
            {
                gps_data_logging_needs_refresh = true;                
            }
            gps_sat_count = gps.satellites.value();
        }

        if (gps_hdop != gps.hdop.value()) 
        {            
            if (gps_hdop > gps.hdop.value() + logging_gps_hdop_threshold || // No threshold variable for hdop, using 1 as example
                gps_hdop < gps.hdop.value() - logging_gps_hdop_threshold) 
            {
                gps_data_logging_needs_refresh = true;                
            }
            gps_hdop = gps.hdop.value();
        }

        //If any parameter has changed, log the updated data
        //After logging on main task we have to flip this bool flag    
    }

    void gps_serial_send_command(const char* cmd) 
    {
        gps_serial.println(cmd);
        Serial.print("Sent: ");
        Serial.println(cmd);
        delay(500); // Give time for the module to respond

        // Read and print the response
        Serial.print("Response: ");
        unsigned long start = millis();
        while (millis() - start < 2000) 
        { // Wait up to 2 seconds for a response
            while (gps_serial.available() > 0) 
            {
                char c = gps_serial.read();
                Serial.print(c); // Print each character of the response
                //delay(1);
            }
            wait(1);
        }
        Serial.println(); // Add a newline after the response
    }

    
#endif
