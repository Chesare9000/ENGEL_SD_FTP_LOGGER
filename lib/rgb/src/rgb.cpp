
#include <Arduino.h>
#include <FastLED.h>

#include <vars.h>
#include <gpio_exp.h>
#include <tools.h>

//due to demos
#include <lux.h>
#include <imu.h>

#include <I2Cdev.h>
#include <MPU6050_6Axis_MotionApps20.h>
#include <oled.h>

#include <ble_demo_fw.h>

#include "rgb.h"

// Define the array of leds
static CRGB leds[rgb_leds_total];

//Our LEDS are the IN-PI42TASPRPGPB

//Var to limit the OLED refresh rate 
int oled_count =0;

bool rgb_leds_enabled = false;

bool rgb_leds_accident_detected_triggered = false;
bool rgb_leds_accident_confirmed_triggered = false;




int last_imu_roll = 0;

enum imu_led_section
{
    imu_led_section_left_fall,
    imu_led_section_left_top,
    imu_led_section_left_mid,
    imu_led_section_center,
    imu_led_section_right_mid,
    imu_led_section_right_top,
    imu_led_section_right_fall
};
int last_imu_section = imu_led_section_center;



void rgb_leds_init() 
{ 
    if(!rgb_leds_enabled)rgb_leds_enable(); //in case we entered from somwhere else

    if(rgb_leds_enabled)
    {
        if(log_enabled)Serial.print("\n -- Initializing RGB_LEDS---");
        //Extra init just in case 
        pinMode(esp_rgb_data_pin, OUTPUT );
        digitalWrite(esp_rgb_data_pin, LOW); 
        FastLED.addLeds<NEOPIXEL, esp_rgb_data_pin>(leds,rgb_leds_total);
        rgb_leds_initialized = true;

        if(log_enabled)Serial.print("-- Done--- \n");
    }
    else 
    {
        if(log_enabled)Serial.println("ERROR ON rgb_leds_init -> RGB_LEDS ARE NOT ENABLED.!");
    }   
}

void rgb_set_led_color(int led_number , char color)
{
    switch(color)
    {
        case 'r': { leds[led_number] = CRGB::Red;    break;}
        case 'g': { leds[led_number] = CRGB::Green;  break;}
        case 'b': { leds[led_number] = CRGB::Blue;   break;}
        case 'y': { leds[led_number] = CRGB::Yellow; break;}
        case 'p': { leds[led_number] = CRGB::Purple; break;}
        case 'w': { leds[led_number] = CRGB::White;  break;}
        case 'o': { leds[led_number] = CRGB::Orange; break;}
        default:  {if(log_enabled)Serial.print("\n--ERROR! COLOR NOT RECOGNIZED!---\n");break;}
    }
}
 
void rgb_leds_enable()
{

    if(!reg_5v_enabled)reg_5v_enable();

    if(reg_5v_enabled)
    {
        if(!lvl_shftr_enabled)lvl_shftr_enable();
    }

    rgb_leds_enabled = true;

    if(!rgb_leds_initialized)rgb_leds_init();
}

void rgb_leds_disable()
{
    //DISABLE ALL FOR THE MOMENT
    if(reg_5v_enabled)reg_5v_disable();
    if(lvl_shftr_enabled)lvl_shftr_disable();
    rgb_leds_enabled = false;
    //TODO , think if DE-INIT?
}

void rgb_led_on(int led_number, char color, int brightness)
{
    if (led_number >= 5) led_number = led_number - 5 ; //Second turn

    if (!rgb_leds_initialized)rgb_leds_init();

    if(rgb_leds_initialized)
    {
       FastLED.setBrightness(brightness);

       rgb_set_led_color(led_number,color);

       //Serial.printf(" -- LED %u ON -- ",led_number); 
       FastLED.show(); 
    }
    else
    {
        if(log_enabled)Serial.println("ERROR ON LEDS_ON -> RGB NOT INIT. ");
    }
}

