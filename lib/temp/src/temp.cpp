
#include <Arduino.h>
#include <Temperature_LM75_Derived.h>

#include "temp.h"
#include <i2c.h>
#include <vars.h>

Generic_LM75 temp_sens(temp_sens_addr_hex);

void temp_init()
{
  // Enable the ALERT pin as active-low.
  temp_sens.setAlertActiveLow();

  // At what temperature should ALERT be enabled when the temperature is
  // increasing? Chosen so that touching the sensor can raise the temperature
  // enough to enable ALERT.
  temp_sens.setTemperatureHighC(board_temp_max_threshold);

  // At what temperature should ALERT be disabled when the temperature is
  // decreasing (to provided hysteresis and prevent rapid cycling of the
  // ALERT pin)?
  temp_sens.setTemperatureLowC(board_temp_min_threshold);
}

int temp_get()
{
  if(!i2c_initialized)i2c_init();

  if(i2c_initialized)
  {
    if(temp_sens.readTemperatureC() != board_temp)
    {
      board_temp = temp_sens.readTemperatureC();
    }
    return board_temp;
  }

  else
  {
    if(log_enabled) Serial.print ("ERROR in get_temp -> I2C not init." );
    return -1;
  } 

 }

void temp_update(int print)
{
  temp_get();

  if(log_enabled && print==2 )
  {  
    Serial.print("TEMP:");
    Serial.print(board_temp);
    Serial.print("C|");
  }  
}
