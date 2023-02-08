#include "mbed.h"
#include "DHT.h"
#include "Watchdog.h"
#include "globals.h"
#include "main.h"
#include "math.h"
#include  "IAP.h"
#include "iostream"

// Main program
int main() {
       //try {
        //   }
        //   catch(int e) {}
    if (!BackupIN) {    
        // Indicate to backup MCU that this MCU is in control
        BackupOUT = 1; 
        pc.printf("This is primary system.\r\n");
    }
    else {
        BackupOUT = 0;    
        //RelayFanOUT = InaroundFanOUT;   
        //RelayFanIN = InaroundFanIN;
        //RelayHeater = InaroundHeater;
        while (BackupIN) {
            pc.printf("In backup mode.\r\n");
            wd.Service();       // Slap that poor dog..
            wait(2);
        }
    }
        
    BackupOUT = 1; 
    
    // Watchdog timer caused the reset of the system
    if (wd.WatchdogCausedReset())  {
        pc.printf("Watchdog caused reset.\r\n");
        transfertoRepairMode();
    }
    
    wait(1);
    readTHSensors();        // Read T&H sensors
    wait(1);
    findAvgTH();
    //actuateRange();         // Turn on actuators if necessary
        
    //wd.Service();       // Slap that poor dog..
    Vibration0.rise(&count0);  // attach the address of the flip function to the rising edge 
    Vibration1.rise(&count1);  // attach the address of the flip function to the rising edge     
        
    wd.Configure(10.0);       // sets the timeout interval
    
    int i = 0;
    // IMPLEMENT WAIT LOOP TO SLAP DOG
    while(1) {
        //while(1); // Watchdog timer test
        
        //try {}
        //catch(int e) {}    

        if(counter1>= 30){
            pc.printf("Excessive vibration at fan OUT detected.\r\n");
            actuateControl();

        }
        if(counter0>= 30){
            pc.printf("Excessive vibration at fan IN detected.\r\n");
            actuateControl();
        }
        counter1=reset;
        counter0=reset;

        if (BackupIN) {
            transfertoRepairMode();
        }
        // Mishap mitigation technique: Check registers
        if (!checkRegisters()) {
            pc.printf("Register test failed.\r\n"); 
            transfertoRepairMode();
        }
        
        // Mishap mitigation technique: Check memory
        if (!checkMem()) {
            pc.printf("Memory test failed.\r\n");
            transfertoRepairMode();
        }
        
        wd.Service();       // Slap that poor dog..
        readTHSensors();      // Read T&H sensors
        findAvgTH();
        actuateRange();
        checkSafetyRange();
        
        
        /*/ -------------------------- COMMENT THIS OUT
        if(i == 0) {
            pc.printf("Heater on.\r\n");
            RelayHeater = 0;
            RelayFanIN = 1;
            RelayFanOUT = 1;
        }
        else if (i == 1) {
            pc.printf("Fan IN on.\r\n");
            RelayHeater = 1;
            RelayFanIN = 0;
            RelayFanOUT = 1;
        }
        else if (i==2) {
            pc.printf("Fan OUT on.\r\n");
            RelayHeater = 1;
            RelayFanIN = 1;
            RelayFanOUT = 0;
            }
        else if(i == 3) {
            
            i = -1;
        }
        i++;
       /*/ -------------------------- COMMENT THIS OUT ^^^^^
        wait(3.0f);         // Wait five seconds per loop
        wraparoundTests();
        inaroundTests();
         wait(3.0f);         // Wait five seconds per loop
        }
}

void findAvgTH() { 
    // VERIFY THAT BOTH SENSORS ARE WORKING
    
    // both sensors failed
    if (error0 != 0 && error1 != 0) {
        pc.printf("Both sensors have failed. Engaging control mechanism.\r\n");
        actuateControl();
    }
    else if (error0 != 0) { // Sensor 0 broken
        tavg = t[1];
        havg = h[1];
    }
    else if (error1 != 0) { // Sensor 1 broken
        tavg = t[0];
        havg = h[0];
    }
    else {          
        tavg = (t[0] + t[1]) / 2;
        havg = (h[0] + h[1]) / 2;
    }
    pc.printf("Average T: %f, Average H: %f\r\n", tavg, havg);      
}


