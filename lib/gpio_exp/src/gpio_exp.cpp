
#include <Arduino.h>

#include "gpio_exp.h"
#include <vars.h>
#include <tools.h>



//Base address for TCA9539A devices 
//Address   A1    A0
//0X74      L     L  <- OURS
//0X75      L     H
//0X76      H     L
//0X77      H     H

//For pins used as input, the TCA9539A devices do not include pullups.


//IN ORDER TO CHANGE THE STATUS OF ONE GPIO We use Masking

//TO MAKE HIGH WITHOUT AFFECTING WE USE OR 
//EXAMPLE 0b01000000 | 0b00001000 = 0b01001000 (can be used as |=)


//TO MAKE LOW WITHOUT AFFECTING WE USE AND  
//EXAMPLE 0b11000000 & 0b01111111 = 0b01000000 (can be used as &=)


//TO TOGGLE WE USE XOR (NOT RECOMMENDED FOR THIS APPLICATION)
//results in a 1 only if the input bits are different, else it results in a 0.
//EXAMPLE 0b11000000 & 0b00000010 = 0b11000010 (can be used as ^=)


//To get the input states we use bitRead(target_byte,bit_position(+<--0=LSB));

//TODO explore bitWrite() as an option to simplify the code


//TCA9539 COMMAND BYTES (Register Addresses)------------
//FIXED REGISTERS (DO NOT CHANGE WITH HW_VERSION)
#define gpioexp_input_port_0X 0X00
#define gpioexp_input_port_1X 0X01

#define gpioexp_output_port_0X 0X02
#define gpioexp_output_port_1X 0X03

#define gpioexp_polartiy_inversion_port_0X 0X04
#define gpioexp_polarity_inversion_port_1X 0X05

#define gpioexp_config_port_0X 0X06
#define gpioexp_config_port_1X 0X07



void gpio_exp_init() //Set HW_VERSION before running this fn.!
{

  //GPIO REGISTERS (CHANGE DEPENDING ON HW_VERSION)------------
  
  
  //GPIOEXP_P0X --------------------

  //USE AS IS FOR BITWISE OR(0->1) AND INVERT(~) for AND(1->0) 

  #define gpioexp_P00_ID000X        0b00000001 //HW. CONTROL ID0
  #define gpioexp_P01_ID00X0        0b00000010 //HW. CONTROL ID1
  #define gpioexp_P02_ID0X00        0b00000100 //HW. CONTROL ID2
  #define gpioexp_P03_IDX000        0b00001000 //HW. CONTROL ID3

  if (hw_version > 3) //Current Config
  {
    #define gpioexp_P04_DEBUG_SWITCH  0b00010000 //INPUT
    #define gpioexp_P05_NRF_RESET     0b00100000 //INPUT ON INIT AND THEN OUTPUT
  }
  
  else
  {
    #define gpioexp_P04_5V_REG_ENA    0b00010000 //OUTPUT
    #define gpioexp_P05_LVL_SHFTR_ENA 0b00100000 //OUTPUT
  }
   
  #define gpioexp_P06_FF1_RST       0b01000000 //<-WARNING! this register manage the 3v3 Rail!
  #define gpioexp_P07_FF2_CLK       0b10000000 

  //1X--------------------------------------------------------

  #define gpioexp_P10_FF2_RST    0b00000001

  if (hw_version > 3) //Current Config
  {
     #define gpioexp_P11_LORA_RESET   0b00000010 //OUTPUT
  }
   
  else
  {
    #define gpioexp_P11_FREE       0b00000010
  }
    
  #define gpioexp_P12_INT_USB    0b00000100

  if(hw_version > 3) //Current Config
  {
    #define gpioexp_P13_INT_CH  0b00001000 //INPUT
  }
  else
  {
    #define gpioexp_P13_FREE    0b00001000
  }
    
  #define gpioexp_P14_INT_RTC_AH 0b00010000
  #define gpioexp_P15_LOW_BAT    0b00100000
  #define gpioexp_P16_INT_TEMP   0b01000000
  #define gpioexp_P17_INT_LUX    0b10000000

  //------------------------------------------------
  //Initialization Defaults

  //ALL PINS ARE INIT BY DEFAULT AS INPUTS ON POWER-ON. 

  //ON CONFIG_PORT REG INPUT IS 1 AND OUTPUT IS 0

  //ON OUTPUT_PORT REG HIGH IS 1 AND LOW IS 0 

  //ALL OUTPUTS ARE INIT ON HIGH, SO WE WONT INITIALIZE THE FFs UNTIL WE NEED THEM.

  //FOR P0

  if(hw_version > 3) //Current Config.
  {
    //Declare ALL PINS AS INPUTS 
    //(IGNORE FFs to AVOID TRIGGERING THEM)
    //P05_NRF_RESET MUST BE AN OUTPUT JUST UNTIL IS NEEDED, THIS TO AVOID SHORT CIRCUIT WITH DEVBOARD BUTTON 
    #define init_0X_config_reg 0b11111111 //ALL INPUTS
    //#define init_0X_output_reg 0b11111111 //ALL LOW(NOT NEEDED) 

    //Declare ALL INPUTS EXCEPT FOR P11 (IGNORE FFs to AVOID TRIGGERING THEM)
    #define init_1X_config_reg 0b11111101 //Just P_11 as OUTPUT (LORA RESET)
    #define init_1X_output_reg 0b11111111 //P_11 on HIGH (LORA RESET IS ACTIVE GND.)

  }
  else //Old Config
  {
    //Declare ALL INPUTS EXCEPT FOR P04 AND P05
    #define init_0X_config_reg 0b11001111
    #define init_0X_output_reg 0b11001111 

    //1X NOT MODIFIED IN THIS VERSION (DEFAULT INIT AS ALL INPUTS)
  }

  if (hw_version > 3) //Current Config
  {
    wait(200);
    if(log_enabled) Serial.println("\n---Initializing GPIO_EXP FOR V4---\n");
    wait(500); 
    
    //FOR 0X---------------------------------------------------
    Wire.beginTransmission(gpio_exp_addr_hex); //IC_ADRESS
    Wire.write(gpioexp_config_port_0X); //CONFIG_PORT_ADRESS
    Wire.write(init_0X_config_reg); //SETUP TO CONFIG REG
    Wire.endTransmission(); 

    /* //NOT NEEDED AS ALL ARE INPUTS

    //Setting the INITIAL OUTPUTS AS LOW
    Wire.beginTransmission(gpio_exp_addr_hex);//IC_ADRESS
    Wire.write(gpioexp_output_port_0X);//OUTPUT_PORT_ADRESS
    Wire.write(init_0X_output_reg);//SETUP TO OUTPUT REG
    Wire.endTransmission();
    
    */

    //FOR 1X----------------------------------------------------
    //CONFIG_REG 1X INIT as INPUTS EXCEPT FROM P11 (IGNORE FFs)

    Wire.beginTransmission(gpio_exp_addr_hex); //IC_ADRESS
    Wire.write(gpioexp_config_port_1X); //CONFIG_PORT_ADRESS
    Wire.write(init_1X_config_reg); //SETUP TO CONFIG REG
    Wire.endTransmission(); 

    //Setting the INITIAL OUTPUTS AS LOW
    Wire.beginTransmission(gpio_exp_addr_hex);//IC_ADRESS
    Wire.write(gpioexp_output_port_1X);//OUTPUT_PORT_ADRESS
    Wire.write(init_1X_output_reg);//SETUP TO OUTPUT REG
    Wire.endTransmission();
  }
  
  else //Old Config
  {
    if(log_enabled) Serial.println("\n---Initializing GPIO_EXP FOR OLD BOARDS < V4 ---\n");
    wait(1000); 
    //Declare ALL INPUTS EXCEPT FOR P04 AND P05
     
    //not initialize any flip-flop as output and will just initialize 
    //them when we need to trigger something (check for pull-down)
      
    //DEFAULT CONFIGURATION : ALL INPUTS EXCEPT FOR P04 AND P05
    //ON CONFIG_PORT REG INPUT IS 1 AND OUTPUT IS 0
    //DEFAULT VALUE for CONFIG_REG 0X = 11001111
    
    Wire.beginTransmission(gpio_exp_addr_hex); //IC_ADRESS
    Wire.write(gpioexp_config_port_0X); //CONFIG_PORT_ADRESS
    Wire.write(init_0X_config_reg); //SETUP TO OUT
    Wire.endTransmission(); 

    //Setting the INITIAL OUTPUTS AS LOW
    Wire.beginTransmission(gpio_exp_addr_hex);
    Wire.write(gpioexp_output_port_0X);
    Wire.write(init_0X_output_reg);
    Wire.endTransmission();

    //CONFIG_REG 1X stays the same as no change is needed = 11111111
  }

  //Printing Saved Config.

  Wire.beginTransmission(gpio_exp_addr_hex);
  Wire.write(gpioexp_config_port_0X);
  Wire.endTransmission();
  if(Wire.requestFrom(gpio_exp_addr_hex,1)>0)
  {
    byte config_port = Wire.read();   
    Serial.print("\ngpioexp_config_port_0X = ");
    Serial.print(config_port,BIN);
    Serial.println();
  }

  //Printing Defaults to Confirm 
  Wire.beginTransmission(gpio_exp_addr_hex);
  Wire.write(gpioexp_output_port_0X);
  Wire.endTransmission();
  if(Wire.requestFrom(gpio_exp_addr_hex,1)>0)
  {
    byte output_port = Wire.read();   
    Serial.print("\ngpioexp_output_port_0X = ");
    Serial.print(output_port,BIN);
    Serial.println();
  }

  Wire.beginTransmission(gpio_exp_addr_hex);
  Wire.write(gpioexp_config_port_1X);
  Wire.endTransmission();
  if(Wire.requestFrom(gpio_exp_addr_hex,1)>0)
  {
    byte config_port = Wire.read();   
    Serial.print("\ngpioexp_config_port_1x = ");
    Serial.print(config_port,BIN);
    Serial.println();
  }

  Wire.beginTransmission(gpio_exp_addr_hex);
  Wire.write(gpioexp_output_port_1X);
  Wire.endTransmission();
  if(Wire.requestFrom(gpio_exp_addr_hex,1)>0)
  {
    byte output_port = Wire.read();   
    Serial.print("\ngpioexp_output_port_1x = ");
    Serial.print(output_port,BIN);
    Serial.println();
  }

  if(log_enabled) Serial.print("\n----- DONE -----\n");

}

