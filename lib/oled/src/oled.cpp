#include <U8g2lib.h>
#include <WiFi.h>

#include <vars.h>
#include <tools.h>
#include <i2c.h>
#include <fuel_gauge.h>

#include "oled.h"

#include <ble_demo_fw.h>
#include <mqtt.h>


#define acc_threshold 100

bool oled_dev_mode_enabled = false;

int old_lux_val = 0;

int aux_counter = 0;

bool oled_needs_clear = false;

//For I2C_manager compliancce
bool oled_needs_refresh = false;


int firebase_oled_last_update = 0;
int firebase_oled_update_interval = 1000;

//For the oled_dev_screen_nr_nr
int oled_dev_screen_nr_default = 6 ;
int oled_dev_screen_nr = oled_dev_screen_nr_default;
int oled_dev_screen_nr_max = 7;


//Display Class 
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R3,
                              /* clock=*/esp_scl_pin,
                              /* data=*/ esp_sda_pin,
                              /* reset=*/U8X8_PIN_NONE);  

void oled_init()
{
    if(log_enabled)Serial.print("\n--OLED_INITIALIZED--\n");

    oled.begin();
    oled.clearBuffer();				
    oled.setFont(u8g2_font_5x7_tf);

    oled.drawStr(0,10,"ENGEL V3");

    //TODO, decide if automatic or manual    
    //create_task_oled_normal_refresh();

    oled_initialized = true;  
}


//For all fns regarding OLED we will 
//update the var: oled_needs_refresh = true;
//and the i2c_manager will decide when to oled.updateDisplay(); 


void oled_clear()
{
    oled.clearBuffer();
    oled.clearDisplay();  // clear RAM in the display
}

void oled_refresh()
{
    oled.sendBuffer();
    //oled.updateDisplay();
    oled_needs_refresh = false; //Resetting flag
}


//This wont work with I2C manager , is just a test
void oled_gps_test(int gps_test_nr)
{
    //changing to normal bigger font
    oled.setFont(u8g2_font_5x7_tf);

    oled.clearBuffer();	

    oled.setCursor(0,20);
    oled.print("GPS TEST" );

    oled.setCursor(0,50);
    oled.printf("LAT : %.2f ", gps_latitude );

    oled.setCursor(0,60);
    oled.printf("LNG : %.2f ", gps_longitude );

    oled.setCursor(0,70);
    oled.printf("KPH : %.2f ", gps_speed_kmh );

    oled.setCursor(0,80);
    oled.printf("SAT : %d ", gps_sat_count );

    
    oled.setCursor(0,100);
    oled.printf("TEST_NR : %d ", gps_test_nr );

    /*
    oled.setCursor(0,90);
    oled.printf("HEA : %f ", gps_heading );

    oled.setCursor(0,100);
    oled.printf("ALT : %f ", gps_altitude );
    */

    oled.sendBuffer();
    //TODO delkete this after i2c_manager implementation
    //oled_needs_refresh = true;
}




void oled_gps_not_detected()
{
    oled.drawStr(0,50,"GPS");
    oled.drawStr(0,60,"NOT DETECTED");

    oled_needs_refresh = true;
}

void oled_gps_waiting_data()
{
    //changing to normal bigger font
    oled.setFont(u8g2_font_5x7_tf);
    oled.clearBuffer();
    oled.drawStr(0,50,"WAITING");
    oled.drawStr(0,60,"GPS DATA");
    oled_needs_refresh = true;
}

void oled_gps_good_data_template()
{
    //changing to normal bigger font
    oled.setFont(u8g2_font_5x7_tf);

    oled.clearBuffer();	

    oled.setCursor(0,20);
    oled.print("3D FIX OK" );

    oled.setCursor(0,50);
    oled.printf("LAT : %f ", gps_latitude );

    oled.setCursor(0,60);
    oled.printf("LNG : %f ", gps_longitude );

    oled.setCursor(0,70);
    oled.printf("KPH : %f ", gps_speed_kmh );

    oled.setCursor(0,80);
    oled.printf("MPH : %f ", gps_speed_mph );

    oled.setCursor(0,90);
    oled.printf("HEA : %f ", gps_heading );

    oled.setCursor(0,100);
    oled.printf("ALT : %f ", gps_altitude );

    oled.setCursor(0,110);
    oled.printf("SAT : %d ", gps_sat_count );

    oled_needs_refresh = true;
}

void oled_gps_bad_data_template()
{
    oled.clearBuffer();	

    oled.setCursor(0,20);
    oled.print("BAD GPS DATA" );

    oled.setCursor(0,50);
    oled.printf("LAT : %f ", gps_latitude );

    oled.setCursor(0,60);
    oled.printf("LONG : %f ", gps_longitude );

    oled.setCursor(0,70);
    oled.printf("KPH : %f ", gps_speed_kmh );

    oled.setCursor(0,80);
    oled.printf("MPH : %f ", gps_speed_mph );

    oled.setCursor(0,90);
    oled.printf("HEA : %f ", gps_heading );

    oled.setCursor(0,100);
    oled.printf("ALT : %f ", gps_altitude );

    oled.setCursor(0,110);
    oled.printf("SAT : %d ", gps_sat_count );

    oled_needs_refresh = true;
}

void oled_gps_spoof_question()
{
    //changing to normal bigger font
    oled.setFont(u8g2_font_5x7_tf);
    oled.clear();
    
    oled.drawStr(0,60,"RUN");
    oled.drawStr(0,50,"GPS_SPOOF?");
    
    oled.drawStr(0,100," NO     YES ");     
    
    oled_needs_refresh = true;    
}


void oled_gps_spoofed_data_template()
{
    oled.clearBuffer();	

    oled.setCursor(0,20);
    oled.print("SPOOFED GPS" );

    oled.setCursor(0,50);
    oled.printf("LAT : %f ", gps_latitude );

    oled.setCursor(0,60);
    oled.printf("LNG : %f ", gps_longitude );

    oled.setCursor(0,70);
    oled.printf("KPH : %f ", gps_speed_kmh );

    oled.setCursor(0,80);
    oled.printf("MPH : %f ", gps_speed_mph );

    oled.setCursor(0,90);
    oled.printf("HEA : %f ", gps_heading );

    oled.setCursor(0,100);
    oled.printf("ALT : %f ", gps_altitude );

    oled.setCursor(0,110);
    oled.printf("SAT : %d ", gps_sat_count );

    oled_needs_refresh = true;
}

void oled_gps_disabled()
{
    oled.clearBuffer();	

    oled.setCursor(0,30);
    oled.print("GPS" );

    oled.setCursor(0,50);
    oled.print("DISABLED!" );

    oled_needs_refresh = true;
}

void oled_gps_spoofing_disabled()
{
    oled.clearBuffer();	

    oled.setCursor(0,30);
    oled.print("GPS SPOOFING" );

    oled.setCursor(0,50);
    oled.print("DISABLED!" );

    oled_needs_refresh = true;
}



void oled_can_not_detected()
{
    oled.drawStr(0,50,"CAN");
    oled.drawStr(0,60,"NOT DETECTED");

    oled_needs_refresh = true;
}

void oled_can_waiting_data()
{
    oled.drawStr(0,50,"WAITING");
    oled.drawStr(0,60,"CAN DATA");
    oled_needs_refresh = true;
}

void oled_error_rtc_not_calibrated()
{

    oled.drawStr(0,30,"ERROR");
    oled.drawStr(0,40,"NO WIFI,");
    oled.drawStr(0,60,"SO RTC NOT");
    oled.drawStr(0,70,"CALIBRATED!");


    oled.drawStr(0,90,"RETURNING");
    oled.drawStr(0,100,"TO ");
    oled.drawStr(0,110,"MENU!");

    oled_needs_refresh = true;

}

void oled_ftp_wifi_success()
{
    oled.drawStr(0,30,"UPLOAD");
    oled.drawStr(0,40,"SUCCESSFUL");  

    oled.drawStr(0,60,"RETURNING");
    oled.drawStr(0,70,"TO");
    oled.drawStr(0,80,"MENU");

    oled_needs_refresh = true;

}

