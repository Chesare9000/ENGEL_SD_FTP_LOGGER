
#include <Arduino.h>

#include "serial.h"
#include <tools.h>
#include <vars.h>

void serial_init()//default is 115200
{
  //in order to have visible output at runtime we wait a second before spitting any log
  if(!serial_initialized)
  {
    Serial.begin(115200);
    while (!Serial);//wait for serial monitor
    wait(1000);
    serial_initialized = true;
    
    if(log_enabled) Serial.println();
    if(log_enabled) Serial.println("---- Serial Initialized----");
    if(log_enabled) Serial.printf("-----ENGEL V%d-------\n",hw_version);
  }
  else if(log_enabled) Serial.println ("ERROR ON SERIAL_INIT()-> Serial already initialized !");
}