void nrf_reset()
{

  if(log_enabled)Serial.print("\n---Resetting NRF---\n");

  if (hw_version < 4) //Current Config
  {
    if(log_enabled) Serial.println("\n---NRF RESET NOT AVAILABLE FOR OLD BOARDS < V4 ---\n");
    return;
  }
   
  if(log_enabled)Serial.print("\n---Changing from input to output---\n");
  
  //Getting Config Register Info
  Wire.beginTransmission(gpio_exp_addr_hex);
  Wire.write(gpioexp_config_port_0X);
  Wire.endTransmission();
  if(Wire.requestFrom(gpio_exp_addr_hex,1)>0)
  {
    byte config_port = Wire.read();   
    Serial.print("\nOrig. gpioexp_config_port_0X = ");
    Serial.print(config_port,BIN);
    Serial.println();

    //ON CONFIG_PORT REG INPUT IS 1 AND OUTPUT IS 0
    
    //TO MAKE LOW WITHOUT AFFECTING WE USE BITWISE AND 
    //We also need to Invert gpioexp_P05_NRF_RESET to match the desired operation
    //gpioexp_P05_NRF_RESET is 0b00100000 and we need 11011111 so the only one changing always to 0 through AND is P05

    //APPLYING AND ON CONFIG_PORT WITH INVERTED BYTE
    
    config_port = config_port & ~gpioexp_P05_NRF_RESET ;

    Serial.print("\nNew gpioexp_config_port_0X = ");
    Serial.print(config_port,BIN);
    Serial.println(); 

    Wire.beginTransmission(gpio_exp_addr_hex);
    Wire.write(gpioexp_config_port_0X);
    Wire.write(config_port);
    Wire.endTransmission();

    Serial.print("\n----- gpioexp_P05_NRF_RESET Should be AN OUTPUT NOW -------- "); 

    //wait(500);
  }
  else 
  {
      if(log_enabled)Serial.println("ERROR ON gpioexp_P05_NRF_RESET ");
  }
  
  //Setting NRF_RESET OUTPUT PIN TO HIGH  -----------------------------------------
  //We first get the register info and then apply a masking
      
  Wire.beginTransmission(gpio_exp_addr_hex);
  Wire.write(gpioexp_output_port_0X);
  Wire.endTransmission();
  if(Wire.requestFrom(gpio_exp_addr_hex,1)>0)
  {
    byte output_port = Wire.read();   
    Serial.print("\nOrig. gpioexp_output_port_0X = ");
    Serial.print(output_port,BIN);
    Serial.println();

    //TO MAKE HIGH WITHOUT AFFECTING WE USE BITWISE OR 
    //APPLYING OR OPERATION ON OUPUT_PORT
    output_port = output_port | gpioexp_P05_NRF_RESET ; 
    
    Serial.print("\nNew gpioexp_output_port_0X = ");
    Serial.print(output_port,BIN);
    Serial.println(); 

    Wire.beginTransmission(gpio_exp_addr_hex);
    Wire.write(gpioexp_output_port_0X);
    Wire.write(output_port);
    Wire.endTransmission();

    Serial.print("\n----- gpioexp_P05_NRF_RESET Should be HIGH NOW -------- ");  

    wait(100);
  }
  else 
  {
    if(log_enabled)Serial.println("ERROR ON gpioexp_P05_NRF_RESET ");
  }

  //Setting to LOW after to trigger the pulse

  Wire.beginTransmission(gpio_exp_addr_hex);
  Wire.write(gpioexp_output_port_0X);
  Wire.endTransmission();
  if(Wire.requestFrom(gpio_exp_addr_hex,1)>0)
  {
    byte output_port = Wire.read();   
    Serial.print("\nOrig. gpioexp_output_port_0X = ");
    Serial.print(output_port,BIN);
    Serial.println();


    //TO MAKE LOW WITHOUT AFFECTING WE USE BITWISE AND 
    //We also need to Invert gpioexp_P05_NRF_RESET to match the desired operation
    //gpioexp_P05_NRF_RESET is 0b00100000 and we need 11011111 so the only one changing always to 0 through AND is P05

    //APPLYING AND ON CONFIG_PORT WITH INVERTED BYTE
    
    output_port = output_port & ~gpioexp_P05_NRF_RESET ;

    Serial.print("\nNew gpioexp_output_port_0X = ");
    Serial.print(output_port,BIN);
    Serial.println(); 

    Wire.beginTransmission(gpio_exp_addr_hex);
    Wire.write(gpioexp_output_port_0X);
    Wire.write(output_port);
    Wire.endTransmission();

    Serial.print("\n----- gpioexp_P05_NRF_RESET Should be LOW NOW -------- ");  

    wait(100);
  }
  else 
  {
      if(log_enabled)Serial.println("ERROR ON gpioexp_P05_NRF_RESET ");
  }
    

  //Returning gpioexp_P05_NRF_RESET TO HIGH

  Wire.beginTransmission(gpio_exp_addr_hex);
  Wire.write(gpioexp_output_port_0X);
  Wire.endTransmission();
  if(Wire.requestFrom(gpio_exp_addr_hex,1)>0)
  {
    byte output_port = Wire.read();   
    Serial.print("\nOrig. gpioexp_output_port_0X = ");
    Serial.print(output_port,BIN);
    Serial.println();

    //TO MAKE HIGH WITHOUT AFFECTING WE USE BITWISE OR 
    //APPLYING OR OPERATION ON OUPUT_PORT
    output_port = output_port | gpioexp_P05_NRF_RESET ; 
    
    Serial.print("\nNew gpioexp_output_port_0X = ");
    Serial.print(output_port,BIN);
    Serial.println(); 

    Wire.beginTransmission(gpio_exp_addr_hex);
    Wire.write(gpioexp_output_port_0X);
    Wire.write(output_port);
    Wire.endTransmission();

    Serial.print("\n----- gpioexp_P05_NRF_RESET Should be HIGH NOW -------- ");  

    wait(100);

  }
  else 
  {
      if(log_enabled)Serial.println("ERROR ON gpioexp_P05_NRF_RESET ");
  }

  //Returning gpioexp_P05_NRF_RESET to be an input

  //Setting NRF_RESET CONFIG PIN TO HIGH  -----------------------------------------
  //We first get the register info and then apply a masking
      
  Wire.beginTransmission(gpio_exp_addr_hex);
  Wire.write(gpioexp_config_port_0X);
  Wire.endTransmission();
  if(Wire.requestFrom(gpio_exp_addr_hex,1)>0)
  {
    byte config_port = Wire.read();   
    Serial.print("\nOrig. gpioexp_config_port_0X = ");
    Serial.print(config_port,BIN);
    Serial.println();

    //TO MAKE HIGH WITHOUT AFFECTING WE USE BITWISE OR 
    //APPLYING OR OPERATION ON CONFIG_PORT
    config_port = config_port | gpioexp_P05_NRF_RESET ; 
    
    Serial.print("\nNew gpioexp_config_port_0X = ");
    Serial.print(config_port,BIN);
    Serial.println(); 

    Wire.beginTransmission(gpio_exp_addr_hex);
    Wire.write(gpioexp_config_port_0X);
    Wire.write(config_port);
    Wire.endTransmission();

    Serial.print("\n----- gpioexp_P05_NRF_RESET Should be an INPUT NOW -------- ");  
  }
  else 
  {
      if(log_enabled)Serial.println("ERROR ON gpioexp_P05_NRF_RESET ");
  }
}