void checkSafetyRange() {
    if (havg >= MAXHUM || havg <= MINHUM || tavg <=MINTEMP || tavg >=MAXTEMP) {
        pc.printf("Conditions outside of range. Actuating control mechanism.\r\n");
        pc.printf("havg: %f!!, tavg: %f!!\r\n", havg, tavg);
        actuateControl();
    }
}

void inaroundTests() {

    if (InaroundFanIN != RelayFanIN) {
        pc.printf("Fan IN inaround failed.\r\n");
    }
    
    if (InaroundFanOUT != RelayFanOUT) {
        pc.printf("Fan OUT inaround failed.\r\n");
    }
    
    if (InaroundHeater != RelayHeater) {
        pc.printf("Heater inaround failed.\r\n");
    }
}

void actuateRange() {
    actuatefanOUT();
    actuateResistor();
    actuatefanIN();
    pc.printf("Fan IN: %s\r\n", RelayFanIN ? "OFF" : "ON");
    pc.printf("Fan OUT: %s\r\n", RelayFanOUT ? "OFF" : "ON");
    pc.printf("Heater: %s\r\n", RelayHeater ? "OFF" : "ON");
}

void actuatefanOUT() {
    
    // turn fan out on
    if ((tavg > MAXTEMP - 5|| havg > MAXHUM - 10) && RelayFanOUT != 1) {
        pc.printf("Turning fan OUT on.\r\n");
        RelayFanOUT = 0;
    }
    
    // turn fan out off
    else if (havg <= MAXHUM - 10 && tavg <= MAXTEMP - 10 && RelayFanOUT == 0) {
        pc.printf("Turning fan OUT off.\r\n");
        RelayFanOUT = 1;
    }  
}

void actuateResistor() {
    // turn heater on
    if ((tavg <= MINTEMP + 5 || havg > MAXHUM - 10) && RelayHeater == 1) {
        pc.printf("Turning heater on.\r\n");
        RelayHeater = 0;
    }
    // turn heater off
    else if (tavg > MINTEMP + 5 && havg <= MAXHUM - 10 && RelayHeater == 0) {
        RelayHeater = 1;
        pc.printf("Turning heater off.\r\n");
    }
}

// Turn fan IN to value determined by input
void actuatefanIN() {
    if (havg <= MINHUM + 5 && RelayFanIN == 1) {
        pc.printf("Turning fan IN on.\r\n");
        RelayFanIN = 0;
    }

    else if (havg > MINHUM + 5 && RelayFanIN == 0) { 
        pc.printf("Turning fan IN off.\r\n");
        RelayFanIN = 1;
    }
}
  
// Turn control lights to value determined by input  
void actuateControl() {
    ControlLED = 1;
    AccessLED = 1;
    while (1) {
        pc.printf("AGENT CONTROL MECHANISM ENGAGED: DO NOT ENTER GREENHOUSE!!!\r\n");
        int tmp1 = ControlLED; int tmp2 = AccessLED;
        pc.printf("Control: %d, Access: %d\r\n", tmp1, tmp2);
        wd.Service();       // Slap that poor dog..
        wait(3);
    }
}

// Wraparound tests for Fan IN, Fan OUT, and Resistor
int wraparoundTests() {
    int success = 1;
    float a = WrapFanIN;        
    float b = WrapFanOUT;
    
    if (RelayFanIN == (WrapFanIN > 0.005f)) {
        success = 0;
        int tmp1 = RelayFanIN;
        pc.printf("Fan IN wraparound failed: %f %d\r\n", a, tmp1);
    }
      
    if (RelayFanOUT == (WrapFanOUT > 0.005f)) {
        success = 0;
        int tmp2 = RelayFanOUT;
        pc.printf("Fan OUT wraparound failed %f %d\r\n", b, tmp2);
    }   
    
    return success;
}

