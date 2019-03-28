#define F_CPU 1000000UL
#define NOOFSAMPLES 128

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define SILNIK1_1 _BV(PC2)
#define SILNIK1_2 _BV(PC3)
#define SILNIK1_PWM _BV(PD6)

#define SILNIK2_1 _BV(PC0)
#define SILNIK2_2 _BV(PC1)
#define SILNIK2_PWM _BV(PD5)


#define ENKODER1 _BV(PB6)
#define ENKODER1_INT _BV(PD2)

#define ENKODER2 _BV(PD4)
#define ENKODER2_INT _BV(PD3)


#define ADC1 _BV(PC5)


#define LED_URUCHOMIENIE _BV(PD0)
#define LED_ROZLADOWANIE _BV(PD1)


struct silnik
{
	uint8_t kierunek; // 1 - przod, 0 - tyl
	uint8_t v; // predkosc
};

struct silnik silnik1 = {1, 0};
struct silnik silnik2 = {1, 0};

struct enkoder
{
	int16_t val;
	uint8_t dir;
	uint8_t lastState;
	uint8_t part;
};

struct enkoder enk1 = {0,1,0,0};
struct enkoder enk2 = {0,1,0,0};

uint8_t ster = 0; // sterowanie programem
uint8_t a;
uint8_t w;

struct bateria
{
	uint16_t ADCval;
	uint8_t part;
};

struct bateria bateria1 = {0, 0};

void SPI_slave_init()
{
	DDRB |= _BV(PB4);
	SPCR = _BV(SPE) | _BV(SPIE);
	SPSR;
	SPDR;
}

void ADC_init()
{
	DDRC &= ~ADC1;
	PORTC &= ~ADC1;
	//MUX0|MUX2 - kanal ADC5
	ADMUX = _BV(REFS0) | _BV(MUX0) | _BV(MUX2);
	//napiecie odniesienia z avcc
	ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADATE) | _BV(ADSC) | _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2) ;
	// ADEN - ADC Enable | ADIE - ADC Interrupt Enable | ADPS - prescaler(w tym przypadku 128) | ADC Auto Trigger Enable | ADCS - ADC Start Conversion
}

void motor_init()
{
	DDRD |= SILNIK1_PWM | SILNIK2_PWM;
	TCCR0A = _BV(COM0A1) | _BV(COM0B1) | _BV(WGM00) | _BV(WGM01);
	TCCR0B = _BV(CS01) | _BV(CS00); // CS - clock source(ustawienie prescalera)
	OCR0A = 80;
	OCR0B = 80;
	
	DDRC |= SILNIK1_1 | SILNIK1_2 | SILNIK2_1 | SILNIK2_2;
	PORTC &= ~(SILNIK1_1 | SILNIK1_2 | SILNIK2_1 | SILNIK2_2);
}

void encoder_init()
{
	DDRD &= ~(ENKODER1_INT | ENKODER2_INT);
	PORTD |= ENKODER1_INT | ENKODER2_INT;

	DDRB &= ~(ENKODER1);
	PORTB |= ENKODER1;
	
	DDRD &= ~(ENKODER2);
	PORTD |= ENKODER2;

	EICRA |= _BV(ISC00) | _BV(ISC10);
	EIMSK |= _BV(INT0) | _BV(INT1);
}

ISR(ADC_vect)
{
	static uint32_t ADCaccum;
	static uint8_t sampleno;
	ADCaccum += ADC;
	sampleno++;
	if(sampleno==NOOFSAMPLES)
	{
		bateria1.ADCval = ADCaccum/NOOFSAMPLES;
		ADCaccum = 0;
		sampleno = 0;
		if(bateria1.ADCval < 690)
			PORTD |= LED_ROZLADOWANIE;
		else
			PORTD &= ~LED_ROZLADOWANIE;
		ADCSRA &= ~_BV(ADEN);
	}
}