void reg_5v_enable()
{
    if(log_enabled)Serial.print("\n---Enabling 5V Regulator---\n");

    if(hw_version > 3) //Current Config
    {
      digitalWrite(esp_reg_5v_en_pin, HIGH);
    }
    
    else //Old Config
    {
      //We first get the register info and then apply a masking
      
      Wire.beginTransmission(gpio_exp_addr_hex);
      Wire.write(gpioexp_output_port_0X);
      Wire.endTransmission();
      if(Wire.requestFrom(gpio_exp_addr_hex,1)>0)
      {
        byte output_port = Wire.read();   
        Serial.print("\nOrig. gpioexp_output_port_0X = ");
        Serial.print(output_port,BIN);
        Serial.println();

        //TO MAKE HIGH WITHOUT AFFECTING WE USE BITWISE OR 
        //APPLYING OR ON OUPUT_PORT
        output_port = output_port | gpioexp_P04_5V_REG_ENA; 
        
        Serial.print("\nNew gpioexp_output_port_0X = ");
        Serial.print(output_port,BIN);
        Serial.println(); 

        Wire.beginTransmission(gpio_exp_addr_hex);
        Wire.write(gpioexp_output_port_0X);
        Wire.write(output_port);
        Wire.endTransmission();
      }
      else 
      {
          if(log_enabled)Serial.println("ERROR ON 5V_REG_INIT");
      }
    }
 
    wait(100); //must wait otherwise might collapse 3v3 rail
    reg_5v_enabled = true;
    if(log_enabled)Serial.print("\n---Done---\n");
}

