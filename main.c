/*
 * Taximetro.c
 *
 * Created: 23/05/2017 05:34:57 p. m.
 * Author : LuisEduardo
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>

#define LIBRE 1
#define OCUPADO 0
#define PAGAR 2

unsigned int atoi(char *str);
void init_PORTK( void );
void itoa(char *str, uint16_t number, uint8_t base);
void taxi_Status( void );
void TIMER0_init( void );
void UART0_init( void );
char UART0_getchar(void);
void UART0_gets(char *str);
void UART0_putchar(char data);
void UART0_puts(char *str);
void viaje_Status( void );

static volatile uint8_t taxiStat = 1;
static volatile uint8_t rebote = 0;
static volatile uint8_t rebotePK2 = 0;
static volatile uint8_t mSecPK2;
static volatile uint16_t mSec = 0;
static volatile uint16_t mSecP = 0;
static volatile uint16_t costoViaje = 0;
static volatile uint16_t costoKM = 10;
static volatile uint16_t pulsoKM = 0;
static volatile uint16_t decSeg = 0;

int main(void)
{
	init_PORTK();
	UART0_init();
	TIMER0_init();
	UART0_puts("INICIO TAXI, LIBRE");
    while (1) 
    {
		taxi_Status();
		
		if( (taxiStat == OCUPADO) && (!(PINK&(1<<PK1))&& (rebotePK2==0)) )
		{
			rebotePK2 = 1;
			mSec = 0;
			mSecP = 0;
			decSeg = 0;
			UART0_puts("\n\r+2.5");
			pulsoKM++;
		}
    }
}

void init_PORTK( void )
{
	DDRK = 0x0C;			//PK0 PK1 in     PK2 PK3 out
	PORTK = 0x0B;			//PK0 PK1 P.on   PK2 0 out   PK3 1 high
}
void UART0_init( void )
{
	UCSR0A = (1<<U2X0);									//doble speed UART0
	UCSR0B = (1<<TXEN0)|(1<<RXEN0);		//TX/RX enable, 9 bit dis
	UCSR0C = (3<<UCSZ00);								//8-bit
	UBRR0 = 103;										//19.2 BAUDS			
}
void TIMER0_init( void )
{
	TCCR0A = (2<<WGM00);				//CTC enable
	TCCR0B = (3<<CS00);					//64PS
	TCNT0  = 0;							//cnt init
	OCR0A  = 250-1;						//1ms
	TIMSK0 = (1<<OCIE0A);				//Interrupt compare A match enable
	sei();
}
char UART0_getchar(void)
 {
	 while ( !(UCSR0A & (1<<RXC0)) );
	 return UDR0; 
 }
void UART0_gets(char *str)
 {
	 unsigned char c;
	 unsigned int i=0;
	 do{
		 c = UART0_getchar();
		 if( (i<=18)&&(c!=8)&&(c!=13) )	//validacion menor al fin del arreglo, backspace y enter
		 {
			 UART0_putchar(c);
			 *str++ = c;
			 i++;
		 }
		 if( (c==8) && (i>0) )		//validacion backspace
		 {
			 UART0_putchar('\b');
			 UART0_putchar(' ');
			 UART0_putchar(8);
			 *str--='\0';
			 i--;
		 }
	 }while(c != 13);
	 *str = '\0';
 }
void UART0_putchar(char data)
 {
	 while ( !(UCSR0A & (1<<UDRE0)) );
	 UDR0 = data;
 }
void UART0_puts(char *str)
 {
	 while(*str)
	 {
		 UART0_putchar(*str++);
	 }
 }
void itoa(char *str, uint16_t number, uint8_t base)
 {
	 unsigned int cociente, residuo,count = 0, i=0, j;
	 char c;
	 cociente = number;
	 do{
		 residuo = cociente%base;
		 cociente = cociente/base;
		 if(residuo > 9)	//si es mayor a 9, agregar el respectivo para imprimir letra.
		 {
			 c = residuo + 55;
			 }else{
			 c = residuo + '0'; //agregar el respectivo para crear el caracter de numero
		 }

		 *str++ = c;
		 count++;
	 }while( cociente != 0 );
	 *str= '\0';
	 str -=count;
	 j = count -1;
	 //==============invertir cadena==================
	 while(i < j)
	 {
		 if( *(str+i) != *(str+j))
		 {
			 c = *(str+i);
			 *(str+i) = *(str+j);
			 *(str+j) = c;
		 }
		 i++;
		 j--;
	 }
 }
unsigned int atoi(char *str)
 {
	 unsigned int num = 0, exp = 1, val, count = 0;
	 //contando digitos en la cadena============
	 while(*str)
	 {
		 str++;
		 count++;
	 }
	 str--;	//no tomando en cuenta '\0'
	 while(count != 0 )
	 {
		 val = *str--;	//tomando el valor
		 val = val - '0';	//obteniendo valor crudo
		 if(val >=0 && val <=9)
		 {
			 num = num + (val * exp);	//almacenando valor crudo*exp en num
			 exp = exp*10;
			 count--;
		 }
	 }
	 return num;
 }
void taxi_Status( void )
{
	char dineroViaje[20];
	
	if( !(PINK &(1<<PK0))&&(rebote == 0) )
	{
		rebote = 1;
		mSec = 0;
		if(taxiStat == LIBRE)
		{
			PORTK &= ~(1<<PK3);				//LED off
			taxiStat = 0;
			costoViaje = 10;
			UART0_puts("\n\rTAXI OCUPADO");
		}else if(taxiStat == OCUPADO)
		{
			taxiStat = 2;
			costoViaje += (((pulsoKM*2.5)/1000)*10); 
			itoa(dineroViaje,costoViaje,10);
			UART0_puts("\n\rPAGAR: ");
			UART0_puts(dineroViaje);
			UART0_getchar();
		}else if(taxiStat == PAGAR)
		{
			taxiStat = 1;
			PORTK ^= (1<<PK3);				//LED on
			UART0_puts("\n\rLIBRE");
			costoViaje = 0;
		}
	}
}
 ISR(TIMER0_COMPA_vect)
 {
	 mSec++;
	 mSecP++;
	 
	 if( (mSecP == 200)&&(PINK&(1<<PK1))&&(taxiStat == OCUPADO) )
	 {
		 decSeg++;
		 mSecP = 0;
		 if(decSeg == 300)
		 {
			  UART0_puts("\n\rCARRO PARADO, se incrementara 12.5mts");
			  pulsoKM +=5;
			  decSeg = 0;
		 }
	 }
	 if( (rebote)&&(mSec == 200) )
	 {
		 rebote = 0;
	 }
	 if( (rebotePK2)&&(mSec <= 50) )
	 {
		 PORTK |= (1<<PK2);
	 }else
	 {
		 PORTK &= ~(1<<PK2);
	 }
	 if( (rebotePK2)&&(mSec == 200))
	 {
		 rebotePK2 = 0;
	 }
 }
