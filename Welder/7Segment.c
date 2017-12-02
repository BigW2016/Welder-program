/*
 * 7Segment.c 
 *
 * Created: 02.12.2017 02:34
 * Author: Wraith
 *
 */ 

#define __SET_DDR(PORT_LETTER, PORT_PIN) ((DDR ## PORT_LETTER)&(1<<(P ## PORT_LETTER ## PORT_PIN)))
#define SET_DDR(PORT_LETTER, PORT_PIN) __SET_DDR(PORT_LETTER, PORT_PIN)

#define __SET_PORT(PORT_LETTER) (PORT ## PORT_LETTER)
#define SET_PORT(PORT_LETTER) __SET_PORT(PORT_LETTER)


#include <avr/pgmspace.h>
#include "7Segment.h"
 
 
 
 const uint8_t SevenSegChar[] PROGMEM =
 {
	 //gfedcba
	 0b00111111,//0
	 0b00000110,//1
	 0b01011011,//2
	 0b01001111,//3
	 0b01100110,//4
	 0b01101101,//5
	 0b01111101,//6
	 0b00000111,//7
	 0b01111111,//8
	 0b01101111,//9
	 0b01000000 //-
 };
 
 
 
 static void SSMix(uint8_t data)
 {
	 if((data)&(0b10000000)) SET_PORT(SPORTDP) |=1<<SS_DP;
	 else SET_PORT(SPORTDP) &=~(1<<SS_DP);
	 if((data)&(0b01000000)) SET_PORT(SPORTG) |=1<<SS_G;
	 else SET_PORT(SPORTG) &=~(1<<SS_G);
	 if((data)&(0b00100000)) SET_PORT(SPORTF) |=1<<SS_F;
	 else SET_PORT(SPORTF) &=~(1<<SS_F);
	 if((data)&(0b00010000)) SET_PORT(SPORTE) |=1<<SS_E;
	 else SET_PORT(SPORTE) &=~(1<<SS_E);
	 if((data)&(0b00001000)) SET_PORT(SPORTD) |=1<<SS_D;
	 else SET_PORT(SPORTD) &=~(1<<SS_D);
	 if((data)&(0b00000100)) SET_PORT(SPORTC) |=1<<SS_C;
	 else SET_PORT(SPORTC) &=~(1<<SS_C);
	 if((data)&(0b00000010)) SET_PORT(SPORTB) |=1<<SS_B;
	 else SET_PORT(SPORTB) &=~(1<<SS_B);
	 if((data)&(0b00000001)) SET_PORT(SPORTA) |=1<<SS_A;
	 else SET_PORT(SPORTA) &=~(1<<SS_A);
 }
 
 void SSegmentInit(void)
 {
	SET_DDR(SPORTDP,SS_DP);
	SET_DDR(SPORTG,SS_G);
	SET_DDR(SPORTF,SS_F);
	SET_DDR(SPORTE,SS_E);
	SET_DDR(SPORTD,SS_D);
	SET_DDR(SPORTC,SS_C);
	SET_DDR(SPORTB,SS_B);
	SET_DDR(SPORTA,SS_A);
	
	SSMix(0);
 }
 
 void SSegmentOut(uint8_t data)
 {
	SSMix(pgm_read_byte(&(SevenSegChar[data])));
 }
 
 void SSegmentOn(void)
 {
	SSMix(1);
 }
 
 void SSegmentOFF(void)
 {
	SSMix(0);	 
 }
 
