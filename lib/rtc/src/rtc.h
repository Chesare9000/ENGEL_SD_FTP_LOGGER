#pragma once

void rtc_calib();

void rtc_test();

void rtc_update();


void rtc_init();

//just one option possible
void set_recurrent_alarm(int days,int hours,int minutes,int seconds);

// Available alarm_id : 0 or 1
bool alarm_check(int alarm_id);
void alarm_clear(int alarm_id);
void alarm_destroy(int alarm_id);

void check_alarms();

void sync_rtc_from_esp32_time();

void rtc_print_time();