void reg_5v_disable()
{
    if(log_enabled)Serial.print("\n---Disabling 5V Regulator---\n");

    if(hw_version > 3) //Current Config
    {
      digitalWrite(esp_reg_5v_en_pin, LOW);
    }   

    else //Old Config
    {
      //We first get the register info and then apply a masking
      
      Wire.beginTransmission(gpio_exp_addr_hex);
      Wire.write(gpioexp_output_port_0X);
      Wire.endTransmission();
      if(Wire.requestFrom(gpio_exp_addr_hex,1)>0)
      {
        byte output_port = Wire.read();   
        Serial.print("\nOrig. gpioexp_output_port_0X = ");
        Serial.print(output_port,BIN);
        Serial.println();

        //TO MAKE LOW WITHOUT AFFECTING WE USE BITWISE AND with Inverted BIN 
        //APPLYING AND ON OUPUT_PORT WITH INVERTED BYTE
        output_port = output_port & ~gpioexp_P04_5V_REG_ENA; 
        
        Serial.print("\nNew gpioexp_output_port_0X = ");
        Serial.print(output_port,BIN);
        Serial.println(); 

        Wire.beginTransmission(gpio_exp_addr_hex);
        Wire.write(gpioexp_output_port_0X);
        Wire.write(output_port);
        Wire.endTransmission();
      }
      else 
      {
        if(log_enabled)Serial.println("ERROR ON 5V_REG_INIT");
      }
    }

    reg_5v_enabled = false;
    if(log_enabled)Serial.print("\n---Done---\n");
}

void lvl_shftr_enable()
{
    if(log_enabled)Serial.print("\n---Enabling Level Shifter---\n");

    if (hw_version > 3) //Current Config
    {
      //TODO (needed?)
    }
    else //Old Config
    {
      //We first get the register info and then apply a masking
      
      Wire.beginTransmission(gpio_exp_addr_hex);
      Wire.write(gpioexp_output_port_0X);
      Wire.endTransmission();
      if(Wire.requestFrom(gpio_exp_addr_hex,1)>0)
      {
        byte output_port = Wire.read();   
        Serial.print("\nOrig. gpioexp_output_port_0X = ");
        Serial.print(output_port,BIN);
        Serial.println();

        //TO MAKE HIGH WITHOUT AFFECTING WE USE BITWISE OR 
        //APPLYING OR ON OUPUT_PORT
        output_port = output_port | gpioexp_P05_LVL_SHFTR_ENA ; 
        
        Serial.print("\nNew gpioexp_output_port_0X = ");
        Serial.print(output_port,BIN);
        Serial.println(); 

        Wire.beginTransmission(gpio_exp_addr_hex);
        Wire.write(gpioexp_output_port_0X);
        Wire.write(output_port);
        Wire.endTransmission();

        lvl_shftr_enabled = true;
      }
      else 
      {
          if(log_enabled)Serial.println("ERROR ON LEVEL_SHIFTER_ENABLE");
      }
    }

    wait(100); //must wait otherwise might collapse 3v3 rail
    if(log_enabled)Serial.print("\n---Done---\n");
}

void lvl_shftr_disable()
{
  if(log_enabled)Serial.print("\n---Disabling Level Shifter---\n");
  
  if (hw_version > 3) //Current Config
  {
     //TODO (needed?)
  }
   
  else //Old Config
  {
    //We first get the register info and then apply a masking
    
    Wire.beginTransmission(gpio_exp_addr_hex);
    Wire.write(gpioexp_output_port_0X);
    Wire.endTransmission();
    if(Wire.requestFrom(gpio_exp_addr_hex,1)>0)
    {
      byte output_port = Wire.read();   
      Serial.print("\nOrig. gpioexp_output_port_0X = ");
      Serial.print(output_port,BIN);
      Serial.println();

      //TO MAKE LOW WITHOUT AFFECTING WE USE BITWISE AND with Inverted BIN 
      //APPLYING AND ON OUPUT_PORT WITH INVERTED BYTE
      output_port = output_port & ~gpioexp_P05_LVL_SHFTR_ENA; 
      
      Serial.print("\nNew gpioexp_output_port_0X = ");
      Serial.print(output_port,BIN);
      Serial.println(); 

      Wire.beginTransmission(gpio_exp_addr_hex);
      Wire.write(gpioexp_output_port_0X);
      Wire.write(output_port);
      Wire.endTransmission();

      lvl_shftr_enabled = false;
    }
    else 
    {
        if(log_enabled)Serial.println("ERROR ON LEVEL_SHIFTER_DISABLE");
    }
  }

  if(log_enabled)Serial.print("\n---Done---\n");

}

//This will send Q1 to low -- CAUTION! 
void ff1_q_low() //This will disconnect all 3v3 dependant, including ESP 
{
  if (hw_version < 4) //OLD Config
  {
    if(log_enabled) Serial.println("\n---FF HANDLING NOT AVAILABLE FOR OLD BOARDS < V4 ---\n");
    return;
  }

  //ON V4 the switch is NC to high so will keep the or on so 
  //we have to disable it by not fitting the resistor on Q2inv
  //This hw bug was fixed on V5
   
  else if(log_enabled)Serial.print("\n --- TRIGGERING FF1 Q LOW--- "); //Disabling 3v3Rail
   
  wait(100);

  //ALL OUTPUTS ARE INIT ON HIGH, SO NO NEED TO SET THE OUTPUT REG FOR FFs
  
  //ONCE IS TRIGGERED IT WILL DISCONNECT ALL AND GO BACK TO INPUT ON NEXT BOOT
  
  //ON CONFIG_PORT REG INPUT IS 1 AND OUTPUT IS 0


  if(log_enabled)Serial.print("\n---Changing FF1 from input to output---\n");
  
  //Getting Config Register Info
  Wire.beginTransmission(gpio_exp_addr_hex);
  Wire.write(gpioexp_config_port_0X);
  Wire.endTransmission();
  if(Wire.requestFrom(gpio_exp_addr_hex,1)>0)
  {
    byte config_port = Wire.read();   
    Serial.print("\nOrig. gpioexp_config_port_0X = ");
    Serial.print(config_port,BIN);
    Serial.println();

    //ON CONFIG_PORT REG INPUT IS 1 AND OUTPUT IS 0
    
    //TO MAKE LOW WITHOUT AFFECTING WE USE BITWISE AND 
    //We also need to Invert gpioexp_P06_FF1_RST to match the desired operation 
    //gpioexp_P06_FF1_RST is 0b01000000 and we need 0b10111111 so the only one changing always to 0 through AND is P06
    //APPLYING AND ON CONFIG_PORT WITH INVERTED BYTE
    
    config_port = config_port & ~gpioexp_P06_FF1_RST ;

    Serial.print("\nNew gpioexp_config_port_0X = ");
    Serial.print(config_port,BIN);
    Serial.println(); 

    

    if(log_enabled)Serial.print("\n --- TRIGGERING NOW FF1 Q LOW --- BYE BYE !"); //Disabling 3v3Rail

    //TODO Make sure to Turn OFF LEDS , Etc.

    wait(100);

    Wire.beginTransmission(gpio_exp_addr_hex);
    Wire.write(gpioexp_config_port_0X);
    Wire.write(config_port);
    Wire.endTransmission();

    //Discharging 3V3_VREG
    wait(500);
     

    //Should never enter here
    if(log_enabled)Serial.print("\n-- YOU SHOULD NOT SEE THIS-- \n");
  }
  else 
  {
    if(log_enabled)Serial.println("ERROR ON gpioexp_P06_FF1_RST ");
  }
    
}

