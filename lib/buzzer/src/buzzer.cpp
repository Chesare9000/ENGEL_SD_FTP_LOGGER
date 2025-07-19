
//Generic Deps.
#include <Arduino.h>

//Custom Deps.
#include <vars.h>
#include <tools.h>
#include "buzzer.h"
#include "pitches.h"

#include <ble_demo_fw.h>

//FOR CHOSEN BUZZER CMT-8503-85B-SMT-TR
//85dB at 10 cm, rated voltage 3.6V_0P, 2,730 Hz, ½ duty square wave



//Testing New Buzzer LIB
//#include <ToneESP32.h>
//ToneESP32 buzzer(esp_buzzer_pin, 0);

//Testing the new tone implementation

#define BUZZER_PWM_CHANNEL 0


void jingle_bells() 
{
    Serial.println("---PLAYING JINGLE BELLS---");

    // notes in the melody:
    int melody[] = 
    {
        NOTE_E5, NOTE_E5, NOTE_E5,
        NOTE_E5, NOTE_E5, NOTE_E5,
        NOTE_E5, NOTE_G5, NOTE_C5, NOTE_D5,
        NOTE_E5,
        NOTE_F5, NOTE_F5, NOTE_F5, NOTE_F5,
        NOTE_F5, NOTE_E5, NOTE_E5, NOTE_E5, NOTE_E5,
        NOTE_E5, NOTE_D5, NOTE_D5, NOTE_E5,
        NOTE_D5, NOTE_G5
    };

    // note durations: 4 = quarter note, 8 = eighth note, etc, also called tempo:
    int noteDurations[] = 
    {
        8, 8, 4,
        8, 8, 4,
        8, 8, 8, 8,
        2,
        8, 8, 8, 8,
        8, 8, 8, 16, 16,
        8, 8, 8, 8,
        4, 4
    }; 

    // iterate over the notes of the melody:
    int size = sizeof(noteDurations) / sizeof(int);

    for (int thisNote = 0; thisNote < size; thisNote++) 
    {

        // to calculate the note duration, take one second divided by the note type.
        //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
        int noteDuration = 1000 / noteDurations[thisNote];
        tone(esp_buzzer_pin, melody[thisNote], noteDuration);

        // to distinguish the notes, set a minimum time between them.
        // the note's duration + 30% seems to work well:
        int pauseBetweenNotes = noteDuration * 1.30;
        wait(pauseBetweenNotes);
        // stop the tone playing:
        noTone(esp_buzzer_pin);
    }
}

