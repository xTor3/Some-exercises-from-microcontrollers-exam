//TIMER
#include "stm32_unict_lib.h"
#include <stdio.h>


//Utility Variables
enum {
	IDLE,
	SET_UP,
	START,
	STOP,
	FINE_TIMER
};
unsigned int state = IDLE;
unsigned int print_enable = 0; unsigned int print_timer = 0;
char s[5];

//Other Variables
double secondi_timer = 5;
double last_secondi_timer = 5;
int timer_led = 0;


//TIMER & IRQ Function
void EXTI4_IRQHandler(void) {
	if (EXTI_isset(EXTI4)) {
		if(state == IDLE || state == STOP) {
			state = START;
		}
		else if(state == START) {
			state = STOP;
		}
		EXTI_clear(EXTI4);
	}
}

void EXTI9_5_IRQHandler(void) {
	if (EXTI_isset(EXTI5)) {
		if(state == SET_UP) {
			secondi_timer--;
		}
		else {
			state = IDLE;
			secondi_timer = last_secondi_timer;
			GPIO_write(GPIOC, 3, 0);
		}
		EXTI_clear(EXTI5);
	}
	if (EXTI_isset(EXTI6)) {
		if(state == SET_UP) {
			secondi_timer++;
		}
		EXTI_clear(EXTI6);
	}
}

void EXTI15_10_IRQHandler(void) {
	if (EXTI_isset(EXTI10)) {
		if(state == IDLE) {
			state = SET_UP;
		}
		else if(state == FINE_TIMER) {
			state = SET_UP;
			secondi_timer = last_secondi_timer;
			GPIO_write(GPIOC, 3, 0);
		}
		else if(state == SET_UP) {
			last_secondi_timer = secondi_timer;
			state = IDLE;
		}
		EXTI_clear(EXTI10);
	}
}

void TIM2_IRQHandler(void) {
	if (TIM_update_check(TIM2)) {
		if(state == START) {
			secondi_timer -= ((double)0.01);
			timer_led++;
			if(timer_led == 50) {
				GPIO_toggle(GPIOB, 0);
				timer_led = 0;
			}
		}
		else {
			GPIO_write(GPIOB, 0, 0);
		}

		if(print_timer == 100) {
			print_enable = 1;
			print_timer = 0;
		}
		print_timer++;
		TIM_update_clear(TIM2);
	}
}

void TIM3_IRQHandler(void) {
	if (TIM_update_check(TIM3)) {

		TIM_update_clear(TIM3);
	}
}

void TIM4_IRQHandler(void) {
	if (TIM_update_check(TIM4)) {

		TIM_update_clear(TIM4);
	}
}


void setup(void) {
	//Display Setup
	DISPLAY_init();

	//GPIO Setup
	GPIO_init(GPIOB);
	GPIO_init(GPIOC);
	GPIO_config_output(GPIOB, 0); //Led Rosso
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

	//Serial Setup
	CONSOLE_init();

	//Start Timer
	TIM_set(TIM2, 0);
	TIM_on(TIM2);
}

void loop(void) {
	//LOOP
	if(state == IDLE || state == SET_UP) {
		sprintf(s, "%2d", (int)secondi_timer);
		if((int)secondi_timer < 10)
			s[0] = '0';
		s[2] = '0';
		s[3] = '0';
		DISPLAY_puts(0, s);
	}
	else if(state == START) {
		if(secondi_timer <= 0.01) {
			state = FINE_TIMER;
		}
		sprintf(s, "%4d", (int)(secondi_timer*100));
		if(secondi_timer < 0.1) {
			s[0] = '0';
			s[1] = '0';
			s[2] = '0';
		}
		else if(secondi_timer < 1) {
			s[0] = '0';
			s[1] = '0';
		}
		else if(secondi_timer < 10)
			s[0] = '0';
		DISPLAY_puts(0, s);
	}
	else if(state == FINE_TIMER) {
		GPIO_write(GPIOC, 3, 1);
	}

	//UART OUTPUT
	if(print_enable){
		printf("%lf %d\n", secondi_timer, state);
		print_enable = 0;
	}
}

int main(void) {
	setup();

	for (;;)
		loop();

	return 0;
}