void ff2_q_high() 
{
  if(hw_version < 4) //Current Config
  {
    if(log_enabled) Serial.println("\n---FF HANDLING NOT AVAILABLE FOR OLD BOARDS < V4 ---\n");
    return;
  }

  if(log_enabled)Serial.print("\n --- Setting FF2 Q HIGH BY TRIGGERING FF2_CLK--- ");

  if(ff2_q_status)
  {
    if(log_enabled)Serial.println(" -- FF2 Q is already HIGH -- Returning! ");
    return;
  }
        
  //ALL OUTPUTS ARE INIT ON HIGH, SO NO NEED TO SET THE OUTPUT REG FOR FFs
    
  //ON CONFIG_PORT REG INPUT IS 1 AND OUTPUT IS 0

  //ATM FF2_CLK IS PULLED DOWN BY HARDWARE, SO INPUT STATE IS ACCEPTABLE AS IT WORKS AS AN OD INTERRUPT 
  if(log_enabled)Serial.print("\n---Changing FF2_CLK from input to output---\n");
  
  //Getting Config Register Info
  Wire.beginTransmission(gpio_exp_addr_hex);
  Wire.write(gpioexp_config_port_0X);
  Wire.endTransmission();
  if(Wire.requestFrom(gpio_exp_addr_hex,1)>0)
  {
    byte config_port = Wire.read();   
    Serial.print("\nOrig. gpioexp_config_port_0X = ");
    Serial.print(config_port,BIN);
    Serial.println();

    //ON CONFIG_PORT REG INPUT IS 1 AND OUTPUT IS 0
    
    //TO MAKE LOW WITHOUT AFFECTING WE USE BITWISE AND 
    //We also need to Invert gpioexp_P06_FF1_RST to match the desired operation 
    //ggpioexp_P07_FF2_CLK is 0b10000000 and we need 0b01111111 so the only one changing always to 0 through AND is P06
    //APPLYING AND ON CONFIG_PORT WITH INVERTED BYTE
    
    config_port = config_port & ~gpioexp_P07_FF2_CLK ;

    Serial.print("\nNew gpioexp_config_port_0X = ");
    Serial.print(config_port,BIN);
    Serial.println(); 
  
    wait(100);

    Wire.beginTransmission(gpio_exp_addr_hex);
    Wire.write(gpioexp_config_port_0X);
    Wire.write(config_port);
    Wire.endTransmission();

    if(log_enabled)Serial.print("\n --- FF2 Q Must be HIGH now --- ");

    //Pulse
    wait(100);

    if(log_enabled)Serial.print("-- Done--- \n");

    if(log_enabled)Serial.print("\n---Changing gpioexp_P07_FF2_CLK back from output to input---\n");

    //TO MAKE HIGH WITHOUT AFFECTING WE USE BITWISE OR 
    //APPLYING OR OPERATION ON CONFIG_PORT
    config_port = config_port | gpioexp_P07_FF2_CLK ; 
    
    Serial.print("\nNew gpioexp_config_port_0X = ");
    Serial.print(config_port,BIN);
    Serial.println(); 

    Wire.beginTransmission(gpio_exp_addr_hex);
    Wire.write(gpioexp_config_port_0X);
    Wire.write(config_port);
    Wire.endTransmission();

    if(log_enabled)Serial.print("\n----- gpioexp_P07_FF2_CLK Should be a PULLED-DOWN INPUT NOW  -------- ");
  
    ff2_q_status = true;

    if(log_enabled)Serial.print("-- Done--- \n");

  }
  else 
  {
      if(log_enabled)Serial.println("ERROR ON gpioexp_P07_FF2_CLK ");
  }
}

void ff2_q_low() 
{
  if(hw_version < 4)//Current Config
  {
    if(log_enabled) Serial.println("\n---FF HANDLING NOT AVAILABLE FOR OLD BOARDS < V4 ---\n");
    return;
  }

  if(log_enabled)Serial.print("\n --- Setting FF2 Q HIGH BY TRIGGERING FF2_RESET--- ");

  if(!ff2_q_status)
  {
    if(log_enabled)Serial.println(" -- FF2 Q is already LOW -- Returning! ");
    return;
  }

  //ALL OUTPUTS ARE INIT ON HIGH, SO NO NEED TO SET THE OUTPUT REG FOR FFs
    
  //ON CONFIG_PORT REG INPUT IS 1 AND OUTPUT IS 0

  //ATM FF2_RESET IS PULLED DOWN BY HARDWARE, SO INPUT STATE IS ACCEPTABLE AS IT WORKS AS AN OD INTERRUPT 
  if(log_enabled)Serial.print("\n---Changing FF2_RESET from input to output---\n");
  
  //Getting Config Register Info
  Wire.beginTransmission(gpio_exp_addr_hex);
  Wire.write(gpioexp_config_port_1X);
  Wire.endTransmission();
  if(Wire.requestFrom(gpio_exp_addr_hex,1)>0)
  {
    byte config_port = Wire.read();   
    Serial.print("\nOrig. gpioexp_config_port_1X = ");
    Serial.print(config_port,BIN);
    Serial.println();

    //ON CONFIG_PORT REG INPUT IS 1 AND OUTPUT IS 0
    
    //TO MAKE LOW WITHOUT AFFECTING WE USE BITWISE AND 
    //We also need to Invert gpioexp_P06_FF1_RST to match the desired operation 
    //ggpioexp_P07_FF2_CLK is 0b00000001 and we need 0b11111110 so the only one changing always to 0 through AND is P06
    //APPLYING AND ON CONFIG_PORT WITH INVERTED BYTE
    
    config_port = config_port & ~gpioexp_P10_FF2_RST ;

    Serial.print("\nNew gpioexp_config_port_1X = ");
    Serial.print(config_port,BIN);
    Serial.println(); 
  
    wait(100);

    Wire.beginTransmission(gpio_exp_addr_hex);
    Wire.write(gpioexp_config_port_1X);
    Wire.write(config_port);
    Wire.endTransmission();

    if(log_enabled)Serial.print("\n --- FF2 Q Must be LOW now --- ");

    //Pulse
    wait(100);

    if(log_enabled)Serial.print("-- Done--- \n");

    if(log_enabled)Serial.print("\n---Changing gpioexp_P10_FF2_RST back from output to input---\n");

    //TO MAKE HIGH WITHOUT AFFECTING WE USE BITWISE OR 
    //APPLYING OR OPERATION ON CONFIG_PORT
    config_port = config_port | gpioexp_P10_FF2_RST; 
    
    Serial.print("\nNew gpioexp_config_port_1X = ");
    Serial.print(config_port,BIN);
    Serial.println(); 

    Wire.beginTransmission(gpio_exp_addr_hex);
    Wire.write(gpioexp_config_port_1X);
    Wire.write(config_port);
    Wire.endTransmission();

    if(log_enabled)Serial.print("\n----- gpioexp_P10_FF2_RST Should be a PULLED-DOWN INPUT NOW -------- ");
    
    ff2_q_status = false;
  if(log_enabled)Serial.print("-- Done--- \n");

  }
  else 
  {
      if(log_enabled)Serial.println("ERROR ON gpioexp_P07_FF2_CLK ");
  }
}

