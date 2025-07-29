
#include <ESP32-TWAI-CAN.hpp>
#include <can.h>
#include <oled.h>
#include <tools.h>
#include <vars.h>
#include <buzzer.h>

//This is for the mubea heavy , TODO later convert into a macro or similar ad change if somother vehicle is chosen
int vehicle_id = mubea_heavy_cargo; 


#define CAN_TX  12  //TODO CHANGE LATER THIS GPIOS , I DONT LIKE THE BOOTSTRAPING ONES
#define CAN_RX   36  //TODO CHANGE LATER THIS GPIOS , I DONT LIKE THE BOOTSTRAPING ONES

CanFrame rxFrame;

//THIS IS NOT USED ANYMORE <- CHANGE THIS TO RELEVANT DATA AND DELETE FOR MAIN and OLED
int8_t can_vel = -120;
int8_t can_rpm = -120;
int8_t can_odo = -120;
int8_t can_soc = -120;

//CAN Values from MUBEA

//Frame 0x777 -> motorGenData
#define mubea_can_hex_motor_gen_data 0x777
int16_t  mubea_can_motor_power  = 127;  
int16_t  mubea_can_motor_rpm    = 127;  
int8_t   mubea_can_motor_temp   = 127; 
uint16_t mubea_can_gen_power    = 127;  
uint8_t  mubea_can_assist_level = 127;

//Frame 0x778 -> batteryData
#define mubea_can_hex_battery_data 0x778
uint8_t  mubea_can_soc         = 127;   
uint8_t  mubea_can_soh         = 127;
int16_t  mubea_can_power       = 127;   
uint16_t mubea_can_voltage     = 127;
uint16_t mubea_can_temperature = 127;

//Frame 0x779 -> VEHICLE_PART1
#define mubea_can_hex_vehicle_part_1 0x779 
uint8_t mubea_can_speed        = 127; 
int8_t mubea_can_direction     = 127; 
uint8_t mubea_can_gear         = 127;
uint32_t mubea_can_mileage     = 127;
        
//Frame 0x77A -> VEHICLE_PART2
#define mubea_can_hex_vehicle_part_2 0x77A
uint16_t mubea_can_error_code   = 127;
uint8_t mubea_can_recuperation = 127; 

//Flags for first appeareance
bool mubea_can_hex_motor_gen_data_first_loop = true;
bool mubea_can_hex_battery_data_first_loop = true;
bool mubea_can_hex_vehicle_part_1_first_loop = true;
bool mubea_can_hex_vehicle_part_2_first_loop =true;

bool can_initialized = false;
//TODO , maybe default false until getting confirmation from firebase?
bool can_enabled = true; //Enabled by default
bool can_upload = true;  //Enabled by default

//not used at the moment , but callback is implemented
int can_refresh_seconds_default = 1;
int can_refresh_seconds = can_refresh_seconds_default;

bool can_init(int can_log_mode) 
{
  ESP32Can.setPins(CAN_TX, CAN_RX);
  ESP32Can.setSpeed(ESP32Can.convertSpeed(500));

  if (ESP32Can.begin()) 
  {
    if(can_log_mode > can_log_mode_silent)
    {
      Serial.println("\n---CAN bus started!");
    }

     //seting first loop for all frames
    mubea_can_hex_motor_gen_data_first_loop = true;
    mubea_can_hex_battery_data_first_loop = true;
    mubea_can_hex_vehicle_part_1_first_loop = true;
    mubea_can_hex_vehicle_part_2_first_loop =true;
    
    return true;
  } 
  else 
  {
    if(can_log_mode > can_log_mode_silent)
    {
      Serial.println("\n---CAN bus failed!");
    }
    oled_can_not_detected();
    buzzer_error();
    return false;
  }
}

bool can_send_activation_message() 
{
  switch(vehicle_id)
  {
    case mubea_heavy_cargo:
    {
      CanFrame txFrame;
      txFrame.identifier = 0x100;   // Required CAN ID
      txFrame.extd = 0;             // Standard frame
      txFrame.rtr = 0;              // Data frame
      txFrame.data_length_code = 8; // 8 bytes

      // Fill with zeros
      for (int i = 0; i < 8; i++) txFrame.data[i] = 0;

      // Set Byte 3, Bit 3 (bit numbering: 0 = LSB)
      txFrame.data[3] |= (1 << 3);

      if (ESP32Can.writeFrame(txFrame)) 
      {
        if(log_enabled) Serial.println("✅ --- Activation CAN frame sent (ID: 0x100, Byte3.Bit3=1).");
        return true;
      } 
      else 
      {
          if(log_enabled) Serial.println("❌ --- Failed to send activation CAN frame.");
          buzzer_error();
          return false;
      }
    }
    break;

    //Here more cases if needed
  }    
}