void rgb_led_blink_once(int led_number, char color, int brightness, int interval_ms)
{
    if (!rgb_leds_initialized)rgb_leds_init();

    if(rgb_leds_initialized)
    {
       FastLED.setBrightness(brightness);

       rgb_set_led_color(led_number,color);

        //Serial.printf(" -- LED %u ON -- ",led_number); 
       FastLED.show(); 

       wait(interval_ms);

       rgb_led_off(led_number);

       wait(interval_ms);

    }
    else
    {
        if(log_enabled)Serial.println("ERROR ON LEDS_ON -> RGB NOT INIT. ");
    }
}

void rgb_led_off(int led_number)
{
    //Serial.printf(" --LED %u Off-- ",led_number);
    leds[led_number] = CRGB::Black;
    FastLED.show(); 
}




void rgb_leds_on(char color , int brightness)
{
    if (!rgb_leds_initialized)rgb_leds_init();

    if(rgb_leds_initialized)
    {
       FastLED.setBrightness(brightness);

       for(int i=0  ; i < rgb_leds_total; i++)
       {
           rgb_set_led_color(i,color); 
       }
       FastLED.show(); 
    }
    else
    {
        if(log_enabled)Serial.println("ERROR ON LEDS_ON -> RGB NOT INIT. ");
    }
}

void rgb_leds_off()
{
    //if(log_enabled)Serial.print("---LEDS_OFF--");
    
    for(int i=0  ; i< rgb_leds_total ; i++)
    {
        leds[i] = CRGB::Black;
    }
    FastLED.show();
}



//later make this a task
void rgb_leds_blink_once(char color , int brightness, int delay_ms)
{
    if (!rgb_leds_initialized)rgb_leds_init();

    if(rgb_leds_initialized)
    {
       FastLED.setBrightness(brightness);

       for(int i=0  ; i < rgb_leds_total; i++)
       {
           rgb_set_led_color(i,color);
       }
       FastLED.show(); 
        
       wait(delay_ms);

       rgb_leds_off(); 
    }
    else
    {
        if(log_enabled)Serial.println("ERROR ON LEDS_BLINK_ONCE -> RGB NOT INIT. ");
    }
}

//TODO for rgb_leds_blink_continuous make a task


/*
mode 
0 no interruptions
1 interruptions based on buttons
2 interruptions based on ble_callback_data
*/
void rgb_leds_fade_once(int mode,char color , 
               int min_brightness, 
               int max_brightness,
               int step_delay_ms )
{
    if (!rgb_leds_initialized)rgb_leds_init();

    if(rgb_leds_initialized)
    {
       //if(log_enabled)Serial.print("\n ---Fading Once ------");
       for(int i=0  ; i < rgb_leds_total; i++)
       {
           rgb_set_led_color(i,color);
       }


       for(int i = min_brightness ; i <= max_brightness ; i++ )
       {
        
        if(mode == 1 && btn_1.is_pressed||btn_2.is_pressed)break;
        else if(mode == 2 && ble_led_mode != ble_led_mode_current)break;
        
        FastLED.setBrightness(i);
        FastLED.show();
        wait(step_delay_ms);    
       }

       for(int i = max_brightness ; i >= min_brightness ; i-- )
       {
        if(mode == 1 && btn_1.is_pressed||btn_2.is_pressed)break;
        else if(mode == 2 && ble_led_mode != ble_led_mode_current)break;
        
        FastLED.setBrightness(i);
        FastLED.show();
        wait(step_delay_ms);    
       }

       rgb_leds_off();
       if(mode ==1 && btn_1.is_pressed||btn_2.is_pressed)Serial.print("---Interrupted by BTN----");
       else if(mode == 2 && ble_led_mode != ble_led_mode_current)Serial.print("---Interrupted by BLE_LED_MODE change----");
       //if(log_enabled)Serial.print("---Done------");
    }
    else
    {
        if(log_enabled)Serial.println("ERROR ON LEDS_FADE_ONCE -> RGB NOT INIT. ");
    }
} 