void imperial_march()
{
    Serial.println("---PLAYING THE IMPERIAL MARCH---");

    // change this to make the song slower or faster
    int tempo = 120;
    
    // notes of the moledy followed by the duration.
    // a 4 means a quarter note, 8 an eighteenth , 16 sixteenth, so on
    // !!negative numbers are used to represent dotted notes,
    // so -4 means a dotted quarter note, that is, a quarter plus an eighteenth!!
    int melody[] = 
    {
        // Dart Vader theme (Imperial March) - Star wars 
        // Score available at https://musescore.com/user/202909/scores/1141521
        // The tenor saxophone part was used
        
        NOTE_A4,-4, NOTE_A4,-4, NOTE_A4,16, NOTE_A4,16, NOTE_A4,16, NOTE_A4,16, NOTE_F4,8, REST,8,
        NOTE_A4,-4, NOTE_A4,-4, NOTE_A4,16, NOTE_A4,16, NOTE_A4,16, NOTE_A4,16, NOTE_F4,8, REST,8,
        NOTE_A4,4, NOTE_A4,4, NOTE_A4,4, NOTE_F4,-8, NOTE_C5,16,

        NOTE_A4,4, NOTE_F4,-8, NOTE_C5,16, NOTE_A4,2,//4
        NOTE_E5,4, NOTE_E5,4, NOTE_E5,4, NOTE_F5,-8, NOTE_C5,16,
        NOTE_A4,4, NOTE_F4,-8, NOTE_C5,16, NOTE_A4,2,
        
        NOTE_A5,4, NOTE_A4,-8, NOTE_A4,16, NOTE_A5,4, NOTE_GS5,-8, NOTE_G5,16, //7 
        NOTE_DS5,16, NOTE_D5,16, NOTE_DS5,8, REST,8, NOTE_A4,8, NOTE_DS5,4, NOTE_D5,-8, NOTE_CS5,16,

        NOTE_C5,16, NOTE_B4,16, NOTE_C5,16, REST,8, NOTE_F4,8, NOTE_GS4,4, NOTE_F4,-8, NOTE_A4,-16,//9
        NOTE_C5,4, NOTE_A4,-8, NOTE_C5,16, NOTE_E5,2,

        NOTE_A5,4, NOTE_A4,-8, NOTE_A4,16, NOTE_A5,4, NOTE_GS5,-8, NOTE_G5,16, //7 
        NOTE_DS5,16, NOTE_D5,16, NOTE_DS5,8, REST,8, NOTE_A4,8, NOTE_DS5,4, NOTE_D5,-8, NOTE_CS5,16,

        NOTE_C5,16, NOTE_B4,16, NOTE_C5,16, REST,8, NOTE_F4,8, NOTE_GS4,4, NOTE_F4,-8, NOTE_A4,-16,//9
        NOTE_A4,4, NOTE_F4,-8, NOTE_C5,16, NOTE_A4,2,
    };

    // sizeof gives the number of bytes, each int value is composed of two bytes (16 bits)
    // there are two values per note (pitch and duration), so for each note there are four bytes
    int notes = sizeof(melody) / sizeof(melody[0]) / 2;

    // this calculates the duration of a whole note in ms
    int wholenote = (60000 * 4) / tempo;

    int divider = 0, noteDuration = 0;

    // iterate over the notes of the melody. 
    // Remember, the array is twice the number of notes (notes + durations)

    for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) 
    {
        // calculates the duration of each note
        divider = melody[thisNote + 1];
        if (divider > 0) 
        {
            // regular note, just proceed
            noteDuration = (wholenote) / divider;
        } 
        else if (divider < 0) 
        {
            // dotted notes are represented with negative durations!!
            noteDuration = (wholenote) / abs(divider);
            noteDuration *= 1.5; // increases the duration in half for dotted notes
        }

        // we only play the note for 90% of the duration, leaving 10% as a pause
        tone(esp_buzzer_pin, melody[thisNote], noteDuration*0.9);

        // Wait for the specief duration before playing the next note.
        wait(noteDuration);
        
        // stop the waveform generation before the next note.
        noTone(esp_buzzer_pin);
    }
}

void cantina_band_star_wars()
{
    // change this to make the song slower or faster
    int tempo = 140;

    // notes of the moledy followed by the duration.
    // a 4 means a quarter note, 8 an eighteenth , 16 sixteenth, so on
    // !!negative numbers are used to represent dotted notes,
    // so -4 means a dotted quarter note, that is, a quarter plus an eighteenth!!
    int melody[] = 
    {
  
        // Cantina BAnd - Star wars 
        // Score available at https://musescore.com/user/6795541/scores/1606876
        NOTE_B4,-4, NOTE_E5,-4, NOTE_B4,-4, NOTE_E5,-4, 
        NOTE_B4,8,  NOTE_E5,-4, NOTE_B4,8, REST,8,  NOTE_AS4,8, NOTE_B4,8, 
        NOTE_B4,8,  NOTE_AS4,8, NOTE_B4,8, NOTE_A4,8, REST,8, NOTE_GS4,8, NOTE_A4,8, NOTE_G4,8,
        NOTE_G4,4,  NOTE_E4,-2, 
        NOTE_B4,-4, NOTE_E5,-4, NOTE_B4,-4, NOTE_E5,-4, 
        NOTE_B4,8,  NOTE_E5,-4, NOTE_B4,8, REST,8,  NOTE_AS4,8, NOTE_B4,8,

        NOTE_A4,-4, NOTE_A4,-4, NOTE_GS4,8, NOTE_A4,-4,
        NOTE_D5,8,  NOTE_C5,-4, NOTE_B4,-4, NOTE_A4,-4,
        NOTE_B4,-4, NOTE_E5,-4, NOTE_B4,-4, NOTE_E5,-4, 
        NOTE_B4,8,  NOTE_E5,-4, NOTE_B4,8, REST,8,  NOTE_AS4,8, NOTE_B4,8,
        NOTE_D5,4, NOTE_D5,-4, NOTE_B4,8, NOTE_A4,-4,
        NOTE_G4,-4, NOTE_E4,-2,
        NOTE_E4, 2, NOTE_G4,2,
        NOTE_B4, 2, NOTE_D5,2,

        NOTE_F5, -4, NOTE_E5,-4, NOTE_AS4,8, NOTE_AS4,8, NOTE_B4,4, NOTE_G4,4, 
    };

    // sizeof gives the number of bytes, each int value is composed of two bytes (16 bits)
    // there are two values per note (pitch and duration), so for each note there are four bytes
    int notes = sizeof(melody) / sizeof(melody[0]) / 2;

    // this calculates the duration of a whole note in ms
    int wholenote = (60000 * 2) / tempo;

    int divider = 0, noteDuration = 0;


    // iterate over the notes of the melody. 
    // Remember, the array is twice the number of notes (notes + durations)
    for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2)
    {
        // calculates the duration of each note
        divider = melody[thisNote + 1];
        if (divider > 0) 
        {
            // regular note, just proceed
            noteDuration = (wholenote) / divider;
        } 
        else if (divider < 0) 
        {
            // dotted notes are represented with negative durations!!
            noteDuration = (wholenote) / abs(divider);
            noteDuration *= 1.5; // increases the duration in half for dotted notes
        }

        // we only play the note for 90% of the duration, leaving 10% as a pause
        tone(esp_buzzer_pin, melody[thisNote], noteDuration*0.9);

        // Wait for the specief duration before playing the next note.
        wait(noteDuration);
        
        // stop the waveform generation before the next note.
        noTone(esp_buzzer_pin);
    }

}

