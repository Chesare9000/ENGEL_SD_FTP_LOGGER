#include <Arduino.h>
#include <Adafruit_LTR329_LTR303.h>

#include <lux.h>
#include <vars.h>
#include <i2c.h>



Adafruit_LTR303 ltr = Adafruit_LTR303();

void lux_init() 
{
    if(!i2c_initialized)i2c_init();

    if(i2c_initialized)
    {
        if(log_enabled)Serial.println("----Initializing LUX LTR-303----");
    
        if (!ltr.begin())
        {
            if(log_enabled) Serial.println("---ERROR -> LTR sensor not found!");
            return;
        } 
        else
        {
            if(log_enabled) Serial.println("\n -- Setting Up Params---");
            // Set gain of 1 (see advanced demo for all options!
            ltr.setGain(LTR3XX_GAIN_1);
            // Set integration time of 200ms (see advanced demo for all options!
            ltr.setIntegrationTime(LTR3XX_INTEGTIME_200);
            // Set measurement rate of 200ms (see advanced demo for all options!
            ltr.setMeasurementRate(LTR3XX_MEASRATE_200);  

            // The LTR-303 has interrupt output support, we can enable the pin output!
            ltr.enableInterrupt(true);
            // The INT pin also has a polarity setting. For active LOW set to 'false',
            // for active HIGH set to 'true'
            ltr.setInterruptPolarity(false);


            // Then set the low threshold (values BELOW this trigger an interrupt)
            ltr.setLowThreshold(lux_low_threshold);
            
            //NOT USING HIGH THRESHOLD at the moment
            // and set the high threshold (values ABOVE this trigger an interrupt)
            //ltr.setHighThreshold(30000);

            Serial.print("Low Threshold: ");  Serial.print(ltr.getLowThreshold());
            //Serial.print("\n High Threshold: ");Serial.println(ltr.getHighThreshold());

            // Finally, default is an interrupt on every value that is under/over the
            // threshold ranges. However, you're more likely to get spurious IRQs, so
            // we can set it to require "N counts in a row" before an IRQ. 1 count is
            // IRQ for each reading, 2 count means we need two outside readings in a row, etc
            // up to 16.
            ltr.setIntPersistance(lux_int_persistance_counts);
            Serial.print("\nConsecutive counts for IRQ: ");  Serial.println(ltr.getIntPersistance());

            lux_initialized = true;
            if(log_enabled) Serial.println("\n --LUX_Sens Initialized ---");        

        }   
    }    
}


int lux_get()
{
  if(!lux_initialized) lux_init();

  if(lux_initialized)
  {
    bool valid;
    uint16_t visible_plus_ir, infrared;

    if (ltr.newDataAvailable()) 
    {
        valid = ltr.readBothChannels(visible_plus_ir, infrared);
        if (valid) 
        {
          //ir not in use atm
          lux_val = visible_plus_ir;
          return lux_val;
        }
        else return -999;
    }
  } 
  
}

//Just store locally without updatring the variables
void lux_print() 
{   
  if(lux_get() && log_enabled)
  {
    //include the log flag here 
    Serial.print("LUX: ");
    Serial.println(lux_val);
  }    
}
