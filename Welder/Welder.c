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

#define __GET_PORT_DATA(PORT_LETTER, PORT_PIN) (((PIN ## PORT_LETTER)&(1<<(P ## PORT_LETTER ## PORT_PIN)))>>(P ## PORT_LETTER ## PORT_PIN))
#define GET_PORT_DATA(PORT_LETTER, PORT_PIN) __GET_PORT_DATA(PORT_LETTER, PORT_PIN)


#define __GET_PORT_DATA(PORT_LETTER, PORT_PIN) (((PIN ## PORT_LETTER)&(1<<(P ## PORT_LETTER ## PORT_PIN)))>>(P ## PORT_LETTER ## PORT_PIN))
#define GET_PORT_DATA(PORT_LETTER, PORT_PIN) __GET_PORT_DATA(PORT_LETTER, PORT_PIN)

#define TriacON PTRIAC |=(1<<PINTRIAC)
#define TriacOFF PTRIAC &=~(1<<PINTRIAC)



#define F_CPU 8000000UL
#define NO_DELAY 1


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "Encoder.h"
#include "7Segment.h"

//���������� ����������
#define DTRIAC DDRC
#define PTRIAC PORTC
#define PINTRIAC 4


//���������� ������������, ���������� �� ������� ���������
#define DSEG0 DDRB
#define PSEG0 PORTB
#define PINSEG0 0


//���������� ������������, ���������� �� ������� ���������
#define DSEG1 DDRD
#define PSEG1 PORTD
#define PINSEG1 7


volatile uint8_t	EncoderFlag = 0;//���� ���������� ��������
volatile uint16_t	EncCounter = 0;
volatile uint8_t	EncCurState=0;//���������� ��������� ��������, ����� ������� ��� ����������� ����������� ��������
volatile uint8_t	EncPushDown=0;//��� ������ � �������


volatile uint8_t	SSegmentFlag = 0;//���� ���������� �������
volatile uint16_t	SSegmentText = 0;//��� �������� ���� �� ���������� (������� � ������� �������)
volatile uint8_t	SSegmentDigit = 1;//������ ���������� (����� 2) ����� ��� ����������� ������ ���� ������������


volatile uint8_t	ButtonFlag = 0;//���� ���������� ������
volatile uint8_t	WelderCount=0;//��� �������� ��������


void InitInt1 (void)
{
	//������������� ���������� ������
	//�� ����� �������-��� ������ ������ �� ������
	EICRA &=0;
	EICRA=(0<<ISC11)|(1<<ISC10);
	EIMSK|=(1<<INT1);
}


ISR(INT0_vect)
{
	//���������� INT0 - ������� ���� ����� 0
	//��������� ������� �������� �� 1 ~20�� (220� 50��)
	WelderCount--;
	
	if (WelderCount>128)
	{
		//���� � �����
		WelderCount=0;
		//��������� ���������� ����
		EIMSK&=~(1<<INT0);
		//��������� ��������
		TriacOFF;
		//��������������� ���������� ������, �.�. ����� �� ������ �� ����� � �������� �����
		InitInt1();

	}

}

ISR(INT1_vect)
{
	//��������������� ���� ���������� ���� ����� ���������� INT0
	sei();
	//���������� INT1 - ������ ������
	ButtonFlag = 1;
}


ISR(PCINT1_vect)
{
	//��������������� ���� �������������� ����� ���������� INT0
	sei();
	EncoderFlag = 1;
	EncCurState <<= 2;
	EncCurState &= 0b00001100;
	EncCurState |= ((GET_ENCODER_PORT_DATA(ENCPOLL_A_PORT, ENCPOLL_A_PIN)<<1)|(GET_ENCODER_PORT_DATA(ENCPOLL_B_PORT, ENCPOLL_B_PIN)));
	
}


ISR(TIMER0_COMPA_vect)
{
	//��������������� ���� �������������� ����� ���������� INT0
	sei();
	//���������� ������� 0
	//20�� ������
	//���������� �������� �������� ��������
	TCNT0 = 0;
	//���������� ���� ���������� ������
	SSegmentFlag = 1;
}



void InitTimer2 (void)
{
	//�� ������� ������ 8 ���
	//������������ �� 1024: 1 ��� - 0,000128 ���, ��� 20�� ����� ~156 �����
	
	TCCR0B=0;
	TCCR0B |= (1<<COM0B0);
	OCR0B = 90;
	
	TCCR0B |= (1<<CS02)|(0<<CS01)|(1<<CS00);
	//��������� ���������� �� ����������
	TIMSK0 |= (1<<OCIE0B);
	TCNT2 = 0;
}

void InitInt0 (void)
{
	//������������� ���������� ��� �������� ���� ����� 0
	//�� ���������� ������
	EICRA &=0;
	EICRA=(1<<ISC01)|(0<<ISC00);
	EIMSK|=(1<<INT0);
}

void InitPCICR (void)
{
	//������������� ���������� ��������
	PCMSK1&=0;
	PCICR&=0;
	//�������� ���������� �� PCINT[14:8]
	PCICR|=(1<<PCIE1);
	//��������� ���������� PCINT8,9,10
	PCMSK1|=(1<<PCINT8)|(1<<PCINT9)|(1<<PCINT10);
}