void tetris()
{
    if(log_enabled)Serial.print("\n ---Playing Tetris--- \n");
    // change this to make the song slower or faster
    int tempo=144; 

    // notes of the moledy followed by the duration.
    // a 4 means a quarter note, 8 an eighteenth , 16 sixteenth, so on
    // !!negative numbers are used to represent dotted notes,
    // so -4 means a dotted quarter note, that is, a quarter plus an eighteenth!!
    int melody[] = 
    {

        //Based on the arrangement at https://www.flutetunes.com/tunes.php?id=192
        
        NOTE_E5, 4,  NOTE_B4,8,  NOTE_C5,8,  NOTE_D5,4,  NOTE_C5,8,  NOTE_B4,8,
        NOTE_A4, 4,  NOTE_A4,8,  NOTE_C5,8,  NOTE_E5,4,  NOTE_D5,8,  NOTE_C5,8,
        NOTE_B4, -4,  NOTE_C5,8,  NOTE_D5,4,  NOTE_E5,4,
        NOTE_C5, 4,  NOTE_A4,4,  NOTE_A4,8,  NOTE_A4,4,  NOTE_B4,8,  NOTE_C5,8,

        NOTE_D5, -4,  NOTE_F5,8,  NOTE_A5,4,  NOTE_G5,8,  NOTE_F5,8,
        NOTE_E5, -4,  NOTE_C5,8,  NOTE_E5,4,  NOTE_D5,8,  NOTE_C5,8,
        NOTE_B4, 4,  NOTE_B4,8,  NOTE_C5,8,  NOTE_D5,4,  NOTE_E5,4,
        NOTE_C5, 4,  NOTE_A4,4,  NOTE_A4,4, REST, 4,

        NOTE_E5, 4,  NOTE_B4,8,  NOTE_C5,8,  NOTE_D5,4,  NOTE_C5,8,  NOTE_B4,8,
        NOTE_A4, 4,  NOTE_A4,8,  NOTE_C5,8,  NOTE_E5,4,  NOTE_D5,8,  NOTE_C5,8,
        NOTE_B4, -4,  NOTE_C5,8,  NOTE_D5,4,  NOTE_E5,4,
        NOTE_C5, 4,  NOTE_A4,4,  NOTE_A4,8,  NOTE_A4,4,  NOTE_B4,8,  NOTE_C5,8,

        NOTE_D5, -4,  NOTE_F5,8,  NOTE_A5,4,  NOTE_G5,8,  NOTE_F5,8,
        NOTE_E5, -4,  NOTE_C5,8,  NOTE_E5,4,  NOTE_D5,8,  NOTE_C5,8,
        NOTE_B4, 4,  NOTE_B4,8,  NOTE_C5,8,  NOTE_D5,4,  NOTE_E5,4,
        NOTE_C5, 4,  NOTE_A4,4,  NOTE_A4,4, REST, 4,
        

        NOTE_E5,2,  NOTE_C5,2,
        NOTE_D5,2,   NOTE_B4,2,
        NOTE_C5,2,   NOTE_A4,2,
        NOTE_GS4,2,  NOTE_B4,4,  REST,8, 
        NOTE_E5,2,   NOTE_C5,2,
        NOTE_D5,2,   NOTE_B4,2,
        NOTE_C5,4,   NOTE_E5,4,  NOTE_A5,2,
        NOTE_GS5,2,

    };

    // sizeof gives the number of bytes, each int value is composed of two bytes (16 bits)
    // there are two values per note (pitch and duration), so for each note there are four bytes
    int notes=sizeof(melody)/sizeof(melody[0])/2; 

    // this calculates the duration of a whole note in ms (60s/tempo)*4 beats
    int wholenote = (60000 * 4) / tempo;

    int divider = 0, noteDuration = 0;

    // iterate over the notes of the melody. 
    // Remember, the array is twice the number of notes (notes + durations)
    for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) 
    {
        // calculates the duration of each note
        divider = melody[thisNote + 1];
        if (divider > 0) {
        // regular note, just proceed
        noteDuration = (wholenote) / divider;
        } else if (divider < 0) {
        // dotted notes are represented with negative durations!!
        noteDuration = (wholenote) / abs(divider);
        noteDuration *= 1.5; // increases the duration in half for dotted notes
        }

        // we only play the note for 90% of the duration, leaving 10% as a pause
        tone(esp_buzzer_pin, melody[thisNote], noteDuration*0.9);

        // Wait for the specief duration before playing the next note.
        wait(noteDuration);
        
        // stop the waveform generation before the next note.
        noTone(esp_buzzer_pin);   
    }
}

