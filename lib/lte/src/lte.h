#pragma once




bool simcom_init();

void simcom_turn_key();

void simcom_send_AT_command(const String& atCommand);

bool simcom_turn_off();


//LTE Related
void simcom_lte_check_network_status();

void simcom_lte_test_ping();

bool simcom_lte_connect(const char* apn);


//If LTE cannot connect we will run the troubleshooter

bool simcom_lte_init();
bool simcom_lte_init_minimal();

void simcom_lte_get_signal_quality();

void simcom_lte_check_signal_strength();

bool simcom_lte_wait_for_network_registration(unsigned long timeoutMs);

bool simcom_lte_check_sim_card_status();

bool simcom_lte_activate_pdp();

bool simcom_lte_check_gprs_connected();


bool simcom_lte_sync_time_and_set_esp32();

//GPS Related

bool simcom_gps_init();

void simcom_gps_update();