int8_t	EncPollDelta()
{
	
	return pgm_read_byte(&(EncState[EncCurState]));
}

/*
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
}*/


void TriacInit(void)
{
	DTRIAC |= (1<<PINTRIAC);
	PTRIAC &=~(1<<PINTRIAC);
}


/*
void TriacON(void)
{
	PTRIAC |=(1<<PINTRIAC);
}
*/

/*
void TriacOFF(void)
{
	PTRIAC &=~(1<<PINTRIAC);
}*/


void SsegmentShow(void)
{
	//�������� ������� ���� (�.�. ��� ������ ���� = 0)
	SSegmentDigit &= 3;
	//����������� ������
	SSegmentDigit ^= 1;
			
	if (!(EncPushDown))
	{
		SSegmentText = (EncCounter/10) << 8;
		SSegmentText = (EncCounter%10);
		SSegmentOut(SSegmentText>>(8*SSegmentDigit));
	}
			
	if (SSegmentDigit)
	{
		PSEG0 &= ~(0<<PINSEG0);
		PSEG1 |= (1<<PINSEG0);
	}
	else
	{
		PSEG0 |= (1<<PINSEG0);
		PSEG1 &= ~(0<<PINSEG0);
	}

}


int main(void)
{
	cli();
	int8_t EncStep =0; //������� ���-�� ����� ��� ��������� ����� �� ����������
	
	//����� ������������� ������ - ���� � ���������
	DDRB=0;
	PORTB=0xFF;
	DDRC=0;
	PORTC=0xFF;
	DDRD=0;
	PORTD=0xFF;

	//����������� ���� ������� ���������
	//�� ����� � ���������� 0
	DSEG0 |= (1<<PINSEG0);
	PSEG0 &= ~(0<<PINSEG0);
	DSEG1 |= (1<<PINSEG1);
	PSEG1 &= ~(0<<PINSEG1);
	
	InitInt1();//������� ������
	InitPCICR();//���������� �� �������� (PCINT8, PCINT9) � ��� ������ PCINT10
	InitTimer2();//��� ���������
	
	//��������� ����������
	SSegmentOn();
	PSEG0 &= ~(0<<PINSEG0);
	PSEG1 |= (1<<PINSEG0);
	_delay_ms(150);
	PSEG0 |= (1<<PINSEG0);
	PSEG1 &= ~(0<<PINSEG0);
	_delay_ms(150);
	SSegmentOFF();
	
	
	EncoderFlag=0;
	ButtonFlag=0;
	
	//������������� ��������� �����
	
	TriacInit(); //������������� ����� ���������
	
	
	
	//��������� ����������
	sei();

    while(1)
    {

		//��������� ���������
		if (SSegmentFlag) 
		{
			SSegmentFlag = 0;
			SsegmentShow();
		}

		//������ ������
		if (ButtonFlag)
		{
			ButtonFlag = 0;
			_delay_ms(20);
			if (GET_PORT_DATA(D,3) == 0)
			//��� ��� ������ - �� �������
			{
				if (!(EncPushDown))
				{
					//���� �� � ������ ����� ��������
					//��������� ���������� ������
					EIMSK&=~(1<<INT1);
					//��������� ���������� ��������
					PCICR&=~(1<<PCIE1);
					//������� ���������� �������� ��� ������
					WelderCount = EncCounter;
					//�������� ��������
					TriacON;
					//��������� ������ ��������
					InitInt0();
					
				}
				else
				{
					//���� �� �� ������ ���������� - ���� �������� �������� � ����� ���� �������� ������	
					TriacON;
				}
			}
			else
			{
				//������ ��������� ��� �������, �� �� �����
				
				//���� �� �� ������ ����������
				if (EncPushDown)
				{
					//��������� ��������
					TriacOFF;
					
				}
				else
				{
					//���� � ������ ����� ��������
					if (WelderCount == 0)
					{
						//��������� �����, �� �� ��� �������� �� ����������
						TriacOFF;
						//������� �������
						//��������������� ���������� �������� (�� ����� ������� ����� ����� ��������� ������ ������� ������ - �������)
						InitPCICR();
						
					}		
				}

			}
		}
		
		
		
		
		
		
		
		
		
		
		
		//���� ���������� ��������
		if (EncoderFlag) {
			//������� ����
			EncoderFlag=0;
			_delay_ms(20);
			//���������� ������ �� ������ ��������
			if (GET_PORT_DATA(C,3) == 0) 
			{
				//����������� ���� ����������� ����
				EncPushDown ^= 1;

				if (EncPushDown)
				{
					//������ - ��������� � ����� ������ ��������
					//���������� �� ���������� "--"
					SSegmentOut(10);
				}
				//��������� ���� ���������� ����������
				SSegmentFlag=1;
			
			}
			else
			{
				//�� ������
				//���������� ��������� ��������
				EncStep += EncPollDelta(EncCurState);
				if (EncStep>0) {
					EncStep=0;	
					EncCounter++;
				}
				if (EncStep<0) {
					EncStep=0;
					EncCounter--;
				}
			
				//�.�. byte �� ���� ������ 0 ���������� 255
				if (EncCounter > 128) EncCounter = 0;
				if (EncCounter > 99) EncCounter = 99;
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			}
		}
    }
}