void nokia()
{
    if(log_enabled)Serial.print("\n ---Playing Nokia--- \n");
    // change this to make the song slower or faster
    int tempo = 180;

    // notes of the moledy followed by the duration.
    // a 4 means a quarter note, 8 an eighteenth , 16 sixteenth, so on
    // !!negative numbers are used to represent dotted notes,
    // so -4 means a dotted quarter note, that is, a quarter plus an eighteenth!!
    int melody[] = 
    {
        // Nokia Ringtone 
        // Score available at https://musescore.com/user/29944637/scores/5266155
        
        NOTE_E5, 8, NOTE_D5, 8, NOTE_FS4, 4, NOTE_GS4, 4, 
        NOTE_CS5, 8, NOTE_B4, 8, NOTE_D4, 4, NOTE_E4, 4, 
        NOTE_B4, 8, NOTE_A4, 8, NOTE_CS4, 4, NOTE_E4, 4,
        NOTE_A4, 2, 
    };

    // sizeof gives the number of bytes, each int value is composed of two bytes (16 bits)
    // there are two values per note (pitch and duration), so for each note there are four bytes
    int notes = sizeof(melody) / sizeof(melody[0]) / 2;

    // this calculates the duration of a whole note in ms
    int wholenote = (60000 * 4) / tempo;

    int divider = 0, noteDuration = 0;

    // iterate over the notes of the melody.
    // Remember, the array is twice the number of notes (notes + durations)
    for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) 
    {
        // calculates the duration of each note
        divider = melody[thisNote + 1];
        if (divider > 0) 
        {
            // regular note, just proceed
            noteDuration = (wholenote) / divider;
        } 
        else if (divider < 0) 
        {
            // dotted notes are represented with negative durations!!
            noteDuration = (wholenote) / abs(divider);
            noteDuration *= 1.5; // increases the duration in half for dotted notes
        }

        // we only play the note for 90% of the duration, leaving 10% as a pause
        tone(esp_buzzer_pin, melody[thisNote], noteDuration * 0.9);

        // Wait for the specief duration before playing the next note.
        delay(noteDuration);

        // stop the waveform generation before the next note.
        noTone(esp_buzzer_pin);
    }
}

void two_tone_alarm()
{
  if(log_enabled)Serial.print("\n ---Playing Two-Tone--- \n");  
  for (int count=1;count<3;count++)
  {
    float f_max=2000;
    float f_min=1000;
    float wait_time=800;

    tone(esp_buzzer_pin,f_max);
    wait(wait_time);
    tone(esp_buzzer_pin,f_min);
    wait(wait_time);
  }
  noTone(esp_buzzer_pin);
}

//mode
// 0 demo for oled 
// 1 ble_demo_app

void rise_fall(int mode)
{
  if(log_enabled && mode == 0) Serial.print("\n ---Playing Rise and Fall--- \n");  
  
  for (int count=1;count<=3;count++)
  {
    if(mode ==1 && ble_buzzer_mode != 3)break;

    float rise_fall_time=500;
    int steps=50;
    float f_max=2600;
    float f_min=1000;
    float wait_time=rise_fall_time/steps;
    float step_size=(f_max-f_min)/steps;
    for (float f =f_min;f<f_max;f+=step_size)
    {
        if(mode ==1 && ble_buzzer_mode != 3)break;
        tone(esp_buzzer_pin,f);
        wait(wait_time);
    }
    for (float f =f_max;f>f_min;f-=step_size)
    {
        if(mode ==1 && ble_buzzer_mode != 3)break;
        tone(esp_buzzer_pin,f);
        wait(wait_time);
    } 
  }
  noTone(esp_buzzer_pin);
  if(mode ==1 && ble_buzzer_mode != 3)Serial.print("\n--rise_fall() Fn. Interrupted!");
}