void lora_reset()
{
  if(hw_version =! 4) //This fn just works on V4 as on <V4 and >V4 LORA RESET is located on ESP_IO14
  {
    if(log_enabled) Serial.println("\n---LORA RESET NOT AVAILABLE FOR VERSION != V4 ---\n");
    return;
  }

  //LORA_RESET_GPIO_PIN MUST BE SET TO LOW AND THEN BACK TO HIGH AFTER SOME TIME

  //Setting to LOW after to trigger the pulse

  Wire.beginTransmission(gpio_exp_addr_hex);
  Wire.write(gpioexp_output_port_1X);
  Wire.endTransmission();
  if(Wire.requestFrom(gpio_exp_addr_hex,1)>0)
  {
    byte output_port = Wire.read();   
    Serial.print("\nOrig. gpioexp_output_port_1X = ");
    Serial.print(output_port,BIN);
    Serial.println();


    //TO MAKE LOW WITHOUT AFFECTING WE USE BITWISE AND 
    //We also need to Invert gpioexp_P11_LORA_RESET to match the desired operation
    //gpioexp_P11_LORA_RESET is 0b00000010 and we need 11111101 so the only one changing always to 0 through AND is P11

    //APPLYING AND ON CONFIG_PORT WITH INVERTED BYTE
    
    output_port = output_port & ~gpioexp_P11_LORA_RESET ;

    Serial.print("\nNew gpioexp_output_port_1X = ");
    Serial.print(output_port,BIN);
    Serial.println(); 

    Wire.beginTransmission(gpio_exp_addr_hex);
    Wire.write(gpioexp_output_port_1X);
    Wire.write(output_port);
    Wire.endTransmission();

    Serial.print("\n----- gpioexp_P11_LORA_RESET Should be LOW NOW -------- ");  

    wait(500);
    
    //Returning gpioexp_P11_LORA_RESET TO HIGH

    //TO MAKE HIGH WITHOUT AFFECTING WE USE BITWISE OR 
    //APPLYING OR OPERATION ON OUPUT_PORT
    output_port = output_port | gpioexp_P11_LORA_RESET ; 

    Serial.print("\nNew gpioexp_output_port_1X = ");
    Serial.print(output_port,BIN);
    Serial.println(); 

    Wire.beginTransmission(gpio_exp_addr_hex);
    Wire.write(gpioexp_output_port_1X);
    Wire.write(output_port);
    Wire.endTransmission();

    Serial.print("\n----- gpioexp_P11_LORA_RESET Should be HIGH NOW -------- ");  
  }
  else 
  {
      if(log_enabled)Serial.println("ERROR ON gpioexp_P11_LORA_RESET ");
  }
}


void gpio_exp_p11_simcom_turn_key()
{
  //This will turn ON or Off depending on the status of the key

  if(hw_version =! 5) //This fn just works on V5 as other versions wil use this pin differently , check fn above if doubting
  {
    if(log_enabled) Serial.println("\n---lte_turn_key NOT AVAILABLE FOR VERSION != V5 ---\n");
    return;
  }
  else
  {
    if(log_enabled) Serial.println("\n---Turning LTE Key---\n");
  }

  //LTE_KEY_GPIO_PIN MUST BE SET TO LOW AND THEN BACK TO HIGH AFTER SOME TIME

  //Setting to LOW after to trigger the pulse

  Wire.beginTransmission(gpio_exp_addr_hex);
  Wire.write(gpioexp_output_port_1X);
  Wire.endTransmission();

  if(Wire.requestFrom(gpio_exp_addr_hex,1)>0)
  {
    byte output_port = Wire.read();   
    Serial.print("\nOrig. gpioexp_output_port_1X = ");
    Serial.print(output_port,BIN);
    Serial.println();


    //TO MAKE LOW WITHOUT AFFECTING WE USE BITWISE AND 
    //We also need to Invert gpioexp_P11 to match the desired operation
    //gpioexp_P11 is 0b00000010 and we need 11111101 so the only one changing always to 0 through AND is P11

    //APPLYING AND ON CONFIG_PORT WITH INVERTED BYTE
    
    output_port = output_port & ~gpioexp_P11_LORA_RESET ;

    Serial.print("\nNew gpioexp_output_port_1X = ");
    Serial.print(output_port,BIN);
    Serial.println(); 

    Wire.beginTransmission(gpio_exp_addr_hex);
    Wire.write(gpioexp_output_port_1X);
    Wire.write(output_port);
    Wire.endTransmission();

    Serial.print("\n----- gpioexp_P11 Should be LOW NOW -------- ");  

    wait(1000);
    
    //Returning gpioexp_P11_LORA_RESET TO HIGH

    //TO MAKE HIGH WITHOUT AFFECTING WE USE BITWISE OR 
    //APPLYING OR OPERATION ON OUPUT_PORT
    output_port = output_port | gpioexp_P11_LORA_RESET ; 

    Serial.print("\nNew gpioexp_output_port_1X = ");
    Serial.print(output_port,BIN);
    Serial.println(); 

    Wire.beginTransmission(gpio_exp_addr_hex);
    Wire.write(gpioexp_output_port_1X);
    Wire.write(output_port);
    Wire.endTransmission();

    Serial.print("\n----- gpioexp_P11_LORA_RESET Should be HIGH NOW -------- ");  
  }
  else 
  {
      if(log_enabled)Serial.println("ERROR ON gpioexp_P11_LORA_RESET ");
  }
}



//TODO : Maybe Later DEvelop the Interrupt Functionalities

//For the moment make a poll of all relevant inputs and act upon on one big loop

