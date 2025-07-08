#pragma once

void btn_1_interr_enable_on_press();

//void btn_1_interr_enable_on_release();

void btn_1_interr_disable();



void btn_2_interr_enable_on_press();

//void btn_2_interr_enable_on_release();

void btn_2_interr_disable();


void interrupts_init();

void IRAM_ATTR isr_mov_switch();

void mov_switch_interrupt_enable();
void mov_switch_interrupt_disable();