void fall()
{
  if(log_enabled)Serial.print("\n ---Playing Fall--- \n");  
  float rise_fall_time = 500;    
  int steps=50;
  float f_max=2000;
  float f_min=500;
  float wait_time=rise_fall_time/steps;
  float step_size=0.97;

  for (int count=1;count<=3;count++)
  {
    for (float f =f_max;f>f_min;f*=step_size)
    {
        tone(esp_buzzer_pin,f);
        wait(wait_time);
    }
  }

  noTone(esp_buzzer_pin);
  
}

void rise()
{
  if(log_enabled)Serial.print("\n ---Playing Rise--- \n");  
  float rise_fall_time=2000;
  int steps=100;
  float f_max=1500;
  float f_min=500;
  float wait_time=rise_fall_time/steps;
  float step_size=1.012;


  for (int count=1;count<=3;count++)
  {
    for (float f =f_min;f<f_max;f*=step_size)
    {
        tone(esp_buzzer_pin,f);
        wait(wait_time);
    }
     
    noTone(esp_buzzer_pin);
    wait(100);
  }
  noTone(esp_buzzer_pin);
}


//testing new lib
/*
void buzzer_test_tone()
{
  Serial.println("LOOPINNG BUZZER");

  buzzer.tone(NOTE_C4, 250);
  buzzer.tone(NOTE_D4, 250);
  buzzer.tone(NOTE_E4, 250);
  buzzer.tone(NOTE_F4, 250);
  buzzer.tone(NOTE_G4, 250);
  buzzer.tone(NOTE_A4, 250);
  buzzer.tone(NOTE_B4, 250);
  buzzer.tone(NOTE_C5, 250);
  delay(250);
  buzzer.tone(NOTE_C5, 250);
  buzzer.tone(NOTE_B4, 250);
  buzzer.tone(NOTE_A4, 250);
  buzzer.tone(NOTE_G4, 250);
  buzzer.tone(NOTE_F4, 250);
  buzzer.tone(NOTvoid OK_sound(){
ledcWriteTone(0, NOTE_A5); // tone 880 HZ freq (PWM_ch,A5)
delay(100); // pause 100ms
ledcWriteTone(0, NOTE_CS6); // tone 1109 Hz freq (PWM_ch,CS6)
delay(100); // pause 100ms
ledcWriteTone(0, NOTE_FS6); // tone 1480 Hz freq (PWM_ch,FS6)
delay(150); // pause 150ms
ledcWrite(0, 0); // tone off
delay(10);E_E4, 250);
  buzzer.tone(NOTE_D4, 250);
  buzzer.tone(NOTE_C4, 250);
  buzzer.noTone();
}
*/


//Old implementation
/*
void buzzer_one_tone(int frequency_hz, int duration_ms, int interval_ms,int cycles)
{
  for (int count= 0 ; count<cycles ; count++)
  {
    tone(esp_buzzer_pin,frequency_hz);
    wait(duration_ms);
    noTone(esp_buzzer_pin);
    wait(interval_ms);
  }
}
*/

//New Implementation

void buzzer_one_tone(int frequency_hz, int duration_ms, int interval_ms,int cycles)
{
  for (int count= 0 ; count < cycles ; count++)
  {
    ledcWriteTone(BUZZER_PWM_CHANNEL, frequency_hz);
    
    wait(duration_ms);

    ledcWrite(BUZZER_PWM_CHANNEL, 0); // Stop tone 
    
    wait(interval_ms);
  }
}







//This is for the menu or specific tones
void buzzer_tone(int selection)
{
    switch(selection)
    {
        case 0:
        {
            if(log_enabled) Serial.print("\n -- RUNNING TONE 0 --\n");
            two_tone_alarm();
            //buzzer_test_tone();
            break; 
        }  
        case 1:
        {
            if(log_enabled) Serial.print("\n -- RUNNING TONE 1 --\n");
            rise();  
            break; 
        }  
        case 2:
        {
            if(log_enabled) Serial.print("\n -- RUNNING TONE 2 --\n");
            fall();
            break; 
        }  
        case 3:
        {
            if(log_enabled) Serial.print("\n -- RUNNING TONE 3 --\n");
            rise_fall(0);
            break; 
        }  
        case 4:
        {
            if(log_enabled) Serial.print("\n -- RUNNING TONE 4 --\n");
            nokia();    
            break; 
        }  
        case 5:
        {
            if(log_enabled) Serial.print("\n -- RUNNING TONE 5 --\n");
            tetris();
            break; 
        }  
        case 6:
        {
            if(log_enabled) Serial.print("\n -- RUNNING TONE 6 --\n");
            jingle_bells();
            break; 
        }  

        default:
        {
            Serial.print("\n -- ERROR , TONE NOT DEFINED --\n");
            break; 
        }       
    }

}






//New sounds without tone()

void buzzer_ok()
{
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_A5); // tone 880 HZ freq (PWM_ch,A5)
  wait(100); // pause 100ms
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_CS6); // tone 1109 Hz freq (PWM_ch,CS6)
  wait(100); // pause 100ms
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_FS6); // tone 1480 Hz freq (PWM_ch,FS6)
  wait(150); // pause 150ms
  ledcWrite(BUZZER_PWM_CHANNEL, 0); // tone off
  wait(10);
}

void buzzer_notification() 
{
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_E6); // 1319 Hz
  wait(80);
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_G6); // 1568 Hz
  wait(80);
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_E7); // 2637 Hz
  wait(120);
  ledcWrite(BUZZER_PWM_CHANNEL, 0);
  wait(10);
}

void buzzer_error() 
{
  for (int i = 0; i < 2; i++) 
  {
    ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_C5); // 523 Hz
    wait(120);
    ledcWrite(BUZZER_PWM_CHANNEL, 0);
    wait(80);
  }
}

void buzzer_alarm() 
{
  for (int i = 0; i < 3; i++) 
  {
    ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_A6); // 1760 Hz
    wait(100);
    ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_D6); // 1175 Hz
    wait(100);
  }
  ledcWrite(BUZZER_PWM_CHANNEL, 0);
  wait(10);
}




// Buzzer Functions - Short Audible Feedback
// ----------------------------------------
// Ultra-short clicks (1-20ms)
void buzzer_heartbeat_short_click() {
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_B7); // 3951 Hz (sharp & short)
  wait(5); // 5ms pulse (barely audible "tick")
  ledcWrite(BUZZER_PWM_CHANNEL, 0);
}

void buzzer_heartbeat_tick() {
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_FS6); // 1480 Hz
  wait(10); // 10ms pulse
  ledcWrite(BUZZER_PWM_CHANNEL, 0);
}

void buzzer_drive_click() {
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_C7); // 2093 Hz
  wait(5); // First click
  ledcWrite(BUZZER_PWM_CHANNEL, 0);
  wait(95); // Pause (100ms total cycle)
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_E7); // 2637 Hz
  wait(5); // Second click
  ledcWrite(BUZZER_PWM_CHANNEL, 0);
}

// Short beeps (20-100ms)
void buzzer_sonar_ping() {
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_A6); // 1760 Hz
  wait(20); // Short beep
  ledcWrite(BUZZER_PWM_CHANNEL, 0);
}

void buzzer_status_ping() {
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_A5); // 880 Hz
  wait(100); // 100ms beep
  ledcWrite(BUZZER_PWM_CHANNEL, 0);
}

// Functional confirmations (50-100ms)
void buzzer_function_done() {
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_E6); // 1319 Hz
  wait(50); // 50ms pulse
  ledcWrite(BUZZER_PWM_CHANNEL, 0);
}

void buzzer_heartbeat() {
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_C7); // 2093 Hz
  wait(100); // 100ms beep
  ledcWrite(BUZZER_PWM_CHANNEL, 0);
}

// Multi-tone alerts
void buzzer_quick_alert() {
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_E6); // 1319 Hz
  wait(50);
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_G6); // 1568 Hz
  wait(50);
  ledcWrite(BUZZER_PWM_CHANNEL, 0);
}

void buzzer_two_tone_confirm() {
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_G5); // 784 Hz
  wait(30);
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_C6); // 1047 Hz
  wait(30);
  ledcWrite(BUZZER_PWM_CHANNEL, 0);
}

// Special variants
void buzzer_knock_confirm() {
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_A4); // 440 Hz
  wait(80); // 80ms pulse
  ledcWrite(BUZZER_PWM_CHANNEL, 0);
}

void buzzer_echo_ping() {
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_F6); // 1397 Hz
  wait(20);
  ledcWrite(BUZZER_PWM_CHANNEL, 0);
  wait(40);
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_F6);
  wait(20);
  ledcWrite(BUZZER_PWM_CHANNEL, 0);
}

