
#ifndef GLOBALS_H
#define GLOBALS_H

// Global definitions
#define EQUATIONSOLUTION 1
#define MAXTEMP 50
#define MINTEMP 40
#define MAXHUM 70
#define MINHUM 50
#define FACTSOLUTION 479001600
#define TANSOLUTION 0.277000
#define INITIAL_WAIT 0
#define MEM_SIZE 256
#define TARGET_SECTOR 29     //  use sector 29 as target sector if it is on LPC1768
// Global pins

InterruptIn Vibration0(p5); // Fan IN
InterruptIn Vibration1(p6); // Fan OUT

// Output to three relays with actuators
DigitalOut RelayFanOUT(p23);
DigitalOut RelayFanIN(p22);
DigitalOut RelayHeater(p21);

AnalogIn WrapFanIN(p16);
AnalogIn WrapFanOUT(p15);

DigitalIn InaroundFanIN(p19); 
DigitalIn InaroundFanOUT(p18); 
DigitalIn InaroundHeater(p20); 

// Communication with backup MCU
DigitalOut BackupOUT(p28);
DigitalIn BackupIN(p27);

// Outputs to LEDs for access control and agent control
DigitalOut AccessLED(p25);
DigitalOut ControlLED(p26);

// Temperature and Humidity sensors
DHT HT0(p29, DHT11);
DHT HT1(p30, DHT11);

// TX, RX (serial communication to screen)
Serial pc(p9, p10);  


// Global variables
float t[] = {0.0f, 0.0f};
float h[] = {0.0f, 0.0f};
float tavg = 0.0f;
float havg = 0.0f;
int error0 = 0;
int error1 = 0;
int transfer = 0;
int reset = 0;
int counter0 = 0; 
int counter1 = 0; 

// Define watchdog timer
Watchdog wd;

// Global functions
long factorial(int);


#endif