bool get_gpio_inputs_status(int mode) //Return True if Something Changed from Original Defaults
{
  //mode =  0:none , 1:necessary , 2:verbose output
  
  //To get the input states we use bitRead(target_byte,bit_position(+<--0=LSB));
  
  //If something Changed return true if not return false;
  
  /*

  FROM 0X
  0 <- HW_ID_0
  1 <- HW_ID_1
  2 <- HW_ID_2
  3 <- HW_ID_3
  4 <- DEBUG MODE
  5 -> NRF_RESET
  6 -> FF1_RESET
  7 -> FF2_CLK 
 
  FROM 1X
  0 -> FF2_RESET 
  1 -> LORA_RESET  
  2 <- INT_USB
  3 <- INT_CHARGER
  4 <- INT_RTC (AH)
  5 <- LOW_BAT
  6 <- INT_TEMP
  7 <- INT_LUX

  //GPIO_EXP Expected Inputs States-------------

  bool int_usb = true;
  bool int_charger = true;
  bool int_rtc = false;
  bool int_low_bat = true;
  bool int_temp = true;
  bool int_lux = true;

  //GPIO EXP Derived Flags----------------------

  bool usb_connected = false;
  bool charging = false;
  bool rtc_alarm = false;
  bool low_bat = false;
  bool overheat = false;
  bool lux_limit = false;
  */

  if(hw_version < 1) get_hw_version(); //TODO There is a bug here that set hw_version to 0 after toggling the lte_-key , fix it  
  //Serial.printf("\n--- running get_gpio_inputs_status() for hw_version = %d ---\n",hw_version);
  
  if(hw_version < 4) //Current Config
  {
    if(log_enabled) Serial.println("\n--- get_gpio_inputs_status() NOT AVAILABLE FOR OLD BOARDS < V4 ---\n");
    return false;
  }
    
  //Getting INPUT REGISTER Info (Just 1X is relevant for Inputs)
  Wire.beginTransmission(gpio_exp_addr_hex);
  Wire.write(gpioexp_input_port_1X);
  Wire.endTransmission();
  if(Wire.requestFrom(gpio_exp_addr_hex,1)>0)
  {
    byte input_port = Wire.read();   
    

    /*
    FROM 1X
    2 <- INT_USB
    3 <- INT_CHARGER
    4 <- INT_RTC (AH)
    5 <- LOW_BAT
    6 <- INT_TEMP
    7 <- INT_LUX

     //GPIO_EXP Expected Inputs States-------------

    bool int_usb = true;
    bool int_charger = true;
    bool int_rtc = false;
    bool int_low_bat = true;
    bool int_temp = true;
    bool int_lux = true;
    */  


    //FOR USB CONNECTION ----------------------------------------
    int_usb = bitRead(input_port,2);

    //If int_usb == 1 : no usb connected
    //If int_usb == 0 : usb connected

    if (!int_usb && !usb_connected) //If the USB is plugged-in 
    {
      usb_connected = true;
      gpio_exp_changed = true; //To alert for the change on interrupt register

      if(mode == 1)
      {
        Serial.print("\n---USB CONNECTED---");
      }
    }    

    else if(int_usb && usb_connected) //From Connected to Disconnected
    {
      usb_connected = false; //USB was disconnected
      gpio_exp_changed = true; //To alert for the change on interrupt register

      if(mode == 1)
      {
        Serial.print("\n---USB DISCONNECTED---");
      }
    } 


    //FOR CHARGER ----------------------------------------
    int_charger = bitRead(input_port,3);

    //If int_charger == 1 : Recharging after termination 
                         // IC disabled 
                         // No valid input power
                         // Battery absent
    
    //If int_charger == 0 : Charging
                         // Low (for first charge cycle)
                         // Charging suspended by thermal loop
    
    //EXTRA (NOT CONSIDERED IN PROGRAMM ATM (TODO))
    
    // IF Safety timers expired then Flashing at 2 Hz
    
    if (!int_charger && !charging) //If we are starting charging
    {
      charging = true;
      gpio_exp_changed = true; //To alert for the change on interrupt register

      if(mode == 1)
      {
        Serial.print("\n---CHARGING---");
      }
    }    

    else if(int_charger && charging) //From Charging to not Charging
    {
      charging = false; //Not Charging
      gpio_exp_changed = true; //To alert for the change on interrupt register

      if(mode == 1)
      {
        Serial.print("\n---NOT CHARGING---");
      }
    } 


    //FOR RTC ALARM ----------------------------------------
    int_rtc = bitRead(input_port,4); //ACTIVE HIGH!

    //If int_rtc == 0 : no rtc alarm
    //If int_rtc == 1 : rtc alarm

    if (int_rtc && !rtc_alarm) //RTC Alarm Triggered
    {
      rtc_alarm = true;
      gpio_exp_changed = true; //To alert for the change on interrupt register

      if(mode == 1)
      {
        Serial.print("\n---RTC ALARM ON---");
      }
    }    

    else if(!int_rtc && rtc_alarm) //From Alarm to Dismissed
    {
      rtc_alarm = false; //Alarm was dismissed
      gpio_exp_changed = true; //To alert for the change on interrupt register

      if(mode == 1)
      {
        Serial.print("\n---RTC ALARM OFF---");
      }
    } 

    //FOR LOW_BAT ----------------------------------------
    int_low_bat = bitRead(input_port,5);

    //If int_low_bat == 1 : bat_level_normal
    //If int_low_bat == 0 : bat_level_low

    //The Interrupt must be manually cleared (done on charger_update())

    if (!int_low_bat && !low_bat) // low_bat interruption is triggered 
    {
      low_bat = true;
      gpio_exp_changed = true; //To alert for the change on interrupt register

      if(mode == 1)
      {
        Serial.print("\n---BATTERY VOLTAGE LOW---");
      }
    }    

    else if(int_low_bat && low_bat) //From LOW BAT VOLTAGE TO NORMAL BAT VOLTAGE
    {
      low_bat = false; //Battery Returned to a normal Voltage
      gpio_exp_changed = true; //To alert for the change on interrupt register

      if(mode == 1)
      {
        Serial.print("\n---BATTERY VOLTAGE NORMAL---");
      }
    } 

    //FOR TEMP ALARM ----------------------------------------
    int_temp = bitRead(input_port,6);

    //If int_temp == 1 : temp OK
    //If int_temp == 0 : temp HIGH
    
    //Thermostat Behaviour so it will clear once the temp is down

    if (!int_temp && !overheat) // from temp OK to HIGH temp 
    {
      overheat = true;
      gpio_exp_changed = true; //To alert for the change on interrupt register

      if(mode == 1)
      {
        Serial.print("\n---TEMP HIGH !---");
      }
    }    

    else if(int_temp && overheat) //From OVERHEATING to NORMAL_TEMP
    {
      overheat = false; //TEMP RETURNED TO A LOW VALUE
      gpio_exp_changed = true; //To alert for the change on interrupt register

      if(mode == 1)
      {
        Serial.print("\n---TEMP OK---");
      }
    } 

    
    //FOR LUX ----------------------------------------
    int_lux = bitRead(input_port,7);

    //If int_lux == 1 : is bright
    //If int_lux == 0 : is dark
    
    //it will clear once the environment is bright again

    if (!int_lux && !dark) // from bright to dark
    {
      dark = true;
      gpio_exp_changed = true; //To alert for the change on interrupt register

      if(mode > 1)
      {
        Serial.print("\n---LUX DARK---");
      }
    }    

    else if(int_lux && dark) //from dark to bright
    {
      dark = false; //returned to bright
      gpio_exp_changed = true; //To alert for the change on interrupt register

      if(mode > 1)
      {
        Serial.print("\n---LUX BRIGHT---");
      }
    } 

    //MODES:
    //0-> Silent
    //1-> Print just when changed from default
    //2-> Print all
    //Printing on VERBOSE MODE (2)
    
    if (mode == 2) // Printing All
    {
      Serial.println("\n--- GPIO_EXP ---");

      if(log_enabled)Serial.print("\n---Getting INPUT REGISTER INFO---\n");     

      Serial.print("\ngpioexp_input_port_1X = ");
      Serial.print(input_port,BIN);
      Serial.println();

      if(usb_connected)Serial.println("\n---USB CONNECTED---");
      else Serial.println("\n---USB DISCONNECTED---");

      if(charging)Serial.println("\n---CHARGING---");
      else Serial.println("\n---NOT CHARGING---");

      if(rtc_alarm)Serial.println("\n---RTC ALARM ON---");
      else Serial.println("\n---RTC ALARM OFF---");

      if(low_bat)Serial.println("\n---BATTERY VOLTAGE LOW---");
      else Serial.println("\n---BATTERY VOLTAGE NORMAL---");

      if(overheat)Serial.println("\n---TEMP HIGH !---");
      else Serial.println("\n---TEMP OK---");

      if(dark)Serial.println("\n---LUX DARK---");
      else Serial.println("\n---LUX BRIGHT---");
    }

    //return the state of the GPIO
    return gpio_exp_changed;

  }
  else //ERROR WHILE READING REGISTER 
  {
      if(log_enabled)Serial.println("ERROR ON gpioexp_P05_NRF_RESET ");
      return false;//TODO Think if change into int fn. and -1 for errors
  }  
}


