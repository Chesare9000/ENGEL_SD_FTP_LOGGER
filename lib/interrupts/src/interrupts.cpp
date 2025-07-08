#include <Arduino.h>

#include <vars.h>
#include "interrupts.h"

//HERE all the ISRs , e.g. Timers and Buttons



//INITIALIZING ALL INTERRUPTS

void interrupts_init()
{
  //attachInterrupt(GPIOPin, ISR, Mode); to enable
  //use detachInterrupt(GPIOPin); to disable

  //ISR – is the name of the function that will be called each time the interrupt occurs. mus be void IRAM_ATTR Name()

  //Mode – defines when the interrupt should be triggered. Five constants are predefined as valid values:

  //LOW	Triggers the interrupt whenever the pin is LOW
  //HIGH	Triggers the interrupt whenever the pin is HIGH
  //CHANGE	Triggers the interrupt whenever the pin changes value, from HIGH to LOW or LOW to HIGH
  //FALLING	Triggers the interrupt when the pin goes from HIGH to LOW
  //RISING	Triggers the interrupt when the pin goes from LOW to HIGH

  //FOR THE MOVEMENT SENSOR
 
  mov_switch_interrupt_enable();

  interrupts_initialized = true;
}






void IRAM_ATTR btn_1_press_isr() 
{
    if(!btn_1.is_pressed)
    {
        btn_1.is_pressed = true;
	    btn_1.number_of_push++;    
    }
	
}

//Defining Buttons Interrupt Inits.

void btn_1_interr_enable_on_press()
{
    attachInterrupt(btn_1.pin,btn_1_press_isr,HIGH);
}

/*
Define this if ever wanted to count time pressed

void btn_1_interr_enable_on_release()
{
    attachInterrupt(esp_btn_1_pin,btn_1_released_isr,LOW);
}
*/

void btn_1_interr_disable()
{
    detachInterrupt(btn_1.pin);
}



void IRAM_ATTR btn_2_press_isr() 
{
    if(!btn_2.is_pressed)
    {
        btn_2.is_pressed = true;
	    //btn_2.number_of_push++;
    }
	
}

//Defining Buttons Interrupt Inits.

void btn_2_interr_enable_on_press()
{
    attachInterrupt(btn_2.pin,btn_2_press_isr,HIGH);
}

/*
Define this if ever wanted to count time pressed

void btn_2_interr_enable_on_release()
{
    attachInterrupt(esp_btn_2_pin,btn_2_released_isr,LOW);
}
*/


void btn_2_interr_disable()
{
    detachInterrupt(btn_2.pin);
}





void mov_switch_interrupt_enable()
{
   attachInterrupt(esp_mov_switch_pin,isr_mov_switch,FALLING);
}

void IRAM_ATTR isr_mov_switch()
{
  movement_detected = true;
  moving = true;
}

void mov_switch_interrupt_disable()
{
   detachInterrupt(esp_mov_switch_pin);
}






/*ISR Doc
attachInterrupt(GPIOPin, ISR, Mode);

GPIOPin is the defines pin on the ESP32

ISR – function called when the interrupt occurs.

Mode
LOW	Triggers the interrupt whenever the pin is LOW
HIGH	Triggers the interrupt whenever the pin is HIGH
CHANGE	Triggers the interrupt whenever the pin changes value, from HIGH to LOW or LOW to HIGH
FALLING	Triggers the interrupt when the pin goes from HIGH to LOW
RISING	Triggers the interrupt when the pin goes from LOW to HIGH
*/











//Maybe later measure the pressed time?