//Demos to run on simple infinite loop

void rgb_leds_demo_shift(int brightness, int delay_ms)
{
    if(!rgb_leds_initialized)rgb_leds_init();

    if(rgb_leds_init)
    {
        FastLED.setBrightness(brightness);

        if(!btn_1.is_pressed && !btn_2.is_pressed)
        {
            leds[0] = CRGB::Red;
            leds[1] = CRGB::Green;
            leds[2] = CRGB::Blue;
            leds[3] = CRGB::Red;
            leds[4] = CRGB::Green;
            FastLED.show();
            wait(delay_ms);
        }

        if(!btn_1.is_pressed && !btn_2.is_pressed)
        {
            leds[0] = CRGB::Green;
            leds[1] = CRGB::Red;
            leds[2] = CRGB::Green;
            leds[3] = CRGB::Blue;
            leds[4] = CRGB::Red;
            FastLED.show();
            wait(delay_ms);
        }

        if(!btn_1.is_pressed && !btn_2.is_pressed)
        {
            leds[0] = CRGB::Red;
            leds[1] = CRGB::Green;
            leds[2] = CRGB::Red;
            leds[3] = CRGB::Green;
            leds[4] = CRGB::Blue;
            FastLED.show();
            wait(delay_ms);
            
        }

        if(!btn_1.is_pressed && !btn_2.is_pressed)
        {
            leds[0] = CRGB::Blue;
            leds[1] = CRGB::Red;
            leds[2] = CRGB::Green;
            leds[3] = CRGB::Red;
            leds[4] = CRGB::Green;
            FastLED.show();
            wait(delay_ms);
        }

        if(!btn_1.is_pressed && !btn_2.is_pressed)
        {
            leds[0] = CRGB::Green;
            leds[1] = CRGB::Blue;
            leds[2] = CRGB::Red;
            leds[3] = CRGB::Green;
            leds[4] = CRGB::Red;;
            FastLED.show();
            wait(delay_ms);
        }

        if(btn_1.is_pressed||btn_2.is_pressed)
        {
            Serial.print("---Interrupted------");
            rgb_leds_off();
        }         
    }
    else
    {
        if(log_enabled)Serial.println("ERROR ON RGB_DEMO_SHFT -> RGB NOT INIT. ");
    }
}

void rgb_leds_demo_simple_change_colors(int delay_ms)
{   
    rgb_leds_on('r',255);
    wait(delay_ms);
    rgb_leds_off();
    wait(delay_ms);
    rgb_leds_on('g',255);
    wait(delay_ms);
    rgb_leds_off();
    wait(delay_ms);
    rgb_leds_on('b',255);
    wait(delay_ms);
    rgb_leds_off();
    wait(delay_ms);
    rgb_leds_on('y',255);
    wait(delay_ms);
    rgb_leds_off();
    wait(delay_ms);
    rgb_leds_on('p',255);
    wait(delay_ms);
    rgb_leds_off();
    wait(delay_ms);  
    rgb_leds_on('w',255);
    wait(delay_ms);
    rgb_leds_off();
    wait(delay_ms);  
}

void rgb_leds_demo_fade_multicolor()
{
  if(!btn_1.is_pressed||!btn_2.is_pressed)rgb_leds_fade_once(1,'r',10,200,10);

  if(!btn_1.is_pressed||!btn_2.is_pressed)rgb_leds_fade_once(1,'g',10,200,10);

  if(!btn_1.is_pressed||!btn_2.is_pressed)rgb_leds_fade_once(1,'b',10,200,10);

  if(!btn_1.is_pressed||!btn_2.is_pressed)rgb_leds_fade_once(1,'y',10,200,10);

  if(!btn_1.is_pressed||!btn_2.is_pressed)rgb_leds_fade_once(1,'p',10,200,10);

  if(!btn_1.is_pressed||!btn_2.is_pressed)rgb_leds_fade_once(1,'w',10,200,10);

  if(btn_1.is_pressed||btn_2.is_pressed)
  {
    Serial.print("---Interrupted------");
    rgb_leds_off();
  }   
}

