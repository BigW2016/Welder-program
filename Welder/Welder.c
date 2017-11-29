/*
 * Welder.c
 *
 * Created: 29.11.2017 16:53:22
 *  Author: Wraith
 */ 

//������� ��� ��������������� ����������� ����������
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


volatile uint16_t RelayCounter = 0;//16 ��� ��� 2� ��������� (16 ����) �����
volatile uint8_t TimerFlag = 0;//���� ���������� �������
volatile uint16_t EncCounter = 0;
volatile uint8_t EncCurState=0;//���������� ��������� ��������, ����� ������� ��� ����������� ����������� ��������
volatile uint8_t EncPushDown=0;//��� ������ � �������


const uint8_t SevenSegChar[] PROGMEM = 
{
	0b10110111, //���� � ����������� � ��������� ����� (������������)
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
	//���������� INT0 - ������� ���� ����� 0
	//
	//���������� �������� ��������
	TCNT2 = 0;
	TimerFlag = 1;
}

void InitTimer2 (void)
{
	//�� ������� ������ 8 ���
	//������������ �� 1024: 1 ��� - 0,000128 ���, ��� 20�� ����� ~156 �����
	TCCR2 |= (1<<CS22)|(1<<CS21)|(1<<CS20);
	OCR2 = 35;//156
	//��������� ���������� �� ����������
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

static uint8_t TurnBitsAround( uint8_t aByte )//����������� ���� � ����� � ����� �������� ���� ������� 
												//- ��-�� ��������
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
	int8_t EncStep =0; //������� ���-�� ����� ��� ��������� ����� �� ����������, ������ 2
	uint8_t i;
	//���� ����������� 0 �� ������ ����������
	//������� ����� �� ����� � ����� 1
	//0-7 ����
	PORTB=0xFF;
	DDRB=0xFF;
	
	//8-15 ����
	PORTC=0xFF;
	DDRC=0xFF;
	
	
	//��������� ����� �� ���� c ���������

	//��� PB0 PB1 ������ - ��� ���������� �����������
	// PB2 PB3 PB4 - ����� � ��������� ��� ������ � ���������
 	DDRB=0x3;//0x00000011
	PORTB=0xFF;
	
	//���� ��� ������ � �����������, �� �����, �������� 0-�����
	DDRD=0xFF;
	PORTD=0;
	
	PORTB=(PORTB&(~2))|1; //�������� ���������� - ��������� 2 ���, �������� 1
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
	PORTB=(PORTB&(~1))|2; //�������� ����������, ��������� 1 ���, ������� 2
	for (i = 0; i < 10; i++) {
		#if !NO_DELAY
			_delay_ms(150);
		#endif
		PORTD = pgm_read_byte(&(SevenSegChar[i]));
	}

		#if !NO_DELAY
			_delay_ms(150);
		#endif

	//����������� ���������� ��� � 20�� ��� ������ � ���������
	//�������������, ����� ��� ������������ ��� ������� ������� 2-3 ���� �� 1-2 ��� ��� ����������� ����� ������
	//��� �� �� ���������
	
	TimerFlag=0;
	InitTimer2();
	temp=0;
	PORTB&=~1;//������������� 2 ���
	
	sei();

    while(1)
    {

		//���� ���������� �������
		if (TimerFlag) {
			TimerFlag=0;
			//���������� ��������� ��������
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
			
			//������� ������������� ������ ����� ��� �������� �������� (���� ���� �� ������)
			//if ((PINB&3)<1) PORTB|=2;
			
			//���������� �������� �� ����������
			if (GET_PORT_DATA(B,0)) {
				temp = EncCounter%10;
			}else{
				temp = EncCounter/10;
			}
			PORTD = pgm_read_byte(&(SevenSegChar[temp]));
			
			//���� ����� ��� ���� ����� (���� ���� ������-�� �� ������)
			if (!(PORTB&3)) PORTB|=1;
			//����������� ��� PB0 � PB1
			PORTB^=3;

			//���������� ����� �� �������
			if (GET_PORT_DATA(B,4)) {
					//�����,
					//���������� ���� - �������������� �� ����������
					EncPushDown=1;
				}else{
					//�� �����
					if (EncPushDown) {
						//��� ����� �� �����
						//����������� ������� ��� � �������� ��������� ���� RelayCounter
						EncPushDown=0;
						if (!(EncCounter)) {
							//���� EncCounter - ��������� ��� ����
							RelayCounter=0;
						} else {
							RelayCounter ^= (1<<(EncCounter-1));	
						}
						//��������� ���� - ��������������� ������!!!
						PORTC = ~RelayCounter;
						PORTA = ~(TurnBitsAround(RelayCounter>>8));
					}
				}
		}
		
		
    }
}