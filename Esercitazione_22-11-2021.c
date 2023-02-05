//ASCENSORE
#include "stm32_unict_lib.h"
#include <stdio.h>
#include <string.h>


//Utility Variables
enum {
	IDLE,
	CHIUSURA_PORTE,
	SPOSTAMENTO,
	APERTURA_PORTE,
	CONFIGURAZIONE
};
unsigned int adc_10, adc_11;
unsigned int state = IDLE;
unsigned int print_enable = 0; unsigned int print_timer = 0;
char s[5];

//Other Variables
#define CHIUSURA_PORTE_DELAY 1.5
#define TOGGLING_PERIOD 200
float speed = 1;
int piano_attuale = 1;
int piano_da_raggiungere = -1;
int prossimo_piano = -1;

int led_timer = 0;
int chiusura_porte_timer = 0;
int spostamento_timer = 0;
int discesa = 0;
int last_state = 0;


//Other Functions:
void visualize_float(float number) {
	//constrain number < 100.00
	if(number >= 100)
		number = 99.99;
	char s[5];
	sprintf(s, "%4d", (int)(number*100));
	DISPLAY_dp(1, 1);
	DISPLAY_puts(0, s);
}

//TIMER & IRQ Function
void EXTI4_IRQHandler(void) {
	if (EXTI_isset(EXTI4)) {
		if(state == IDLE && piano_attuale != 2) {
			piano_da_raggiungere = 2;
			state = CHIUSURA_PORTE;
		}
		else if(prossimo_piano == -1 && piano_da_raggiungere != 2 && state != IDLE) {
			prossimo_piano = 2;
		}
		EXTI_clear(EXTI4);
	}
}

void EXTI9_5_IRQHandler(void) {
	if (EXTI_isset(EXTI5)) {
		if(state == IDLE && piano_attuale != 3) {
			piano_da_raggiungere = 3;
			state = CHIUSURA_PORTE;
		}
		else if(prossimo_piano == -1 && piano_da_raggiungere != 3 && state != IDLE) {
			prossimo_piano = 3;
		}
		EXTI_clear(EXTI5);
	}
	if (EXTI_isset(EXTI6)) {
		if(state != CONFIGURAZIONE) {
			last_state = state;
			state = CONFIGURAZIONE;
		}
		else {
			state = last_state;
			DISPLAY_dp(1, 0);
		}
		EXTI_clear(EXTI6);
	}
}

void EXTI15_10_IRQHandler(void) {
	if (EXTI_isset(EXTI10)) {
		if(state == IDLE && piano_attuale != 1) {
			piano_da_raggiungere = 1;
			state = CHIUSURA_PORTE;
		}
		else if(prossimo_piano == -1 && piano_da_raggiungere != 1 && state != IDLE) {
			prossimo_piano = 1;
		}
		EXTI_clear(EXTI10);
	}
}

void TIM2_IRQHandler(void) {
	if (TIM_update_check(TIM2)) {

		if(state == CHIUSURA_PORTE || state == APERTURA_PORTE) {
			if(chiusura_porte_timer == CHIUSURA_PORTE_DELAY*1000) {
				if(state == APERTURA_PORTE) {
					if(piano_attuale != piano_da_raggiungere)
						state = CHIUSURA_PORTE;
					else
						state = IDLE;
				}
				else {
					state = SPOSTAMENTO;
				}
				GPIO_write(GPIOB, 0, 0);
				led_timer= 0;
				chiusura_porte_timer = 0;
			}
			if(led_timer == 200) {
				GPIO_toggle(GPIOB, 0);
				led_timer = 0;
			}
			chiusura_porte_timer++;
			led_timer++;
		}

		else if(state == SPOSTAMENTO) {
			if(led_timer == 200) {
				GPIO_toggle(GPIOC, 2);
				led_timer = 0;
			}
			led_timer++;

			if(piano_da_raggiungere < piano_attuale) {
				discesa = 1;
				if(spostamento_timer >= 1000*speed) {
					piano_attuale--;
					spostamento_timer = 0;
				}
			}
			else if(piano_da_raggiungere > piano_attuale) {
				discesa = 0;
				if(spostamento_timer >= 1000*speed) {
					piano_attuale++;
					spostamento_timer = 0;
				}
			}
			else {
				if(prossimo_piano != -1) {
					piano_da_raggiungere = prossimo_piano;
					prossimo_piano = -1;
				}
				state = APERTURA_PORTE;
				GPIO_write(GPIOC, 2, 0);
				spostamento_timer = 0;
				led_timer = 0;
			}
			spostamento_timer++;
		}

		//UART Timer
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
	TIM_config_timebase(TIM2, 8400, 10); //100Hz, (8400, 10): 1000Hz
	TIM_enable_irq(TIM2, IRQ_UPDATE);

	//ADC Setup
	ADC_init(ADC1, ADC_RES_10, ADC_ALIGN_RIGHT);
	ADC_channel_config(ADC1, GPIOC, 0, 10);
	ADC_on(ADC1);
	ADC_sample_channel(ADC1, 10);

	//Serial Setup
	CONSOLE_init();

	//Start Timer
	TIM_set(TIM2, 0);
	TIM_on(TIM2);
}

void loop(void) {
	//ADC1, 10
	ADC_start(ADC1);
	while (!ADC_completed(ADC1)) {}
	adc_10 = ADC_read(ADC1);

	//LOOP
	if(state == IDLE) {
		sprintf(s,"%1d%s", piano_attuale, "   ");
		DISPLAY_puts(0, s);
	}
	else if(state == CHIUSURA_PORTE) {
		strcpy(s, "CLOS");
		DISPLAY_puts(0, s);
	}
	else if(state == APERTURA_PORTE) {
		strcpy(s, "OPEN");
		DISPLAY_puts(0, s);
	}
	else if(state == SPOSTAMENTO) {
		if(discesa) {
			sprintf(s,"%1d%s", piano_attuale-1, "-  ");
			DISPLAY_puts(0, s);
		}
		else {
			sprintf(s,"%1d%s", piano_attuale, "-  ");
			DISPLAY_puts(0, s);
		}
	}
	else if(state == CONFIGURAZIONE) {
		//granularità di 0.1 -> 6 valori fra 0.4 e 1 - > 1023/6 = 170,5
		int contat = 0;
		if(adc_10 < 5)
			adc_10 = 0;
		while(170.5*contat < adc_10)
			contat++;
		speed = 0.4 + 0.1*contat;
		visualize_float(speed);
	}

	//UART OUTPUT
	if(print_enable){
		printf("%d %d\n", prossimo_piano, piano_da_raggiungere);
		print_enable = 0;
	}
}

int main(void) {
	setup();

	for (;;)
		loop();

	return 0;
}