void rgb_led_demo_pong(char color ,int brightness , int delay_ms)
{
    for(int i=0 ; i<5 ;i++)
    {
        if(btn_1.is_pressed||btn_2.is_pressed)break;
        rgb_leds_off();
        rgb_led_on(i,color,brightness);
        wait(delay_ms);    
    }
    for(int i=4 ; i>-1 ;i--)
    {
        if(btn_1.is_pressed||btn_2.is_pressed)break;
        rgb_leds_off();
        rgb_led_on(i,color,brightness);
        wait(delay_ms);    
    }
    
    if(btn_1.is_pressed||btn_2.is_pressed)
    {
        Serial.print("---Interrupted------");
        rgb_leds_off();
    }   
}

void rgb_led_demo_rotate(char color ,int brightness , int delay_ms)
{
    for(int i=0 ; i<5 ;i++)
    {
        if(btn_1.is_pressed||btn_2.is_pressed)break;
        rgb_leds_off();
        rgb_led_on(i,color,brightness);
        wait(delay_ms);    
    }
    
    if(btn_1.is_pressed||btn_2.is_pressed)
    {
        Serial.print("---Interrupted------");
        rgb_leds_off();
    }   
}

void rgb_leds_demo_lux(char color,int max_brightness,int lux_min,int lux_max)
{
    if( lux_get() && !btn_1.is_pressed && !btn_2.is_pressed )
    {
        //Nightlight Mode 

        int brightness=0;

        if (lux_val > lux_max) brightness = 0;

        else if (lux_val < lux_min) brightness = max_brightness;
        
        else brightness = map(lux_val,lux_max,lux_min,0,max_brightness);  
       
        rgb_leds_on(color,brightness); 

        oled_demo_lux(color,brightness);    

        Serial.printf("\nLUX:%u | LEDS:%u ",lux_val,brightness);
        
        wait(100); //Refresh every 100ms
    } 
    else wait(10); //To never loop on null
} 

