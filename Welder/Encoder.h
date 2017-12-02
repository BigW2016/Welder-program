/*
 * Encoder.h
 *
 * Created: 16.04.2016 16:53:22
 * Author: Wraith
 *
 * created from EncPoll by Dx http://easyelectronics.ru/repository.php?act=view&id=13
 */ 

#include <inttypes.h>

//���� A
#define ENCPOLL_A_PORT C
#define ENCPOLL_A_PIN 2
//���� B
#define ENCPOLL_B_PORT C
#define ENCPOLL_B_PIN 1 
//���� ������ ��������
#define ENCPOLL_BUTT_PORT C
#define ENCPOLL_BUTT_PIN 3



void EncoderInit(void);		//�������� ��������� ��������
int8_t GetEncoder(uint8_t*);		//���������� ��������� �������� �� ������ ����������� ���������
uint8_t GetEncoderPortData(void); //��������� �������� ��������� ��������� ��������
uint8_t GetEncoderButt(void);		//���������� ��������� ����� ������ ��������



