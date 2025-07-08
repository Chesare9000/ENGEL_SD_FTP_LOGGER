//This Lib contain the relevant code to use engel 
//as transceiver with a remote control

#include <lora.h>
#include <LoRaTransceiver.h>
#include <oled.h>
#include <vars.h>
#include <tools.h>
#include <serial.h>
#include <gpio_exp.h> 
#include <tasks.h> 
#include <interrupts.h>

#define test   0
#define manual 1
#define slow   2
#define fast   3
int mode = test; //Default

String data_to_send_via_lora = "";

int msg_nr = 0 ;
unsigned long packet_nr = 0;

bool test_begin    = false;
int  test_interval = 5000 ; //in ms
unsigned long test_timer = 0;


int engel_lora_id = -1;
int engel_lora_status = -1;
int engel_bat_soc = -1;
int engel_last_message_since_seconds = -1 ;


#define lora_band  868E6  //LoRa BAND e.g. 433E6,868E6,915E6

/*
lora_status 

00 - OFF 
01 - ON but NOT Init
02 - Init but not used

10 - Rx mode - NO Signal detected
11 - Rx mode - Signal detected 

20 - Tx mode - NO ACK received
21 - Tx mode - ACK Received

*/

int lora_status = 0;
bool lora_initialized = false;

//Define the Lora Transceiver Entity
LoRaTransceiver lora(esp_v_cs,lora_reset,esp_lora_DIO_0_pin,5,2000); 

void lora_on()
{
  if(lora_status > 0 )
  {

    if(log_enabled) Serial.print("ERROR : LORA IS ALREADY ON");
    return;
  }
  else
  {
    if(log_enabled)Serial.print("\n---Turning ON LORA!---\n");

    //lora_vcc_en signal is the Q2 on the FF2    
    ff2_q_high();
    
    lora_initialized = true;
    lora_status = 1; //ON but not init
  }  
  
  
}

void lora_off()
{
  if(lora_status < 1 )
  {
    if(log_enabled) Serial.print("ERROR : LORA IS ALREADY OFF");
    return;
  }
  else
  {
    if(log_enabled)Serial.print("\n---Turning OFF LORA!---\n");

    //lora_vcc_en signal is the Q2 on the FF2    
    ff2_q_low();
    
    lora_initialized = false;
    lora_status = 0;
  }    
}




/*OLD IMPLEMENTATION
void lora_init()
{
    //to test declare V4 as V5 and hard-wire to Pin 14 but fix for V5 and seal V4 with LORA disabled 
    LoRa.setPins(esp_v_cs, esp_lora_reset_pin, esp_lora_DIO_0_pin);
    Serial.println("\n--- Starting Lora");
    while (!LoRa.begin(lora_band)) 
    {
        Serial.print(".");
        delay(500);
    }
    LoRa.setSyncWord(0xF3);  // ranges from 0-0xFF
    
    /*Supported values are 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, and 500E3.*/
    //signalBandwidth - signal bandwidth in Hz, defaults to 125E3.
    //LoRa.setSignalBandwidth(62.5E3);
    
    //LoRa.setTxPower(20,RF_PACONFIG_PASELECT_PABOOST);	
    //LoRa.setTxPower(20);
    //LoRa.setSpreadingFactor(12);
    //LoRa.setGain(6);
    
    //Serial.println("--- Lora Ready");
//}


//TRANSCEIVER IMPLEMENTATION (NEW)
void lora_init()
{
  // Create a new LoRaTransceiver object
  // SS , RST , DIO0 , (OPT) NACK Retry Limit (Default = 3), (OPT) ACK Timeout in milliseconds (Default = 2000)

  lora.begin(915E6);
  
  

}


void lora_update_terminal()
{
  if(log_enabled)
  {
    /*
    Serial.print("\n\nSending Lora Packet Nr.");
    Serial.print(tx_count);
    Serial.println(" Containing:\n");
        
    Serial.print("T1: "); Serial.println(T1Wh);
    Serial.print("T2: "); Serial.println(Watt);
    Serial.print("Sum: "); Serial.println(SumWh);

    String data = String(tx_count) + "|" + String(T1Wh) + "|" + String(Watt) + "|" + String(SumWh);
            
    Serial.print("\nSending Packet");
    Serial.print(" -> ");
    Serial.println(data);
    */
  }  
}

void lora_send_string(String message)
{
  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();
}

void lora_prepare_message()
{
  //Updating Variables

  //TODO HERE UPDATE SOC , ID , Last second , ETC

  //Preparing the Message 
  data_to_send_via_lora = String(engel_lora_id) + "|" 
                        + String(engel_lora_status) + "|" 
                        + String(engel_bat_soc) + "|" 
                        + String(engel_last_message_since_seconds);

  int lora_data_length = data_to_send_via_lora.length(); 
  lora_data_length++; 
  
  char lora_char[lora_data_length]; //variable for data to send
  data_to_send_via_lora.toCharArray(lora_char, lora_data_length); //change data type from string to uint8_t
}