void rgb_leds_demo_imu_pos(int imu_output_mode,int brightness,char axis,int max_val)
{
    if(btn_1.is_pressed||btn_2.is_pressed)
    {
        Serial.print("---Interrupted------");
        rgb_leds_off();
    }
    else
    {
        //imu_output_mode = 2 ; //Silent Mode refreshing vars and flag 
        //if(oled_enabled) oled_demo_message_imu_booting();
               
        if(imu_new_data)
        {
            //Serial.printf("\n Y:%f P:%f R:%f ",imu_yaw,imu_pitch,imu_roll);

            if (imu_output_mode ==2 ) //Demo Color Leds
            {            
                if(axis == 'x')
                {
                    //Example on max_val = 30;
                    if(imu_pitch > max_val)
                    {
                        //x>30
                        //Blink Here
                        rgb_led_blink_once(0,'r',100,200);

                    }
                    else if(imu_pitch <= max_val && imu_pitch > max_val/2)
                    {
                        // 30>x>15
                        rgb_leds_off();
                        rgb_led_on(0,'r',100);

                    }
                    else if(imu_pitch <= max_val/2 && imu_pitch > max_val/3)
                    {
                        rgb_leds_off();
                        rgb_led_on(1,'y',100);
                        //15>x>10

                    }
                    else if(imu_pitch <= max_val/3 && imu_pitch > ((max_val/3)*-1))
                    {

                       // 10>x>-10 Center 
                       rgb_leds_off();
                       rgb_led_on(2,'g',100);
                    }
                    else if(imu_pitch <= ((max_val/3)*-1) && imu_pitch > ((max_val/2)*-1))
                    {
                        //-10>x>-15
                        rgb_leds_off();
                        rgb_led_on(3,'y',100);

                    }
                    else if(imu_pitch <= ((max_val/2)*-1) && imu_pitch > ((max_val)*-1))
                    {
                        //-10>x>-15
                        rgb_leds_off();
                        rgb_led_on(4,'r',100);

                    }
                    else if(imu_pitch <= ((max_val)*-1))
                    {
                        rgb_led_blink_once(4,'r',100,200);

                        //-10>x>-15
                        //Blink here
                    }
                }

                else if(axis == 'y')
                {
                    //Example on max_val = 30;
                    if(imu_roll > max_val)
                    {
                        //x>30
                        //Blink Here
                        rgb_led_blink_once(0,'r',100,200);

                    }
                    else if(imu_roll <= max_val && imu_roll > max_val/2)
                    {
                        // 30>x>15
                        rgb_leds_off();
                        rgb_led_on(0,'r',100);

                    }
                    else if(imu_roll <= max_val/2 && imu_roll > max_val/3)
                    {
                        rgb_leds_off();
                        rgb_led_on(1,'y',100);
                        //15>x>10

                    }
                    else if(imu_roll <= max_val/3 && imu_roll > ((max_val/3)*-1))
                    {

                       // 10>x>-10 Center 
                       rgb_leds_off();
                       rgb_led_on(2,'g',100);
                    }
                    else if(imu_roll <= ((max_val/3)*-1) && imu_roll > ((max_val/2)*-1))
                    {
                        //-10>x>-15
                        rgb_leds_off();
                        rgb_led_on(3,'y',100);

                    }
                    else if(imu_roll <= ((max_val/2)*-1) && imu_roll > ((max_val)*-1))
                    {
                        //-10>x>-15
                        rgb_leds_off();
                        rgb_led_on(4,'r',100);

                    }
                    else if(imu_roll <= ((max_val)*-1))
                    {
                        rgb_led_blink_once(4,'r',100,200);

                        //-10>x>-15
                        //Blink here
                    }
                }
                else if(axis == 'z')
                {
                    //Example on max_val = 30;
                    if(imu_yaw > max_val)
                    {
                        //x>30
                        //Blink Here
                        rgb_led_blink_once(0,'r',100,200);

                    }
                    else if(imu_yaw <= max_val && imu_yaw > max_val/2)
                    {
                        // 30>x>15
                        rgb_leds_off();
                        rgb_led_on(0,'r',100);

                    }
                    else if(imu_yaw <= max_val/2 && imu_yaw > max_val/3)
                    {
                        rgb_leds_off();
                        rgb_led_on(1,'y',100);
                        //15>x>10

                    }
                    else if(imu_yaw <= max_val/3 && imu_yaw > ((max_val/3)*-1))
                    {

                       // 10>x>-10 Center 
                       rgb_leds_off();
                       rgb_led_on(2,'g',100);
                    }
                    else if(imu_yaw <= ((max_val/3)*-1) && imu_yaw > ((max_val/2)*-1))
                    {
                        //-10>x>-15
                        rgb_leds_off();
                        rgb_led_on(3,'y',100);

                    }
                    else if(imu_yaw <= ((max_val/2)*-1) && imu_yaw > ((max_val)*-1))
                    {
                        //-10>x>-15
                        rgb_leds_off();
                        rgb_led_on(4,'r',100);

                    }
                    else if(imu_yaw <= ((max_val)*-1))
                    {
                        rgb_led_blink_once(4,'r',100,200);

                        //-10>x>-15
                        //Blink here
                    }
                }
            }
            
            if(oled_enabled) oled_demo_imu_pos(axis); //update OLED
            
            imu_new_data =false; //Resetting Flag
        }
    }   
       
}

