//CANCELLO AUTOMATICO
#include "stm32_unict_lib.h"
#include <stdio.h>
#include <string.h>


//Utility Variables
enum {
	CHIUSO,
	APERTURA,
	APERTO,
	CHIUSURA
};
unsigned int state = CHIUSO;
unsigned int print_enable = 0; unsigned int print_timer = 0;

//Other Variables
#define Tc 4
#define Ta 2
unsigned int timer_cancello = 0;
unsigned int timer_led = 0;
char s[5];


//TIMER & IRQ Function
void EXTI4_IRQHandler(void) {
	if (EXTI_isset(EXTI4)) {
		if(state == APERTO) {
			state = CHIUSURA;
		}
		EXTI_clear(EXTI4);
	}
}

void EXTI9_5_IRQHandler(void) {
	if (EXTI_isset(EXTI5)) {
		if(state == APERTO) {
			timer_cancello = 0;
		}
		else if(state == CHIUSURA) {
			timer_cancello = Tc*1000-timer_cancello;
			state = APERTURA;
		}
		EXTI_clear(EXTI5);
	}
}

void EXTI15_10_IRQHandler(void) {
	if (EXTI_isset(EXTI10)) {
		if(state == CHIUSO) {
			state = APERTURA;
		}
		else if(state == CHIUSURA) {
			timer_cancello = Tc*1000-timer_cancello;
			state = APERTURA;
		}
		EXTI_clear(EXTI10);
	}
}

void TIM2_IRQHandler(void) {
	if (TIM_update_check(TIM2)) {
		if(state == APERTURA) {
			if(timer_cancello == Tc*1000) {
				state = APERTO;
				timer_cancello = 0;
			}
		}
		else if(state == APERTO) {
			if(timer_cancello == Ta*1000) {
				state = CHIUSURA;
				timer_cancello = 0;
			}
		}
		else if(state == CHIUSURA) {
			if(timer_cancello == Tc*1000) {
				state = CHIUSO;
				timer_cancello = 0;
			}
		}
		if(state != CHIUSO)
			timer_cancello++;


		if(state != CHIUSO && state != APERTO) {
			timer_led++;
			if(timer_led == 500) {
				GPIO_toggle(GPIOB, 0);
				timer_led = 0;
			}
		}
		else {
			timer_led = 0;
			GPIO_write(GPIOB, 0, 0);
		}
		if(print_timer == 1000) {
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
	s[4] = '\0';

	//GPIO Setup
	GPIO_init(GPIOB);
	GPIO_config_output(GPIOB, 0); //Led Rosso
	GPIO_config_input(GPIOB, 10); //Pulsante X
	GPIO_config_input(GPIOB, 4); //Pulsante Y
	GPIO_config_input(GPIOB, 5); //Pulsante Z
	GPIO_config_EXTI(GPIOB, EXTI10);
	GPIO_config_EXTI(GPIOB, EXTI4);
	GPIO_config_EXTI(GPIOB, EXTI5);
	EXTI_enable(EXTI10, FALLING_EDGE);
	EXTI_enable(EXTI4, FALLING_EDGE);
	EXTI_enable(EXTI5, FALLING_EDGE);

	//Timer Setup
	TIM_init(TIM2);
	TIM_config_timebase(TIM2, 8400, 10); //1000Hz
	TIM_enable_irq(TIM2, IRQ_UPDATE);

	//Serial Setup
	CONSOLE_init();

	//Start Timer
	TIM_set(TIM2, 0);
	TIM_on(TIM2);
}

void loop(void) {
	if(state == CHIUSO)
		strcpy(s, "----");
	if(state == APERTURA) {
		if(timer_cancello >= 0.75*(Tc*1000))
			strcpy(s, "-   ");
		else if(timer_cancello >= 0.5*(Tc*1000))
				strcpy(s, "--  ");
		else if(timer_cancello >= 0.25*(Tc*1000))
			strcpy(s, "--- ");
	}
	else if(state == CHIUSURA) {
		if(timer_cancello >= 0.75*(Tc*1000))
			strcpy(s, "--- ");
		else if(timer_cancello >= 0.5*(Tc*1000))
				strcpy(s, "--  ");
		else if(timer_cancello >= 0.25*(Tc*1000))
			strcpy(s, "-   ");
	}
	if(state == APERTO)
		strcpy(s, "    ");


	DISPLAY_puts(0, s);

	//UART OUTPUT
	if(print_enable){
		printf("%d\n", timer_cancello);
		print_enable = 0;
	}
}

int main(void) {
	setup();

	for (;;)
		loop();

	return 0;
}