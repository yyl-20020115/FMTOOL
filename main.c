#include <REG51.H>

#define CMD_READ 0x55
#define CMD_WRITE 0xAA
#define REPLY_READ 0x66
#define REPLY_WRITE 0xBB



sfr AUXR=0x8E;

sbit A0 = P3^6;
sbit A1 = P3^5;
sbit A2 = P1^7;
sbit A3 = P1^6;
sbit A4 = P1^5;
sbit A5 = P1^4;
sbit A6 = P1^3;
sbit A7 = P1^2;
sbit A8 = P0^2;
sbit A9 = P0^3;
sbit A10 = P0^6;
sbit A11 = P0^4;
sbit A12 = P1^1;
sbit A13 = P0^1;
sbit A14 = P1^0;

sbit DQ0 = P2^0;
sbit DQ1 = P2^1;
sbit DQ2 = P2^2;
sbit DQ3 = P2^6;
sbit DQ4 = P2^7;
sbit DQ5 = P2^5;
sbit DQ6 = P2^4;
sbit DQ7 = P2^3;

sbit N_WE = P0^0;
sbit N_OE = P0^5;
sbit N_CE = P0^7;


unsigned char input_buffer[5]={0};

unsigned char input_length = 0;
unsigned char last_cmd = 0;

unsigned int pointer = 0;
unsigned int start = 0;
unsigned int length = 0;

void Delay(unsigned int xms)
{
	unsigned char data i, j;
  while(xms)
	{
		i = 2;
		j = 199;    
		do
		{
			while (--j);
		} while (--i);
		xms--;
	}
}

void SetAddress(unsigned p)
{
	A0 = p &0x01;
	p>>=1;
	A1 = p &0x01;
	p>>=1;
	A2 = p &0x01;
	p>>=1;
	A3 = p &0x01;
	p>>=1;
	A4 = p &0x01;
	p>>=1;
	A5 = p &0x01;
	p>>=1;
	A6 = p &0x01;
	p>>=1;
	A7 = p &0x01;
	p>>=1;
	A8 = p &0x01;
	p>>=1;
	A9 = p &0x01;
	p>>=1;
	A10 = p &0x01;
	p>>=1;
	A11 = p &0x01;
	p>>=1;
	A12 = p &0x01;
	p>>=1;
	A13 = p &0x01;
	p>>=1;
	A14 = p &0x01;	
}

unsigned char ReadAt(unsigned int p)
{
	unsigned char c = 0;
	N_WE = 1;
	
	SetAddress(p);

	N_CE = 0;
	N_OE = 0;
	
	c |= DQ7;
  c <<=1;
	c |= DQ6;
  c <<=1;
	c |= DQ5;
  c <<=1;
	c |= DQ4;
  c <<=1;
	c |= DQ3;
  c <<=1;
	c |= DQ2;
  c <<=1;
	c |= DQ1;
  c <<=1;
	c |= DQ0;   	
	
	N_WE = 1;
	N_OE = 1;
	N_CE = 1;
	return 0;
}
void WriteAt(unsigned int p, unsigned char c)
{
	
	SetAddress(p);
	
	DQ0 = c & 0x01;
	c>>=1;
	DQ1 = c & 0x01;
	c>>=1;
	DQ2 = c & 0x01;
	c>>=1;
	DQ3 = c & 0x01;
	c>>=1;
	DQ4 = c & 0x01;
	c>>=1;
	DQ5 = c & 0x01;
	c>>=1;
	DQ6 = c & 0x01;
	c>>=1;
	DQ7 = c & 0x01;

	N_CE = 0;
	N_OE = 1;
	N_WE = 0;

	Delay(1);

	N_WE = 1;
	N_OE = 0;
	N_CE = 1;	
}

void ProcessCommand(){
	if(last_cmd==0){
		switch(last_cmd = input_buffer[0])
		{
			case CMD_READ:
				pointer = start = (((unsigned int)input_buffer[1])<<8)|(input_buffer[2]);
				length = (((unsigned int)input_buffer[3])<<8)|(input_buffer[4]);
				break;
			case CMD_WRITE:
				pointer = start = (((unsigned int)input_buffer[1])<<8)|(input_buffer[2]);
				length = (((unsigned int)input_buffer[3])<<8)|(input_buffer[4]);
				break;
		}
	}	
}


void UART_SendByte(unsigned char Byte){
	SBUF = Byte;
	while(TI == 0);
	TI = 0;
}

void Uart1_Isr(void) interrupt 4
{
	if (TI)
	{
		TI = 0;
	}

	if(last_cmd==CMD_WRITE && RI){
		RI = 0;
		if(pointer<start+length){	
			WriteAt(pointer,SBUF);
		}else{
			SBUF = REPLY_WRITE;
			pointer = start;
			last_cmd = 0;
		}
		return;
	}
	
	if (RI)
	{
		if(input_length<=sizeof(input_buffer)){
			input_buffer[input_length] = SBUF;
			input_length++;
		}
		
		if(input_length == sizeof(input_buffer)+1){
			ProcessCommand();
			input_length = 0;
		}
		RI = 0;
	}
}

void Uart1_Init(void)
{
	SCON = 0x50;
	AUXR |= 0x40;
	AUXR &= 0xFE;
	TMOD &= 0x0F;
	TL1 = 0xE8;
	TH1 = 0xFF;
	ET1 = 0;
	TR1 = 1;
	ES = 1;
}

int main()
{
	N_WE = 1; //disable write
	N_OE = 1; //disable read
	N_CE = 1; //disable cheap enable
	Uart1_Init();
	
	while(1){
		if(last_cmd==CMD_READ){
			UART_SendByte(REPLY_READ);	
			UART_SendByte((start>>8)&0xff);
			UART_SendByte((start)&0xff);
			UART_SendByte((length>>8)&0xff);
			UART_SendByte((length)&0xff);
			
			for(pointer=start;pointer<start+length;pointer++){				
				UART_SendByte(ReadAt(pointer));
			}
			last_cmd = 0;
		}
	}
}