int get_hw_variant()
{
  /*

  #define gpioexp_P00_ID000X        0b00000001 //HW. CONTROL ID0
  #define gpioexp_P01_ID00X0        0b00000010 //HW. CONTROL ID1
  #define gpioexp_P02_ID0X00        0b00000100 //HW. CONTROL ID2
  #define gpioexp_P03_IDX000        0b00001000 //HW. CONTROL ID3

  FROM 1X
  0 <- HW_ID0
  1 <- HW_ID1 
  2 <- HW_ID2
  3 <- HW_ID3
  
  Expected Inputs States-------------

  VARIANT CONTROL

  1xxx  = Integrated (on E-bike)
  0xxx  = Standalone (Access.)

  x100  = DEV
  x011  = PRO
  x010  = LTE+GPS
  x001  = LORA
  x000  = LITE

  */

  //READING THE RELEVANT REGISTER (P0X)
  Wire.beginTransmission(gpio_exp_addr_hex);
  Wire.write(gpioexp_input_port_0X);
  Wire.endTransmission();

  if(Wire.requestFrom(gpio_exp_addr_hex,1)>0)
  {
    byte input_port = Wire.read();    
    
    //FOR READING THE VARIANT RESISTORS -------
    //If hw_id_X == 1 : pulled to 3V3_REG
    //If hw_id_X == 0 : pulled to GND

    if(log_enabled)
    {
      Serial.print("\nGetting HW_VARIANT_REG : ");  
      Serial.print(input_port,BIN);
      Serial.println(); 
    }

    //PINS FOR VARIANT CONTROL
    hw_id_0 = bitRead(input_port,0);if(log_enabled)Serial.printf("HW_ID_0: %d\n",hw_id_0);
    hw_id_1 = bitRead(input_port,1);if(log_enabled)Serial.printf("HW_ID_1: %d\n",hw_id_1);
    hw_id_2 = bitRead(input_port,2);if(log_enabled)Serial.printf("HW_ID_2: %d\n",hw_id_2);
    
    // THIS ONE IS FOR STANDALONE OR EBIKE mode
    hw_id_3 = bitRead(input_port,3);if(log_enabled)Serial.printf("HW_ID_3: %d\n",hw_id_3);

    //Assigning Variants 

    if(log_enabled)
    {
      Serial.print("\n---VARIANT DETECTED : ");
    }

    if (!hw_id_2 && !hw_id_1 && !hw_id_0) //LITE (X000)
    {
      hw_variant = hw_variant_lite ;

      if(log_enabled)
      {
        Serial.print("LITE\n");
      }
    }
    else if (!hw_id_2 && !hw_id_1 && hw_id_0) //LORA (X001)
    {
      hw_variant = hw_variant_lora ;

      if(log_enabled)
      {
        Serial.print("LORA\n");
      }
    }
    else if (!hw_id_2 && hw_id_1 && !hw_id_0) //LTE_GPS (X010)
    {
      hw_variant = hw_variant_lte_gps ;

      if(log_enabled)
      {
        Serial.print("LTE_GPS\n");
      }
    }
    else if (!hw_id_2 && hw_id_1 && hw_id_0) //PRO (X011)
    {
      hw_variant = hw_variant_pro ;

      if(log_enabled)
      {
        Serial.print("PRO\n");
      }
    }
    
    else if (hw_id_2 && !hw_id_1 && !hw_id_0) //DEV (X100)
    {
      hw_variant = hw_variant_devel ;

      if(log_enabled)
      {
        Serial.print("DEV\n");
      }
    } 
  }
  return hw_variant;
}

bool get_debug_mode()
{
  if(log_enabled)
  {
    Serial.print("\n---DEBUG MODE : ");
  }
  /*
  FROM 0X
  #define gpioexp_P04_DEBUG_SWITCH  0b00010000 //INPUT
  */

  //READING THE RELEVANT REGISTER (P0X)
  Wire.beginTransmission(gpio_exp_addr_hex);
  Wire.write(gpioexp_input_port_0X);
  Wire.endTransmission();
  
  if(Wire.requestFrom(gpio_exp_addr_hex,1)>0)
  {
    byte input_port = Wire.read();    
    
    //If P04 == 1 : DEBUG ON
    //If P04 == 0 : DEBUG OFF

    debug_mode = bitRead(input_port,4);

    if(debug_mode)
    {
      if(log_enabled) Serial.print("ON\n\n");      
    }
    else
    {
      if(log_enabled) Serial.print("OFF\n\n");  
    }
  }
  return debug_mode;
}






