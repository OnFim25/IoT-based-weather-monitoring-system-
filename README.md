# IoT based weather monitoring system

This is an IoT based weather monitoring system using a DHT11 sensor to read temprature and humidity and 
store it on a mongoDB database for analysis

Components used:
1) ARM lpc2148 microcontroller
2) DHT11 temprature and humidity sensor
3) ESP-01 WiFi module
4) 1x 5k Resistor
5) Bread board
6) Jumper wires

Before powering up:
1) start by running the server code in python and copy the IP address the server is running on
2) replace the IP address at the action part of the clear all button inside the List_data.html file with server IP using Notepad
3) restart the server
4) change the IP address on the C-code of the arm controller also. and compile the code.
5) upload the code to ARM controller
6) connect the data pin of the DHT11 sensor to P0.4 on the microcontroller with a 5k pullup resister
NB: Make sure to connect both the server and the ESP01 on the same network.

working:
The controller asks for the DHT11 sensor to send sensor data and checks for checksum errors
if no checksum errors are present, the data is added onto a GET request and is send serialy to the ESP01 module. this request is recived by the server 
and stores the data and time in the mongoDB database.
The the contorller sends data to the server almost every 24.5 seconds (this is the minimum possible time between each data upload since ESP01 takes time to send data)

To view the data:
The data is uploaded through the get request http://<IP address of the server>:<port address>/upload_data?temp=<temprature data>&humi=<humidity data>
The data on the database can be accessed by   http://<IP address of the server>:<port address>/show_data  