void oled_logger_error_on_sd()
{
    oled.drawStr(0,30,"ERROR"); 
    oled.drawStr(0,40,"ON SD"); 

    oled.drawStr(0,60,"RETURNING");
    oled.drawStr(0,70,"TO");
    oled.drawStr(0,80,"MENU");

    oled_needs_refresh = true;

}

void oled_logger_blak_box_killed()
{
    oled.drawStr(0,30,"BLACKBOX"); 
    oled.drawStr(0,40,"KILLED"); 

    oled.drawStr(0,60,"RETURNING");
    oled.drawStr(0,70,"TO");
    oled.drawStr(0,80,"MENU");

    oled_needs_refresh = true;

}



void oled_starting_ftp_via_wifi()
{
    oled.drawStr(0,30,"RUNNING");
    oled.drawStr(0,40,"FTP");  
    oled.drawStr(0,50,"VIA");
    oled.drawStr(0,60,"WIFI");
  
    oled_needs_refresh = true;
}


void oled_can_data_template()
{
  //setting to smaller font   
  oled.setFont(u8g2_font_tiny5_tr); 
  oled.clearBuffer();	

  oled.setCursor(0,10);
  if(mubea_can_motor_power == 127) oled.print("m_pow:?");
  else oled.printf("m_pow:%d W",mubea_can_motor_power);

  oled.setCursor(0,20);
  if(mubea_can_motor_rpm == 127) oled.print("m_rpm:?");
  else oled.printf("m_rpm:%d rpm",mubea_can_motor_rpm);

  oled.setCursor(0,30);
  if(mubea_can_motor_temp == 127) oled.print("m_tmp:?");
  else oled.printf("m_tmp:%d C",mubea_can_motor_temp);

  oled.setCursor(0,40);
  if(mubea_can_gen_power == 127) oled.print("g_pow:?");
  else oled.printf("g_pow:%d W",mubea_can_gen_power);

  oled.setCursor(0,50);
  if(mubea_can_assist_level == 127) oled.print("a_lvl:?");
  else oled.printf("a_lvl:%d",mubea_can_assist_level);
 

  oled.setCursor(25,50);
  if(mubea_can_soc == 127) oled.print("soc:?");
  else oled.printf("soc:%d%% ",mubea_can_soc);

  oled.setCursor(0,60);
  if(mubea_can_soh == 127) oled.print("h:?");
  else oled.printf("h:%d%%",mubea_can_soh);

  oled.setCursor(28,60);
  if(mubea_can_power == 127) oled.print("p:?");
  else oled.printf("p:%d W",mubea_can_power);

  oled.setCursor(0,70);
  float voltage = (float(mubea_can_voltage) / 1000.0f);
  if(mubea_can_voltage == 127) oled.print("volt:?");
  else oled.printf("volt:%.2f V",voltage); 
  
  oled.setCursor(0,80);
  if(mubea_can_temperature == 127) oled.print("temp:?");
  else oled.printf("temp:%d C",mubea_can_temperature - 273);
  
  oled.setCursor(0,90);
  if(mubea_can_speed == 127) oled.print("speed:?");
  else oled.printf("speed:%d km/h",mubea_can_speed);
 

  oled.setCursor(0,100);
  if(mubea_can_direction == 127) oled.print("dir:?");
  else oled.printf("dir:%d",mubea_can_direction);
 

  oled.setCursor(22,100);
  if(mubea_can_gear == 127) oled.print("gear:?");
  else oled.printf("gear:%d",mubea_can_gear);
  
  oled.setCursor(0,110);
  if(mubea_can_mileage == 127) oled.print("mil:?");
  else oled.printf("mil:%d km",mubea_can_mileage);

  oled.setCursor(0,120);
  if(mubea_can_error_code == 127) oled.print("e_code:?");
  else oled.printf("e_code:%d",mubea_can_error_code);

  oled.setCursor(37,120);
  if(mubea_can_recuperation == 127) oled.print("rcp:?");
  else oled.printf("rcp:%d",mubea_can_recuperation);

  oled_needs_refresh = true;
}

void oled_can_disabled()
{
    //changing to normal bigger font
    oled.setFont(u8g2_font_5x7_tf);
    oled.clearBuffer();	

    oled.setCursor(0,30);
    oled.print("CAN" );

    oled.setCursor(0,50);
    oled.print("DISABLED!" );

    oled_needs_refresh = true;
}


/*TODO void oled_printf(int x , int y , String Message)*/

void oled_test()
{
    oled.drawStr(0,10,"TEST0");
    oled.drawStr(0,20,"TEST1A");
    oled.drawStr(0,30,"TEST2AA");
    oled.drawStr(0,40,"TEST3AAA");
    oled.drawStr(0,50,"TEST4AAA");
    oled.drawStr(0,60,"TEST5AAA");
    oled.drawStr(0,70,"TEST6");
    oled.drawStr(0,80,"TEST7");
    oled.drawStr(0,90,"TEST8");
    oled.drawStr(0,100,"TEST9");
    oled.drawStr(0,110,"TEST10");
    oled.drawStr(0,120,"TEST11");
    
    //oled.updateDisplay();
    
    oled_needs_refresh = true;
}  

void oled_template_mode_ride() //Show the relevant info for the mode
{
    //Eliminate any previous value for the oled
    oled.clearBuffer();

    //Calendar
    oled.setCursor(0,10);
    oled.printf("%02d/%02d/%04d",day,month,year);    
    
    //Time
    oled.setCursor(0,20);
    oled.printf("%02d:%02d:%02d",hour,minute,second);    
    
    //Riding Mode
    oled.drawStr(0,30,"ENGEL_V4");
    oled.drawStr(0,40,"MODE : PARK");
    oled.drawStr(0,50,"STATUS : 10");  

    oled.setCursor(0,60);
    oled.printf("X:%d",(int)imu_pitch);

    oled.setCursor(25,60);
    //oled.printf("| %d",imu_acc_x); //Real Value
    if(imu_acc_x > acc_threshold) oled.print("| 1");
    else if(imu_acc_x < (acc_threshold*-1)) oled.print("| -1");
    else oled.print("| 0");


    oled.setCursor(0,70);
    oled.printf("Y:%d",(int)imu_roll);

    oled.setCursor(25,70);
    //oled.printf("| %d",imu_acc_y); //Real Value
    if(imu_acc_y > acc_threshold) oled.print("| 1");
    else if(imu_acc_y < (acc_threshold*-1)) oled.print("| -1");
    else oled.print("| 0");


    oled.setCursor(0,80);
    oled.printf("Z:%d",(int)imu_yaw);

    oled.setCursor(25,80);
    //oled.printf("| %d",imu_acc_z); //Real Value
    if(imu_acc_z > acc_threshold) oled.print("| 1");
    else if(imu_acc_z < (acc_threshold*-1)) oled.print("| -1");
    else oled.print("| 0");
       
    
    oled.setCursor(0,90);
    oled.printf("TEMP: %d C",board_temp);

    oled.setCursor(0,100);
    oled.printf("LUX : %d",lux_val);   

    oled.setCursor(0,110);
    oled.printf("BAT : %dp",bat_percent);

    if(lora_enabled) oled.drawStr(0,120,"LORA: ON");
    else oled.drawStr(0,120,"LORA: OFF"); 

    //Send
    //oled.updateDisplay();
    oled_needs_refresh = true;
} 

void oled_template_mode_devel(int cursor_pos) //Show the relevant info for the mode
{
    //Eliminate any previous value for the oled
    oled.clearBuffer();

    //DEVEL MODE
    oled.drawStr(0,10,"ENGEL_V4");
    oled.drawStr(0,20,"DEVEL. MENU");

    //OPTIONS
    oled.setCursor(0,30);
    oled.printf("SELECT MODE");  

    
    oled.setCursor(15,50);
    oled.printf("DEMO");

    oled.setCursor(15,60);
    oled.printf("FIRST");

    oled.setCursor(15,70);
    oled.printf("PARK");   

    oled.setCursor(15,80);
    oled.printf("RIDE");

    oled.setCursor(15,90);
    oled.printf("BLE");

    oled.setCursor(15,100);
    oled.printf("WIFI");

    oled.setCursor(15,110);
    oled.printf("MUBEA");

    oled.setCursor(15,120);
    oled.printf("EXIT");

    //Here cursor

    oled.setCursor(0,cursor_pos);
    oled.printf("->");  

    //Send    
    //oled.updateDisplay();
    oled_needs_refresh = true;
} 

