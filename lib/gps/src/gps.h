
#pragma once

bool gps_init();  
void gps_poll();
void gps_log_serial();

void gps_spoof();

//bool gps_turn_on();

void gps_turn_on();
void gps_turn_off();
void gps_reset();

void gps_serial_send_command(const char* cmd);

enum gps_status
{
    gps_status_not_detected,
    gps_status_off,
    gps_status_on_not_fix,
    gps_status_on_fix,
};
