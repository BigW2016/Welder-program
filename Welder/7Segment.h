/*
 * 7Segment.h 
 *
 * Created: 02.12.2017 02:34
 * Author: Wraith
 *
 */ 

#include <inttypes.h>

#define SS_DP	0 	//define MCU pin connected to DP segment
#define SS_A	5 	//define MCU pin connected to A segment
#define SS_B	6 	//define MCU pin connected to B segment
#define SS_C	1 	//define MCU pin connected to C segment
#define SS_D	3 	//define MCU pin connected to D segment
#define SS_E	2 	//define MCU pin connected to E segment
#define SS_F	5 	//define MCU pin connected to F segment
#define SS_G	4 	//define MCU pin connected to G segment


#define SPORTDP	C		//DP segment assignment
#define SPORTA	D		//A segment assignment
#define SPORTB	D		//B segment assignment
#define SPORTC	B		//C segment assignment
#define SPORTD	B		//D segment assignment
#define SPORTE	B		//E segment assignment
#define SPORTF	B		//F segment assignment
#define SPORTG	B		//G segment assignment


void SSegmentInit(void);		//Настойка регистров управления индикатором
void SSegmentOut(uint8_t);		//Out number or "--"
void SSegmentOn(void);		//LED ON
void SSegmentOFF(void);		//LED OFF