void oled_template_logger_sd_ftp(int cursor_pos) //Show the relevant info for the mode
{
    //Eliminate any previous value for the oled
    oled.clearBuffer();

    oled.drawStr(0,10,"ENGEL_V4");

    oled.drawStr(0,20,"LOGGER MENU");

    oled.setCursor(0,30);
    if(WiFi.isConnected())
    {
        //setting to smaller font   
        oled.setFont(u8g2_font_tiny5_tr); 
        oled.print(WiFi.localIP());
        oled.setFont(u8g2_font_5x7_tf);
    }
    else oled.printf("WiFi : N/C");

    oled.setCursor(0,40);
    oled.printf("BB:%d ms", black_box_logging_interval_milliseconds);

    //Consider starting here the Arrow gap on x

    oled.setCursor(10,50);
    oled.printf("WiFi LIST");
    
    oled.setCursor(10,60);
    oled.print("WIFI CONN.");

    oled.setCursor(10,70);
    oled.print("LOG_MS");   

    oled.setCursor(10,80);
    oled.print("START LOG");

    oled.setCursor(10,90);
    oled.print("FTP_HS");

    oled.setCursor(10,100);
    oled.print("FTP_WIFI");

    oled.setCursor(10,110);
    oled.print("LEDS");

    oled.setCursor(10,120);
    oled.print("OTA");

    //Here cursor

    oled.setCursor(0,cursor_pos);
    oled.printf(">");  

    //Send    
    //oled.updateDisplay();
    oled_needs_refresh = true;
}



void oled_template_logger_wifi_menu(int cursor_pos)
{
    //Eliminate any previous value for the oled
    oled.clearBuffer();

    //DEVEL MODE
    oled.drawStr(0,10,"ENGEL_V4");
    oled.drawStr(0,20,"WIFI MENU");

    //OPTIONS
    oled.setCursor(10,50);
    oled.printf("CESAR");  

    oled.setCursor(10,60);
    oled.printf("MARVIN");

    oled.setCursor(10,70);
    oled.printf("MOTION");

    oled.setCursor(10,80);
    oled.printf("HQ");   

    oled.setCursor(10,90);
    oled.printf("OTA");

    //Here cursor

    oled.setCursor(0,cursor_pos);
    oled.printf(">");  

    //Send    
    //oled.updateDisplay();
    oled_needs_refresh = true;
} 

void oled_template_logger_ms_menu(int cursor_pos)
{
    //Eliminate any previous value for the oled
    oled.clearBuffer();

    //DEVEL MODE
    oled.drawStr(0,10,"ENGEL_V4");
    oled.drawStr(0,20,"LOGGER_MS");
    oled.drawStr(0,30,"SELECT GAP");

    //OPTIONS
    oled.setCursor(10,50);
    oled.printf("10 ms");  

    oled.setCursor(10,60);
    oled.printf("20 ms");

    oled.setCursor(10,70);
    oled.printf("50 ms");

    oled.setCursor(10,80);
    oled.printf("100 ms");   

    oled.setCursor(10,90);
    oled.printf("1000 ms");

    //Here cursor

    oled.setCursor(0,cursor_pos);
    oled.printf(">");  

    //Send    
    //oled.updateDisplay();
    oled_needs_refresh = true;
} 


void oled_template_demo(int cursor_pos) //Show the relevant info for the mode
{
    //Eliminate any previous value for the oled
    oled.clearBuffer();

    //DEVEL MODE
    oled.drawStr(0,10,"ENGEL_V4");
    oled.drawStr(0,20,"DEMO");

    //OPTIONS
    oled.setCursor(0,30);
    oled.printf("SELECT DEMO");  

    
    oled.setCursor(15,50);
    oled.printf("LEDS ");

    oled.setCursor(15,60);
    oled.printf("LUX");

    oled.setCursor(15,70);
    oled.printf("IMU POS");   

    oled.setCursor(15,80);
    oled.printf("IMU ACC");

    oled.setCursor(15,90);
    oled.printf("LOCK");

    oled.setCursor(15,100);
    oled.printf("BUZZER");

    oled.setCursor(15,110);
    oled.printf("STATUS");

    oled.setCursor(15,120);
    oled.printf("EXIT");

    //if(cursor_pos>100)cursor_pos=120;

    oled.setCursor(0,cursor_pos);
    oled.printf("->");  

    //Send    
    //oled.updateDisplay();
    oled_needs_refresh = true;
} 

void oled_template_buzzer(int cursor_pos)
{
    //Eliminate any previous value for the oled
    oled.clearBuffer();

    //DEVEL MODE
    oled.drawStr(0,10,"ENGEL_V4");
    oled.drawStr(0,20,"DEMO BUZZER");

    //OPTIONS
    oled.setCursor(0,30);
    oled.printf("SELECT TONE");  

    
    oled.setCursor(15,50);
    oled.printf("TWO TONES");

    oled.setCursor(15,60);
    oled.printf("RISE");

    oled.setCursor(15,70);
    oled.printf("FALL");   

    oled.setCursor(15,80);
    oled.printf("RISE & FALL");

    oled.setCursor(15,90);
    oled.printf("NOKIA");

    oled.setCursor(15,100);
    oled.printf("TETRIS");

    oled.setCursor(15,110);
    oled.printf("J.BELLS");

    oled.setCursor(15,120);
    oled.printf("EXIT");

    oled.setCursor(0,cursor_pos);
    oled.printf("->");  

   //Will refresh when possible
    oled_needs_refresh = true;
} 

void oled_logger_error_wifi()
{
    //Eliminate any previous value for the oled
    oled.clearBuffer();

    //DEVEL MODE
    oled.drawStr(0,10,"ENGEL_V4");
    oled.drawStr(0,20,"LOGGER");

  
    oled.setCursor(0,50);
    oled.printf("ERROR");

    oled.setCursor(0,60);
    oled.printf("WIFI NOT");

    oled.setCursor(0,80);
    oled.print("CONNECTED");   

    oled.setCursor(0,90);
    oled.printf("RETURNING");

    oled.setCursor(0,100);
    oled.print("TO MENU");

   //Will refresh when possible
    oled_needs_refresh = true;
} 


void oled_logger_uploading(int current , int total )
{
    //Eliminate any previous value for the oled
    oled.clearBuffer();

    //DEVEL MODE
    oled.drawStr(0,10,"ENGEL_V4");
    oled.drawStr(0,20,"LOGGER");

  
    oled.setCursor(0,50);
    oled.printf("UPLOADING");

    oled.setCursor(0,60);
    oled.printf("FILE");

    oled.setCursor(0,80);
    oled.print(current);   

    oled.setCursor(0,90);
    oled.printf("OF");

    oled.setCursor(0,100);
    oled.print(total);

   //Will refresh when possible
    oled_needs_refresh = true;
} 

void oled_logger_separating(int part)
{
    //Eliminate any previous value for the oled
    oled.clearBuffer();

    //DEVEL MODE
    oled.drawStr(0,10,"ENGEL_V4");
    oled.drawStr(0,20,"LOGGER");

  
    oled.setCursor(0,50);
    oled.printf("SEPARATING");

    oled.setCursor(0,60);
    oled.printf("FILE");

    oled.setCursor(0,80);
    oled.print("EXTRACTING");   

    oled.setCursor(0,90);
    oled.printf("PART");

    oled.setCursor(0,100);
    oled.print(part);

   //Will refresh when possible
    oled_needs_refresh = true;
} 














void oled_demo_patterns(int pattern)
{
    oled.clearBuffer();

    oled.drawStr(0,10,"ENGEL_V4");
    oled.drawStr(0,20,"DEMO_LEDS");

    oled.setCursor(0,50);
    oled.printf("PATTERN %d",pattern);

    //Send    
    //oled.updateDisplay();
    oled_needs_refresh = true;
}


void oled_demo_lux(char color , int brightness)
{
    if (lux_val != old_lux_val) //Clear screen if lux_val changed
    {
        oled.clearBuffer();
        old_lux_val = lux_val;
    }
    
    oled.drawStr(0,10,"ENGEL_V4");
    oled.drawStr(0,20,"DEMO_LUX");

    oled.setCursor(0,50);
    oled.printf("LUX: %d",lux_val);

    oled.setCursor(0,70);
    switch (color)
    {
        case 'r': {oled.printf("COLOR: RED"); break;}
        case 'g': {oled.printf("COLOR: GREEN"); break;}
        case 'b': {oled.printf("COLOR: BLUE"); break;}
        case 'w': {oled.printf("COLOR: WHITE"); break;}
        default : {oled.printf("COLOR: ERROR"); break;}
    }

    oled.setCursor(0,90);
    oled.printf("PWM: %d",brightness);

    oled_needs_refresh = true;
}

//LOCK
void oled_demo_lock(int lock_status,int remaining_attempts)
{
    oled.clearBuffer();
    
    oled.drawStr(0,10,"ENGEL_V4");
    oled.drawStr(0,20,"DEMO_LOCK");

    if(lock_status == 0) //In Progress
    {
        oled.drawStr(0,50,"INTRODUCE");
        oled.drawStr(0,60,"COMBINATION");
    }

    else if (lock_status == 1 ) //Wrong Combination
    {
        oled.drawStr(0,50,"WRONG");
        oled.drawStr(0,60,"COMBINATION");

        oled.drawStr(0,80,"Remaining ");
        oled.setCursor(0,90);
        oled.printf("Attempts: %d",remaining_attempts);

    }

    else if (lock_status == 2 ) //Correct Combination
    {
        oled.drawStr(0,50,"CORRECT");
        oled.drawStr(0,60,"COMBINATION");

        oled.drawStr(0,80,"SYSTEM");
        oled.drawStr(0,90,"UNLOCKED");
    }

    oled_needs_refresh = true;
}

//DEMO STATUS

void oled_demo_status(int screen ) 
{
    oled.clearBuffer();

    switch(screen)
    {
        case 0: //Battery Related
        {
            oled.drawStr(0,10,"ENGEL_V4");
            oled.drawStr(0,20,"DEMO_STATUS");

            oled.setCursor(0,30);
            if(aux_counter > 999)aux_counter=0;
            oled.printf("Nr.: %d",aux_counter++);

            oled.setCursor(0,40);
            oled.printf("V_BAT: %.2f",bat_voltage);
            
            oled.setCursor(0,50);
            oled.printf("SOC: %d%%",bat_percent);

            oled.setCursor(0,60);
            if(usb_connected)oled.print("USB-IN:YES");
            else oled.print("USB-IN:NO");

            oled.setCursor(0,70);
            if(charging)oled.print("CHARGING:YES");
            else oled.print("CHARGING: NO");

            oled.setCursor(0,80);
            if(low_bat)oled.print("LOW_BAT?:YES");
            else oled.print("LOW_BAT?: NO");

            //oled.setCursor(0,90);
            //if(charging)oled.printf("C_RATE: %.1f",bat_c_rate);
            //else oled.print("C_RATE: N/A ");

            oled.setCursor(0,90);
            oled.printf("TEMP:%d_C",board_temp);
            
            oled.setCursor(0,100);
            if(moving)oled.printf("MOVING?:YES");
            else oled.print("MOVING?:NO");

            oled.setCursor(0,110);
            if(dark)oled.printf("LUX:DARK");
            else oled.print("LUX:BRIGHT");

            oled.setCursor(0,120);
            oled.printf("<NEXT EXIT>");   

            break;
        }

        case 1: //IMU RELATED
        {
            /*
            AVAILABLE OUTPUTS ON IMU IF Vectors are needed
            // orientation/motion vars
            Quaternion q;           // [w, x, y, z]         quaternion container
            VectorInt16 aa;         // [x, y, z]            accel sensor measurements
            VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
            VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
            VectorFloat gravity;    // [x, y, z]            gravity vector
            float euler[3];         // [psi, theta, phi]    Euler angle container
            float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector
            */

           //imu_run() handled by imu_needed on i2c_manager()     
           if(imu_new_data)
           {
                oled.drawStr(0,10,"ENGEL_V4");
                oled.drawStr(0,20,"IMU_STATUS");

                oled.drawStr(0,30,"PARSED TO INT");

                oled.setCursor(0,60);
                oled.printf("YAW: %d",int(imu_yaw));
                
                oled.setCursor(0,80);
                oled.printf("ROLL: %d",int(imu_roll));
                
                oled.setCursor(0,100);
                oled.printf("PITCH: %d",int(imu_pitch));

                oled.setCursor(0,120);
                oled.printf("<NEXT EXIT>"); 
                    
                /*
                oled.setCursor(0,70);
                oled.printf("ACC_X: %d",imu_acc_x);

                oled.setCursor(0,80);
                oled.printf("ACC_Y: %d",imu_acc_y);

                oled.setCursor(0,90);
                oled.printf("ACC_Z: %d",imu_acc_z);


                oled.setCursor(0,100);
                oled.printf("ACC_G_X: %d",imu_acc_comp_grav_x);

                oled.setCursor(0,110);
                oled.printf("ACC_G_Y: %d",imu_acc_comp_grav_y);

                oled.setCursor(0,120);
                oled.printf("ACC_G_Z: %d",imu_acc_comp_grav_z);
                */

                imu_new_data =false; //Resetting Flag
           }

            

            break;
        }

        case 2: //BLE APP Related - Just apply for APP_BLE_DEMO mode
        {
            oled.drawStr(0,10,"ENGEL_V4");
            oled.drawStr(0,20,"DEMO_STATUS");

            oled.setCursor(0,30);
            if(aux_counter > 999)aux_counter=0;
            oled.printf("Nr.: %d",aux_counter++);

            oled.setCursor(0,40);
            oled.printf("LED_MODE: %d",ble_led_mode);

            oled.setCursor(0,50);
            oled.printf("LED_COL: %c",ble_led_color);

            oled.setCursor(0,60);
            oled.printf("LED_BRI: %d",ble_led_brightness);

            oled.setCursor(0,70);
            oled.printf("BZR_MODE: %d",ble_buzzer_mode);
                        
            oled.setCursor(0,80);
            oled.printf("ALM_MODE: %d",ble_alarm_mode);

            oled.setCursor(0,90);           
            if(callback_received)oled.printf("CLBCK?:YES");
            else oled.print("CLBCK?:NO");

            oled.setCursor(0,100);
            if(running_task_ble_led_mode)oled.printf("TSK_LED?:YES");
            else oled.printf("TSK_LED?:NO");

            oled.setCursor(0,110);
            if(running_task_ble_buzzer)oled.printf("TSK_BZR?:YES");
            else oled.printf("TSK_BZR?:NO"); 

            oled.setCursor(0,120);
            if(running_task_ble_alarm)oled.printf("TSK_ALRM?:YES");
            else oled.printf("TSK_ALRM?:NO");          

            break;
        }



        default:
        {
            Serial.print("\n--ERROR, SCREEN NR. NOT RECOGNIZED \n");
            break;
        }


    }

    oled_needs_refresh = true;
}


void oled_demo_message_imu_booting()
{
    oled.clearBuffer();
    
    oled.drawStr(0,10,"ENGEL_V4");
    //oled.drawStr(0,20,"DEMO_IMU_POS");

    oled.drawStr(0,40,"BOOTING IMU");

    oled_needs_refresh = true;
}


void oled_demo_imu_pos(char axis)
{
    //changing to normal bigger font
    oled.setFont(u8g2_font_5x7_tf);
    oled.clearBuffer();
    
    oled.drawStr(0,10,"ENGEL_V4");
    oled.drawStr(0,20,"IMU_POS");

    oled.drawStr(15,40,"PITCH");
    oled.setCursor(15,50);
    oled.printf("X : %d",int(imu_pitch));

    oled.drawStr(15,60,"ROLL");
    oled.setCursor(15,70);
    oled.printf("Y : %d",int(imu_roll));

    oled.drawStr(15,80,"YAW");
    oled.setCursor(15,90);
    oled.printf("Z : %d",int(imu_yaw));

    int selection =0;
    switch (axis)
    {
        case 'x': {selection = 40;break;}
        case 'y': {selection = 60;break;}
        case 'z': {selection = 80;break;}
    }
    
    oled.setCursor(0,selection);
    oled.printf("->");  

    oled_needs_refresh = true;
}

void oled_demo_imu_acc(char axis)
{
    //changing to normal bigger font
    oled.setFont(u8g2_font_5x7_tf);
    oled.clearBuffer();
    
    oled.drawStr(0,10,"ENGEL_V4");
    oled.drawStr(0,20,"IMU_ACC");

    oled.drawStr(15,40,"ALONG X");
    oled.setCursor(15,50);
    oled.printf("ACC : %d",imu_acc_x);

    oled.drawStr(15,60,"ALONG Y");
    oled.setCursor(15,70);
    oled.printf("ACC : %d",imu_acc_y);

    oled.drawStr(15,80,"ALONG Z");
    oled.setCursor(15,90);
    oled.printf("ACC : %d",imu_acc_z);

    int selection =0;
    switch (axis)
    {
        case 'x': {selection = 40;break;}
        case 'y': {selection = 60;break;}
        case 'z': {selection = 80;break;}
    }
    
    oled.setCursor(0,selection);
    oled.printf("->");  

    oled_needs_refresh = true;
}


void refresh_oled_ble_normal()
{
    //Depending on Status it will change

    //0 -> Initialized and Waiting for Connection

    //1 -> Connected and Just Transmitting

    //2 -> Connected and Transceiving 

    //3 -> Disconnected and Re-Advertising

    ///WARNING MAKE SURE YOU DONT RACE OR MUTEX ANY I2C IF CONVERTING THIS ONGOING TASK

    oled.clearBuffer();

    oled.drawStr(0,10,"ENGEL_V4");
    oled.drawStr(0,20,"BLE");
    oled.drawStr(0,30,"Normal Task");
    
    if(ble_oled_status == 0 )
    {
        oled.drawStr(0,50,"WAITING");
        oled.drawStr(0,60,"FOR BLE");
        oled.drawStr(0,70,"CONNECTION");
    }

    else if (ble_oled_status == 1 || ble_oled_status == 2)
    {
        oled.drawStr(0,40,"-Connected-");

        oled.drawStr(0,60,"--- TX ---");
        
        oled.setCursor(0,70);
        oled.printf("BEAT:%d",ble_beat);
        oled.setCursor(0,80);
        oled.printf("TEMP:%d",board_temp);
        oled.setCursor(0,90);
        oled.printf("LUX:%d",lux_val);
    }

    if(ble_oled_status == 2)
    {
        oled.drawStr(0,110,"--- RX: ---");
        
        oled.setCursor(0,120);
        oled.printf("RGB_PWM:%d",ble_rgb_pwm);
    }

    else if (ble_oled_status == 3)
    {
        oled.drawStr(0,40,"BLE");
        oled.drawStr(0,60,"DISCONNECTED");

        oled.drawStr(0,80,"ADVERTISING");
        oled.drawStr(0,100,"& WAITINNG");
        oled.drawStr(0,120,"BLE CONN.");  
    }
    
    oled_needs_refresh = true;
}


void oled_wifi()
{
    //Based on global var : wifi_status

    //0 OFF
    //1 ON but not Connected
    //2 Searching (and printing) Networks
    //3 Connecting
    //4 Connected 

    oled.clearBuffer();
    
    oled.drawStr(0,10,"ENGEL_V4");
    oled.drawStr(0,20,"WIFI");

    if (wifi_status==0)
    {
        oled.drawStr(0,40,"WIFI");
        oled.drawStr(0,60,"DISCONNECTED");
        oled.drawStr(0,80,"OR OFF ");
    }
    
    else if(wifi_status == 1)
    {
        oled.drawStr(0,40,"WIFI");
        oled.drawStr(0,80,"IDLE");
    }
    else if(wifi_status == 2)
    {
        oled.drawStr(0,40,"WIFI");
        oled.drawStr(0,80,"SEARCHING");
        oled.drawStr(0,90,"NETWORKS");
    }
    else if(wifi_status == 3)
    {
        oled.drawStr(0,40,"WIFI");
        oled.drawStr(0,60,"CONNECTING");      
        oled.drawStr(0,80,"TO");  

        oled.setFont(u8g2_font_04b_03_tr); //<- Mod Font
        oled.setCursor(0,100);
        oled.print(wifi_ssid); 

        oled.setFont(u8g2_font_5x7_tf); //<- Original Font

    }
    else if(wifi_status == 4)
    {
        oled.drawStr(0,40,"CONNECTED");
        oled.drawStr(0,50,"TO");

        oled.setFont(u8g2_font_04b_03_tr); //<- Mod Font
        oled.setCursor(0,60);
        oled.print(wifi_ssid);

        oled.setFont(u8g2_font_5x7_tf); //<- Original Font
        oled.drawStr(0,80,"IP Adress:");

        oled.setFont(u8g2_font_04b_03_tr); //<- Mod Font
        oled.setCursor(0,90);
        oled.print(WiFi.localIP());
        oled.setFont(u8g2_font_5x7_tf); //<- Original Font
    }  

    oled_needs_refresh = true;
}

//Used just on DEMO
void oled_wifi_demo()
{
    //Based on global var : wifi_status

    //0 Not Init (OFF)
    //1 Initializing (WIFI+Sockets+Json)
    //2 Initialized
    //3 Searching (and printing) Networks
    //4 Connecting
    //5 Connected 
    //6 Transmitting (no client)
    //7 Transmitting + Receiving (client connected)
    //--> Disconnected , back to 0 

    oled.clearBuffer();
    
    oled.drawStr(0,10,"ENGEL_V4");
    oled.drawStr(0,20,"WIFI_DEMO");

    if (wifi_status==0)
    {
        oled.drawStr(0,40,"WIFI");
        oled.drawStr(0,60,"DISCONNECTED");
        oled.drawStr(0,80,"OR NOT INIT.");
    }
    else if(wifi_status == 1)
    {
        oled.drawStr(0,40,"WIFI");
        oled.drawStr(0,60,"INITIALIZING");
    }
    else if(wifi_status == 2)
    {
        oled.drawStr(0,40,"WIFI");
        oled.drawStr(0,60,"INITIALIZED");
    }
    else if(wifi_status == 3)
    {
        oled.drawStr(0,40,"WIFI");
        oled.drawStr(0,80,"SEARCHING");
        oled.drawStr(0,90,"NETWORKS");
    }
    else if(wifi_status == 4)
    {
        oled.drawStr(0,40,"WIFI");
        oled.drawStr(0,60,"CONNECTING");       
    }
    else if(wifi_status == 5)
    {
        oled.drawStr(0,40,"WIFI");
        oled.drawStr(0,60,"CONNECTED");   
    }

    else if(wifi_status == 6)
    {
        oled.drawStr(0,40,"CONNECTED");
        oled.drawStr(0,50,"TO");

        oled.setFont(u8g2_font_04b_03_tr); //<- Mod Font
        oled.setCursor(0,60);
        oled.print(wifi_ssid);

        oled.setFont(u8g2_font_5x7_tf); //<- Original Font
        oled.drawStr(0,80,"IP Adress:");

        oled.setFont(u8g2_font_04b_03_tr); //<- Mod Font
        oled.setCursor(0,90);
        oled.print(WiFi.localIP());

        oled.setFont(u8g2_font_5x7_tf); //<- Original Font
        oled.drawStr(0,110,"WAITING FOR");
        oled.drawStr(0,120,"CLIENT");
    }  

    else if(wifi_status == 7)
    {
        oled.drawStr(0,40,"CONNECTED");

        oled.setCursor(0,50);
        oled.printf("Clients : %d",wifi_clients);
        
        oled.setCursor(0,70);
        if(wifi_alarm_status)oled.printf("ALARM : ON");
        else oled.printf("ALARM : OFF");

        oled.setCursor(0,90);
        switch(wifi_rgb_led)
        {
            case 0: oled.printf("RGB: RED");break;
            case 1: oled.printf("RGB: GREEN");break;
            case 2: oled.printf("RGB: BLUE");break;
        }

        oled.setCursor(0,110);
        oled.printf("RGB_PWM : %d",wifi_rgb_pwm);        
    }
    
    oled_needs_refresh = true;
}



void oled_mqtt(int cycle_nr)
{
    oled.clearBuffer();
    
    oled.drawStr(0,10,"ENGEL_V4");
        
    if (wifi_connected)
    {
        oled.drawStr(0,30,"WIFI");
        oled.drawStr(0,40,"CONNECTED");
        oled.drawStr(0,50,"TO");

        oled.setFont(u8g2_font_04b_03_tr); //<- Mod Font
        oled.setCursor(0,60);
        oled.print(wifi_ssid);

        oled.setFont(u8g2_font_5x7_tf); //<- Original Font
        oled.drawStr(0,70,"IP Adress:");

        oled.setFont(u8g2_font_04b_03_tr); //<- Mod Font
        oled.setCursor(0,80);
        oled.print(WiFi.localIP());
        oled.setFont(u8g2_font_5x7_tf); //<- Original Font
    }

    else
    {
        oled.drawStr(0,30,"WIFI");
        oled.drawStr(0,40,"DISCONNECTED");
        oled.drawStr(0,50,"OR OFF ");
    }
    
    if(mqtt_connected)
    {
        oled.drawStr(0,100,"MQTT");
        oled.drawStr(0,110,"CONNECTED");
        oled.drawStr(0,120,"CYCLE:");
        oled.setCursor(30,120);
        oled.print(cycle_nr);

    }

    else
    {
        oled.drawStr(0,100,"MQTT");
        oled.drawStr(0,110,"DISCONNECTED");
    }

    oled_needs_refresh = true;
}


void oled_firebase(int cycle_nr)
{
    oled.clearBuffer();
    
    oled.drawStr(0,10,"ENGEL_V4");
        
    if (wifi_connected)
    {
        oled.drawStr(0,30,"WIFI");
        oled.drawStr(0,40,"CONNECTED");
        oled.drawStr(0,50,"TO");

        oled.setFont(u8g2_font_04b_03_tr); //<- Mod Font
        oled.setCursor(0,60);
        oled.print(wifi_ssid);

        oled.setFont(u8g2_font_5x7_tf); //<- Original Font
        oled.drawStr(0,70,"IP Adress:");

        oled.setFont(u8g2_font_04b_03_tr); //<- Mod Font
        oled.setCursor(0,80);
        oled.print(WiFi.localIP());
        oled.setFont(u8g2_font_5x7_tf); //<- Original Font
    }

    else
    {
        oled.drawStr(0,30,"WIFI");
        oled.drawStr(0,40,"DISCONNECTED");
        oled.drawStr(0,50,"OR OFF ");
    }
    
    if(firebase_connected)
    {
        oled.drawStr(0,100,"FIREBASE");
        oled.drawStr(0,110,"CONNECTED");
        oled.drawStr(0,120,"CYCLE:");
        oled.setCursor(30,120);
        oled.print(cycle_nr);

    }

    else
    {
        oled.drawStr(0,100,"FIREBASE");
        oled.drawStr(0,110,"DISCONNECTED");
    }

    oled_needs_refresh = true;
}

void oled_firebase_question()
{
   //changing to normal bigger font
   oled.setFont(u8g2_font_5x7_tf);
   oled.clear();
   
   oled.drawStr(0,60,"RUN");
   oled.drawStr(0,50,"FIREBASE?");
   
   oled.drawStr(0,100," NO     YES ");     
   
   oled_needs_refresh = true;    
}

void oled_firebase_running()
{
   //changing to normal bigger font
   oled.setFont(u8g2_font_5x7_tf);
   oled.clear();
   
   oled.drawStr(0,60,"RUNNING");
   oled.drawStr(0,50,"FIREBASE");
   
   oled_needs_refresh = true;    
}

void oled_task_logger_sd_ftp_running()
{
   //changing to normal bigger font
   oled.setFont(u8g2_font_5x7_tf);
   oled.clear();
   
   oled.drawStr(0,50,"RUNNING");
   oled.drawStr(0,60,"FTP + SD");
   oled.drawStr(0,70,"LOGGER");
   
   oled_needs_refresh = true;    
}



void oled_ota(char* ssid)
{
    
    oled.clearBuffer();
    
    oled.drawStr(0,10,"ENGEL_V4");
    oled.drawStr(0,20,"OTA MODE");

    
    if(WiFi.status() != WL_CONNECTED)
    {
        oled.drawStr(0,40,"WIFI");
        oled.drawStr(0,60,"CONNECTING");      
        oled.drawStr(0,80,"TO");  

        oled.setFont(u8g2_font_04b_03_tr); //<- Mod Font
        oled.setCursor(0,100);
        oled.print(ssid); 

        oled.setFont(u8g2_font_5x7_tf); //<- Original Font
    }
    else
    {
        oled.drawStr(0,40,"CONNECTED");
        oled.drawStr(0,50,"TO");

        oled.setFont(u8g2_font_04b_03_tr); //<- Mod Font
        oled.setCursor(0,60);
        oled.print(ssid);

        oled.setFont(u8g2_font_5x7_tf); //<- Original Font
        oled.drawStr(0,80,"IP Adress:");

        oled.setFont(u8g2_font_04b_03_tr); //<- Mod Font
        oled.setCursor(0,90);
        oled.print(WiFi.localIP());
        oled.setFont(u8g2_font_5x7_tf); //<- Original Font
    }  

    oled_needs_refresh = true;
}

void oled_logger_wifi(char* ssid)
{
    
    oled.clearBuffer();
    
    oled.drawStr(0,10,"ENGEL_V4");
    oled.drawStr(0,20,"LOGGER");

    
    if(WiFi.status() != WL_CONNECTED)
    {
        oled.drawStr(0,40,"WIFI");
        oled.drawStr(0,60,"CONNECTING");      
        oled.drawStr(0,80,"TO");  

        oled.setFont(u8g2_font_04b_03_tr); //<- Mod Font
        oled.setCursor(0,100);
        oled.print(ssid); 

        oled.setFont(u8g2_font_5x7_tf); //<- Original Font
    }
    else
    {
        oled.drawStr(0,40,"CONNECTED");
        oled.drawStr(0,50,"TO");

        oled.setFont(u8g2_font_04b_03_tr); //<- Mod Font
        oled.setCursor(0,60);
        oled.print(ssid);

        oled.setFont(u8g2_font_5x7_tf); //<- Original Font
        oled.drawStr(0,80,"IP Adress:");

        oled.setFont(u8g2_font_04b_03_tr); //<- Mod Font
        oled.setCursor(0,90);
        oled.print(WiFi.localIP());
        oled.setFont(u8g2_font_5x7_tf); //<- Original Font
    }  

    oled_needs_refresh = true;
}

void oled_logger_wifi_failed()
{
    
    oled.clearBuffer();
    
    oled.drawStr(0,10,"ENGEL_V4");
    oled.drawStr(0,20,"LOGGER");

    
    if(WiFi.status() != WL_CONNECTED)
    {
        oled.drawStr(0,40,"WIFI");
        oled.drawStr(0,60,"ERROR");      


        oled.drawStr(0,80,"COULDNT");  
        oled.drawStr(0,100,"CONNECT");
        
        oled.drawStr(0,110,"RESTARTING");
        oled.drawStr(0,120,"ESP");
    }

    oled_needs_refresh = true;
}








//this is for the RTC ALARM (TODO RELABEL THIS LATER)
void oled_alarm_loop(int alarm_id)
{
    oled.clear();

    oled.setCursor(0,30);
    oled.printf("ALARM_%d",alarm_id);
    oled.drawStr(0,40,"ACTIVE !");

    oled.drawStr(0,70,"PRESS ANY");
    oled.drawStr(0,80,"BUTTON TO");
    oled.drawStr(0,90,"DISMISS!");    
    
    oled_needs_refresh = true;
}  

void oled_alarm_cleared(int alarm_id)
{
    oled.clear();

    oled.setCursor(0,30);
    oled.printf("ALARM_%d",alarm_id);
    oled.drawStr(0,40,"CLEARED !");    
    
    oled_needs_refresh = true;
}  

