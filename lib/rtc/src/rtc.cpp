
#include <Arduino.h>
#include <MCP7940.h>  // Include the MCP7940 RTC library
#include <sys/time.h>
#include <time.h>

#include <rtc.h>
#include <vars.h>
#include <tools.h>
#include <oled.h>

enum alarmTypes 
{
  matchSeconds,
  matchMinutes,
  matchHours,
  matchDayOfWeek,
  matchDayOfMonth,
  Unused1,
  Unused2,
  matchAll,
  Unknown
};


/***************************************************************************************************
** Declare all program constants                                                                  **
***************************************************************************************************/
const uint32_t SERIAL_SPEED        = 115200;  // Set the baud rate for Serial I/O
const uint8_t  LED_PIN             = esp_built_in_led_pin;      // Arduino built-in LED pin number
const uint8_t  SPRINTF_BUFFER_SIZE = 32;      // Buffer size for sprintf()
/***************************************************************************************************
** Declare global variables and instantiate classes                                               **
***************************************************************************************************/
MCP7940_Class MCP7940;                           // Create an instance of the MCP7940
char          inputBuffer[SPRINTF_BUFFER_SIZE];  // Buffer for sprintf()/sscanf()
/***************************************************************************************************
** Method Setup(). This is an Arduino IDE method which is called upon boot or restart. It is only **
** called one time and then control goes to the main loop, which loop indefinately.               **
***************************************************************************************************/
//TODO , maybe think if the rtc is necessary , maybe just to check for waking up and checking bat


int second = 0;
int minute = 0;
int hour = 0;
int day = 0;
int month = 0;
int year = 0;

bool rtc_calibrated = false;

//OLD IMPLEMENTATION , STILL WORKS WITH WIFI BUT NOT LTE
/*
void rtc_calib() //TODO Just to test , delete this llater and do it correctly
{                 // Arduino standard setup method
  if(log_enabled) Serial.print(F("\nStarting SetAndCalibrate program\n"));  // Show program information
  if(log_enabled) Serial.print(F("- Compiled with c++ version "));
  if(log_enabled) Serial.print(F(__VERSION__));  // Show compiler information
  if(log_enabled) Serial.print(F("\n- On "));
  if(log_enabled) Serial.print(F(__DATE__));
  if(log_enabled) Serial.print(F(" at "));
  if(log_enabled) Serial.print(F(__TIME__));
  if(log_enabled) Serial.print(F("\n"));
  while (!MCP7940.begin()) {  // Initialize RTC communications
    if(log_enabled) Serial.print(F("Unable to find MCP7940M. Checking again in 3s."));  // Show error text
    wait(3000);                                                          // wait a second
  }  // of loop until device is located
  if(log_enabled) Serial.print(F("MCP7940 initialized."));
  while (!MCP7940.deviceStatus()) {  // Turn oscillator on if necessary
    if(log_enabled) Serial.print(F("Oscillator is off, turning it on."));
    bool deviceStatus = MCP7940.deviceStart();  // Start oscillator and return state
    if (!deviceStatus) {                        // If it didn't start
      if(log_enabled) Serial.print(F("Oscillator did not start, trying again."));  // Show error and
      wait(1000);                                                   // wait for a second
    }                // of if-then oscillator didn't start
  }                  // of while the oscillator is off
  MCP7940.adjust();  // Set to library compile Date/Time
  //if(log_enabled) Serial.print(F("Enter the following serial commands:"));
  //if(log_enabled) Serial.print(F("SETDATE yyyy-mm-dd hh:mm:ss"));
  //if(log_enabled) Serial.print(F("CALDATE yyyy-mm-dd hh:mm:ss"));
  pinMode(LED_PIN, OUTPUT);  // Declare built-in LED as output

  rtc_calibrated = true;

} 
*/

//NEW CALIB IMPLEMENTATION < WORKS WITH LTE

