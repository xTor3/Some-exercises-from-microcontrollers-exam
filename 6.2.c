//Firmware per l'irrigazione
#include "stm32_unict_lib.h"
#include <stdio.h>
#include <string.h>


//Utility Variables
enum {
	IDLE,
	SETUP,
	SETUP2
};
unsigned int adc_10, adc_11;
unsigned int state = IDLE;
unsigned int print_enable = 0; unsigned int print_timer = 0;
char s[5];

//Other Variables
int minuti = 0;
int secondi = 0;
int minuti_st = 0;
int secondi_st = 10;
int minuti_en = 1;
int secondi_en = 2;
int scelta = 0;
int aux_min, aux_sec;
int timer_aux = 0;

//TIMER & IRQ Function
void EXTI4_IRQHandler(void) {
	if (EXTI_isset(EXTI4)) {
		if(state == SETUP || state == SETUP2) {
			state = IDLE;
			GPIO_write(GPIOB, 0, 0);
		}
		EXTI_clear(EXTI4);
	}
}

void EXTI9_5_IRQHandler(void) {
	if (EXTI_isset(EXTI5)) {
		if(state == SETUP) {
			state = SETUP2;
		}
		EXTI_clear(EXTI5);
	}
	if (EXTI_isset(EXTI6)) {

		EXTI_clear(EXTI6);
	}
}

void EXTI15_10_IRQHandler(void) {
	if (EXTI_isset(EXTI10)) {
		if(state != SETUP && state != SETUP2) {
			state = SETUP;
			GPIO_write(GPIOB, 0, 1);
		}
		else if(state == SETUP) {
			switch(scelta) {
			case 0:
				scelta = 1;
				break;
			case 1:
				scelta = 2;
				break;
			case 2:
				scelta = 0;
				break;
			}
		}
		else if(state == SETUP2) {
			switch(scelta) {
			case 0:
				minuti = aux_min;
				secondi = aux_sec;
				break;
			case 1:
				minuti_st = aux_min;
				secondi_st = aux_sec;
				break;
			case 2:
				minuti_en = aux_min;
				secondi_en = aux_sec;
				break;
			}
			GPIO_write(GPIOB, 0, 0);
			state = IDLE;
		}
		EXTI_clear(EXTI10);
	}
}

void TIM2_IRQHandler(void) {
	if (TIM_update_check(TIM2)) {
		if(timer_aux == 100) {
			secondi++;
			if(secondi == 60) {
				minuti++;
				if(minuti == 60) {
					minuti = 0;
				}
				secondi = 0;
			}
			timer_aux = 0;
		}
		timer_aux++;

		if(print_timer == 100) {
			print_enable = 1;
			print_timer = 0;
		}
		print_timer++;
		TIM_update_clear(TIM2);
	}
}


void setup(void) {
	//Display Setup
	DISPLAY_init();

	//GPIO Setup
	GPIO_init(GPIOB);
	GPIO_init(GPIOC);
	GPIO_config_output(GPIOB, 0); //Led Rosso
	GPIO_config_output(GPIOC, 2); //Led Giallo
	GPIO_config_output(GPIOC, 3); //Led Verde

	GPIO_config_input(GPIOB, 10); //Pulsante X
	GPIO_config_input(GPIOB, 4); //Pulsante Y
	GPIO_config_input(GPIOB, 5); //Pulsante Z
	GPIO_config_input(GPIOB, 6); //Pulsante T
	GPIO_config_EXTI(GPIOB, EXTI10);
	GPIO_config_EXTI(GPIOB, EXTI4);
	GPIO_config_EXTI(GPIOB, EXTI5);
	GPIO_config_EXTI(GPIOB, EXTI6);
	EXTI_enable(EXTI10, FALLING_EDGE);
	EXTI_enable(EXTI4, FALLING_EDGE);
	EXTI_enable(EXTI5, FALLING_EDGE);
	EXTI_enable(EXTI6, FALLING_EDGE);

	//Timer Setup
	TIM_init(TIM2);
	TIM_config_timebase(TIM2, 8400, 100); //100Hz, (8400, 10): 1000Hz
	TIM_enable_irq(TIM2, IRQ_UPDATE);

	//ADC Setup
	ADC_init(ADC1, ADC_RES_10, ADC_ALIGN_RIGHT);
	ADC_channel_config(ADC1, GPIOC, 0, 10);
	ADC_channel_config(ADC1, GPIOC, 1, 11);
	ADC_on(ADC1);

	//Serial Setup
	CONSOLE_init();

	//Start Timer
	TIM_set(TIM2, 0);
	TIM_on(TIM2);
}

void loop(void) {
	//ADC1, 10
	ADC_sample_channel(ADC1, 10); //Lasciare se devo controllare entrambi i trim, altrimenti spostare nel setup
	ADC_start(ADC1);
	while (!ADC_completed(ADC1)) {}
	adc_10 = ADC_read(ADC1);
	//ADC1, 11
	ADC_sample_channel(ADC1, 11); //Lasciare se devo controllare entrambi i trim, altrimenti spostare nel setup
	ADC_start(ADC1);
	while (!ADC_completed(ADC1)) {}
	adc_11 = ADC_read(ADC1);

	//LOOP
	if(state == IDLE) {
		sprintf(s, "%2d", minuti);
		sprintf(&s[2], "%2d", secondi);
		DISPLAY_puts(0, s);
	}
	else if(state == SETUP) {
		switch(scelta) {
		case 0:
			strcpy(s, "ti  ");
			break;
		case 1:
			strcpy(s, "St  ");
			break;
		case 2:
			strcpy(s, "En  ");
			break;
		}
		DISPLAY_puts(0, s);
	}
	else if(state == SETUP2) {
		aux_min = (int)(((float)adc_10/1023)*59);
		aux_sec = (int)(((float)adc_11/1023)*59);
		sprintf(s, "%2d", aux_min);
		sprintf(&s[2], "%2d", aux_sec);
		DISPLAY_puts(0, s);
	}

	if(minuti_st == minuti_en) {
		if(minuti >= minuti_st && secondi >= secondi_st && secondi <= secondi_en) {
			GPIO_write(GPIOC, 2, 1);
		}
		else {
			GPIO_write(GPIOC, 2, 0);
		}
	}
	else {
		if((minuti >= minuti_st && minuti < minuti_en && secondi >= secondi_st && secondi <= 59) || (minuti <= minuti_en && minuti > minuti_st && secondi <= secondi_en)) {
			GPIO_write(GPIOC, 2, 1);
		}
		else {
			GPIO_write(GPIOC, 2, 0);
		}
	}

	//UART OUTPUT
	if(print_enable){
		printf("%d %d\n", adc_10, adc_11);
		print_enable = 0;
	}
}

int main(void) {
	setup();

	for (;;)
		loop();

	return 0;
}
