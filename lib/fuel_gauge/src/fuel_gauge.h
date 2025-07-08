#pragma once

#include <Arduino.h>
#include <vars.h>



/*
bool fuel_gauge_initialized = false;

bool charging = false; //Based on Charging status Pin  
bool fuel_gauge_pgood = false; //Pin to indicate usb connected with 5V

int bat_percent = -1;
float bat_voltage = -1; 
*/

void fuel_gauge_init();

void fuel_gauge_update(int print);

int   fuel_gauge_get_bat_percent();
float fuel_gauge_get_bat_accurate_percent();
float fuel_gauge_get_bat_voltage();
float fuel_gauge_get_bat_c_rate();
