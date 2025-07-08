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
