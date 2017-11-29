/*
 * Welder.c
 *
 * Created: 29.11.2017 16:53:22
 *  Author: Wraith
 */ 

//макросы для автоматического определения параметров
#define __GET_DDR(DDR_LETTER) DDR ## DDR_LETTER
#define GET_DDR(DDR_LETTER) __GET_DDR(DDR_LETTER)

#define __GET_PORT(PORT_LETTER) PORT ## PORT_LETTER
#define GET_PORT(PORT_LETTER) __GET_PORT(PORT_LETTER)

#define __GET_PORT_DATA(PORT_LETTER, PORT_PIN) ((PIN ## PORT_LETTER)&(1<<(PIN ## PORT_LETTER ## PORT_PIN)))
#define GET_PORT_DATA(PORT_LETTER, PORT_PIN) __GET_PORT_DATA(PORT_LETTER, PORT_PIN)


#define F_CPU 8000000UL
#define NO_DELAY 1


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "Encoder.h"


volatile uint16_t RelayCounter = 0;//16 бит для 2х регистров (16 реле) сразу
volatile uint8_t TimerFlag = 0;//флаг прерывания таймера
volatile uint16_t EncCounter = 0;
volatile uint8_t EncCurState=0;//содержится состояние энкодера, нужно хранить для определения направления вращения
volatile uint8_t EncPushDown=0;//для работы с кнопкой


const uint8_t SevenSegChar[] PROGMEM = 
{
	0b10110111, //биты в соотвествии с разводкой платы (нестандартно)
	0b10000010,
	0b00111110,
	0b10011110,
	0b10001011,
	0b10011101,
	0b10111101,
	0b10000110,
	0b10111111,
	0b10011111
};


ISR(INT0_vect)
{
	//прерывание INT0 - переход фазы через 0
	//
	//сбрасываем значение счетчика
	TCNT2 = 0;
	TimerFlag = 1;
}

void InitTimer2 (void)
{
	//На частоте тактов 8 МГц
	//предделитель на 1024: 1 тик - 0,000128 сек, для 20мс нужно ~156 тиков
	TCCR2 |= (1<<CS22)|(1<<CS21)|(1<<CS20);
	OCR2 = 35;//156
	//разрешаем прерыванеи по совпадению
	TIMSK |= (1<<OCIE2);
	TCNT2 = 0;
}

int8_t	EncPollDelta()
{
	
	EncCurState <<= 2;
	EncCurState &= 0b00001100;
	EncCurState += GET_ENCODER_PORT_DATA(ENCPOLL_A_PORT, ENCPOLL_A_PIN)<<1|GET_ENCODER_PORT_DATA(ENCPOLL_B_PORT, ENCPOLL_B_PIN);
	
	return pgm_read_byte(&(EncState[EncCurState]));
}

static uint8_t TurnBitsAround( uint8_t aByte )//инвертируем биты в байте и меням соседние биты местами 
												//- из-за разводки
{
return (aByte & 0x80 ? 0x02 : 0) |
(aByte & 0x40 ? 0x01 : 0) |
(aByte & 0x20 ? 0x08 : 0) |
(aByte & 0x10 ? 0x04 : 0) |
(aByte & 0x08 ? 0x20 : 0) |
(aByte & 0x04 ? 0x10 : 0) |
(aByte & 0x02 ? 0x80 : 0) |
(aByte & 0x01 ? 0x40 : 0);
}

int main(void)
{
	cli();
	volatile uint8_t temp=0;
	int8_t EncStep =0; //счетчик кол-ва шагов для изменения цифры на индикаторе, сейчас 2
	uint8_t i;
	//реле управляются 0 на выходе микросхемы
	//поэтому порты на выход и сразу 1
	//0-7 реле
	PORTB=0xFF;
	DDRB=0xFF;
	
	//8-15 реле
	PORTC=0xFF;
	DDRC=0xFF;
	
	
	//остальные порты на вход c подтяжкой

	//Пин PB0 PB1 выходы - для управления индикатором
	// PB2 PB3 PB4 - входы с подтяжной для работы с энкодером
 	DDRB=0x3;//0x00000011
	PORTB=0xFF;
	
	//порт для работы с индикатором, на выход, значение 0-гасим
	DDRD=0xFF;
	PORTD=0;
	
	PORTB=(PORTB&(~2))|1; //проверка индикатора - выключаем 2 бит, включаем 1
	for (i = 0; i < 10; i++) {
		#if !NO_DELAY
			_delay_ms(150);
		#endif
		PORTD = pgm_read_byte(&(SevenSegChar[i]));
	}
	
		#if !NO_DELAY
			_delay_ms(150);
		#endif

	PORTD = 0;
	PORTB=(PORTB&(~1))|2; //проверка индикатора, выключаем 1 бит, включам 2
	for (i = 0; i < 10; i++) {
		#if !NO_DELAY
			_delay_ms(150);
		#endif
		PORTD = pgm_read_byte(&(SevenSegChar[i]));
	}

		#if !NO_DELAY
			_delay_ms(150);
		#endif

	//настраиваем прерывание раз в 20мс для работы с энкодером
	//дополнительно, будем его использовать для мигания цифрами 2-3 раза за 1-2 сек для отображения смены канала
	//что бы не мгновенно
	
	TimerFlag=0;
	InitTimer2();
	temp=0;
	PORTB&=~1;//устанавливаем 2 бит
	
	sei();

    while(1)
    {

		//было прерывание таймера
		if (TimerFlag) {
			TimerFlag=0;
			//определяем положение энкодера
			EncStep += EncPollDelta(EncCurState);
			if (EncStep>3) {
				EncStep=0;	
				EncCounter++;
			}
			if (EncStep<-3) {
				EncStep=0;
				EncCounter--;
			}
			
			if (EncCounter > 200) EncCounter = 16;
			if (EncCounter > 16) EncCounter = 0;
			
			//убираем потенциальную ошибку когда оба сегмента погашены (чего быть не должно)
			//if ((PINB&3)<1) PORTB|=2;
			
			//выставляем значение на индикаторе
			if (GET_PORT_DATA(B,0)) {
				temp = EncCounter%10;
			}else{
				temp = EncCounter/10;
			}
			PORTD = pgm_read_byte(&(SevenSegChar[temp]));
			
			//если вдруг оба бита сняты (чего быть вообще-то не должно)
			if (!(PORTB&3)) PORTB|=1;
			//инвертируем Пин PB0 и PB1
			PORTB^=3;

			//определяем нажат ли энкодер
			if (GET_PORT_DATA(B,4)) {
					//нажат,
					//выставляем флаг - инвертирование по отпусканию
					EncPushDown=1;
				}else{
					//не нажат
					if (EncPushDown) {
						//был нажат до этого
						//инвертируем нажатый бит в регистре состояния реле RelayCounter
						EncPushDown=0;
						if (!(EncCounter)) {
							//если EncCounter - выключаем все реле
							RelayCounter=0;
						} else {
							RelayCounter ^= (1<<(EncCounter-1));	
						}
						//управляем реле - инвертированный сигнал!!!
						PORTC = ~RelayCounter;
						PORTA = ~(TurnBitsAround(RelayCounter>>8));
					}
				}
		}
		
		
    }
}