/*
 * Welder.c
 *
 * Created: 29.11.2017 16:53:22
 *  Author: Wraith
 */ 

//������� ��� ��������������� ����������� ����������

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
volatile uint8_t	EncCounter = 2;//������� ��������/����������
uint8_t	EncCurState=0;//���������� ��������� ��������, ����� ������� ��� ����������� ����������� ��������
uint8_t	EncPushDown=0;//��� ������ � �������


volatile uint8_t	SSegmentFlag = 0;//���� ���������� �������
union { uint16_t full; uint8_t word[2];} SSegmentText; //��� �������� ���� �� ���������� (������� � ������� �������)


//volatile uint16_t	SSegmentText = 0;//��� �������� ���� �� ���������� (������� � ������� �������)
uint8_t	SSegmentDigit = 1;//������ ���������� (����� 2) ����� ��� ����������� ������ ���� ������������


volatile uint8_t	ButtonFlag = 0;//���� ���������� ������
volatile uint8_t	WelderCount=0;//��� �������� ��������


void InitIntButt (void)
{
	//������������� ���������� ������
	//�� ����� �������-��� ������ ������ �� ������
	EICRA|=((0<<ISC11)|(1<<ISC10));
	EIMSK|=(1<<INT1);
}

void InitPCICR (void)
{
	//������������� ���������� ��������
	PCMSK1=0;
	PCICR=0;
	//�������� ���������� �� PCINT[14:8]
	PCICR|=(1<<PCIE1);
	//��������� ���������� PCINT8,9,10
	PCMSK1|=(1<<PCINT8)|(1<<PCINT9)|(1<<PCINT10);
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
		InitIntButt();
		//��������������� ���������� ��������
		InitPCICR();

	}

}

ISR(INT1_vect)
{
	//���������� INT1 - ������ ������
	ButtonFlag = 1;
}


ISR(PCINT1_vect)
{
	//��������������� ���� �������������� ����� ���������� INT0
	sei();
	EncoderFlag = 1;

}


ISR(TIMER0_COMPA_vect)
{
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

void InitIntZero (void)
{
	//������������� ���������� ��� �������� ���� ����� 0
	//�� ���������� ������
	EICRA|=((1<<ISC01)|(0<<ISC00));
	EIMSK|=(1<<INT0);
}


void TriacInit(void)
{
	//�����, 0
	DTRIAC |= (1<<PINTRIAC);
	PTRIAC &=~(1<<PINTRIAC);
}

void ButtonInit(void)
{
	//���� � ���������
	DDRD &= ~(1<<PD3);
	PORTD |=(1<<PD3);
}

void ZerodetectPortInit(void)
{
	//���� � ���������
	DDRD &= ~(1<<PD2);
	PORTD |=(1<<PD2);
}

void SsegmentShow(void)
{
	//�������� ������� ���� (�.�. ��� ������ ���� = 0)
	SSegmentDigit &= 3;
	//����������� ������
	SSegmentDigit ^= 1;
			
	if (!(EncPushDown))//������ ������, ���� ���������� "--"
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
	
	//����� ������������� ������ - ���� � ���������
	DDRB=0;
	PORTB=0xFF;
	DDRC=0;
	PORTC=0xFF;
	DDRD=0;
	PORTD=0xFF;

	
	//!!!!!!!!!!!!!!!!!!��������� ������!!!!!!!!!
	
	//����������� ���� ��� ������������ � ������� ��������� �� ����� � ���������� 0
	DSEG0 |= (1<<PINSEG0);
	PSEG0 &= ~(1<<PINSEG0);
	
	DSEG1 |= (1<<PINSEG1);
	PSEG1 &= ~(1<<PINSEG1);
	
	SSegmentInit();// ������������� ������ ����������
	EncoderInit();//������������� ������ ��������
	TriacInit(); //������������� ����� ���������
	ButtonInit();//������������� ����� ������
	ZerodetectPortInit();//������������� ����� ������� ����
		
	
	//!!!!!!!!!!!!!!����������!!!!!!!!!!!!!!!
	
	InitIntButt();//���������� �� ������� ������
	InitPCICR();//���������� �� �������� (PCINT8, PCINT9) � ��� ������ PCINT10
	InitTimer2();//���������� ������� ��� ���������
	
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	
		
	//��������� ����������
	SSegmentOn();
	PSEG0 |= (1<<PINSEG0);
	PSEG1 |= (1<<PINSEG1);
	#ifdef DELAY
	_delay_ms(150);
	#endif
	PSEG0 &= ~(1<<PINSEG0);
	PSEG1 &= ~(1<<PINSEG1);
	SSegmentOFF();
	
	
	
	//��������� ������� �������� ��������
	EncCounter = eeprom_read_byte((uint8_t*)10);
	if ((EncCounter > 128)|(EncCounter<2)) EncCounter = 2;
	if (EncCounter > 99) EncCounter = 99;

	
	//��������� ����������
	//sei();

	EncoderFlag=0;
	ButtonFlag=0;

    while(1)
    {

		//��������� ���������
		if (SSegmentFlag) 
		{
			SSegmentFlag = 0;
			SsegmentShow();
		}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		//������ ������
		if (ButtonFlag)
		{
			ButtonFlag = 0;
			#ifdef DELAY
			_delay_ms(150);
			#endif
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
					InitIntZero();
					
				}
				else
				{
					//��������� ���������� ��������
					PCICR&=~(1<<PCIE1);
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
					//��������������� ���������� ��������
					InitPCICR();
					
				}
				else
				{
					//���� � ������ ����� ��������
					if (WelderCount == 0)
					{
						//��������� ��������, �� �� ��� �������� �� ����������
						TriacOFF;
						//������� �������, ��� ��� ��� ������� � ����������, �� �� ������ ������
						//��������������� ���������� ��������
						InitPCICR();
						
					}		
				}

			}
		}
		
		
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!		
		
		
		//���� ���������� ��������
		if (EncoderFlag) 
		{
			//������� ����
			EncoderFlag=0;
			#ifdef DELAY
			_delay_ms(150);
			#endif
			//��������� ���� ���������� ����������
			SSegmentFlag=1;
			//���������� ������ �� ������ ��������
			if (GetEncoderButt() == 0) 
			{
				//����������� ���� ����������� ����
				EncPushDown ^= 1;

				if (EncPushDown)
				{
					//������ - ��������� � ����� ������ ��������
					//���������� �� ���������� "--"
					SSegmentOut(10);
				}
			}
			else
			{
				//�� ������ - ������� �������
				//���������� ��������� ��������
				temp=EncCounter;
				EncCounter+=GetEncoder(&EncCurState);
				//�.�. byte �� ���� ������ 0 ���������� 255
				if ((EncCounter > 128)|(EncCounter<2)) EncCounter = 2;
				if (EncCounter > 99) EncCounter = 99;

				if (temp !=EncCounter)
				{
					//�������� ���������� - ���������� � ������ (��� �� ��� ��������� ������������)
					eeprom_write_byte((uint8_t*)10,EncCounter);
				}
			
			}
		}
    }
}