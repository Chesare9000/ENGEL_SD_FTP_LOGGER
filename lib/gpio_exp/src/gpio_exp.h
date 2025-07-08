#pragma once

#include <Arduino.h>
#include <vars.h>


//User defined Functions

void gpio_exp_init(); //Custom for V3 check any other rev before uploading!


//TODO , change for different version
void reg_5v_enable();
void reg_5v_disable();

void lvl_shftr_enable();
void lvl_shftr_disable();

void ff1_q_low();

void ff2_q_low();
void ff2_q_high();

void nrf_reset();

void lora_reset();

bool get_gpio_inputs_status(int mode);

int get_hw_variant();

bool get_debug_mode();

void gpio_exp_p11_simcom_turn_key();



