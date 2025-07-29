#pragma once





//FNS.

bool wifi_connect();
bool wifi_connect_to(String ssid, String pass ); 

bool wifi_disconnect();

void wifi_off();

bool update_time_via_wifi();