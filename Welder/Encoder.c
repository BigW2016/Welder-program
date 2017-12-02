/*
 * Encoder.c
 *
 * Created: 16.04.2016 16:53:22
 * Author: Wraith
 *
 * created from EncPoll by Dx http://easyelectronics.ru/repository.php?act=view&id=13
 */ 

#define __GET_ENCODER_PORT_DATA(PORT_LETTER, PORT_PIN) (((PIN ## PORT_LETTER)&(1<<(P ## PORT_LETTER ## PORT_PIN)))>>(P ## PORT_LETTER ## PORT_PIN))
#define GET_ENCODER_PORT_DATA(PORT_LETTER, PORT_PIN) __GET_ENCODER_PORT_DATA(PORT_LETTER, PORT_PIN)

#define __SET_DDR(PORT_LETTER, PORT_PIN) ((DDR ## PORT_LETTER)&=(~(1<<(P ## PORT_LETTER ## PORT_PIN))))
#define SET_DDR(PORT_LETTER, PORT_PIN) __SET_DDR(PORT_LETTER, PORT_PIN)

#define __SET_PORT(PORT_LETTER, PORT_PIN) ((PORT ## PORT_LETTER)|=(1<<(P ## PORT_LETTER ## PORT_PIN)))
#define SET_PORT(PORT_LETTER, PORT_PIN) __SET_PORT(PORT_LETTER, PORT_PIN)

#include "Encoder.h"
#include <avr/pgmspace.h>


const	int8_t	EncState[] PROGMEM =
{
	0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0
};


void EncoderInit(void)		//Настойка регистров управления индикатором
{
	SET_DDR(ENCPOLL_A_PORT,ENCPOLL_A_PIN);
	SET_PORT(ENCPOLL_A_PORT,ENCPOLL_A_PIN);	
	
	SET_DDR(ENCPOLL_B_PORT,ENCPOLL_B_PIN);
	SET_PORT(ENCPOLL_B_PORT,ENCPOLL_B_PIN);
	
	SET_DDR(ENCPOLL_BUTT_PORT,ENCPOLL_BUTT_PIN);
	SET_PORT(ENCPOLL_BUTT_PORT,ENCPOLL_BUTT_PIN);
	
}

uint8_t GetEncoderPortData(void) //получение текущего состояния регистров энкодера
{
	return ((GET_ENCODER_PORT_DATA(ENCPOLL_A_PORT, ENCPOLL_A_PIN)<<1)|(GET_ENCODER_PORT_DATA(ENCPOLL_B_PORT, ENCPOLL_B_PIN)));
}


int8_t GetEncoder(uint8_t *OldState)	//Возвращает положение энкодера на основе предыдущего состояния
{
	int8_t temp;
	
	*OldState <<= 2;
	*OldState &= 0b00001100;
	*OldState |= GetEncoderPortData();
	temp = *OldState;
	return pgm_read_byte(&(EncState[temp]));
}

uint8_t GetEncoderButt(void)
{
	return (GET_ENCODER_PORT_DATA(ENCPOLL_BUTT_PORT,ENCPOLL_BUTT_PIN));
}