// Transfer control to backup MCU
void transfertoRepairMode() {  
    BackupOUT = 0; 
    pc.printf("Failure occurred: Transferring control to backup.\r\n");
    // Stuck in this loop indicates that the backup MCU is in control
    while(1) {
        pc.printf("In failure mode.\r\n");
        wd.Service();       // Slap that poor dog..
        wait(1);
    }
}
    
// Calculate solution to known equations to verify that registers
// are performing correctly
int checkRegisters() {
    //try {
    double solutiontan = tan(atan(tan(atan(0.277)))); // Change this for ALU test
    long solutionfact = factorial(12);
    //}
    //catch(...) {
     //       pc.printf("Error in calculation: Most likely calculating tan(pi)");
     //       transfertoRepairMode();

     //   }
    if (solutiontan != TANSOLUTION || solutionfact != FACTSOLUTION) {
        return 0; 
        }
    return 1;
}

// Read two temperature and humidity sensors. Stores value in global array
void readTHSensors() {
    error0 = HT0.readData();
    if(error0 == 0) {
        t[0] = HT0.ReadTemperature(CELCIUS);
        h[0] = HT0.ReadHumidity() + 50.0f;
        pc.printf("T: %f, H: %f\r\n", t[0], h[0]);
    }
    else {      
        pc.printf("Failure Reading Sensor 0 (Exit code %d) \r\n", error0);
    }
        
    error1 = HT1.readData();
    if(error1 == 0) {
        t[1] = HT1.ReadTemperature(CELCIUS);
        h[1] = HT1.ReadHumidity() + 50.0f;
        pc.printf("T: %f, H: %f\r\n", t[1], h[1]);
    }
    else {
        pc.printf("Failure Reading Sensor 1 (Exit code %d) \r\n", error1);
    }
    return;
}   

void count0() {   
// Kind of a try-catch test
    if (counter0 != 2147483647) counter0++;
    else pc.printf("Vibration sensor exceeded max int size.\r\n");
}

void count1() { 
// Kind of a try-catch test
    if (counter1 != 2147483647) counter1++;
    else pc.printf("Vibration sensor exceeded max int size.\r\n");
}
   
// Taken from https://www.programmingsimplified.com/c-program-find-factorial#:~:text=Factorial%20program%20in%20C%20using%20a%20for%20loop%2C,is%20defined%20as%20one%2C%20i.e.%2C%200%21%20%3D%201.
// Finds factorial of input

long factorial(int n)
{
  int c;
  long r = 1;

  for (c = 1; c <= n; c++)
    r = r * c;

  return r;
}


/**    IAP demo : demo code for internal Flash memory access library
 *
 *        The internal Flash memory access is described in the LPC1768 and LPC11U24 usermanual.
 *            http://www.nxp.com/documents/user_manual/UM10360.pdf
 *            http://www.nxp.com/documents/user_manual/UM10462.pdf
 *
 *               LPC1768 --
 *                    Chapter  2: "LPC17xx Memory map"
 *                    Chapter 32: "LPC17xx Flash memory interface and programming"
 *                    refering Rev. 01 - 4 January 2010
 *
 *               LPC11U24 --
 *                    Chapter  2: "LPC11Uxx Memory mapping"
 *                    Chapter 20: "LPC11Uxx Flash programming firmware"
 *                    refering Rev. 03 - 16 July 2012
 *
 *  This main.cpp demonstrates how the flash can be erased and wrote.
 *
 *  This program tries to...
 *    0. read device ID and serial#
 *    1. check if the targat sector blank
 *    2. erase the sector if it was not blank
 *    3. write into the flash (prepare before write)
 *    4. verify the data by IAP command
 *    5. show the content of the flash
 *
 *  The Flash must be erased as sectors. No overwrite can be done like SRAM.
 *  So erase should be done in size of 4K or 32K.
 *
 *  Writing sector can be done with size of 256, 512, 1024 or 4096.
 *  If other size is used, the IAP returns an error.
 *  The SRAM memory should be allocated in
 *
 *
 *        Released under the MIT License: http://mbed.org/license/mit
 *
 *        revision 1.0  09-Mar-2010   1st release
 *        revision 1.1  12-Mar-2010   chaged: to make possible to reserve flash area for user
 *                                            it can be set by USER_FLASH_AREA_START and USER_FLASH_AREA_SIZE in IAP.h
 *        revision 2.0  26-Nov.2012   LPC11U24 code added
 *        revision 2.1  26-Nov-2012   EEPROM access code imported from Suga koubou san's (http://mbed.org/users/okini3939/) library
 *                                            http://mbed.org/users/okini3939/code/M0_EEPROM_test/
 *        revision 3.0  09-Jan-2015   LPC812 and LPC824 support added
 *        revision 3.1  13-Jan-2015   LPC1114 support added
 *        revision 3.1.1 16-Jan-2015  Target MCU name changed for better compatibility across the platforms
 *        revision 3.1.2 10-Mar-2015  merged with pull requests. reinvoke_isp() added and modified read_serial() to return a pointer.
 *        revision 3.1.2.1 15-Aug-2018  added: just minor feature in "memdump()" function
 *        revision 3.1.3   16-Aug-2018  "write page" function demo for LPC81X/LPC82X
 */

