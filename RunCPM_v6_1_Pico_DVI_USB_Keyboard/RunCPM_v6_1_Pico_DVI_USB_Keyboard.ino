// SPDX-FileCopyrightText: 2023 Mockba the Borg
//
// SPDX-License-Identifier: MIT

/*
  SD card connection

  This example shows how to read and write data to and from an SD card file
  The circuit:
   SD card attached to SPI bus as follows:
   // Arduino-pico core
   ** MISO - Pin 21 - GPIO 16
   ** MOSI - Pin 25 - GPIO 19
   ** CS   - Pin 22 - GPIO 17
   ** SCK  - Pin 24 - GPIO 18
*/

// only AVR and ARM CPU
// #include <MemoryFree.h>

#include "globals.h"

//#define USE_VT100

#ifdef USE_VT100
#define TEXT_BOLD "\033[1m"
#define TEXT_NORMAL "\033[0m"
#else
#define TEXT_BOLD ""
#define TEXT_NORMAL ""
#endif

// =========================================================================================
// Guido Lehwalder's Code-Revision-Number
// =========================================================================================
#define GL_REV "GL20230716.0"

#include <SPI.h>

#define SPI_DRIVER_SELECT 0
#define USE_SPI_ARRAY_TRANSFER 1 // Pico Filesystem-Speedup?

#include <SdFat.h>        // One SD library to rule them all - Greinman SdFat from Library Manager
// #include <ESP8266SdFat.h>    // SdFat-version from RP2040 arduino-core

// =========================================================================================
// Board definitions go into the "hardware" folder, if you use a board different than the
// Arduino DUE, choose/change a file from there and reference that file here
// =========================================================================================

// Raspberry Pi Pico - normal (LED = GPIO25)
#include "hardware/pico/pico_sd_spi_dvi_usbkey.h"
//#include "hardware/pico/pico_sd_spi.h"

// Raspberry Pi Pico W(iFi)   (LED = GPIO32)
//#include "hardware/pico/pico_w_sd_spi.h"

// =========================================================================================
// Startup Messages Text (please write it yourself)
// =========================================================================================
#define PICO_OR_W " Pico "
#define PICOCORE_VER "v0.0.0"
#define SDFAT_VER "v0.0.0"
#define PICODVI_VER "v0.0.0"
#define TINYUSB_VER "v0.0.0"

/*
#ifndef BOARD_TEXT
#define BOARD_TEXT USB_MANUFACTURER " " USB_PRODUCT
#endif
*/

#include "abstraction_arduino.h"

// =========================================================================================
// Delays for LED blinking
// =========================================================================================
#define sDELAY 100
#define DELAY 1200

// =========================================================================================
// Serial port speed
// =========================================================================================
#define SERIALSPD 115200

// =========================================================================================
// PUN: device configuration
// =========================================================================================
#ifdef USE_PUN
File32 pun_dev;
int pun_open = FALSE;
#endif

// =========================================================================================
// LST: device configuration
// =========================================================================================
#ifdef USE_LST
File32 lst_dev;
int lst_open = FALSE;
#endif

#include "ram.h"
#include "console.h"
#include "cpu.h"
#include "disk.h"
#include "host.h"
#include "cpm.h"
#ifdef CCP_INTERNAL
#include "ccp.h"
#endif


void setup(void) {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW ^ LEDinv);
//digitalWrite(LED, LOW);

// =========================================================================================
// Serial Port Definition
// =========================================================================================
//   Serial =USB / Serial1 =UART0/COM1 / Serial2 =UART1/COM2

   Serial1.setRX(1); // Pin 2
   Serial1.setTX(0); // Pin 1

// Serial2.setRX(5); // Pin 7
// Serial2.setTX(4); // Pin 6

// or

//   Serial1.setRX(17); // Pin 22
//   Serial1.setTX(16); // Pin 21

//   Serial2.setRX(21); // Pin 27
//   Serial2.setTX(20); // Pin 26
// =========================================================================================

  // _clrscr();
  // _puts("Opening serial-port...\r\n");  


