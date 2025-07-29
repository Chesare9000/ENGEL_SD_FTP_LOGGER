#pragma once

#include <vars.h>

enum can_log_mode
{
    can_log_mode_silent,
    can_log_mode_moderate,
    can_log_mode_verbose
};

bool can_init(int can_log_mode);
void can_poll(int can_log_mode);
void can_log_serial(int can_log_mode);


//TODO blink this with the id so no question needed, wil work every time based on config for that vehicle
bool can_send_activation_message();


//todo make a config maro to have flags bassed on the onfig of the vehicle id

