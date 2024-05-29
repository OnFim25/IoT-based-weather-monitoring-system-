#include <lpc214x.h>
#include <stdio.h>
#include <stdint.h>

#define DHT11_PIN (1 << 16)
#define pclk 60000000

char rec = '\0';

// intialize clock freequency to 60MHz from Oscillator freeq of 12MHz
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
// Initialize UART with baude rate
void init_uart(int baud){
	unsigned int val = pclk / (16 * baud);
	PINSEL0 |= 0x00000005;

	U0LCR = 0X83;
	U0FDR = 0X10;
	U0DLM = (val >> 8)  & 0xff;
	U0DLL = val & 0xff;
	U0LCR = 0X03;
}

//send char 
void send(char ch){
	U0THR = ch;
	while(!(U0LSR & (1 << 5)));
}

//recive char
char recive(){
	char ch;
	while(!(U0LSR & 0X01));
	ch = U0RBR;
	return ch;
	
}

//send string
void send_str(char * str){
	int i = 0;
	for(i = 0; str[i] != '\0'; i++){
		send(str[i]);
	}
}
//===========================Timer delay part====================//

//millisecond delay
void delay_ms(unsigned int val){
	T0CTCR = 0X00;
	T0PR = 60000 - 1;
	T0TCR = 0X01;
	T0TC = 0;
	while(T0TC < val );
	T0TCR = 0X00;
}

//microsecond delay
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

//this function waits for an "\r" form the keyboard
void wait_AT(){
		while(rec != '\r'){
		rec = recive();
		send(rec);
	}
	rec = '\0';
}
//////////////////////////////////////////

// Initialize ESP-01 to connect to WiFi
void connect_wifi(){
   	send_str("AT\r\n");      //AT
	delay_ms(1000);
	//wait_AT();
	
	send_str("AT+CWJAP=\"Quest_10\",\"aCBQISclt@202!\"\r\n"); // WIFI Connection name and password to connect
	delay_ms(5000);	// During intialization, the code waits for 11 seconds (5 + 6) to connect to WiFi
}														// After Initialization, the code waits for only 6 seconds from the send_to_server()

//Function to send temprature and humidity data to server 
void send_to_server(int temp_val, int humi_val){	 
																						//this function has a total delay of 23 seconds
	int i = 0;									
 	char get_req[100];
	
	delay_ms(6000); // wait for reconnection
	send_str("AT\r\n");
	
		
	delay_ms(3000);
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	send_str("AT+CIPSTART=\"TCP\",\"127.0.0.1\",5000\r\n"); //this should be the ip address of the server
	delay_ms(3000);
	//wait_AT();


	send_str("AT+CIPSEND=73\r\n"); //size of the chars send in get request. this value should be >= size of the GET request
	delay_ms(2000);
	

	sprintf(get_req, "GET /upload_data?temp=%d&humi=%d HTTP/1.1\r\n", temp_val, humi_val);  //making the GET request with sensor values
	
	send_str(get_req);  
	send_str("Host: 127.0.0.1\r\n\r\n");//send the GET request along with IP address of the server(Host)
	delay_ms(3000);
	
	
	for(i = 0; i < 10; i++){ //If value in AT+CIPSEND > size of GET request. this loop fills the remaining spaces for the GET request
		send('a');									//this loop is not neccessery if AT+CIPSEND = size of GET request	
	}
	
	
	delay_ms(5000);
	
	//wait_AT();
	
	send_str("AT+CIPCLOSE\r\n"); // manually close the current connection to restart again
	//send_str("AT+RST\r\n"); // if close is not restarting the connection, the use this command 
	delay_ms(2000);

}

////////////////////////////////////////DH11 communication//////////////////////
//function to send start signal to dht11
void dht11_request(void)
{
	IO0DIR = IO0DIR | 0x00000010;	/* Configure DHT11 pin as output (P0.4 used here) */
	IO0PIN = IO0PIN & 0xFFFFFFEF; /* Make DHT11 pin LOW for atleast 18 milliseconds */
	delay_ms(20);
	IO0PIN = IO0PIN | 0x00000010; /* Make DHT11 pin HIGH and wait for response */
}

//function to wait for the resoponce from DHT11
void dht11_response(void)
{  
	IO0DIR = IO0DIR & 0xFFFFFFEF;	/* Configure DHT11 pin as output */
	while( IO0PIN & 0x00000010 );	/* Wait till response is HIGH */

	while( (IO0PIN & 0x00000010) == 0 );	/* Wait till response is LOW */

	while( IO0PIN & 0x00000010 );	/* Wait till response is HIGH */	/* This is end of response */
}

//function to recive data from the DHT11
//the recived data will be an 8bit unsigned int (uint8_t) datatype from stdint.h
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

//======================Main Function==================//
int main(){
	uint8_t humidity_integer, humidity_decimal, temp_integer, temp_decimal, checksum; 
	
	init_pll();
	init_uart(9600);
	
	delay_ms(2000); //after powering up, wait for at least 1 second for DHT11 to enter sleep mode
	connect_wifi();
	while(1){
		
		delay_ms(500);
		
		dht11_request();
		dht11_response();
		
		humidity_integer = dht11_data();
		humidity_decimal = dht11_data();
		temp_integer = dht11_data();
		temp_decimal = dht11_data();
		checksum = dht11_data(); //all data bytes must be read to check data integrity and to also recive accurate data from DHT11

	  if( (humidity_integer + humidity_decimal + temp_integer + temp_decimal) == checksum ){ //send data to server only if there are no checksum errors
				send_to_server(temp_integer, humidity_integer); 
		}	
		
		// Data is read and send to server aproximatly every 24.5 seconds
	}
}
