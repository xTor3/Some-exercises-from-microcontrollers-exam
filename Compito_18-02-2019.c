//AUTOPILOTA
#include "stm32_unict_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


//Utility Variables
enum {
	RUN,
	SETUP
};
unsigned int adc_10, adc_11;
unsigned int state = RUN;
unsigned int print_enable = 0; unsigned int print_timer = 0;

//Other Variables
float RottaEffettiva = 0;
float RottaDesiderata = 90;
float Alettoni = 1;

float LastRottaDesiderata;
int visualize_adc_enable = 0;
int visualize_adc_timer = 0;

float TMAX = 0;
float Turbolenza = 0;


//TIMER & IRQ Function
void EXTI4_IRQHandler(void) {
	if (EXTI_isset(EXTI4)) {

		if(state == SETUP) {
			GPIO_write(GPIOB, 0, 0);
			state = RUN;
		}

		EXTI_clear(EXTI4);
	}
}

void EXTI15_10_IRQHandler(void) {
	if (EXTI_isset(EXTI10)) {

		if(state == RUN) {
			LastRottaDesiderata = RottaDesiderata;
			GPIO_write(GPIOB, 0, 1);
			state = SETUP;
		}
		else if(state == SETUP) {
			RottaDesiderata = LastRottaDesiderata;
			GPIO_write(GPIOB, 0, 0);
			state = RUN;
		}

		EXTI_clear(EXTI10);
	}
}

void TIM2_IRQHandler(void) {
	if (TIM_update_check(TIM2)) {

		if(state == RUN) {
			RottaEffettiva += Turbolenza;

			Alettoni = (RottaDesiderata - RottaEffettiva) * 0.08f;
			if(Alettoni > 10)
				Alettoni = 10;
			else if(Alettoni < -10)
				Alettoni = -10;

			RottaEffettiva = RottaEffettiva + Alettoni * 0.04f;

			if(RottaDesiderata - RottaEffettiva < 2)
				GPIO_write(GPIOC, 3, 1);
			else
				GPIO_write(GPIOC, 3, 0);
		}
		else
			GPIO_write(GPIOC, 3, 0);

		if(print_timer == 10) {
			print_enable = 1;
			print_timer = 0;
		}
		if(visualize_adc_timer == 50) {
			visualize_adc_enable = 1;
			visualize_adc_timer = 0;
		}

		print_timer++;
		visualize_adc_timer++;
		TIM_update_clear(TIM2);
	}
}

//Other Function
void visualize_float(float number) {
	//constrain number < 100.00
	if(number >= 100)
		number = 99.99;
	char s[5];
	sprintf(s, "%d", (int)(number*100));
	DISPLAY_dp(1, 1);
	if(number < 0.1) {
		DISPLAY_putc(3, s[0]);
		DISPLAY_putc(2, '0');
		DISPLAY_putc(1, '0');
		DISPLAY_putc(0, ' ');
	}
	else if(number < 1) {
		DISPLAY_putc(3, s[1]);
		DISPLAY_putc(2, s[0]);
		DISPLAY_putc(1, '0');
		DISPLAY_putc(0, ' ');
	}
	else if(number < 10) {
		DISPLAY_putc(3, s[2]);
		DISPLAY_putc(2, s[1]);
		DISPLAY_putc(1, s[0]);
		DISPLAY_putc(0, ' ');
	}
	else {
		DISPLAY_putc(3, s[3]);
		DISPLAY_putc(2, s[2]);
		DISPLAY_putc(1, s[1]);
		DISPLAY_putc(0, s[0]);
	}
}

void visualize_int(int number) {
	for(int i = 0; i < 4; i++)
		DISPLAY_dp(i, 0);
	char s[5];
	sprintf(s, "%4d", number);
	DISPLAY_puts(0, s);
}


void setup(void) {
	srand(time(NULL));
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
	GPIO_config_EXTI(GPIOB, EXTI10);
	GPIO_config_EXTI(GPIOB, EXTI4);
	EXTI_enable(EXTI10, FALLING_EDGE);
	EXTI_enable(EXTI4, FALLING_EDGE);

	//Timer Setup
	TIM_init(TIM2);
	TIM_config_timebase(TIM2, 8400, 100); //100Hz
	TIM_enable_irq(TIM2, IRQ_UPDATE);

	//ADC Setup
	ADC_init(ADC1, ADC_RES_12, ADC_ALIGN_RIGHT);
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
	ADC_sample_channel(ADC1, 10);
	ADC_start(ADC1);
	while (!ADC_completed(ADC1)) {}
	adc_10 = ADC_read(ADC1);
	//ADC1, 11
	ADC_sample_channel(ADC1, 11);
	ADC_start(ADC1);
	while (!ADC_completed(ADC1)) {}
	adc_11 = ADC_read(ADC1);

	//LOOP
	if(state == RUN)
		visualize_int((int)RottaEffettiva);
	else if(state == SETUP) {
		if(visualize_adc_enable) {
			if(adc_11 < 4)
				adc_11 = 0;
			else if(adc_11 > 4091)
				adc_11 = 4095;
			RottaDesiderata = ((float)adc_11/4095)*360 - 180;
			visualize_int((int)RottaDesiderata);
			visualize_adc_enable = 0;
		}
	}

	TMAX = ((float)adc_10/4095)*2;
	int r = rand() % (2*(int)(TMAX*100)+1);
	r -= (int)(TMAX*100);
	Turbolenza = ((float)r) / 100;

	//UART OUTPUT
	if(print_enable){
		printf("Effettiva: %0.2f, Desiderata: %0.2f, TMAX: %0.2f, Turbolenza: %0.2f\n", RottaEffettiva, RottaDesiderata, TMAX, Turbolenza);
		print_enable = 0;
	}
}

int main(void) {
	setup();

	for (;;)
		loop();

	return 0;
}