// =========================================================================================
// "port_init_early()" function performs initial settings for USB keyboard input 
// and DVI (HDMI display) output.
// "port_init_early()" function exists in the "pico_sd_spi_dvi_usbkey.h" file
// in "hardware\pico\" folder.
// =========================================================================================
  if (!port_init_early()) {
    return;
  }


  Serial1.begin(SERIALSPD);
#if defined(WAIT_SERIAL)
  while (!Serial1) {	// Wait until serial1 is connected
    digitalWrite(LED, HIGH^LEDinv);
    delay(sDELAY);
    digitalWrite(LED, LOW^LEDinv);
    delay(DELAY);
  }
#endif

#ifdef DEBUGLOG
  _sys_deletefile((uint8 *)LogName);
#endif

  
// =========================================================================================  
// Printing the Startup-Messages
// =========================================================================================

  _clrscr();

  // if (bootup_press == 1)
  //   { _puts("Recognized \e[1m#\e[0m key as pressed! :)\r\n\r\n");
  //   }
  
  _puts("CP/M Emulator " TEXT_BOLD "v" VERSION "" TEXT_NORMAL "   by   " TEXT_BOLD "Marcelo  Dantas" TEXT_NORMAL "\r\n");
  _puts("----------------------------------------------\r\n");  
  _puts("       running  on   Raspberry Pi [" TEXT_BOLD "");
  _puts(PICO_OR_W);
  _puts("" TEXT_NORMAL "]\r\n");
  _puts("       compiled with RP2040 Core  [" TEXT_BOLD "");
  _puts(PICOCORE_VER);
  _puts("" TEXT_NORMAL "]\r\n");  
  _puts("           Arduino   SDFat        [" TEXT_BOLD "");
  _puts(SDFAT_VER);
  _puts("" TEXT_NORMAL "]\r\n");  
  _puts("           Libraries PicoDVI      [" TEXT_BOLD "");
  _puts(PICODVI_VER);
  _puts("" TEXT_NORMAL "]\r\n");  
  _puts("                     TinyUSB      [" TEXT_BOLD "");
  _puts(TINYUSB_VER);
  _puts("" TEXT_NORMAL "]\r\n");
  _puts("----------------------------------------------\r\n");

  _puts("Revision             [" TEXT_BOLD "");
  _puts(GL_REV);
  _puts("" TEXT_NORMAL "]\r\n");

  _puts("BIOS              at [" TEXT_BOLD "0x");
  _puthex16(BIOSjmppage);
  _puts("" TEXT_NORMAL "]\r\n");

  _puts("BDOS              at [" TEXT_BOLD "0x");
  _puthex16(BDOSjmppage);
  _puts("" TEXT_NORMAL "]\r\n");

  _puts("CCP " CCPname " at [" TEXT_BOLD "0x");
  _puthex16(CCPaddr);
  _puts("" TEXT_NORMAL "]\r\n");

#if BANKS > 1
  _puts("Banked Memory        [" TEXT_BOLD "");
  _puthex8(BANKS);
  _puts("" TEXT_NORMAL "]banks\r\n");
#else
  _puts("Banked Memory        [" TEXT_BOLD "");
  _puthex8(BANKS);
  _puts("" TEXT_NORMAL "]bank\r\n");
#endif