void lora_test(int test_interval) //in Seconds)
{
    
  //update_display();

  //update_terminal();

  lora_prepare_message();
  
  //lora_send(data_to_send_via_lora);
          
  //tx_count++;

  delay(test_interval*1000);

}


//This is for the Receiver (Remote Control) 
//Run together with a timer to Allow last message

//FIRST DISPLAY IT AND THEN SPLIT TO UPDATE PARAMETERS

int packet_rx = 0;
int lora_rssi = 0;

//LOOP THIS ON TASK TO ALWAYS LISTEN
void rx_loop() 
{
  int packetSize = LoRa.parsePacket();

  if (packetSize) // If packet received 
  {     
    //Starting from Packet Nr.0
    Serial.print("\n--- Received packet ! (RX_Nr:");
    Serial.print(packet_rx);
    Serial.print(") -> '");

    String message = "";
  
    while (LoRa.available()) 
    {
      message += (char)LoRa.read();
    }

    Serial.print(message);

    Serial.print("' <- End Of Packet");

    Serial.print(",  RSSI : ");
    lora_rssi = LoRa.packetRssi();
    Serial.println(lora_rssi);
    
    Serial.println("\nParsing Info....");
    
    split_packet(message); //This Will update the Variables
    
    //After All has been Updated then we can 
    //updateDisplay();

    packet_rx++; //Increase Received Packet's Count to display on next iteration- 
    
  }
}









void split_packet(String packet)
{
  char str[100];
  packet.toCharArray(str, 100);
  char * pch;
  pch = strtok (str, "|");
  int turn = 0;

  while (pch != NULL)
  {
    String sementara = pch;
    turn++;

    // The Structure of the Message is:
    // id|status|soc|last

    if (turn == 1) 
    {
      Serial.print("LORA_ID:");
      Serial.println(pch);
      engel_lora_id = sementara.toFloat();
    }
    if (turn == 2) 
    {
      Serial.print("LORA_STATUS:");
      Serial.println(pch);
      engel_lora_status = sementara.toFloat();
    }
    if (turn == 3) 
    {
      Serial.print("BAT_SOC:");
      Serial.println(pch);
      engel_bat_soc = sementara.toFloat();
    }
    if (turn == 4) 
    {
      Serial.print("LAST LORA_TX SINCE: ");
      Serial.print(pch);
      Serial.println(" SECONDS");
      engel_last_message_since_seconds = sementara.toFloat();
    }
    pch = strtok (NULL, "|");
    delay(100);
  }
  turn = 0;
}




















//lora_DEMO TASK ----------

TaskHandle_t task_lora_demo_handle = NULL;

void create_task_lora_demo() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n-- creating task_lora_demo --");

    task_lora_demo_i2c_declare();

    xTaskCreate
    (
        lora_demo,           //Function Name (must be a while(1))
        "task_lora_demo",    //Logging Name
        4096,                //Stack Size
        NULL,                //Passing Parameters
        8,                   //Task Priority
        &task_lora_demo_handle
    );   

    if(log_enabled) Serial.print("-- done --\n");
}

void task_lora_demo_i2c_declare()
{
    if(log_enabled)Serial.print("\ntask_lora_demo_i2c_declared\n");
    //This Taks will use the following I2C_Devs

    //imu_needed++;
    rgb_needed++;
    //temp_needed++;
    //lux_needed++;
    //rtc_needed++;
    //fuelgauge_needed++;
    oled_needed++;
}

void task_lora_demo_i2c_release()
{
    if(log_enabled)Serial.print("\ntask_lora_demo_i2c_released\n");
    
    //This Taks will release the following I2C_Devs
    
    //imu_needed--;
    rgb_needed--;
    //temp_needed--;
    //lux_needed--;
    //rtc_needed--;
    //fuelgauge_needed--;
    oled_needed--;
}

void lora_demo(void * parameters)
{
  //OUT with BTN2
  btn_2_interr_enable_on_press();

  while(1)
  {
    if(btn_2.is_pressed) //Getting Out
    {
      btn_2_interr_disable();
    
      if(log_enabled) Serial.print(" ----- Back to Menu ! \n ");   


      //Here set the lora to low power or power it off
      lora_off();

      oled_clear();

      wait_for_btn_2_release(); //will reset the btn.pressed flag also

      create_task_devel_menu();

      task_lora_demo_i2c_release();

      vTaskDelete(NULL);//Delete itself
    }
    
    else if (!lora_initialized) 
    {
      if(log_enabled)Serial.print("\n---Initializing lora---\n");
      lora_init();
      
      
      if(oled_enabled) //oled_lora_demo();

    }

    else
    {

    }

  }
  
}





















///This is a snippet to test the remote that will interface with engel

