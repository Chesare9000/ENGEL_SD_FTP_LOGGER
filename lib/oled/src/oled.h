#pragma once

void oled_init();

void oled_clear();
void oled_refresh();


void oled_test();

void oled_gps_test(int gps_test_nr);

void oled_gps_not_detected();
void oled_gps_waiting_data();
void oled_gps_good_data_template();
void oled_gps_bad_data_template();
void oled_gps_spoofed_data_template();
void oled_gps_spoof_question();
void oled_gps_disabled();
void oled_gps_spoofing_disabled();

void oled_task_logger_sd_ftp_running();

void oled_black_box_question();
void oled_sd_not_detected();
void oled_sd_running_mode_black_box();
void oled_black_box_info();
void oled_black_box_info_on_firebase_task();


void oled_can_not_detected();
void oled_can_waiting_data();
void oled_can_data_template();
void oled_can_disabled();


void oled_template_mode_ride();

void oled_template_mode_devel(int cursor_pos);
void oled_template_demo(int cursor_pos);
void oled_template_buzzer(int cursor_pos);


void oled_template_logger_sd_ftp(int cursor_pos);

void oled_template_logger_ms_menu(int cursor_pos);


void oled_demo_patterns(int pattern);
void oled_demo_lux(char color , int brightness);


void oled_demo_lock(int lock_status,int remaining_attempts);

void oled_demo_imu_pos(char axis);
void oled_demo_imu_acc(char axis);

void oled_error_rtc_not_calibrated();

void oled_demo_status(int screen);

void oled_demo_message_imu_booting();

void refresh_oled_ble_normal();

void oled_wifi();
void oled_wifi_demo();


void oled_mqtt(int cycle_nr);
void oled_firebase(int cycle_nr);

void oled_ota(char* ssid);
void oled_logger_wifi(char* ssid);
void oled_logger_wifi_failed();

void oled_template_logger_wifi_menu(int cursor_pos);

void oled_alarm_loop(int alarm_id);
void oled_alarm_cleared(int alarm_id); //Just dismissed but if recurrent will continue happening
void oled_alarm_cleared_and_destroyed(int alarm_id);//Deactivates recurrent alarms


void oled_parking_alarm_disclaimer();
void oled_parking_alarm_movement_detected();
void oled_parking_alarm_triggered();
void oled_parking_alarm_dismissed();
void oled_parking_alarm_snoozed();

void oled_imu_message_accident_detected();
void oled_imu_message_accident_confirmed();
void oled_imu_message_press_btn1_to_dismiss();
void oled_imu_message_accident_dismissed();

void oled_imu_sim_question();
void oled_imu_sim();

void oled_imu_graph_question();
void oled_imu_graph();

void oled_imu_log();
void oled_imu_log_question();

void oled_firebase_question();
void oled_firebase_running();


void oled_riding_mode_enabled();



void oled_main_status_changed_to_parking();

void oled_dev_info(int screen_nr);