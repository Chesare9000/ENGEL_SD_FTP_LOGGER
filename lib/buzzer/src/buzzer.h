#pragma once 

void buzzer_tone(int selection);

//Melodies
void jingle_bells();
void imperial_march();
void cantina_band_star_wars();
void tetris();
void nokia();
void two_tone_alarm();
void rise_fall(int mode);
void fall();
void rise();


//New Fns without tone() 
void buzzer_ok();

void buzzer_notification();
void buzzer_error();

void buzzer_alarm();


// Buzzer Feedback Functions
// ------------------------
// Ultra-short clicks (1-20ms)
void buzzer_heartbeat_short_click();
void buzzer_heartbeat_tick();
void buzzer_drive_click();

// Short beeps (20-100ms)
void buzzer_sonar_ping();
void buzzer_status_ping();

// Functional confirmations (50-100ms)
void buzzer_function_done();
void buzzer_heartbeat();

// Multi-tone alerts
void buzzer_quick_alert();
void buzzer_two_tone_confirm();

// Special variants
void buzzer_knock_confirm();
void buzzer_echo_ping();
void buzzer_heartbeat_double();



// Additional patterns

void buzzer_startup();

void buzzer_shutdown();

void buzzer_warning();

void buzzer_success();

void buzzer_attention();









void buzzer_short_rise_and_fall();


void buzzer_one_tone(int frequency_hz, int duration_ms, int interval_ms,int cycles);

//Should not be used on deployment
void buzzer_test_tone();