void buzzer_heartbeat_double() {
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_G6); // 1568 Hz
  wait(50);
  ledcWrite(BUZZER_PWM_CHANNEL, 0);
  wait(50);
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_G6);
  wait(50);
  ledcWrite(BUZZER_PWM_CHANNEL, 0);
}



















// Additional patterns

void buzzer_startup() 
{
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_C5);
  wait(80);
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_E5);
  wait(80);
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_G5);
  wait(120);
  ledcWrite(BUZZER_PWM_CHANNEL, 0);
  wait(10);
}

void buzzer_shutdown() 
{
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_G5);
  wait(80);
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_E5);
  wait(80);
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_C5);
  wait(120);
  ledcWrite(BUZZER_PWM_CHANNEL, 0);
  wait(10);
}

void buzzer_warning() 
{
  for (int i = 0; i < 2; i++) 
  {
    ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_F5);
    wait(60);
    ledcWrite(BUZZER_PWM_CHANNEL, 0);
    wait(60);
  }
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_DS5);
  wait(200);
  ledcWrite(BUZZER_PWM_CHANNEL, 0);
  wait(10);
}

void buzzer_success() 
{
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_G5);
  wait(60);
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_B5);
  wait(60);
  ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_D6);
  wait(120);
  ledcWrite(BUZZER_PWM_CHANNEL, 0);
  wait(10);
}

void buzzer_attention() 
{
  for (int i = 0; i < 4; i++) {
    ledcWriteTone(BUZZER_PWM_CHANNEL, NOTE_C6);
    wait(40);
    ledcWrite(BUZZER_PWM_CHANNEL, 0);
    wait(40);
  }
  wait(10);
}



void buzzer_short_rise_and_fall()
{
  // Short rise and fall
  for (int i=1; i<20; i++) 
  {
    ledcWriteTone(BUZZER_PWM_CHANNEL, i * 100);
    wait(10);    
  }
  for (int i=10; i>0; i--) 
  {
    ledcWriteTone(BUZZER_PWM_CHANNEL, i * 100);
    wait(10);    
  }
  ledcWrite(BUZZER_PWM_CHANNEL, 0); // Stop tone 
}