ISR(INT0_vect)
{
	uint8_t Lstate = PIND & ENKODER1_INT;
	if((enk1.lastState == 0)&&(Lstate != 0))
	{
		uint8_t val = PINB & ENKODER1;
		if((val == 0)&&(enk1.dir == 1))
			enk1.dir = 0;
		else if((val != 0)&&(enk1.dir == 0))
			enk1.dir = 1;
	}

	enk1.lastState = Lstate;

	if(enk1.dir)
		enk1.val--;
	else
		enk1.val++;
}

ISR(INT1_vect)
{
	uint8_t Lstate = PIND & ENKODER2_INT;
	if((enk2.lastState == 0)&&(Lstate != 0))
	{
		uint8_t val = PIND & ENKODER2;
		if((val == 0)&&(enk2.dir == 1))
			enk2.dir = 0;
		else if((val != 0)&&(enk2.dir == 0))
			enk2.dir = 1;
	}

	enk2.lastState = Lstate;

	if(enk2.dir)
		enk2.val--;
	else
		enk2.val++;
}

ISR(SPI_STC_vect)
{
	a = SPDR;
	SPDR = 0;
	if(ster==0)
	{
		ster = a >> 1;
		if(ster == 1)
		{
			a <<= 7;
			a >>= 7;
			if(a)
				silnik1.kierunek = 1;
			else
				silnik1.kierunek = 0;
		}
		else if(ster == 2)
		{
			a <<= 7;
			a >>= 7;
			if(a)
				silnik2.kierunek = 1;
			else
				silnik2.kierunek = 0;
		}
	}
	else
	{
		if(ster == 1)
		{
			silnik1.v = a;
			if(silnik1.v == 0)
			{
				PORTC &= ~(SILNIK1_1 | SILNIK1_2);
			}
			else if(silnik1.kierunek)
			{
				PORTC |= SILNIK1_1;
				PORTC &= ~SILNIK1_2;
			}
			else
			{
				PORTC &= ~SILNIK1_1;
				PORTC |= SILNIK1_2;
			}
			OCR0A = a;
			ster = 0;
		}
		else if(ster == 2)
		{
			silnik2.v = a;
			if(silnik2.v == 0)
			{
				PORTC &= ~(SILNIK2_1 | SILNIK2_2);
			}
			else if(silnik2.kierunek)
			{
				PORTC |= SILNIK2_1;
				PORTC &= ~SILNIK2_2;
			}
			else
			{
				PORTC &= ~SILNIK2_1;
				PORTC |= SILNIK2_2;
			}
			OCR0B = a;
			ster = 0;
		}
	}
	
	if(ster==6)
	{
		if(enk1.part == 0)
		{		
			SPDR = enk1.val;
			enk1.part = 1;
		}
		else
		{
			SPDR = (enk1.val>>8);
			enk1.part = 0;
			enk1.val = 0;
			ster = 0;
		}
	}
	else if(ster==7)
	{
		if(enk2.part == 0)
		{		
			SPDR = enk2.val;
			enk2.part = 1;
		}
		else
		{
			SPDR = (enk2.val>>8);
			enk2.part = 0;
			enk2.val = 0;
			ster = 0;
		}
	}
	else if(ster==8)
	{
		if(bateria1.part == 0)
		{		
			SPDR = bateria1.ADCval;
			bateria1.part = 1;
		}
		else
		{
			SPDR = (bateria1.ADCval>>8);
			bateria1.part = 0;
			ster = 0;
		}
	}
	else if(ster == 9)
	{
		PORTD ^= LED_URUCHOMIENIE;
		ster = 0;
	}
	else if (ster!=1&&ster!=2)
		ster = 0;
}

int main()
{
	SPI_slave_init();
	motor_init();
	encoder_init();
	ADC_init();
	sei();
	DDRD |= LED_URUCHOMIENIE | LED_ROZLADOWANIE;
	PORTD &= ~LED_ROZLADOWANIE;
	PORTD |= LED_URUCHOMIENIE;
	while(1)
	{
		ADCSRA |= _BV(ADEN) | _BV(ADSC);
		_delay_ms(1000);
	}
}
