#ifndef MAIN_H
#define MAIN_H


// Functions
void readTHSensors();
void transfertoRepairMode();
int checkRegisters();
void actuateResistor();
void actuatefanOUT();
void actuatefanIN();
void actuateControl();
void actuateRange();
int wraparoundTests();
int checkMemory();
int checkMem();
void findAvgTH();
void count0();
void count1();
void checkSafetyRange();
void inaroundTests();

#endif