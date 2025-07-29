#include <Arduino.h>
#include <tools.h>
#include <i2c.h>
#include <temp.h>
#include <oled.h>

#include <fuel_gauge.h>

#include <MAX17048.h>

MAX17048 fuel_gauge;

//If in doubt check 
//https://github.com/mjbcopland/MAX17048/blob/master/MAX17048.h

void fuel_gauge_init()
{
    if(!i2c_initialized)i2c_init();

    if (!fuel_gauge_initialized)
    {
        if(log_enabled)Serial.println("\n\n---Initializing Fuel Gauge---");

        fuel_gauge.attatch(Wire);

        //fuel_gauge.quickStart();

        //Setting the Alert Thresholds
        //TODO Think if Voltage is the best alert or SOC is better?
        //Advantage from Voltage is that is immediately cleare when a charger is plugged in comparison to SOC
        //Test and decide 
        fuel_gauge.vAlertMinThreshold(low_bat_threshold);

        wait(100);

        if(log_enabled)
        {
            Serial.println("\n\n---Initializing Fuel Gauge---");
            
            Serial.print("VCELL ADC : ");
            Serial.println(fuel_gauge.adc());
            Serial.print("VCELL V   : ");
            Serial.println(fuel_gauge.voltage());
            Serial.print("VCELL SOC : ");
            Serial.print(fuel_gauge.percent());
            Serial.println("%");
            Serial.print("VCELL SOC : ");
            Serial.print(fuel_gauge.accuratePercent());
            Serial.println("%");
            Serial.println();
        }

        fuel_gauge.lowVoltage();

        fuel_gauge_initialized = true;
    }
    else Serial.println("\n\n---ERROR, FUEL GAUGE ALREADY INITIALIZED!");
}

void fuel_gauge_update(int print) // 0:none , 1:necessary , 2:verbose 
{
    //Possible Values
    //fuel_gauge.adc()
    //fuel_gauge.accuratePercent()

    //Practical Values

    fuel_gauge_get_bat_voltage(); 
    fuel_gauge_get_bat_percent();
    fuel_gauge_get_bat_c_rate();

    if(log_enabled && print > 1) 
    {
        Serial.printf("|BAT_V:%.2fV|",bat_voltage); 
        Serial.printf("BAT_%%:%d%%|",bat_percent);
    }       
    
    if(fuel_gauge.lowSOC()) //Low Bat Alert
    {
        if(log_enabled && print > 0) Serial.print("\n ---- LOW BAT SOC! \n");
    }

    if(fuel_gauge.highVoltage()) //High Bat Alert
    {
        if(log_enabled && print > 0) Serial.print("\n ---- BAT HIGH VOLTAGE! \n");
    }

    if(fuel_gauge.lowVoltage() && !charging) //Low Bat Alert
    {
        if(log_enabled && print > 0) Serial.print("\n ---- LOW BAT Voltage! \n");
    }

    if(fuel_gauge.chnageSOC()) //Low Bat Alert
    {
        if(log_enabled && print > 0) Serial.print("\n ---- BAT SOC CHANGED! \n");
    }

    //Clearing LowVoltage Alert if voltage is now OK
    if(bat_voltage > low_bat_threshold && low_bat)
    {
        fuel_gauge.clearAlert(); //The low_bat flag is cleared on GPIO_EXP
        if(log_enabled && print > 0) Serial.println("\n---LOW_BAT_VOLTAGE Alert Cleared---");
    }
}

int fuel_gauge_get_bat_percent()
{
    if(fuel_gauge.percent() != bat_percent)
    {
        bat_percent = fuel_gauge.percent();
    }
    return bat_percent;
}

float fuel_gauge_get_bat_accurate_percent()
{
    return fuel_gauge.accuratePercent(); //NOT STORED AS GLOBAL VAR
}

float fuel_gauge_get_bat_voltage()
{
    if(fuel_gauge.voltage() != bat_voltage)
    {
        bat_voltage = fuel_gauge.voltage();
    }
    return bat_voltage; 
}

float fuel_gauge_get_bat_c_rate()
{

    if(fuel_gauge.crate() != bat_c_rate)
    {
        bat_c_rate = fuel_gauge.crate();
    }
    return bat_c_rate; 
}