void oled_alarm_cleared_and_destroyed(int alarm_id)
{
    oled.clear();

    oled.setCursor(0,30);
    oled.printf("ALARM_%d",alarm_id);
    oled.drawStr(0,40,"CLEARED !");    
    oled.drawStr(0,40,"& DESTROYED !");
    
    oled_needs_refresh = true;
}  




//PARKING ALARM

void oled_parking_alarm_disclaimer()
{
    oled.clear();

    oled.drawStr(0,30,"PARK. ALARM");
    oled.drawStr(0,40,"ACTIVE !");
    
    oled.drawStr(0,60,"MODE:");

    if(backend_parking_alarm_mode)oled.drawStr(0,70,"LOUD");
    else                       oled.drawStr(0,70,"SILENT");

    oled.drawStr(0,80,"BUTTON OR");
    oled.drawStr(0,90,"BACKEND TO ");
    oled.drawStr(0,100,"DISMISS!");    
    
    oled_needs_refresh = true;
}  

void oled_parking_alarm_movement_detected()
{
    oled.clear();

    oled.drawStr(0,30,"PARK. ALARM");
    oled.drawStr(0,40,"MOVEMENT");
    oled.drawStr(0,40,"DETECTED!");

    
    oled.drawStr(0,60,"MODE:");

    if(backend_parking_alarm_mode) oled.drawStr(0,70,"LOUD");
    else                        oled.drawStr(0,70,"SILENT");
    
    oled.drawStr(0,80,"IDLE FOR");
    oled.drawStr(0,90,"5 SECS. TO");
    oled.drawStr(0,100,"DISMISS!");    
    
    oled_needs_refresh = true;
} 

void oled_parking_alarm_triggered()
{
    oled.clear();

    oled.drawStr(0,30,"PARK. ALARM");
    oled.drawStr(0,40,"TRIGGERED!");
    
    oled.drawStr(0,60,"MODE:");

    if(backend_parking_alarm_mode) oled.drawStr(0,70,"LOUD");
    else                        oled.drawStr(0,70,"SILENT");
    
    oled.drawStr(0,80,"BUTTON OR");
    oled.drawStr(0,90,"BACKEND TO ");
    oled.drawStr(0,100,"DISMISS!");    
    
    oled_needs_refresh = true;
} 

void oled_parking_alarm_dismissed()
{
    oled.clear();

    oled.drawStr(0,50,"PARK. ALARM");
    oled.drawStr(0,60,"CLEARED");
    oled.drawStr(0,70,"RETURNING");
    oled.drawStr(0,80,"TO NORMAL");     
    
    oled_needs_refresh = true;
}  

void oled_parking_alarm_snoozed()
{
    oled.clear();

    oled.drawStr(0,50,"PARK. ALARM");
    oled.drawStr(0,60,"SNOOZED");
    oled.drawStr(0,70,"FOR");
    
    oled.setCursor(0,80);
    //oled.print(parking_alarm_snooze_time_ms/1000);
    oled.print(backend_parking_alarm_snooze_time_ms/1000);
    oled.drawStr(0,90,"SECONDS");     
    
    oled_needs_refresh = true;
}


void oled_imu_message_accident_detected()
{
    oled.clear();

    
    oled.drawStr(0,40,"ACIDENT");
    oled.drawStr(0,40,"DETECTED!");

    
    oled.drawStr(0,80,"RETURN TO");
    oled.drawStr(0,90,"GOOD POS. TO");
    oled.drawStr(0,100,"DISMISS!");    
    
    oled_needs_refresh = true;
} 

void oled_imu_message_accident_confirmed()
{
    oled.clear();

    oled.drawStr(0,30,"ACCIDENT");
    oled.drawStr(0,40,"CONFIRMED!");
    
    oled.drawStr(0,60,"SENDING");
    oled.drawStr(0,60,"NOTIFICATION");

    
    oled.drawStr(0,80,"GOOD POS.");
    oled.drawStr(0,90,"AND BUTTON");
    oled.drawStr(0,100,"TO DISMISS!");    
    
    oled_needs_refresh = true;
} 

void oled_imu_message_press_btn1_to_dismiss()
{
    oled.clear();

    oled.drawStr(0,50,"PRESS");
    oled.drawStr(0,60,"BTN_1");
    oled.drawStr(0,70,"TO");
    oled.drawStr(0,80,"DISMISS");     
    
    oled_needs_refresh = true;

}


void oled_imu_message_accident_dismissed()
{
    oled.clear();

    oled.drawStr(0,50,"ACCIDENT");
    oled.drawStr(0,60,"DISMISSED");
    oled.drawStr(0,70,"RETURNING");
    oled.drawStr(0,80,"TO NORMAL");     
    
    oled_needs_refresh = true;    
}

void oled_imu_graph_question()
{
    //changing to normal bigger font
    oled.setFont(u8g2_font_5x7_tf);
    oled.clear();
    
    oled.drawStr(0,60,"RUN");
    oled.drawStr(0,50,"IMU_GRAPH?");
    
    oled.drawStr(0,100," NO     YES ");     
    
    oled_needs_refresh = true;    
}

void oled_imu_graph()
{
    //changing to normal bigger font
    oled.setFont(u8g2_font_5x7_tf);
    oled.clear();

    oled.drawStr(0,50,"IMU_GRAPH");
    oled.drawStr(0,60,"RUNNING");
    oled.drawStr(0,70,"GRAPH");
    oled.drawStr(0,80,"ON SCREEN");     
    
    oled_needs_refresh = true;    
}

void oled_imu_sim_question()
{
    //changing to normal bigger font
    oled.setFont(u8g2_font_5x7_tf);
    oled.clear();
    
    oled.drawStr(0,60,"RUN");
    oled.drawStr(0,50,"IMU_SIM?");
    
    oled.drawStr(0,100," NO     YES ");     
    
    oled_needs_refresh = true;    
}

void oled_imu_sim()
{
    //changing to normal bigger font
    oled.setFont(u8g2_font_5x7_tf);
    oled.clear();

    oled.drawStr(0,50,"IMU_SIM");
    oled.drawStr(0,60,"RUNNING");
    oled.drawStr(0,70,"SIMULATION");
    oled.drawStr(0,80,"ON SCREEN");     
    
    oled_needs_refresh = true;    

}


void oled_imu_log()
{
    //changing to normal bigger font
    oled.setFont(u8g2_font_5x7_tf);
    oled.clear();

    oled.drawStr(0,60,"RUNNING");
    oled.drawStr(0,70,"IMU_LOG");
    oled.drawStr(0,80,"ON OLED");     
    
    oled_needs_refresh = true;    
}

void oled_imu_log_question()
{
   //changing to normal bigger font
   oled.setFont(u8g2_font_5x7_tf);
   oled.clear();
   
   oled.drawStr(0,60,"RUN");
   oled.drawStr(0,50,"IMU_LOG?");
   
   oled.drawStr(0,100," NO     YES ");     
   
   oled_needs_refresh = true;    
}

//For the IMU we reuse the oled_demo_imu_pos or acc

void oled_sd_not_detected()
{
    //changing to normal bigger font
    oled.setFont(u8g2_font_5x7_tf);
    oled.clear();

    oled.drawStr(0,50,"SD CARD");
    oled.drawStr(0,60,"NOT");
    oled.drawStr(0,70,"DETECTED");  
    
    oled_needs_refresh = true;    
}

void oled_sd_running_mode_black_box()
{
    //changing to normal bigger font
    oled.setFont(u8g2_font_5x7_tf);
    oled.clear();

    oled.drawStr(0,50,"SD CARD");
    oled.drawStr(0,60,"RUNNING");
    oled.drawStr(0,70,"BLACK_BOX");  
    oled.drawStr(0,70,"MODE");  
    
    oled_needs_refresh = true;    

}

void oled_black_box_question()
{

    //changing to normal bigger font
    oled.setFont(u8g2_font_5x7_tf);
    oled.clear();
    
    oled.drawStr(0,60,"RUN");
    oled.drawStr(0,50,"BLACK_BOX?");
    
    oled.drawStr(0,100," NO     YES ");     
    
    oled_needs_refresh = true;    
}

