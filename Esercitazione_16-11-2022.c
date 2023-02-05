//SENSORI PRESENZA CON LUCI
#include "stm32_unict_lib.h"
#include <stdio.h>
#include <string.h>


//Utility Variables
enum {
	RUN,
	CONFIG
};
unsigned int adc_10, adc_11;
unsigned int state = RUN;
unsigned int print_enable = 0; unsigned int print_timer = 0;

//Other Variables
int Tl1, Tl2;
int Ti = 10;
int Ts = 10;
int Tc = 10;

unsigned int Li_enable = 0; unsigned int Li_timer = 0; unsigned int Li_timer_2 = 0;
unsigned int Ls_enable = 0; unsigned int Ls_timer = 0; unsigned int Ls_timer_2 = 0;
unsigned int Lc_enable = 0; unsigned int Lc_timer = 0; unsigned int Lc_timer_2 = 0;

char s[5];


//TIMER & IRQ Function
void EXTI4_IRQHandler(void) {
	if (EXTI_isset(EXTI4)) {

		if(state == RUN) {
			Ls_enable = 1;
			Ls_timer_2 = 0;
		}

		EXTI_clear(EXTI4);
	}
}

void EXTI9_5_IRQHandler(void) {
	if (EXTI_isset(EXTI5)) {

		if(state == RUN) {
			Lc_enable = 1;
			Lc_timer_2 = 0;
		}

		EXTI_clear(EXTI5);
	}
	if (EXTI_isset(EXTI6)) {

		if(state == RUN)
			state = CONFIG;

		EXTI_clear(EXTI6);
	}
}

void EXTI15_10_IRQHandler(void) {
	if (EXTI_isset(EXTI10)) {

		if(state == RUN) {
			Li_enable = 1;
			Li_timer_2 = 0;
		}
		EXTI_clear(EXTI10);
	}
}

void TIM2_IRQHandler(void) {
	if (TIM_update_check(TIM2)) {

		if(print_timer == 1000) {
			print_enable = 1;
			print_timer = 0;
		}

		if( (Li_enable && (Li_timer >= Tl1)) || (Li_enable && (Ti*1000 - Li_timer_2) <= 4000 && (Li_timer >= Tl2)) ) {
			GPIO_toggle(GPIOB, 0);
			Li_timer = 0;
		}
		if(Li_timer_2 == Ti*1000) {
			GPIO_write(GPIOB, 0, 0);
			Li_enable = 0;
			Li_timer = 0;
			Li_timer_2 = 0;
		}

		if( (Ls_enable && (Ls_timer >= Tl1)) || (Ls_enable && (Ts*1000 - Ls_timer_2) <= 4000 && (Ls_timer >= Tl2)) ) {
			GPIO_toggle(GPIOC, 2);
			Ls_timer = 0;
		}
		if(Ls_timer_2 == Ts*1000) {
			GPIO_write(GPIOC, 2, 0);
			Ls_enable = 0;
			Ls_timer = 0;
			Ls_timer_2 = 0;
		}

		if( (Lc_enable && (Lc_timer >= Tl1)) || (Lc_enable && (Tc*1000 - Lc_timer_2) <= 4000 && (Lc_timer >= Tl2)) ) {
			GPIO_toggle(GPIOC, 3);
			Lc_timer = 0;
		}
		if(Lc_timer_2 == Tc*1000) {
			GPIO_write(GPIOC, 3, 0);
			Lc_enable = 0;
			Lc_timer = 0;
			Lc_timer_2 = 0;
		}


		if(Li_enable) {
			Li_timer++;
			Li_timer_2++;
		}
		if(Ls_enable) {
			Ls_timer++;
			Ls_timer_2++;
		}
		if(Lc_enable) {
			Lc_timer++;
			Lc_timer_2++;
		}
		print_timer++;
		TIM_update_clear(TIM2);
	}
}

void getstring(char * s, int maxlen) {
	int i = 0;
	for (;;) {
		char c = readchar();
		if (c == 13) {
			printf("\n");
			s[i] = 0;
			return;
		}
		else if (c == 8) {
			if (i > 0) {
				--i;
				__io_putchar(8); // BS
				__io_putchar(' '); // SPAZIO
				__io_putchar(8); // BS
			}
		}
		else if (c >= 32) { // il carattere appartiene al set stampabile
			if (i < maxlen) {
				__io_putchar(c); // echo del carattere appena inserito
				// inserisci il carattere nella stringa
				s[i] = c;
				i++;
			}
		}
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
	TIM_config_timebase(TIM2, 8400, 10); //1000Hz
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

	s[4] = '\0';

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
	Tl1 = (int)(((float)adc_11/1023)*200);
	Tl1 += 300;

	Tl2 = (int)(((float)adc_10/1023)*100);
	Tl2 += 100;

	if(Li_enable)
		s[0] = 'I';
	else
		s[0] = ' ';
	if(Ls_enable)
		s[1] = 'S';
	else
		s[1] = ' ';
	if(Lc_enable)
		s[2] = 'C';
	else
		s[2] = ' ';
	DISPLAY_puts(0, s);

	if(state == CONFIG) {
		for(int j = 0; j < 3; j++) {
			char s1[3];
			do {
				printf("Inserisci una intero compreso fra 5 e 20:");
				fflush(stdout);
				getstring(s1, 2);
			} while(atoi(s1) < 5 || atoi(s1) > 20);

			printf("Numero Inserito: %d\n", atoi(s1));

			if(j == 0) {
				Ti = atoi(s1);
			}
			else if(j == 1) {
				Ts = atoi(s1);
			}
			else
				Tc = atoi(s1);

			printf("\n");
		}

		state = RUN;
	}

	//UART OUTPUT
	if(print_enable){
		printf("%d %d %d %d\n", adc_10, adc_11, Tl1, Tl2);

		print_enable = 0;
	}
}

int main(void) {
	setup();

	for (;;)
		loop();

	return 0;
}