// Call this after LTE time sync
void sync_rtc_from_esp32_time()
{
  if (!MCP7940.begin()) 
  {
    Serial.println("[RTC] Unable to find MCP7940M. Aborting RTC sync.");
    return;
  }

  if (!MCP7940.deviceStatus()) 
  {
    Serial.println("[RTC] Oscillator is off. Turning it on...");
    if (!MCP7940.deviceStart()) {
      Serial.println("[RTC] Failed to start oscillator.");
      return;
    }
  }

  time_t now = time(nullptr);
  struct tm *tm_time = localtime(&now);

  // Convert struct tm to DateTime and set RTC
  DateTime dt(
    tm_time->tm_year + 1900,
    tm_time->tm_mon + 1,
    tm_time->tm_mday,
    tm_time->tm_hour,
    tm_time->tm_min,
    tm_time->tm_sec
  );
  MCP7940.adjust(dt);  // Sets the RTC with the current ESP32 time

  Serial.printf("[RTC] Time synced to RTC: %04d-%02d-%02d %02d:%02d:%02d\n",
                tm_time->tm_year + 1900, tm_time->tm_mon + 1, tm_time->tm_mday,
                tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec);

  rtc_calibrated = true;
}

void rtc_calib()
{
  Serial.print(F("\n[RTC] Starting RTC setup and calibration\n"));
  Serial.print(F("- Compiled with c++ version: "));
  Serial.print(F(__VERSION__));
  Serial.print(F("\n- Compiled on "));
  Serial.print(F(__DATE__));
  Serial.print(F(" at "));
  Serial.println(F(__TIME__));

  while (!MCP7940.begin()) {
    Serial.println(F("[RTC] Unable to find MCP7940M. Checking again in 3s..."));
    wait(3000);
  }

  Serial.println(F("[RTC] MCP7940 initialized."));

  while (!MCP7940.deviceStatus()) {
    Serial.println(F("[RTC] Oscillator is off, turning it on."));
    if (!MCP7940.deviceStart()) {
      Serial.println(F("[RTC] Oscillator did not start, trying again in 1s."));
      wait(1000);
    }
  }

  sync_rtc_from_esp32_time();
  pinMode(LED_PIN, OUTPUT);  // Optional debug indicator
}



/***************************************************************************************************
** Method readCommand(). This function checks the serial port to see if there has been any input. **
** If there is data it is read until a terminator is discovered and then the command is parsed    **
** and acted upon                                                                                 **
***************************************************************************************************/

void readCommand() 

