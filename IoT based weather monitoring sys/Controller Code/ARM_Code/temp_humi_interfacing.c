#include <lpc214x.h>
#include <stdio.h>
#include <stdint.h>

#define DHT11_PIN (1 << 16)
#define pclk 60000000

char rec = '\0';

void init_pll(){
	PLL0CFG = 0X24;
	PLL0CON = 0X01;
	PLL0FEED = 0XAA;
	PLL0FEED = 0X55;
	while(PLL0STAT & 0X400 == 0);
	PLL0CON = 0X03;
	VPBDIV = 0X01;
	PLL0FEED = 0XAA;
	PLL0FEED = 0X55;
}

//=======================UART part=======================//
void init_uart(int baud){
	unsigned int val = pclk / (16 * baud);
	PINSEL0 |= 0x00000005;

	U0LCR = 0X83;
	U0FDR = 0X10;
	U0DLM = (val >> 8)  & 0xff;
	U0DLL = val & 0xff;
	U0LCR = 0X03;
}

void send(char ch){
	U0THR = ch;
	while(!(U0LSR & (1 << 5)));
}

char recive(){
	char ch;
	while(!(U0LSR & 0X01));
	ch = U0RBR;
	return ch;
	
}

void send_str(char * str){
	int i = 0;
	for(i = 0; str[i] != '\0'; i++){
		send(str[i]);
	}
}
//===========================Timer delay part====================//

void delay_ms(unsigned int val){
	T0CTCR = 0X00;
	T0PR = 60000 - 1;
	T0TCR = 0X01;
	T0TC = 0;
	while(T0TC < val );
	T0TCR = 0X00;
}

void delay_us(unsigned int val){
	T0CTCR = 0X00;
	T0PR = 60 - 1;
	T0TCR = 0X01;
	T0TC = 0;
	while(T0TC < val );
	T0TCR = 0X00;
}

//===========================I2C interfacing====================//


/////////////////esp-01 interfacing//////////////////////////

void connect_wifi(){
   	send_str("AT\r\n");      //AT
	delay_ms(1000);
	while(rec != '\r'){
		rec = recive();
		send(rec);
	}
	rec = '\0';
	
	send_str("AT+CWJAP=\"No connection\",\"neerajneeraj\"\r\n"); // WIFI Connect
	delay_ms(5000);
	while(rec != '\r'){
		rec = recive();
		send(rec);
	}
	rec = '\0';
}

void send_to_server(int temp_val, int humi_val){	 //this function sends data to server and sends a reset command
													 //this function has a total delay of 23 seconds
	int i = 0;										//this should be inside an infinite while loop to send continuesly
 	char get_req[100];
	
	delay_ms(6000);
	send_str("AT\r\n");
	delay_ms(1000);
	while(rec != '\r'){
		rec = recive();
		send(rec);
	}
	rec = '\0';
		
	delay_ms(3000);
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	send_str("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80\r\n"); //this should be the ip address of the server
	delay_ms(3000);
	while(rec != '\r'){
		rec = recive();
		send(rec);
	}
	rec = '\0';


	send_str("AT+CIPSEND=100\r\n"); //aprox size of the chars send through wifi (this is the size of the write link) 
	//delay(1000);
	while(rec != '\r'){
		rec = recive();
		send(rec);
	}
	rec = '\0';

	sprintf(get_req, "GET http://127.0.0.1:5000/update_data?temp=%d&humi=%d\r\n", temp_val, humi_val);  //Get request for server
	
	send_str(get_req);  
	
	delay_ms(3000);
	while(rec != '\r'){
		rec = recive();
		send(rec);
	}
	rec = '\0';
	
	for(i = 0; i < 25; i++){ //reason for this is still unknown
		send('a');
	}
	
	
	delay_ms(5000);
	
	while(rec != '\r'){
		rec = recive();
		send(rec);
	}
	rec = '\0';
	
	send_str("AT+RST\r\n"); //Data is only repetedly send after a reset. it is not clear why and how to avoid it
	delay_ms(2000);

}

////////////////////////////////////////DH11//////////////////////

void dht11_request(void)
{
	IO0DIR = IO0DIR | 0x00000010;	/* Configure DHT11 pin as output (P0.4 used here) */
	IO0PIN = IO0PIN & 0xFFFFFFEF; /* Make DHT11 pin LOW for minimum 18 seconds */
	delay_ms(20);
	IO0PIN = IO0PIN | 0x00000010; /* Make DHT11 pin HIGH and wait for response */
	//delay_us(40);
}

void dht11_response(void)
{  
	IO0DIR = IO0DIR & 0xFFFFFFEF;	/* Configure DHT11 pin as output */
	while( IO0PIN & 0x00000010 );	/* Wait till response is HIGH */
//send_str("hi");
	while( (IO0PIN & 0x00000010) == 0 );	/* Wait till response is LOW */
//send_str("hi");
	while( IO0PIN & 0x00000010 );	/* Wait till response is HIGH */	/* This is end of response */
}

uint8_t dht11_data(void)
{
	int8_t count;
	uint8_t data = 0;
	for(count = 0; count<8 ; count++)	/* 8 bits of data */
	{
		while( (IO0PIN & 0x00000010) == 0 );	/* Wait till response is LOW */
		delay_us(30);	/* delay greater than 24 usec */
		if ( IO0PIN & 0x00000010 ) /* If response is HIGH, 1 is received */
			data = ( (data<<1) | 0x01 );
		else	/* If response is LOW, 0 is received */
			data = (data<<1);
		while( IO0PIN & 0x00000010 );	/* Wait till response is HIGH (happens if 1 is received) */
	}
	return data;
}

int main(){
	uint8_t humidity_integer, humidity_decimal, temp_integer, temp_decimal, checksum; 
	
	init_pll();
	init_uart(9600);
	delay_ms(2000);
	//delay_ms(3000);
	connect_wifi();
	while(1){
		
		delay_ms(500);
		
		dht11_request();
		dht11_response();
		
		humidity_integer = dht11_data();
		humidity_decimal = dht11_data();
		temp_integer = dht11_data();
		temp_decimal = dht11_data();
		checksum = dht11_data();

	   	if( (humidity_integer + humidity_decimal + temp_integer + temp_decimal) == checksum ){
			
			

			send_to_server(temp_integer, humidity_integer);

		 	/* sprintf(temp_str, "temprature = %d%\r", temp_integer);
			 sprintf(humi_str, "humidity = %d%", humidity_integer);

			 send_str(temp_str);
			 send_str(humi_str);
			 send_str(" \r");*/
		}
	}
}
