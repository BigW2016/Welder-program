/*
 * Welder.c
 *
 * Created: 29.11.2017 16:53:22
 *  Author: Wraith
 */ 

//макросы для автоматического определения параметров

#define __GET_PORT_DATA(PORT_LETTER, PORT_PIN) (((PIN ## PORT_LETTER)&(1<<(P ## PORT_LETTER ## PORT_PIN)))>>(P ## PORT_LETTER ## PORT_PIN))
#define GET_PORT_DATA(PORT_LETTER, PORT_PIN) __GET_PORT_DATA(PORT_LETTER, PORT_PIN)

#define TriacON PTRIAC |=(1<<PINTRIAC)
#define TriacOFF PTRIAC &=~(1<<PINTRIAC)

#define F_CPU 8000000UL
#define DELAY


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include "7Segment.h"
#include "Encoder.h"

//управление симистором
#define DTRIAC DDRC
#define PTRIAC PORTC
#define PINTRIAC 4


//управление транзистором, отвечающим за младший индикатор
#define DSEG0 DDRB
#define PSEG0 PORTB
#define PINSEG0 0


//управление транзистором, отвечающим за старший индикатор
#define DSEG1 DDRD
#define PSEG1 PORTD
#define PINSEG1 7


volatile uint8_t	EncoderFlag = 0;//флаг прерывания энкодера
volatile uint8_t	EncCounter = 2;//счетчик периодов/показатель
uint8_t	EncCurState=0;//содержится состояние энкодера, нужно хранить для определения направления вращения
uint8_t	EncPushDown=0;//для работы с кнопкой


volatile uint8_t	SSegmentFlag = 0;//флаг прерывания таймера
union { uint16_t full; uint8_t word[2];} SSegmentText; //для хранения цифр на индикаторе (старший и младший разряды)


//volatile uint16_t	SSegmentText = 0;//для хранения цифр на индикаторе (старший и младший разряды)
uint8_t	SSegmentDigit = 1;//разряд индикатора (всего 2) нужно для отображения обооих цифр одновременно


volatile uint8_t	ButtonFlag = 0;//флаг прерывания кнопки
volatile uint8_t	WelderCount=0;//для подсчета периодов


void InitIntButt (void)
{
	//инициализация прерывания кнопки
	//по обоим фронтам-для ручной сварки по кнопке
	EICRA|=((0<<ISC11)|(1<<ISC10));
	EIMSK|=(1<<INT1);
}

void InitPCICR (void)
{
	//инициализация прерываний энкодера
	PCMSK1=0;
	PCICR=0;
	//включаем прерывание на PCINT[14:8]
	PCICR|=(1<<PCIE1);
	//разрешаем прерывание PCINT8,9,10
	PCMSK1|=(1<<PCINT8)|(1<<PCINT9)|(1<<PCINT10);
}

ISR(INT0_vect)
{
	//прерывание INT0 - переход фазы через 0
	//уменьшаем счетчик периодов на 1 ~20мс (220В 50Гц)
	WelderCount--;
	
	if (WelderCount>128)
	{
		//ушел в минус
		WelderCount=0;
		//запрещаем прерывание фазы
		EIMSK&=~(1<<INT0);
		//отключаем симистор
		TriacOFF;
		//восстанавливаем прерывание кнопки, т.к. иначе не зайдет по флагу в основном цикле
		InitIntButt();
		//восстанавливаем прерывание энкодера
		InitPCICR();

	}

}

ISR(INT1_vect)
{
	//прерывание INT1 - Нажата кнопка
	ButtonFlag = 1;
}


ISR(PCINT1_vect)
{
	//восстанавливаем флаг прерыванияесли вдруг произойдет INT0
	sei();
	EncoderFlag = 1;

}


ISR(TIMER0_COMPA_vect)
{
	//прерывание таймера 0
	//20мс прошло
	//сбрасываем значение счетчика энкодера
	TCNT0 = 0;
	//выставляем флаг обновления экрана
	SSegmentFlag = 1;
}



void InitTimer2 (void)
{
	//На частоте тактов 8 МГц
	//предделитель на 1024: 1 тик - 0,000128 сек, для 20мс нужно ~156 тиков
	
	TCCR0B=0;
	TCCR0B |= (1<<COM0B0);
	OCR0B = 90;
	
	TCCR0B |= (1<<CS02)|(0<<CS01)|(1<<CS00);
	//разрешаем прерыванеи по совпадению
	TIMSK0 |= (1<<OCIE0B);
	TCNT2 = 0;
}

void InitIntZero (void)
{
	//инициализация прерывания при переходе фазы через 0
	//по спадающему фронту
	EICRA|=((1<<ISC01)|(0<<ISC00));
	EIMSK|=(1<<INT0);
}


void TriacInit(void)
{
	//выход, 0
	DTRIAC |= (1<<PINTRIAC);
	PTRIAC &=~(1<<PINTRIAC);
}

void ButtonInit(void)
{
	//вход с подтяжкой
	DDRD &= ~(1<<PD3);
	PORTD |=(1<<PD3);
}

void ZerodetectPortInit(void)
{
	//вход с подтяжкой
	DDRD &= ~(1<<PD2);
	PORTD |=(1<<PD2);
}

