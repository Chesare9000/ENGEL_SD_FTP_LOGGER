#pragma once

#include <Arduino.h>





enum nvs_log_mode
{
    nvs_log_mode_silent,
    nvs_log_mode_moderate,
    nvs_log_mode_verbose
};


//Shortcut Now DELETE LATER

void update_wifi_ssid_from_nvs(int nvs_log_mode);

void update_wifi_pass_from_nvs(int nvs_log_mode);

void update_firebase_api_key_from_nvs(int nvs_log_mode);

void update_firebase_database_url_from_nvs(int nvs_log_mode);


enum nvs_mode
{
    nvs_mode_demo,
    nvs_mode_test,
    nvs_mode_devel,
    nvs_mode_prod
};

enum nvs_wifi_status
{
    nvs_wifi_status_uninitialized,
    nvs_wifi_status_initialized,
    nvs_wifi_status_connected
};

enum nvs_main_status
{
    nvs_main_status_first_time_boot,
    nvs_main_status_ride,
    nvs_main_status_park_no_alarm,
    nvs_main_status_park_alarm
};

enum nvs_alarm_mode
{
    nvs_alarm_mode_silent,
    nvs_alarm_mode_loud,
};


void nvs_retrieve_all(int nvs_log_mode);

int nvs_get_lte_status(int nvs_log_mode);

void nvs_set_lte_status(int nvs_log_mode);

int nvs_get_boot_count(bool increase , int nvs_log_mode); //In Case we come from a fresh reset and not just polling we can increase it  

void nvs_set_boot_count(unsigned int given_count,int nvs_log_mode);

//This will update wifi_ssid and wifi_pass
void nvs_set_wifi_credentials(String ssid, String pass, int nvs_log_mode);

bool nvs_get_wifi_credentials(int nvs_log_mode);

void nvs_delete();

/*

List of States and Variables that will be saved on NVS and recovered upon reboot

-nvs_boot_count
-nvs_mode (upon boot we will decide what to do based on this flag)
-nvs_wifi_status
-nvs_main_status(will give us the current state within the chosen mode)
-nvs_alarm_mode

*/