{
  static uint8_t inputBytes = 0;              // Variable for buffer position
  while (Serial.available()) {                // Loop while incoming serial data
    inputBuffer[inputBytes] = Serial.read();  // Get the next byte of data
    if (inputBuffer[inputBytes] != '\n' &&
        inputBytes < SPRINTF_BUFFER_SIZE)  // keep on reading until a newline
      inputBytes++;                        // shows up or the buffer is full
    else {
      inputBuffer[inputBytes] = 0;                 // Add the termination character
      for (uint8_t i = 0; i < inputBytes; i++)     // Convert the whole input buffer
        inputBuffer[i] = toupper(inputBuffer[i]);  // to uppercase characters
      if(log_enabled) Serial.print(F("\nCommand \""));
      Serial.write(inputBuffer);
      if(log_enabled) Serial.print(F("\" received.\n"));
      /**********************************************************************************************
      ** Parse the single-line command and perform the appropriate action. The current list of **
      ** commands understood are as follows: **
      ** **
      ** SETDATE      - Set the device time **
      ** CALDATE      - Calibrate device time **
      ** **
      **********************************************************************************************/
      enum commands { SetDate, CalDate, Unknown_Command };  // of commands enumerated type
      commands command;                                     // declare enumerated type
      char     workBuffer[10];                              // Buffer to hold string compare
      sscanf(inputBuffer, "%s %*s", workBuffer);            // Parse the string for first word
      if (!strcmp(workBuffer, "SETDATE"))
        command = SetDate;  // Set command number when found
      else if (!strcmp(workBuffer, "CALDATE"))
        command = CalDate;  // Set command number when found
      else
        command = Unknown_Command;                              // Otherwise set to not found
      uint16_t tokens, year, month, day, hour, minute, second;  // Variables to hold parsed dt/tm
      switch (command) {                                        // Action depending upon command
        /*******************************************************************************************
        ** Set the device time and date                                                           **
        *******************************************************************************************/
        case SetDate:  // Set the RTC date/time
          tokens = sscanf(inputBuffer,
                          "%*s %hu-%hu-%hu %hu:%hu:%hu;",  // Use sscanf() to parse the date/
                          &year, &month, &day, &hour, &minute, &second);  // time into variables
          if (tokens != 6)                                   // Check to see if it was parsed
            if(log_enabled) Serial.print(F("Unable to parse date/time\n"));  
          else {                                             
            MCP7940.adjust(
                DateTime(year, month, day, hour, minute, second));  // Adjust the RTC date/time
            if(log_enabled) Serial.print(F("Date has been set."));               
          }       // of if-then-else the date could be parsed
          break;  //
        /*******************************************************************************************
        ** Calibrate the RTC and reset the time                                                   **
        *******************************************************************************************/
        case CalDate:  // Calibrate the RTC
          tokens = sscanf(inputBuffer,
                          "%*s %hu-%hu-%hu %hu:%hu:%hu;",  // Use sscanf() to parse the date/
                          &year, &month, &day, &hour, &minute, &second);  // time into variables
          if (tokens != 6)  // Check to see if it was parsed
            if(log_enabled) Serial.print(F("Unable to parse date/time\n"));
          else {
            int8_t trim =
                MCP7940.calibrate(DateTime(year, month, day,  // Calibrate the crystal and return
                                           hour, minute, second));  // the new trim offset value
            if(log_enabled) Serial.print(F("Trim value set to "));
            if(log_enabled) Serial.print(trim * 2);  // Each trim tick is 2 cycles
            if(log_enabled) Serial.print(F(" clock cycles every minute"));
          }  // of if-then-else the date could be parsed
          break;
        /*******************************************************************************************
        ** Unknown command                                                                        **
        *******************************************************************************************/
        case Unknown_Command:  // Show options on bad command
        default:
          if(log_enabled) Serial.print(F("Unknown command. Valid commands are:"));
          if(log_enabled) Serial.print(F("SETDATE yyyy-mm-dd hh:mm:ss"));
          if(log_enabled) Serial.print(F("CALDATE yyyy-mm-dd hh:mm:ss"));
      }                // of switch statement to execute commands
      inputBytes = 0;  // reset the counter
    }                  // of if-then-else we've received full command
  }                    // of if-then there is something in our input buffer
} 


/***************************************************************************************************
** This is the main program for the Arduino IDE, it is an infinite loop and keeps on repeating.   **
***************************************************************************************************/
void rtc_test() 
{
  static uint8_t secs;                 // store the seconds value
  DateTime       now = MCP7940.now();  // get the current time
  if (secs != now.second()) {          // Output if seconds have changed
    sprintf(inputBuffer, "%04d-%02d-%02d %02d:%02d:%02d",
            now.year(),  // Use sprintf() to pretty print
            now.month(), now.day(), now.hour(), now.minute(),
            now.second());                         // date/time with leading zeros
    if(log_enabled) Serial.print(inputBuffer);                   // Display the current date/time
    secs = now.second();                           // Set the counter variable
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));  // Toggle the LED
  }                                                // of if the seconds have changed
  readCommand();                                   // See if serial port had incoming
} 

void rtc_update()
{
  //if(!rtc_calibrated) rtc_calib();

  DateTime now = MCP7940.now();  // get the current time

  if (second != now.second()) // Output if seconds have changed
  {   
    year   = now.year();
    month  = now.month(); 
    day    = now.day(); 
    hour   = now.hour(); 
    minute = now.minute();
    second = now.second();           
  }                           
  readCommand();          
}

