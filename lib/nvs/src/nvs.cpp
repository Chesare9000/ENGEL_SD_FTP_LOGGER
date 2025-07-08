//NVS is a subdependency, all heavy lifting is made by Preferences.h

#include <Arduino.h>
#include <Preferences.h>
#include <iostream>
#include <vars.h>
#include <nvs.h>
#include <tools.h>


#include <nvs_flash.h>

//Overwriting Vars to
#include <wifi.h>
#include <firebase.h>

Preferences nvs;

unsigned int esp_boot_count = 0;

bool nvs_wifi_credentials_retrieved = false;

//If NVS is empty and we are in production mode we will set a first-time-boot
//else depending on the mode we will proceed with other actions  

bool nvs_reset_esp_bootcount() //Return Confirmation for the operation and value rest
{
    //TODO
}

void nvs_retrieve_all(int nvs_log_mode) //Here we will update values 
{
    if(nvs_log_mode > nvs_log_mode_silent )Serial.print("\n -- Retrieving All info from NVS --");

    //Getting Boot Count and increasing if its not the first boot
    esp_boot_count = nvs_get_boot_count(true, nvs_log_mode_moderate);

    if(esp_boot_count == 1 )
    {
        if(log_enabled) Serial.print("\n---FIRST TIME BOOT , RUNNING FACTORY CHECK ---");
        //TODO later implement some auto checks
    }
    else
    {
        if(log_enabled) Serial.printf("\n--- ESP_BOOT_COUNT : %d ---\n",esp_boot_count);
    }

    //Getting WIFI Credentials

    nvs_wifi_credentials_retrieved = nvs_get_wifi_credentials(nvs_log_mode_verbose); 
    
    if(!nvs_wifi_credentials_retrieved)
    {
        Serial.println("ERROR while retrieving WIFI Credentials , setting default ones to NVS");
        
        nvs_set_wifi_credentials(wifi_ssid_default, wifi_pass_default, nvs_log_mode_verbose);
    }

    //Getting LTE Status

    nvs_get_lte_status(nvs_log_mode_moderate);

}

int nvs_get_lte_status(int nvs_log_mode)
{
    if(nvs_log_mode > nvs_log_mode_silent && log_enabled) Serial.print("\n--- Getting LTE Status from NVS ---");

    nvs.begin("lte_info", false);
    lte_status = nvs.getInt("lte_status", 0); //Default value is 0
    nvs.end();

    if(lte_status == 0)//NVS info not found
    {
        if(nvs_log_mode > nvs_log_mode_silent && log_enabled) Serial.print("\n--- LTE Status not set in NVS, setting to default (lte_never_initialized) ---");
        lte_status = lte_never_initialized;
        nvs_set_lte_status(nvs_log_mode_silent); //Set the default value to NVS
    }
    else //If the NVS was at some point initialized
    {
        if(nvs_log_mode > nvs_log_mode_silent && log_enabled) Serial.print("\n--- LTE Status retrieved from NVS:");

        switch(lte_status)
        {
            case lte_nvs_info_not_found:
                if(nvs_log_mode > nvs_log_mode_silent && log_enabled) Serial.print("lte_nvs_info_not_found ---\n\n");
                break;
            case lte_never_initialized:
                if(nvs_log_mode > nvs_log_mode_silent && log_enabled) Serial.print("lte_never_initialized ---\n\n");
                break;
            case lte_off_due_to_hard_reset:
                if(nvs_log_mode > nvs_log_mode_silent && log_enabled) Serial.print("lte_off_due_to_hard_reset ---\n\n");
                break;
            case lte_off_due_to_deep_sleep:
                if(nvs_log_mode > nvs_log_mode_silent && log_enabled) Serial.print("lte_off_due_to_deep_sleep ---\n\n");
                break;
            case lte_on_but_not_working:
                if(nvs_log_mode > nvs_log_mode_silent && log_enabled) Serial.print("lte_on_but_not_working ---\n\n");
                break;
            case lte_on_and_working:
                if(nvs_log_mode > nvs_log_mode_silent && log_enabled) Serial.print("lte_on_and_working ---\n\n");
                break;
            default:
                if(nvs_log_mode > nvs_log_mode_silent && log_enabled) Serial.print("ERROR : Unknown LTE Status , NOT saving to NVS ---\n\n");
                break;    //just logging here atm
        }
    }

    return lte_status;
}

//TODO maybe all setters should be bool to confirm correct writing?

void nvs_set_lte_status(int nvs_log_mode)
{
    if(nvs_log_mode > nvs_log_mode_silent && log_enabled) Serial.print("\n--- Setting LTE Status in NVS : ");

    switch(lte_status)
    {
        case lte_nvs_info_not_found:
            if(nvs_log_mode > nvs_log_mode_silent && log_enabled) Serial.print("lte_nvs_info_not_found");
            break;
        case lte_off_due_to_hard_reset:
            if(nvs_log_mode > nvs_log_mode_silent && log_enabled) Serial.print("lte_off_due_to_hard_reset");
            break;
        case lte_off_due_to_deep_sleep:
            if(nvs_log_mode > nvs_log_mode_silent && log_enabled) Serial.print("lte_off_due_to_deep_sleep");
            break;
        case lte_on_but_not_working:
            if(nvs_log_mode > nvs_log_mode_silent && log_enabled) Serial.print("lte_on_but_not_working");
            break;
        case lte_on_and_working:
            if(nvs_log_mode > nvs_log_mode_silent && log_enabled) Serial.print("lte_on_and_working");
            break;
        default:
            if(nvs_log_mode > nvs_log_mode_silent && log_enabled) Serial.print("ERROR : Unknown LTE Status , NOT saving to NVS");
            return; //Exit if unknown status    
    }

    if(nvs_log_mode > nvs_log_mode_silent && log_enabled) Serial.print(" ---\n");

    nvs.begin("lte_info", false);
    nvs.putInt("lte_status", lte_status);
    nvs.end();

    //if(nvs_log_mode > nvs_log_mode_silent && log_enabled) Serial.printf("\n--- LTE Status set to %d in NVS ---", lte_status);
}