void    memdump( char *p, int n );
int     write_page( char *src, int target_page );

IAP     iap;

// Taken from IAP library -- not our code
int checkMem()
{
    char    mem[ MEM_SIZE ];    //  memory, it should be aligned to word boundary
    int     *serial_number;
    int     r;

    printf( "\r\n\r\n=== IAP: Flash memory writing test ===\r\n" );
    printf( "  device-ID = 0x%08X\r\n", iap.read_ID() );

    serial_number = iap.read_serial();

    printf( "  serial# =" );
    for ( int i = 0; i < 4; i++ )
        printf( " %08X", *(serial_number + i) );
    printf( "\r\n" );

    printf( "  CPU running %dkHz\r\n", SystemCoreClock / 1000 );
    printf( "  user reserved flash area: start_address=0x%08X, size=%d bytes\r\n", iap.reserved_flash_area_start(), iap.reserved_flash_area_size() );
    printf( "  read_BootVer=0x%08X\r\r\n", iap.read_BootVer() );

    for ( int i = 0; i < MEM_SIZE; i++ )
        mem[ i ]    = i & 0xFF;

    //  blank check: The mbed will erase all flash contents after downloading new executable

    r   = iap.blank_check( TARGET_SECTOR, TARGET_SECTOR );
    printf( "blank check result = 0x%08X\r\n", r );

    //  erase sector, if required

    if ( r == SECTOR_NOT_BLANK ) {
        iap.prepare( TARGET_SECTOR, TARGET_SECTOR );
        r   = iap.erase( TARGET_SECTOR, TARGET_SECTOR );
        printf( "erase result       = 0x%08X\r\n", r );
    }

    // copy RAM to Flash

    iap.prepare( TARGET_SECTOR, TARGET_SECTOR );
    r   = iap.write( mem, sector_start_adress[ TARGET_SECTOR ], MEM_SIZE );
    pc.printf( "copied: SRAM(0x%08X)->Flash(0x%08X) for %d bytes. (result=0x%08X)\r\n", mem, sector_start_adress[ TARGET_SECTOR ], MEM_SIZE, r );

    // compare

    r   = iap.compare( mem, sector_start_adress[ TARGET_SECTOR ], MEM_SIZE );
    printf( "compare result     = \"%s\"\r\n", r ? "FAILED" : "OK" );
    
    return !r;

//#define WRITE_NEXT_BLOCK
#ifdef WRITE_NEXT_BLOCK

    // copy RAM to Flash

    iap.prepare( TARGET_SECTOR, TARGET_SECTOR );
    r   = iap.write( mem, sector_start_adress[ TARGET_SECTOR ] + 256, MEM_SIZE );
    pc.printf( "copied: SRAM(0x%08X)->Flash(0x%08X) for %d bytes. (result=0x%08X)\r\n", mem, sector_start_adress[ TARGET_SECTOR ], MEM_SIZE, r );

    // compare

    r   = iap.compare( mem, sector_start_adress[ TARGET_SECTOR ] + 256, MEM_SIZE );
    pc.printf( "compare result     = \"%s\"\r\n", r ? "FAILED" : "OK" );
#endif

/*

    pc.printf( "showing the flash contents...\r\n" );
    memdump( sector_start_adress[ TARGET_SECTOR ], MEM_SIZE * 3 );


#if defined(TARGET_LPC81X) || defined(TARGET_LPC82X)
    iap.prepare( TARGET_SECTOR, TARGET_SECTOR );
    r   = iap.erase_page( 241, 241 );   //  241 is page number for sector 7 with 64 byte offset

    pc.printf( "\r\nerase page test\r\n" );
    pc.printf( "erase page result     = \"%s\"\r\n", r ? "FAILED" : "OK" );
    pc.printf( "showing memory dump to confirm 0x00003C40 to 0x00003C7F are erased\r\n(should be changed to 0xFF)\r\n" );

    memdump( sector_start_adress[ TARGET_SECTOR ], MEM_SIZE );


    char test_data[ 64 ];

    for ( int i = 0; i < 64; i++ )
        test_data[ i ]  = 0xAA;

    r   = iap.write_page( test_data, 241 );   //  241 is page number for sector 7 with 64 byte offset

    pc.printf( "\r\nwrite page test\r\n" );
    pc.printf( "write page result     = \"%s\"\r\n", r ? "FAILED" : "OK" );
    pc.printf( "showing memory dump to confirm 0x00003C40 to 0x00003C7F are written\r\n(should be changed to 0xAA)\r\n" );

    memdump( sector_start_adress[ TARGET_SECTOR ], MEM_SIZE );

#endif


#if defined(TARGET_LPC11UXX)    //  SAMPLE OF EEPROM ACCESS (LPC11U24 only)
    printf( "IAP: EEPROM writing test\r\n" );
    char    mem2[ MEM_SIZE ];

    r   = iap.write_eeprom( mem, (char*)TARGET_EEPROM_ADDRESS, MEM_SIZE );
    pc.printf( "copied: SRAM(0x%08X)->EEPROM(0x%08X) for %d bytes. (result=0x%08X)\r\n", mem, TARGET_EEPROM_ADDRESS, MEM_SIZE, r );

    r   = iap.read_eeprom( (char*)TARGET_EEPROM_ADDRESS, mem2, MEM_SIZE );
    pc.printf( "copied: EEPROM(0x%08X)->SRAM(0x%08X) for %d bytes. (result=0x%08X)\r\n", TARGET_EEPROM_ADDRESS, mem, MEM_SIZE, r );

    // compare
    r = memcmp(mem, mem2, MEM_SIZE);
    pc.printf( "compare result     = \"%s\"\r\n", r ? "FAILED" : "OK" );
    pc.printf( "showing the EEPROM contents...\r\n" );
    memdump( mem2, MEM_SIZE );
#endif

    pc.printf( "Done! (=== IAP: Flash memory writing test ===)\r\n\r\n" );
    */
}

#include    <ctype.h>

void memdump( char *base, int n )
{
    unsigned int    *p;
    char            s[17]   = { '\x0' };

    pc.printf( "  memdump from 0x%08X for %d bytes", (unsigned long)base, n );

    p   = (unsigned int *)((unsigned int)base & ~(unsigned int)0x3);

    for ( int i = 0; i < (n >> 2); i++, p++ ) {
        if ( !(i % 4) ) {
            pc.printf( " : %s\r\n  0x%08X :", s, (unsigned int)p );

            for ( int j = 0; j < 16; j++)
                s[ j ]  = isgraph( (int)(*((char *)p + j)) ) ? (*((char *)p + j)) : ' ';
        }
        pc.printf( " 0x%08X", *p );
    }
    pc.printf( " : %s\r\n", s );

}
