/// @file Watchdog.cpp provides the interface to the Watchdog module
///
/// This provides basic Watchdog service for the mbed. You can configure
/// various timeout intervals that meet your system needs. Additionally,
/// it is possible to identify if the Watchdog was the cause of any 
/// system restart.
/// 
/// Adapted from Simon's Watchdog code from http://mbed.org/forum/mbed/topic/508/
///
/// @note Copyright &copy; 2011 by Smartware Computing, all rights reserved.
///     This software may be used to derive new software, as long as
///     this copyright statement remains in the source file.
/// @author David Smart
///
#include "mbed.h"
#include "Watchdog.h"

/// Watchdog gets instantiated at the module level
Watchdog::Watchdog() {
    wdreset = (LPC_WDT->WDMOD >> 2) & 1;    // capture the cause of the previous reset
}

/// Load timeout value in watchdog timer and enable
void Watchdog::Configure(float s) {
    LPC_WDT->WDCLKSEL = 0x1;                // Set CLK src to PCLK
    uint32_t clk = SystemCoreClock / 16;    // WD has a fixed /4 prescaler, PCLK default is /4
    LPC_WDT->WDTC = (uint32_t)(s * (float)clk);
    LPC_WDT->WDMOD = 0x3;                   // Enabled and Reset
    Service();
}

/// "Service", "kick" or "feed" the dog - reset the watchdog timer
/// by writing this required bit pattern
void Watchdog::Service() {
    LPC_WDT->WDFEED = 0xAA;
    LPC_WDT->WDFEED = 0x55;
}

/// get the flag to indicate if the watchdog causes the reset
bool Watchdog::WatchdogCausedReset() {
    return wdreset;
}