void SsegmentShow(void)
{
	//обнуляем старшие биты (т.к. они должны быть = 0)
	SSegmentDigit &= 3;
	//инвертируем разряд
	SSegmentDigit ^= 1;
			
	if (!(EncPushDown))//кнопка нажата, надо отобразить "--"
	{
		SSegmentText.word[1] = (EncCounter/10);
		SSegmentText.word[0] = (EncCounter%10);
		SSegmentOut(SSegmentText.word[SSegmentDigit]);
	}
			
	if (SSegmentDigit)
	{
		PSEG0 &= ~(1<<PINSEG0);
		PSEG1 |= (1<<PINSEG0);
	}
	else
	{
		PSEG0 |= (1<<PINSEG0);
		PSEG1 &= ~(1<<PINSEG0);
	}

}


int main(void)
{
	cli();
	int8_t temp =0; 
	
	//общая инициализация портов - вход с подтяжкой
	DDRB=0;
	PORTB=0xFF;
	DDRC=0;
	PORTC=0xFF;
	DDRD=0;
	PORTD=0xFF;

	
	//!!!!!!!!!!!!!!!!!!Настройка портов!!!!!!!!!
	
	//настраиваем пины баз транзисторов в катодах сегментов на выход и выставляем 0
	DSEG0 |= (1<<PINSEG0);
	PSEG0 &= ~(1<<PINSEG0);
	
	DSEG1 |= (1<<PINSEG1);
	PSEG1 &= ~(1<<PINSEG1);
	
	SSegmentInit();// инициализация портов индикатора
	EncoderInit();//инициализация портов энкодера
	TriacInit(); //инициализация порта симистора
	ButtonInit();//инициализация порта кнопки
	ZerodetectPortInit();//инициализация порта детекта фазы
		
	
	//!!!!!!!!!!!!!!ПРЕРЫВАНИЯ!!!!!!!!!!!!!!!
	
	InitIntButt();//прерывания от нажатия кнопки
	InitPCICR();//прерывания от энкодера (PCINT8, PCINT9) и его кнопки PCINT10
	InitTimer2();//прерывания таймера для индикации
	
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	
		
	//проверяем индикаторы
	SSegmentOn();
	PSEG0 |= (1<<PINSEG0);
	PSEG1 |= (1<<PINSEG1);
	#ifdef DELAY
	_delay_ms(150);
	#endif
	PSEG0 &= ~(1<<PINSEG0);
	PSEG1 &= ~(1<<PINSEG1);
	SSegmentOFF();
	
	
	
	//загружаем прошлое значение выдержки
	EncCounter = eeprom_read_byte((uint8_t*)10);
	if ((EncCounter > 128)|(EncCounter<2)) EncCounter = 2;
	if (EncCounter > 99) EncCounter = 99;

	
	//разрешаем прерывания
	//sei();

	EncoderFlag=0;
	ButtonFlag=0;

    while(1)
    {

		//обновляем индикатор
		if (SSegmentFlag) 
		{
			SSegmentFlag = 0;
			SsegmentShow();
		}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		//нажата кнопка
		if (ButtonFlag)
		{
			ButtonFlag = 0;
			#ifdef DELAY
			_delay_ms(150);
			#endif
			if (GET_PORT_DATA(D,3) == 0)
			//все еще нажата - не дребезг
			{
				if (!(EncPushDown))
				{
					//если мы в режиме счета периодов
					//запрещаем прерывание кнопки
					EIMSK&=~(1<<INT1);
					//запрещаем прерывание энкодера
					PCICR&=~(1<<PCIE1);
					//заносим количество периодов для сварки
					WelderCount = EncCounter;
					//включаем симистор
					TriacON;
					//запускаем отсчет периодов
					InitIntZero();
					
				}
				else
				{
					//запрещаем прерывание энкодера
					PCICR&=~(1<<PCIE1);
					//если мы на ручном управлении - надо включить симистор и ждать пока отпустят кнопку	
					TriacON;
				}
			}
			else
			{
				//кнопку отпустили или дребезг, но не важно
				
				//если мы на ручном управлении
				if (EncPushDown)
				{
					//отключаем симистор
					TriacOFF;
					//восстанавливаем прерывание энкодера
					InitPCICR();
					
				}
				else
				{
					//если в режиме счета периодов
					if (WelderCount == 0)
					{
						//отключаем симистор, но он уже отключен из прерывания
						TriacOFF;
						//периоды истекли, это все уже сделано в прерывании, но на всякий случай
						//восстанавливаем прерывание энкодера
						InitPCICR();
						
					}		
				}

			}
		}
		
		
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!		
		
		
		//было прерывание энкодера
		if (EncoderFlag) 
		{
			//снимаем флаг
			EncoderFlag=0;
			#ifdef DELAY
			_delay_ms(150);
			#endif
			//поднимаем флаг обновления индикатора
			SSegmentFlag=1;
			//определяем нажата ли кнопка энкодера
			if (GetEncoderButt() == 0) 
			{
				//инвертируем флаг отображения цифр
				EncPushDown ^= 1;

				if (EncPushDown)
				{
					//нажата - переходим в режим ручной выдержки
					//выставляем на индикаторе "--"
					SSegmentOut(10);
				}
			}
			else
			{
				//не нажата - крутили энкодер
				//определяем положение энкодера
				temp=EncCounter;
				EncCounter+=GetEncoder(&EncCurState);
				//т.к. byte то если меньше 0 становится 255
				if ((EncCounter > 128)|(EncCounter<2)) EncCounter = 2;
				if (EncCounter > 99) EncCounter = 99;

				if (temp !=EncCounter)
				{
					//значение изменилось - записываем в память (что бы при включении восстановить)
					eeprom_write_byte((uint8_t*)10,EncCounter);
				}
			
			}
		}
    }
}