int set_shutdown_reason()
{

}

int get_shutdown_reason()
{
    
}



int nvs_get_boot_count(bool increase , int nvs_log_mode) //This will always print
{
    nvs.begin("esp_info",false);
    esp_boot_count = nvs.getUInt("esp_boot_count",0);
    nvs.end();

    //If we get the default value (0) then the NVS is empty
    if (esp_boot_count == 0)
    {
        if(nvs_log_mode > nvs_log_mode_silent) Serial.print("\n -- NVS is empty , setting esp_boot_count to 1 --- ");
        esp_boot_count++;       
        nvs_set_boot_count(esp_boot_count,nvs_log_mode_moderate);
        return esp_boot_count;
    }
    else //esp_boot_count > 0 
    {       
        esp_boot_count++;
        nvs_set_boot_count(esp_boot_count,nvs_log_mode_moderate);
        if(nvs_log_mode > nvs_log_mode_moderate) Serial.printf(" \n---Boot Count : %d -- ", esp_boot_count );
        return esp_boot_count;    
    }
}

void nvs_set_boot_count(unsigned int given_count,int nvs_log_mode)
{
    nvs.begin("esp_info",false);
    nvs.putUInt("esp_boot_count",given_count);
    nvs.end();
}




String get_user_id()
{
    //expected an String user_id return from Volatile Memory
}

String get_user_id_from_nvs()
{
    //expected an String user_id return from NON-Volatile Memory
}

//Just overwrite it but not save it
void set_user_id(String user_id)
{

}

//Save it
void set_user_id_to_nvs(String user_id)
{

}


//This will update wifi_ssid and wifi_pass
void nvs_set_wifi_credentials(String ssid, String pass, int nvs_log_mode)
{
    if(nvs_log_mode > nvs_log_mode_silent && log_enabled)
    {
        Serial.print("\n--- Setting WIFI Credentials to NVS ");

        if(nvs_log_mode > nvs_log_mode_moderate)
        {
            Serial.printf("\n--- SSID : %s ", ssid.c_str());
            Serial.printf("\n--- PASS : %s ", pass.c_str());
        }
    } 

    Serial.print("\n--- Writing WIFI Credentials to NVS ");
    wait(100);
    nvs.begin("wifi_info", false);
    wait(100);
    Serial.print("\n--- Writing SSID to NVS ");
    nvs.putString("ssid", ssid); 
    Serial.print(" ---> Done");
   
    Serial.print("\n--- Writing Pass to NVS ");
    nvs.putString("pass", pass);
    Serial.print(" ---> Done");
    wait(100);
    nvs.end();
    wait(100);
    Serial.print("\n--- NVS Closed ");
}

bool nvs_get_wifi_credentials(int nvs_log_mode)
{
   if(nvs_log_mode > nvs_log_mode_silent && log_enabled)
   {
        Serial.println("\n--- Getting WiFi_credentials_from_NVS");
   } 

    nvs.begin("wifi_info", false);

    wifi_ssid = nvs.getString("ssid","");

    wifi_pass = nvs.getString("pass","");

    nvs.end();

    if (wifi_ssid == "" || wifi_pass == "")
    {
        //Print Regardless of Log config as is a big error
        if(log_enabled) Serial.print("\n---ERROR : No values found for WIFI Credentials !---\n");
        wifi_has_credentials = false;
        return false;
    }
    else //If there was credentials stored
    {
        if(log_enabled && nvs_log_mode > nvs_log_mode_moderate)
        {
            Serial.printf("\n--- SSID : %s ---", wifi_ssid.c_str());
            Serial.printf("\n--- PASS : %s ---", wifi_pass.c_str());
            Serial.println();
        }
        wifi_has_credentials = true;
        return true;   
    }
}

void nvs_delete()
{
    Serial.println("\n---Deleting Flash ---");
	nvs_flash_erase();
}

//TODO all this functions underneath 

void update_firebase_api_key_from_nvs(int nvs_log_mode)
{

   if(nvs_log_mode == nvs_log_mode_verbose && log_enabled)
   {
    Serial.println("\n--- Updating firebase_api_key_from_nvs");
   }

   //EMULATING NOW BUT MAKE FOR REAL LATER
   char value_from_nvs[50] = "AIzaSyDPxQ-3--VDEGU37vpG7FdTiweSYohak68";

   //strcpy(firebase_api_key,value_from_nvs);

}

void update_firebase_database_url_from_nvs(int nvs_log_mode)
{
   if(nvs_log_mode == nvs_log_mode_verbose && log_enabled)
   {
    Serial.println("\n--- Updating firebase_database_url_from_nvs ---");
   } 
   
   //EMULATING NOW BUT MAKE FOR REAL LATER
   char value_from_nvs[100] = "https://engel-dev-61ef3-default-rtdb.europe-west1.firebasedatabase.app/";

   //strcpy(firebase_database_url,value_from_nvs);
}



//Handle by NVS later
//int backend_config = backend_config_rtdatabase;  

/*
ADD THESE AS WELL

//TODO change this to NVS
const char* mqtt_broker_domain = "192.168.178.162";
uint16_t    mqtt_broker_port   = 1883;

const char* mqtt_broker_client_id   = "engel";
const char* mqtt_broker_client_pass = "engel";

*/



//SET THE PRIMITIVE FNs to save and retrieve from NVS , make generic for all and encapsulate in a single fn for all as well
//make OTA






//NVS Related Fns.