void rgb_leds_demo_imu_acc(int imu_output_mode,bool acc_raw,int brightness,char axis,int max_val)
{

    if(btn_1.is_pressed||btn_2.is_pressed)
    {
        Serial.print("---Interrupted------");
        rgb_leds_off();
    }
    else
    {   
        //if(oled_enabled) oled_demo_message_imu_booting();
        //imu_output_mode = 2 ; //Silent Mode refreshing vars and flag

        if(imu_new_data)
        {
            //TO DO  , maybe later make a local var to not
            //overwrite acc_raw for compensated on global usage 

            //Iff flag is raised then take the compensated acc. values
            if(!acc_raw)
            {
                imu_acc_x = imu_acc_comp_grav_x;
                imu_acc_y = imu_acc_comp_grav_y;
                imu_acc_z = imu_acc_comp_grav_z;
            }

            //if(imu_acc_comp_grav_x> 100 || imu_acc_comp_grav_x< -100)Serial.printf("\n X:%d Y:%d Z:%d ",imu_acc_comp_grav_x,imu_acc_comp_grav_y,imu_acc_comp_grav_z);
            //Serial.printf("\n X:%d Y:%d Z:%d ",imu_acc_comp_grav_x,imu_acc_comp_grav_y,imu_acc_comp_grav_z);

            if (imu_output_mode ==2 ) //Demo Color Leds
            {            
                if(axis == 'x')
                {
                    //Example on max_val = 30;
                    if(imu_acc_x > max_val)
                    {
                        //x>30
                        rgb_led_on(0,'r',100);

                    }
                    else if(imu_acc_x <= max_val && imu_acc_x > max_val/2)
                    {
                        // 30>x>15
                        //rgb_leds_off();
                        rgb_led_on(0,'y',100);

                    }
                    else if(imu_acc_x <= max_val/2 && imu_acc_x > max_val/3)
                    {
                        //rgb_leds_off();
                        rgb_led_on(1,'g',100);
                        //15>x>10

                    }
                    else if(imu_acc_x <= max_val/3 && imu_acc_x > ((max_val/3)*-1))
                    {
                       // 10>x>-10 Center 
                       rgb_leds_off();
                       rgb_led_on(2,'g',100);
                    }
                    else if(imu_acc_x <= ((max_val/3)*-1) && imu_acc_x > ((max_val/2)*-1))
                    {
                        //-10>x>-15
                        //rgb_leds_off();
                        rgb_led_on(3,'g',100);

                    }
                    else if(imu_acc_x <= ((max_val/2)*-1) && imu_acc_x > ((max_val)*-1))
                    {
                        //-10>x>-15
                        //rgb_leds_off();
                        rgb_led_on(4,'y',100);

                    }
                    else if(imu_acc_x <= ((max_val)*-1))
                    {

                        rgb_led_on(4,'r',100);
                        //-10>x>-15
                        //Blink here
                    }
                }

                else if(axis == 'y')
                {
                    //Example on max_val = 30;
                    if(imu_acc_y > max_val)
                    {
                        //x>30
                        //Blink Here
                        rgb_led_blink_once(0,'r',100,200);

                    }
                    else if(imu_acc_y <= max_val && imu_acc_y > max_val/2)
                    {
                        // 30>x>15
                        rgb_leds_off();
                        rgb_led_on(0,'r',100);

                    }
                    else if(imu_acc_y <= max_val/2 && imu_acc_y > max_val/3)
                    {
                        rgb_leds_off();
                        rgb_led_on(1,'y',100);
                        //15>x>10

                    }
                    else if(imu_acc_y <= max_val/3 && imu_acc_y > ((max_val/3)*-1))
                    {

                       // 10>x>-10 Center 
                       rgb_leds_off();
                       rgb_led_on(2,'g',100);
                    }
                    else if(imu_acc_y <= ((max_val/3)*-1) && imu_acc_y > ((max_val/2)*-1))
                    {
                        //-10>x>-15
                        rgb_leds_off();
                        rgb_led_on(3,'y',100);

                    }
                    else if(imu_acc_y <= ((max_val/2)*-1) && imu_acc_y > ((max_val)*-1))
                    {
                        //-10>x>-15
                        rgb_leds_off();
                        rgb_led_on(4,'r',100);

                    }
                    else if(imu_acc_y <= ((max_val)*-1))
                    {
                        rgb_led_blink_once(4,'r',100,200);

                        //-10>x>-15
                        //Blink here
                    }
                }
                else if(axis == 'z')
                {
                    //Example on max_val = 30;
                    if(imu_acc_z > max_val)
                    {
                        //x>30
                        //Blink Here
                        rgb_led_blink_once(0,'r',100,200);

                    }
                    else if(imu_acc_z <= max_val && imu_acc_z > max_val/2)
                    {
                        // 30>x>15
                        rgb_leds_off();
                        rgb_led_on(0,'r',100);

                    }
                    else if(imu_acc_z <= max_val/2 && imu_acc_z > max_val/3)
                    {
                        rgb_leds_off();
                        rgb_led_on(1,'y',100);
                        //15>x>10

                    }
                    else if(imu_acc_z <= max_val/3 && imu_acc_z > ((max_val/3)*-1))
                    {

                       // 10>x>-10 Center 
                       rgb_leds_off();
                       rgb_led_on(2,'g',100);
                    }
                    else if(imu_acc_z <= ((max_val/3)*-1) && imu_acc_z > ((max_val/2)*-1))
                    {
                        //-10>x>-15
                        rgb_leds_off();
                        rgb_led_on(3,'y',100);

                    }
                    else if(imu_acc_z <= ((max_val/2)*-1) && imu_acc_z > ((max_val)*-1))
                    {
                        //-10>x>-15
                        rgb_leds_off();
                        rgb_led_on(4,'r',100);

                    }
                    else if(imu_acc_z <= ((max_val)*-1))
                    {
                        rgb_led_blink_once(4,'r',100,200);

                        //-10>x>-15
                        //Blink here
                    }
                }
            }

            if(oled_enabled) oled_demo_imu_acc(axis); //update OLED

            imu_new_data =false; //Resetting Flag
        }
    }   
       
}




