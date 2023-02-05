//GESTORE CODE
#include "stm32_unict_lib.h"
#include <stdio.h>
#include <string.h>


//Utility Variables
enum {
	RUN,
	TICKET,
	CALL
};
char s[5];
unsigned int state = RUN;
unsigned int print_enable = 0; unsigned int print_timer = 0;

//Other Variables
unsigned int tickets = 0;
int first = 1;
unsigned int timer_ticket = 0;
int on_off = 1;
int contatore = 0;
unsigned int actual_ticket = 0;

int timer_2 = 0;
unsigned int tempo_medio = 0;
unsigned int tempo_medio_ms = 0;
unsigned int tempo_medio_timer = 0;
unsigned int aux;


int visualize_number(int num) {
	if(timer_ticket == 50 || first) {
		contatore++;
		if(on_off) {
			strcpy(s, "    ");
			DISPLAY_puts(0, s);
			if(num < 10)
				sprintf(s, "%1d", num);
			else
				sprintf(s, "%2d", num);
			DISPLAY_puts(0, s);
			first = 0;
		}
		else {
			strcpy(s, "    ");
			DISPLAY_puts(0, s);
		}
		on_off = !on_off;
		timer_ticket = 0;
		if(contatore == 5) {
			timer_ticket = 0;
			contatore = 0;
			on_off = 1;
			first = 1;
			GPIO_write(GPIOB, 0, 0);
			state = RUN;
		}
	}
}

//TIMER & IRQ Function
void EXTI4_IRQHandler(void) {
	if (EXTI_isset(EXTI4)) {
		if(state == RUN && actual_ticket < tickets) {
			state = CALL;
			actual_ticket++;
			GPIO_write(GPIOB, 0, 1);
		}
		EXTI_clear(EXTI4);
	}
}

void EXTI15_10_IRQHandler(void) {
	if (EXTI_isset(EXTI10)) {
		if(tickets < 99) {
			state = TICKET;
			tickets++;
			GPIO_write(GPIOB, 0, 1);
		}
		EXTI_clear(EXTI10);
	}
}

void TIM2_IRQHandler(void) {
	if (TIM_update_check(TIM2)) {
		if(state == TICKET || state == CALL)
			timer_ticket++;

		if(tickets > actual_ticket && actual_ticket != 0) {
			timer_2++;
			tempo_medio_timer++;
			if(timer_2 == 50) {
				tempo_medio_ms += tempo_medio_timer;
				tempo_medio = (int)((float)tempo_medio_ms/(100*actual_ticket));
				tempo_medio_timer = 0;
				timer_2 = 0;
			}
		}
		else {
			tempo_medio = 0;
		}

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
	GPIO_config_output(GPIOB, 0); //Led Rosso

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

	//Serial Setup
	CONSOLE_init();

	//Start Timer
	TIM_set(TIM2, 0);
	TIM_on(TIM2);
}

void loop(void) {

	//UART OUTPUT
	if(print_enable){
		printf("%d %d\n", 1, 2);
		print_enable = 0;
	}

	if(state == RUN) {
		if(!actual_ticket)
			strcpy(s, "NO");
		else
			sprintf(s, "%2d", actual_ticket);
		if(tempo_medio > 99)
			tempo_medio = 99;
		sprintf(&s[2], "%2d", tempo_medio);
		DISPLAY_puts(0, s);
	}
	else if(state == TICKET)
		visualize_number(tickets);
	else if(state == CALL)
		visualize_number(actual_ticket);
}

int main(void) {
	setup();

	for (;;)
		loop();

	return 0;
}