//Serial1.printf("Free Memory          [\e[1m%d bytes\e[0m]\r\n", freeMemory());

  _puts("CPU-Clock            [" TEXT_BOLD "");
  _putdec((clock_get_hz( clk_sys ) + 500'000) / 1'000'000);
  _puts("Mhz" TEXT_NORMAL "]\r\n");

// =========================================================================================
// Redefine SPI-Pins - if needed : (SPI.) = SPI0 / (SPI1.) = SPI1
// =========================================================================================
  SPI.setRX(16);   // MISO
  SPI.setCS(17);   // Card Select
  SPI.setSCK(18);  // Clock
  SPI.setTX(19);   // MOSI

// =========================================================================================
// Setup SD card writing settings
// Info at: https://github.com/greiman/SdFat/issues/285#issuecomment-823562829
// =========================================================================================

// older SDINIT config
#define SDINIT SS, SD_SCK_MHZ(SDMHZ)

// #define SPI_SPEED SPI_FULL_SPEED           // full speed is 50Mhz - to fast for the most SDCard
// #define SPI_FULL_SPEED SD_SCK_MHZ(50)      // full speed is 50Mhz - to fast for the most SDCard

// older SD_CONFIG sample
// #define SD_CONFIG SdSpiConfig(5, DEDICATED_SPI, SPI_FULL_SPEED, &SPI)

// =========================================================================================
// NEW SD_CONFIG formerly SDINIT
// =========================================================================================
#define SDFAT_FILE_TYPE 1           // Uncomment for Due, Teensy or RPi Pico
#define ENABLE_DEDICATED_SPI 1      // Dedicated SPI 1=ON 0=OFF
#define SDMHZ_TXT "19"              // for outputing SDMHZ-Text
// normal is 12Mhz because of https://www.pschatzmann.ch/home/2021/03/14/rasperry-pico-with-the-sdfat-library/
#define SDMHZ 19                    // setting 19 Mhz for SPI-Bus
#define SS 17
// select required SPI-Bus : (&SPI) = SPI0 / (&SPI1) = SPI1
#define SD_CONFIG SdSpiConfig(SS, DEDICATED_SPI, SD_SCK_MHZ(SDMHZ), &SPI)
// #define SD_CONFIG SdSpiConfig(SS, DEDICATED_SPI, SPI_FULL_SPEED, &SPI)
// =========================================================================================


// =========================================================================================
// SPI SD Calibrate Test
// =========================================================================================
// int CALMHZ;
// CALMHZ=50;
// #define CALMHZ 50
// #define SD_CALIBRATE SdSpiConfig(SS, DEDICATED_SPI, SD_SCK_MHZ(CALMHZ), &SPI)
// _puts("While begin...\r\n");
// while (!SD.begin(SD_CALIBRATE) && CALMHZ > 1) {
// 
//   Serial.printf("SD-Access failed at MHZ:  %d \r\n", CALMHZ);
//   CALMHZ = CALMHZ - 1;  
//   #define SD_CALIBRATE SdSpiConfig(SS, DEDICATED_SPI, SD_SCK_MHZ(CALMHZ), &SPI)
//  
//                                  }
// Serial.printf("DEBUG: SD-Access failed at MHZ:  %d \r\n", CALMHZ);
// _puts("While end...\r\n");

  _puts("Init MicroSD-Card    [   " TEXT_BOLD "");
//  old SDINIT
//  if (SD.begin(SDINIT)) {

// NEW SDCONFIG = formerly SDINIT
if (SD.begin(SD_CONFIG)) {
      _puts(SDMHZ_TXT);
      _puts("Mhz" TEXT_NORMAL "]\r\n");
      _puts("----------------------------------------------");

                        
    if (VersionCCP >= 0x10 || SD.exists(CCPname)) {
      while (true) {
        _puts(CCPHEAD);
        _PatchCPM();
	Status = 0;
#ifndef CCP_INTERNAL
        if (!_RamLoad((char *)CCPname, CCPaddr)) {
          _puts("\r\n");
          _puts("Unable to load the CCP.\r\nCPU halted.\r\n");
          break;
        }
        Z80reset();
        SET_LOW_REGISTER(BC, _RamRead(DSKByte));
        PC = CCPaddr;
        Z80run();
#else
        _ccp();
#endif
        if (Status == 1)
          break;
#ifdef USE_PUN
        if (pun_dev)
          _sys_fflush(pun_dev);
#endif
#ifdef USE_LST
        if (lst_dev)
          _sys_fflush(lst_dev);
#endif
      }
    } else {
      _puts("\r\n");
      _puts("Unable to load CP/M CCP.\r\nCPU halted.\r\n");
    }
  } else {
    _puts("\r\n");
    _puts("Unable to initialize SD card.\r\nCPU halted.\r\n");
  }
}

void loop(void) {
  digitalWrite(LED, HIGH^LEDinv);
  delay(DELAY);
  digitalWrite(LED, LOW^LEDinv);
  delay(DELAY);
  digitalWrite(LED, HIGH^LEDinv);
  delay(DELAY);
  digitalWrite(LED, LOW^LEDinv);
  delay(DELAY * 4);
}