/*

OTHER TESTED SOUNDS (NOT USED ATM)

int gap=1000;

void zap1()
{
    for (float f=3000;f>40;f=f*0.93){
    tone(esp_buzzer_pin,f);
    wait(10);
  }
}

void zap2()
{
    for (float f=3000;f>10;f=f*0.85){
    tone(esp_buzzer_pin,2*f);
    wait(5);
    tone(esp_buzzer_pin,f);
    wait(5); 
  }
}
void risefall()
{
  float rise_fall_time=180;
  int steps=50;
  float f_max=2600;
  float f_min=1000;
  float wait_time=rise_fall_time/steps;
  float step_size=(f_max-f_min)/steps;
  for (float f =f_min;f<f_max;f+=step_size){
    tone(esp_buzzer_pin,f);
    wait(wait_time);
  }
   for (float f =f_max;f>f_min;f-=step_size){
    tone(esp_buzzer_pin,f);
    wait(wait_time);
  }
}
void fall(float rise_fall_time)
{
  int steps=50;
  float f_max=2000;
  float f_min=500;
  float wait_time=rise_fall_time/steps;
  float step_size=0.97;
  for (float f =f_max;f>f_min;f*=step_size){
    tone(esp_buzzer_pin,f);
    wait(wait_time);
  }
}
void rise()
{
  float rise_fall_time=2000;
  int steps=100;
  float f_max=1500;
  float f_min=500;
  float wait_time=rise_fall_time/steps;
  float step_size=1.012;
  for (float f =f_min;f<f_max;f*=step_size){
    tone(esp_buzzer_pin,f);
    wait(wait_time);
  }
  noTone(esp_buzzer_pin);
  wait(100);
  
}

void twotone()
{
  float f_max=1500;
  float f_min=1000;
  float wait_time=800;
  tone(esp_buzzer_pin,f_max);
  wait(wait_time);
  tone(esp_buzzer_pin,f_min);
  wait(wait_time);
  
}
void other_alarms() 
{
  // optional alarms for later check:
  for (int count=1;count<=10;count++)
  {
    risefall();
  }
  noTone(esp_buzzer_pin);
  wait(gap);
  for (int count=1;count<=10;count++)
  {
    fall(300);
  } 
  noTone(esp_buzzer_pin);
  wait(gap); 
  for (int count=1;count<=5;count++)
  {
    fall(600);
  }
  noTone(esp_buzzer_pin);
  wait(gap); 
  for (int count=1;count<5;count++)
  {
    rise();
  }
  noTone(esp_buzzer_pin);
  wait(gap); 
  for (int count=1;count<5;count++)
  {
    twotone();
  }
  noTone(esp_buzzer_pin);
  wait(gap); 
  for (int count=1;count<10;count++)
  {
    zap1();
  }
  noTone(esp_buzzer_pin);
  wait(gap); 
  for (int count=1;count<10;count++)
  {
    zap2();
  }
  noTone(esp_buzzer_pin);
  wait(gap); 
  
  
}



void Toneone() {
  int melody[] = { //tone array
    NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0,
    NOTE_B3, NOTE_B3, NOTE_B3, NOTE_B3, NOTE_A3, NOTE_G3,
    NOTE_B3, NOTE_B3, NOTE_B3, NOTE_B3, NOTE_A3, NOTE_G3,
    NOTE_C4, NOTE_G3, NOTE_G3, NOTE_G3, 0
  };
  int noteDurations[] = {4, 8, 8, 4, 4, 4, 8, 8, 8, 8, 4, 4, 8, 8, 8, 8, 4, 4, 4, 8, 8, 4, 4};
  for (int thisNote = 0; thisNote < 23; thisNote++) {
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(esp_buzzer_pin, melody[thisNote], noteDuration); //play tone
    int pauseBetweenNotes = noteDuration * 1.30;
    wait(pauseBetweenNotes);//wait
    noTone(esp_buzzer_pin);//tone off
  }
}

void Tonetwo() {
  int melody2[] = {NOTE_GS7, NOTE_DS8, NOTE_GS7, 0, NOTE_DS8, NOTE_DS8, 0, NOTE_GS7, NOTE_GS7};
  int noteDurations[] = {4, 8, 8, 4, 8, 8, 4, 4, 4};
  for (int thisNote = 0; thisNote < 9; thisNote++) {
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(esp_buzzer_pin, melody2[thisNote], noteDuration);
    int pauseBetweenNotes = noteDuration * 1.30;
    wait(pauseBetweenNotes);
    noTone(esp_buzzer_pin);
  }
}

void Tonethree() {
  int melody3[] = {NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6, NOTE_C7,
                   NOTE_CS7, NOTE_D7, NOTE_DS7, NOTE_E7 };
  int noteDurations[] = {4, 8, 8, 4, 8, 8, 4, 4, 4};
  for (int thisNote = 0; thisNote < 9; thisNote++) {
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(esp_buzzer_pin, melody3[thisNote], noteDuration);
    int pauseBetweenNotes = noteDuration * 1.30;
    wait(pauseBetweenNotes);
    noTone(esp_buzzer_pin);
  }
}

void Tonefour() {
  int melody4[] = {  NOTE_F5, NOTE_C6, NOTE_AS5, NOTE_C6, NOTE_AS5, NOTE_C6, NOTE_GS5,
                     NOTE_AS5, NOTE_C6, NOTE_AS5, NOTE_GS5, NOTE_FS5, NOTE_F5, NOTE_C6,
                     NOTE_AS5, NOTE_C6, NOTE_AS5, NOTE_C6, NOTE_GS5, NOTE_AS5, NOTE_C6,
                     NOTE_AS5, NOTE_F5, NOTE_C6, NOTE_AS5, NOTE_C6, NOTE_AS5, NOTE_C6,
                     NOTE_GS5, NOTE_AS5, NOTE_C6, NOTE_AS5, NOTE_GS5, NOTE_FS5, NOTE_DS5,
                     NOTE_F5, NOTE_FS5, NOTE_GS5, NOTE_FS5, NOTE_F5, NOTE_DS5, NOTE_FS5, NOTE_F5
                  };
  int noteDurations[] = {4, 8, 4, 8, 8, 8, 4, 8, 8, 4, 8, 8, 4, 8, 4, 8, 8, 8, 4, 8, 8, 2, 4, 8, 4, 8, 8, 8, 4, 8, 8, 4, 8, 8, 4, 8, 8, 4, 8, 8, 4, 8, 2};
  for (int thisNote = 0; thisNote < 43; thisNote++) {
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(esp_buzzer_pin, melody4[thisNote], noteDuration);
    int pauseBetweenNotes = noteDuration * 1.30;
    wait(pauseBetweenNotes);
    noTone(esp_buzzer_pin);
  }
}

void alarm_3() 
{
  Toneone();
  wait(100);
  Tonetwo();
  wait(100);
  Tonethree();
  wait(100);
  Tonefour();//uncomment this tone
  wait(100);
}


*/