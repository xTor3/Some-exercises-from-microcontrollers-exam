#EROGATORE BEVANDE CALDE
#include "stm32_unict_lib.h"
#include <stdio.h>


//UTILITY
enum {
	SCELTA,
	MONETE,
	EROGAZIONE,
	RESTO
};
unsigned int adc_10, adc_11;
unsigned int state = SCELTA;

float prezzi[3] = {0.35, 0.4, 0.45};
#define X 0.5f
#define Y 0.2f
#define Z 0.1f

int adc_10_count = 0;

int print_enable = 0;
int print_timer = 0;

int bevanda;
char s[5];
float monete_inserite = 0;
int timer_erogazione = 0;
int timer_led_rosso = 0;
int led_rosso_enable = 0;
float resto = 0;

int monete_presenti[4] = {10, 10, 10, 0};
float tagli_moneta[4] = {0.05, 0.1, 0.2, 0.5};
int monete_restituite[4] = {0, 0, 0, 0};
int moneta_visualizzare = 5;
int moneta_visualizzata = 0;
int timer_monete = 0;


void EXTI4_IRQHandler(void) {
	if (EXTI_isset(EXTI4)) {
		if(state == SCELTA)
			state = MONETE;

		if(state == MONETE) {
			monete_presenti[2]++;
			monete_inserite += Y;
			led_rosso_enable = 1;
			GPIO_write(GPIOB, 0, 1);
		}

		EXTI_clear(EXTI4);
	}
}

void EXTI9_5_IRQHandler(void) {
	if (EXTI_isset(EXTI5)) {
		if(state == SCELTA)
			state = MONETE;

		if(state == MONETE) {
			monete_presenti[1]++;
			monete_inserite += Z;
			led_rosso_enable = 1;
			GPIO_write(GPIOB, 0, 1);
		}
		EXTI_clear(EXTI5);
	}
	if (EXTI_isset(EXTI6)) {
		if(state == MONETE && monete_inserite >= prezzi[bevanda-1] ) {
			state = EROGAZIONE;
			GPIO_write(GPIOC, 3, 1);
		}

		EXTI_clear(EXTI6);
	}
}

void EXTI15_10_IRQHandler(void) {
	if (EXTI_isset(EXTI10)) {
		if(state == SCELTA)
			state = MONETE;

		if(state == MONETE) {
			monete_presenti[3]++;
			monete_inserite += X;
			led_rosso_enable = 1;
			GPIO_write(GPIOB, 0, 1);
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
		if(timer_led_rosso == 50) {
			led_rosso_enable = 0;
			timer_led_rosso = 0;
			GPIO_write(GPIOB, 0, 0);
		}
		if(timer_erogazione == 100) {
			state = RESTO;
			GPIO_write(GPIOC, 3, 0);
			timer_erogazione = 0;
		}
		if(state == RESTO && !moneta_visualizzata) {
			if(timer_monete == 100) {
				moneta_visualizzata = 1;
			}
		}

		if(led_rosso_enable)
			timer_led_rosso++;
		if(!moneta_visualizzata && state == RESTO)
			timer_monete++;
		if(state == EROGAZIONE)
			timer_erogazione++;
		print_timer++;
		TIM_update_clear(TIM2);
	}
}

void print_monete(void) {
	for(int i = 0; i < 4; i++) {
		while(monete_restituite[i] != 0) {
			moneta_visualizzata = 0;
			printf("%d\n", monete_restituite[i]);
			moneta_visualizzare = i;
			sprintf(s,"%d", (int)(tagli_moneta[i]*100));
			DISPLAY_putc(0, '0');
			DISPLAY_putc(1, '0');
			if(!i) {
				DISPLAY_putc(2, '0');
				DISPLAY_putc(3, '5');
			}
			else
				DISPLAY_puts(2, s);
			timer_monete = 0;
			while(!moneta_visualizzata) { }
			monete_restituite[i]--;
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
	TIM_config_timebase(TIM2, 8400, 100); //100Hz - 10ms
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

	if(state == SCELTA) {
		adc_10_count = 0;
		while(adc_10 > adc_10_count*341.33)
			adc_10_count++;
		if(adc_10_count == 0)
			adc_10_count = 1;
		switch(adc_10_count) {
			case 1:
				bevanda = 1;
				DISPLAY_puts(0, "CA");
				break;
			case 2:
				bevanda = 2;
				DISPLAY_puts(0, "TE");
				break;
			case 3:
				bevanda = 3;
				DISPLAY_puts(0, "CI");
				break;
		}
	}
	else if(state == MONETE) {
		sprintf(s, "%d", (int)(monete_inserite*100));
		/*
		DISPLAY_putc(0, '0');
		for(int i = 1; i < 4; i++)
			DISPLAY_putc(i, s[i-1]);
		*/
		DISPLAY_dp(1, 1);
		if(monete_inserite < 1) {
			DISPLAY_putc(0, '0');
			DISPLAY_putc(1, '0');
			DISPLAY_puts(2, s);
		}
		else{
			DISPLAY_putc(0, '0');
			DISPLAY_puts(1, s);
		}
	}
	else if(state == RESTO) {
		resto = monete_inserite - prezzi[bevanda-1];

		for(int i = 3; i >= 0; i--) {
			if(monete_presenti[i] != 0 && resto > 0) {
				while((resto-tagli_moneta[i]) >= -0.01) {
					monete_presenti[i]--;
					resto -= tagli_moneta[i];
					monete_restituite[i]++;
				}
			}
		}

		for(int i = 0; i < 4; i++) {
			printf("%d ", monete_restituite[i]);
		}
		printf("\n");
		for(int i = 0; i < 4; i++) {
			printf("%d ", monete_presenti[i]);
		}
		printf("\n");

		print_monete();
		monete_inserite = 0;

		for(int j = 0; j < 4; j++) {
			DISPLAY_putc(j, ' ');
		}
		DISPLAY_dp(1,0);
		state = SCELTA;
	}

	if(print_enable) {
		printf("%d %0.2f\n", adc_10_count, monete_inserite);
		print_enable = 0;
	}
}

int main(void) {
	setup();
	for (;;)
		loop();

	return 0;
}