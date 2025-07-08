#pragma once



enum can_log_mode
{
    can_log_mode_silent,
    can_log_mode_moderate,
    can_log_mode_verbose
};

bool can_init(int can_log_mode);
void can_poll(int can_log_mode);
void can_log_serial(int can_log_mode);