//New versions here (out of demo) will return true if risk is detected

bool rgb_leds_follow_imu_pos_y(int brightness,int max_val, int imu_log_mode)
{
    bool risk_present = false;   

    if(imu_new_data & last_imu_roll != imu_roll )
    {
        //Resetting Vars from old state
        if(!accident_detected && rgb_leds_accident_detected_triggered)
        {
            rgb_leds_accident_detected_triggered = false;
            rgb_leds_accident_confirmed_triggered = false;
        }

        //here if the accident was already confirmed we will override the led routine 
        //with a blinking yellow and later a blinking red

        if(accident_confirmed && !rgb_leds_accident_confirmed_triggered)
        {
            rgb_leds_on('r',200);
            rgb_leds_accident_confirmed_triggered = true;
        }

        else if(accident_detected && !rgb_leds_accident_detected_triggered)
        {
            rgb_leds_on('y',200);
            rgb_leds_accident_detected_triggered = true;
        }   
                
        //Commented examples on max_val = 30;
        if(imu_roll > max_val)
        {
            //Checking if needing to wipe the leds
            if(last_imu_section != imu_led_section_left_fall )
            {
                if(!accident_detected) rgb_leds_off();
                last_imu_section = imu_led_section_left_fall;
                if(log_enabled && imu_log_mode == imu_log_mode_moderate) Serial.print("\nIMU_POS Changed to LEFT_FALL");
            }           
            //x>30
            //Blink Here
            if(!accident_detected) 
            {
                rgb_led_blink_once(0,'r',brightness,200);  
                //if(log_enabled)Serial.print(".");
            }
            
            risk_present = true; //danger detected        

        }
        else if(imu_roll <= max_val && imu_roll > max_val/2)
        {
            //Checking if needing to wipe the leds
            if(last_imu_section != imu_led_section_left_top )
            {
                // 30>x>15
                if(!accident_detected) 
                {
                    rgb_leds_off();
                    rgb_led_on(0,'r',brightness);
                }
                //if(log_enabled)Serial.print(".");
                last_imu_section = imu_led_section_left_top;
                if(log_enabled && imu_log_mode == imu_log_mode_moderate) Serial.print("\nIMU_POS Changed to LEFT_TOP");
                
            } 
            //Outside the Loop to keep the counter
            risk_present = true; //danger detected        
        }
        else if(imu_roll <= max_val/2 && imu_roll > max_val/3)
        {            
            //Checking if needing to wipe the leds
            if(last_imu_section != imu_led_section_left_mid )
            {
                //15>x>10
                if(!accident_detected) 
                {
                    rgb_leds_off();
                    rgb_led_on(1,'y',brightness);
                }                
                //if(log_enabled)Serial.print(".");
                last_imu_section = imu_led_section_left_mid;
                if(log_enabled && imu_log_mode == imu_log_mode_moderate) Serial.print("\nIMU_POS Changed to LEFT_MID");
            } 
            //Outside the Loop to keep the counter
            risk_present = false; //danger not detected
        }
        else if(imu_roll <= max_val/3 && imu_roll > ((max_val/3)*-1))
        {
            //Checking if needing to wipe the leds
            if(last_imu_section != imu_led_section_center )
            {
                // 10>x>-10 Center 
                if(!accident_detected) 
                {
                    rgb_leds_off();
                    //rgb_led_on(2,'g',100);
                    rgb_leds_on('r',brightness);
                }
                //if(log_enabled)Serial.print(".");
                last_imu_section = imu_led_section_center;
                if(log_enabled && imu_log_mode == imu_log_mode_moderate) Serial.print("\nIMU_POS Changed to CENTER");
            }     
            //Outside the Loop to keep the counter
            risk_present = false; //emergency dismissed if was triggered
        }
        else if(imu_roll <= ((max_val/3)*-1) && imu_roll > ((max_val/2)*-1))
        {
            //Checking if needing to wipe the leds
            if(last_imu_section != imu_led_section_right_mid )
            {
                //-10>x>-15
                if(!accident_detected) 
                {
                    rgb_leds_off();
                    rgb_led_on(3,'y',brightness);
                    //if(log_enabled)Serial.print(".");
                }
                last_imu_section = imu_led_section_right_mid;
                if(log_enabled && imu_log_mode == imu_log_mode_moderate) Serial.print("\nIMU_POS Changed to RIGHT_MID");
            } 
            //Outside the Loop to keep the counter
            risk_present = false; //danger not detected
        }
        else if(imu_roll <= ((max_val/2)*-1) && imu_roll > ((max_val)*-1))
        {
            //Checking if needing to wipe the leds
            if(last_imu_section != imu_led_section_right_top )
            {
                if(!accident_detected) 
                {
                    rgb_leds_off();
                    //-10>x>-15
                    rgb_led_on(4,'r',brightness);
                }
                //if(log_enabled)Serial.print(".");
                last_imu_section = imu_led_section_right_top;
                if(log_enabled && imu_log_mode == imu_log_mode_moderate) Serial.print("\nIMU_POS Changed to RIGHT_TOP");
            } 
            //Outside the Loop to keep the counter
            risk_present = true; //danger detected
        }
        else if(imu_roll <= ((max_val)*-1))
        {
            //Checking if needing to wipe the leds
            if(last_imu_section != imu_led_section_right_fall )
            {

                if(!accident_detected) rgb_leds_off();
                last_imu_section = imu_led_section_right_fall;
                if(log_enabled && imu_log_mode == imu_log_mode_moderate) Serial.print("\nIMU_POS Changed to RIGHT_FALL");
            } 

            //-10>x>-15
            //Blink here
            if(!accident_detected) 
            {
                rgb_led_blink_once(4,'r',brightness,200);
                //if(log_enabled)Serial.print(".");    
            }        
            risk_present = true;
        }  

        else
        {
            if(log_enabled)
            {
                Serial.print("ERROR, IMU POSITION NOT CATEGORIZED");
            }
        }          

        last_imu_roll = imu_roll;

        imu_new_data = false; //Resetting Flag

        return risk_present;
    }   
    
}



















//TODO maybe move all demos to demo.cpp?