void rtc_init()//Might not be necessary but good failsafe
{
  if(rtc_initialized)
  {
    if(log_enabled)Serial.print("\n---RTC ALREADY INITIALIZED , EXITING rtc_init() \n");
    return;
  }
  
  while (!MCP7940.begin())  // Loop until the RTC communications are established
  {
    if(log_enabled)Serial.println(F("\n---Unable to find MCP7940. Checking again in 1s."));
    wait(1000);
    //TODO make an exit later if it is not found
  } 

  if(log_enabled) Serial.print(F("\n--- RTC (MCP7940) initialized ---"));
  if(log_enabled) Serial.print("\n---RTC Calib. needed after network time sync--");

  while (!MCP7940.deviceStatus())  // Turn oscillator on if necessary
  {
    //TODO makje an exit here if its not found after some time
    if(log_enabled) Serial.println(F("\n---Oscillator is off, turning it on."));
    bool deviceStatus = MCP7940.deviceStart();  // Start oscillator and return state
    if (!deviceStatus)                          // If it didn't start
    {
      Serial.println(F("\n---Oscillator did not start, trying again."));
      wait(1000);
    }  // of if-then oscillator didn't start
  }    // of while the oscillator is of





  Serial.println("\n --- Calibrating Clock --");
  //rtc_calib();

  rtc_initialized = true;

  //To Test Time acccording to the RTC
  /*
  DateTime now = MCP7940.now();  // get the current time
  // Use sprintf() to pretty print date/time with leading zeroes
  sprintf(inputBuffer, "%04d-%02d-%02d %02d:%02d:%02d", 
          now.year(), now.month(), now.day(),
          now.hour(), now.minute(), now.second());
  
  if(log_enabled) 
  {
    Serial.print("\n--- RTC INFO -> ");
    Serial.println(inputBuffer);
  }

  */  
}

void set_recurrent_alarm(int days,int hours,int minutes,int seconds) //just one option possible
{
  /*
  0	Alarms off
  1	Minutes match
  2	Hours match
  3	Day-of-Week matches
  4	Date matches
  7	Seconds, Minutes, Hours, Day-of-Week, Date and Month match
  */
  
  DateTime now = MCP7940.now();  // get the current time

  if(minutes > 0 )
  {
    if(log_enabled) 
    {
      if (minutes == 1) Serial.print("\n-- Setting recurrent alarm for every minute\n");
      else Serial.printf("\n-- Setting recurrent alarm for every %d minutes \n",minutes);
    }

    //Setting ALARM 0
    // Match once a minute at second Nr. 0
    MCP7940.setAlarm(0, matchSeconds, now - TimeSpan(0, 0, 0, now.second()),true);  

    //TODO : LATER IMPLEMENT FOR MORE MINS IF NECESSARY
  }

  //else print error at the end

}

bool alarm_check(int alarm_id)
{
  if (MCP7940.isAlarm(alarm_id))  // When alarm X is triggered
  {
    if(log_enabled) Serial.printf(" --- ALARM_%d ACTIVE!",alarm_id);
    return true;
  } 
  else return false;
}

void alarm_clear(int alarm_id)
{
  MCP7940.clearAlarm(alarm_id);
  if(log_enabled) Serial.printf(" --- ALARM_%d CLEARED!",alarm_id);
}

void alarm_destroy(int alarm_id)
{
  MCP7940.setAlarm(alarm_id,0,0,0);
  if(log_enabled) Serial.printf(" --- ALARM_%d DESTROYED!",alarm_id);
}

//Will check both and hold the loop if needed
void check_alarms()
{
  //FOR Alarm_0
  if (alarm_check(0))
	{
		Serial.println("\n-- ALARM_0 Active ");

		oled_needed++;
		// If the ALARM_0 is active on boot we will loop until dismissed

		oled_alarm_loop(0);

		wait(500);

		// Stuck untill a button is pressed
		while (1)
		{
			if (digitalRead(esp_btn_1_pin) || digitalRead(esp_btn_2_pin))
				break;
			else
				wait(100);
		}

		alarm_clear(0);
		alarm_destroy(0);
		oled_alarm_cleared_and_destroyed(0);

		wait(3000);

		oled_needed--;
	}

  //TODO : For ALARM_1

}
 

  
                   
  




