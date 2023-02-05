//DISTRIBUTORE DI CARBURANTE
#include "stm32_unict_lib.h"
#include <stdio.h>


//Utility Variables
enum {
	IDLE,
	EROGAZIONE,
	FINE_EROGAZIONE
};
unsigned int adc_10, adc_11;
unsigned int state = IDLE;
unsigned int print_enable = 0; unsigned int print_timer = 0;

//Other Variables
int importo_inserito = 0;

#define COSTO_CARBURANTE 1.5
int led_giallo_timer = 0;
double carburante_da_erogare;
int erogazione_timer = 0;
double carburante_erogato = 0;


//TIMER & IRQ Function
void EXTI4_IRQHandler(void) {
	if (EXTI_isset(EXTI4)) {

		if(state == IDLE) {
			importo_inserito += 5;
		}

		EXTI_clear(EXTI4);
	}
}

void EXTI9_5_IRQHandler(void) {
	if (EXTI_isset(EXTI5)) {

		if(state == IDLE) {
			importo_inserito = 0;
		}

		EXTI_clear(EXTI5);
	}
	if (EXTI_isset(EXTI6)) {

		if(state == IDLE) {
			state = EROGAZIONE;
			carburante_da_erogare = (float)importo_inserito/COSTO_CARBURANTE;
		}
		else if(state == FINE_EROGAZIONE) {
			state = IDLE;
			importo_inserito = 0;
			carburante_erogato = 0;
			GPIO_write(GPIOB, 0, 0);
		}

		EXTI_clear(EXTI6);
	}
}

void EXTI15_10_IRQHandler(void) {
	if (EXTI_isset(EXTI10)) {

		if(state == IDLE) {
			importo_inserito += 20;
		}

		EXTI_clear(EXTI10);
	}
}

void TIM2_IRQHandler(void) {
	if (TIM_update_check(TIM2)) {

		if(print_timer == 100) {
			print_enable = 1;
			print_timer = 0;
		}

		if(led_giallo_timer == 50) {
			if(state == EROGAZIONE)
				GPIO_toggle(GPIOC, 2);
			led_giallo_timer = 0;
		}

		if(erogazione_timer == 2) {
			if(state == EROGAZIONE) {
				if(carburante_erogato + (double)0.01 <= carburante_da_erogare) {
					carburante_erogato += (double)0.01;
				}
				else {
					state = FINE_EROGAZIONE;
					GPIO_write(GPIOC, 2, 0);
					GPIO_write(GPIOB, 0, 1);
				}
			}
			erogazione_timer = 0;
		}

		print_timer++;
		led_giallo_timer++;
		erogazione_timer++;
		TIM_update_clear(TIM2);
	}
}

//Other Function
void visualize_float(float number) {
	//constrain number < 100.00
	if(number >= 100)
		number = 99.99;
	char s[5];
	sprintf(s, "%4d", (int)(number*100));
	DISPLAY_dp(1, 1);
	DISPLAY_puts(0, s);
}

void visualize_int(int number) {
	for(int i = 0; i < 4; i++)
		DISPLAY_dp(i, 0);
	char s[5];
	sprintf(s, "%4d", number);
	DISPLAY_puts(0, s);
}


void setup(void) {
	//Display Setup
	DISPLAY_init();

	//GPIO Setup
	GPIO_init(GPIOB);
	GPIO_init(GPIOC);
	GPIO_config_output(GPIOB, 0); //Led Rosso
	GPIO_config_output(GPIOC, 2); //Led Giallo

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
	TIM_config_timebase(TIM2, 8400, 100); //100Hz
	TIM_enable_irq(TIM2, IRQ_UPDATE);

	//Serial Setup
	CONSOLE_init();

	//Start Timer
	TIM_set(TIM2, 0);
	TIM_on(TIM2);
}

void loop(void) {
	if(state == IDLE) {
		visualize_int(importo_inserito);
	}
	if(state == EROGAZIONE) {
		visualize_float(carburante_erogato);
	}

	//UART OUTPUT
	if(print_enable){
		printf("%d %lf\n", importo_inserito, carburante_erogato);

		print_enable = 0;
	}
}

int main(void) {
	setup();

	for (;;)
		loop();

	return 0;
}