void can_poll(int can_log_mode)
{
  if (ESP32Can.readFrame(rxFrame, 1000)) 
  {
    //Once all have a first loop we will update just upon change
    if( mubea_can_hex_motor_gen_data_first_loop || 
        mubea_can_hex_battery_data_first_loop   || 
        mubea_can_hex_vehicle_part_1_first_loop ||
        mubea_can_hex_vehicle_part_2_first_loop && 
        can_log_mode > can_log_mode_silent)
    {
      //Serial.print("\n----------------------------------------------------");
      Serial.printf("\nReceived Frame ID: 0x%03X", rxFrame.identifier);
    }
    
    switch (rxFrame.identifier) 
    {
      case mubea_can_hex_motor_gen_data: //0x777
      {
        if( mubea_can_hex_motor_gen_data_first_loop) 
        {
          if(can_log_mode > can_log_mode_moderate) Serial.println(" (motorGenData) "); 
          mubea_can_hex_motor_gen_data_first_loop = false;
        }

        if(rxFrame.data[0] | (rxFrame.data[1]<<8) != mubea_can_motor_power )
        {
          mubea_can_motor_power = rxFrame.data[0] | (rxFrame.data[1]<<8) ;

          if(can_log_mode > can_log_mode_moderate)
          {
            Serial.printf("Motor Power: %d W\n", mubea_can_motor_power);
          }
        }

        if(rxFrame.data[2] | (rxFrame.data[3]<<8) != mubea_can_motor_rpm)
        {
          mubea_can_motor_rpm = rxFrame.data[2] | (rxFrame.data[3]<<8) ;

          if(can_log_mode > can_log_mode_moderate)
          {
            
            Serial.printf("Motor RPM: %d RPM\n", mubea_can_motor_rpm);
          }
        }

        if( (rxFrame.data[4] + 50) != mubea_can_motor_temp )
        {
          mubea_can_motor_temp = rxFrame.data[4] + 50 ;

          if(can_log_mode > can_log_mode_moderate)
          {
            
            Serial.printf("Motor Temp: %d°C\n", mubea_can_motor_temp);
          }
        }

        if( ( rxFrame.data[5] | (rxFrame.data[6]<<8)) != mubea_can_gen_power )
        {
          mubea_can_gen_power = rxFrame.data[5] | (rxFrame.data[6]<<8); 

          if(can_log_mode > can_log_mode_moderate)
          {
            
            Serial.printf("Generator Power: %d W\n", mubea_can_gen_power);
          }

        }

        if( rxFrame.data[7] != mubea_can_assist_level )
        {
          mubea_can_assist_level = rxFrame.data[7];

          if(can_log_mode > can_log_mode_moderate)
          {
            
            Serial.printf("Assist Level: %d\n", mubea_can_assist_level);
          }
        }
      }
      break;

      case mubea_can_hex_battery_data:  // 0x778
      {

        if( mubea_can_hex_battery_data_first_loop) 
        {
          if(can_log_mode > can_log_mode_moderate) Serial.println(" (batteryData) "); 
          mubea_can_hex_battery_data_first_loop = false;
        }

        if( rxFrame.data[0] != mubea_can_soc )
        {
          mubea_can_soc = rxFrame.data[0] ;

          if(can_log_mode > can_log_mode_moderate)
          {
            
            Serial.printf("Battery SOC: %d%%\n", mubea_can_soc);
          }
        }

        if( rxFrame.data[1] != mubea_can_soh )
        {
          mubea_can_soh = rxFrame.data[1] ;

          if(can_log_mode > can_log_mode_moderate)
          {
            
            Serial.printf("Battery SOH: %d%%\n", mubea_can_soh);
          }
        }

        if((rxFrame.data[2] | (rxFrame.data[3]<<8)) != mubea_can_power )
        {
          mubea_can_power = rxFrame.data[2] | (rxFrame.data[3]<<8) ; 

          if(can_log_mode > can_log_mode_moderate)
          {
            
            Serial.printf("Battery Power: %d W\n", mubea_can_power);
          }
        }

        if((rxFrame.data[4] | (rxFrame.data[5]<<8)) != mubea_can_voltage )
        {
          mubea_can_voltage = rxFrame.data[4] | (rxFrame.data[5]<<8); 

          if(can_log_mode > can_log_mode_moderate)
          {
            
            Serial.printf("Battery Voltage: %d mV\n", mubea_can_voltage);
          }
        }

        if((rxFrame.data[6] | (rxFrame.data[7]<<8)) != mubea_can_temperature )
        {
          mubea_can_temperature = rxFrame.data[6] | (rxFrame.data[7]<<8) ;

          if(can_log_mode > can_log_mode_moderate)
          {
            
            Serial.printf("Battery Temp: %d K or %d °C\n", mubea_can_temperature , mubea_can_temperature-273);
          }
        }
      }
      break;

      case mubea_can_hex_vehicle_part_1:  //0x779
      {

        if( mubea_can_hex_vehicle_part_1_first_loop) 
        {
          if(can_log_mode > can_log_mode_moderate)  Serial.println(" (VEHICLE_PART1)");
          mubea_can_hex_vehicle_part_1_first_loop = false;
        }

        if( rxFrame.data[0] != mubea_can_speed )
        {
          mubea_can_speed = rxFrame.data[0]; 

          if(can_log_mode > can_log_mode_moderate)
          {
            
            Serial.printf("Vehicle Speed: %d km/h\n", mubea_can_speed);
          }
        }

        if( rxFrame.data[1] != mubea_can_direction )
        {
          mubea_can_direction = rxFrame.data[1];  

          if(can_log_mode > can_log_mode_moderate)
          {
            
            Serial.printf("Direction: %d\n", mubea_can_direction);
          }
        }

        if( rxFrame.data[2] != mubea_can_gear )
        {
          mubea_can_gear = rxFrame.data[2];  

          if(can_log_mode > can_log_mode_moderate)
          {
            
            Serial.printf("Gear: %d\n", mubea_can_gear);
          }
        }

        if(  (rxFrame.data[3]         | 
             (rxFrame.data[4] << 8  ) | 
             (rxFrame.data[5] << 16 ) | 
             (rxFrame.data[6] << 24 ) ) 
             != mubea_can_gear )
        {
          mubea_can_mileage = rxFrame.data[3] | 
                            (rxFrame.data[4] << 8  ) | 
                            (rxFrame.data[5] << 16 ) | 
                            (rxFrame.data[6] << 24 ); 

          if(can_log_mode > can_log_mode_moderate)
          {
            
            Serial.printf("Mileage: %lu km\n", mubea_can_mileage);
          }
        }        
      }
      break;

      case mubea_can_hex_vehicle_part_2:  //0x77A
      {

        if( mubea_can_hex_vehicle_part_2_first_loop) 
        {
          if(can_log_mode > can_log_mode_moderate) Serial.println(" (VEHICLE_PART2)");

          mubea_can_hex_vehicle_part_2_first_loop = false;
        }

        if( (rxFrame.data[0] | (rxFrame.data[1]<<8)) != mubea_can_error_code )
        {
          mubea_can_error_code = rxFrame.data[0] | (rxFrame.data[1]<<8);  

          if(can_log_mode > can_log_mode_moderate)
          {
            
            Serial.printf("Vehicle Error Code: 0x%04X\n", mubea_can_error_code);
          }
        }

        if( rxFrame.data[2] != mubea_can_recuperation )
        {
          mubea_can_recuperation = rxFrame.data[2];

          if(can_log_mode > can_log_mode_moderate)
          {
            
            Serial.printf("Recuperation Mode: %d\n", mubea_can_recuperation);
          }
        }
      }
      break;

      default:
      {
        if(can_log_mode > can_log_mode_silent)
        {
          Serial.println("Unknown CAN ID, ignoring...");
        }
      }
      break;
    }
    
    if( (mubea_can_hex_motor_gen_data_first_loop || 
         mubea_can_hex_battery_data_first_loop   || 
         mubea_can_hex_vehicle_part_1_first_loop ||
         mubea_can_hex_vehicle_part_2_first_loop && 
         can_log_mode > can_log_mode_silent        ) ||
         can_log_mode > can_log_mode_moderate          )
      {
        //Serial.print("\n----------------------------------------------------");
        Serial.print("\n----------------------------------------------------\n");
      }
  }
  else wait(10);
}