void oled_black_box_info()
{
    //changing to normal bigger font
    oled.setFont(u8g2_font_5x7_tf);
    oled.clearBuffer();

    oled.drawStr(0,10,"ENGEL_V5");
    oled.drawStr(0,20,"BLACK_BOX");
    
    oled.setCursor(0,30);
    oled.printf("# %d",black_box_log_nr);

    oled.setCursor(0,40);
    oled.printf("V_BAT: %.2f",bat_voltage);
    
    oled.setCursor(0,50);
    oled.printf("SOC: %d%%",bat_percent);

    oled.setCursor(0,60);
    if(usb_connected)oled.print("USB-IN:YES");
    else oled.print("USB-IN:NO");

    oled.setCursor(0,70);
    if(charging)oled.print("CHARGING:YES");
    else oled.print("CHARGING: NO");

    oled.setCursor(0,80);
    if(low_bat)oled.print("LOW_BAT?:YES");
    else oled.print("LOW_BAT?: NO");

    oled.setCursor(0,90);
    oled.printf("TEMP:%d_C",board_temp);
    
    oled.setCursor(0,100);
    if(moving)oled.printf("MOVING?:YES");
    else oled.print("MOVING?:NO");

    oled.setCursor(0,110);
    if(dark)oled.printf("LUX:DARK");
    else oled.print("LUX:BRIGHT");

    oled.setCursor(0,120);
    oled.printf("<EXIT NEXT>");   
    
    oled_needs_refresh = true;    
}

void oled_black_box_info_on_firebase_task()
{
    //changing to normal bigger font
    oled.setFont(u8g2_font_5x7_tf);
    oled.clearBuffer();

    oled.drawStr(0,10,"ENGEL_V5");
    oled.drawStr(0,20,"BLACK_BOX");
    
    oled.setCursor(0,30);
    oled.printf("# %d",black_box_log_nr);

    oled.setCursor(0,40);
    oled.printf("V_BAT: %.2f",bat_voltage);
    
    oled.setCursor(0,50);
    oled.printf("SOC: %d%%",bat_percent);

    oled.setCursor(0,60);
    if(usb_connected)oled.print("USB-IN:YES");
    else oled.print("USB-IN:NO");

    oled.setCursor(0,70);
    if(charging)oled.print("CHARGING:YES");
    else oled.print("CHARGING: NO");

    oled.setCursor(0,80);
    if(low_bat)oled.print("LOW_BAT?:YES");
    else oled.print("LOW_BAT?: NO");

    oled.setCursor(0,90);
    oled.printf("TEMP:%d_C",board_temp);
    
    oled.setCursor(0,100);
    if(moving)oled.printf("MOVING?:YES");
    else oled.print("MOVING?:NO");

    oled.setCursor(0,110);
    if(dark)oled.printf("LUX:DARK");
    else oled.print("LUX:BRIGHT");

    oled.setCursor(0,120);
    oled.printf("HeartBeat:%d",firebase_heartbeat_count);   
    
    oled_needs_refresh = true;    
}




void oled_riding_mode_enabled()
{
    oled.clear();

    oled.drawStr(0,50,"RIDING");
    oled.drawStr(0,60,"MODE");
    
    oled.drawStr(0,80,"ENABLED");     
    
    oled_needs_refresh = true;   
}

void oled_main_status_changed_to_parking()
{
    oled.clear();

    oled.drawStr(0,50,"MAIN");
    oled.drawStr(0,60,"STATUS");
    oled.drawStr(0,70,"CHANGED TO:");
    
    oled.drawStr(0,90,"PARKING");     
    
    oled_needs_refresh = true;   

}


void oled_overall_status()
{
    //maybe Date , time , 
    //Definetely bootcount , time since last reboot , wifi/lte mode , connected? black box loop , firebase heartbeat
    
    oled.setFont(u8g2_font_5x7_tf);
    oled.clearBuffer();

    oled.setCursor(0,10);
    if(firebase_connection_method == connection_via_wifi)
    {
        if(WiFi.status() == WL_CONNECTED)
        {
            oled.printf("W_RSSI:%d",WiFi.RSSI()); 
        }
        else
        {
            oled.print("WIFI N/C");
        }
    } 
    else if (firebase_connection_method == connection_via_lte)
    {
        if(lte_status == lte_on_and_working )
        {
            oled.printf("LTE OK"); 
        }
        else
        {
            oled.printf("LTE N/A"); 
        }
    }
    //TODO later make for LTE

    oled.setCursor(0,20);
    oled.printf("BootNr:%d",esp_boot_count);

    oled.setCursor(0,30);
    oled.printf("FB_HB:%d",firebase_heartbeat_count);   
    
    oled.setCursor(0,40);
    if(task_gps_active && gps_locked) oled.printf("GPS_SAT:%d",gps_sat_count);
    else if(task_gps_active && !gps_locked) oled.printf("GPS:No Lock");
    else if (!task_gps_active)oled.print("GPS:OFF");
    
    oled.setCursor(0,50);
    //oled.printf("Heap:%d",ESP.getFreeHeap());
    oled.printf("MWR:%d",mwr); //Minutes since last reboot

    oled.setCursor(0,60);
    if(task_movement_monitor_active)oled.printf("MWM:%d",minutes_without_moving);
    else
    {
        if(moving)oled.printf("MOVING?:YES");
        else oled.print("MOVING?:NO");
    }

    oled.setCursor(0,70);    
    oled.printf("BB#:%d",black_box_log_nr);

    oled.setCursor(0,80);
    if(task_sd_active)oled.printf("BB_ms:%d", black_box_logging_interval_milliseconds);
    else oled.print("BB:OFF");

    oled.setCursor(0,90);
    oled.printf("V_BAT:%.2f",bat_voltage);

    oled.setCursor(0,100);
    oled.printf("SD_USE:%.2f%%",sd_space_used_percent);

    oled.setCursor(0,110);
    if(imu_running)oled.printf("X:%d",int(imu_pitch));

    oled.setCursor(30,110);
    if(imu_running)oled.printf("Y:%d",int(imu_roll));
    else oled.print("IMU:OFF"); 

    oled.setCursor(0,120);
    if(imu_running)oled.printf("Z:%d",int(imu_yaw));
       
    oled_needs_refresh = true;    
}

void oled_dev_info(int screen_nr)
{
     //We got the mutex , changing OLED info request to i2c_dev_manager
     switch (screen_nr)
     {
        //if oled_dev_screen_nr == 0 the oled is deactivated 
        case 0: //Turns off the oled 
        {
            if(firebase_first_loop || oled_needs_clear)
            {
                if(log_enabled)Serial.print("\n---TURNING OFF OLED VIA FIREBASE_TASK ---\n");
                oled_clear();
                oled_needs_clear = false;
            }           
        }
        break;

        case 1: 
        {
            oled_black_box_info_on_firebase_task();
        } 
        break;

        case 2: //GPS
        {                                                                
            if(gps_locked)  //Good GPS Signal
            {
                oled_gps_good_data_template();
                //Whatever happens we here reset the flag
                gps_data_logging_needs_refresh = false;
            }
            else 
            {                                    
                if(gps_initialized) oled_gps_bad_data_template();
                else  oled_gps_waiting_data(); 
            }
        }
        break;

        case 3: //CAN
        {
            if(can_enabled)oled_can_data_template();
            else oled_can_disabled();
        }
        break;

        //IMU
        //Axis doesnt matter as is just for the arrow reference
        case 4: 
        {
            oled_demo_imu_pos('x'); 
        }
        break;
        
        case 5: 
        {
            oled_demo_imu_acc('x'); 
        }
        break;

        case 6: 
        {
            oled_overall_status(); 
        }
        break;

        case 7: 
        {
            oled_ota(ota_ssid); 
        }
        break;
        
        //remember to modify the oled_screen_nr_max accordingly 
     } 
}




/*
useful examples

//Clear SpaceHolder and update the parameter

oled.drawStr(0,10,"TILT:");
oled.drawStr(0,20,"PAN:");
oled.drawStr(0,30,"ACC:");
oled.drawStr(0,40,"STR:");
oled.drawStr(0,50,"WHT:");
oled.drawStr(0,60,"RED:");

oled.setCursor(30,10);oled.print(servo_tilt_now);
Serial.print("FPV:");if(fpv){Serial.print("ON");}else{Serial.print("OFF");}Serial.print(" | ");
*/