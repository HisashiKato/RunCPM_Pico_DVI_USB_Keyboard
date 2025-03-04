// SPDX-FileCopyrightText: 2023 Mockba the Borg
//
// SPDX-License-Identifier: MIT

/*
  SD card connection

  This example shows how to read and write data to and from an SD card file
  The circuit:
   SD card attached to SPI bus as follows:
   // Arduino-pico core
   ** SCK  - Pin 4 - GPIO 2
   ** MOSI - Pin 5 - GPIO 3
   ** MISO - Pin 6 - GPIO 4
   ** CS   - Pin 7 - GPIO 5
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
#define GL_REV "GL20250304.0"

#include <SPI.h>
#include <SdFat.h>        // One SD library to rule them all - Greinman SdFat from Library Manager

// =========================================================================================
// Board definitions go into the "hardware" folder, if you use a board different than the
// Arduino DUE, choose/change a file from there and reference that file here
// =========================================================================================

// Raspberry Pi Pico - normal (LED = GPIO25)
#include "hardware/pico/pico_sd_spi_dvi_usbkey.h"

// =========================================================================================
// Startup Messages Text (please write it yourself)
// =========================================================================================
#define PICO_OR_W "Pico 2"
#define PICOCORE_VER "v0.0.0"
#define SDFAT_VER "v0.0.0"
#define PICODVI_VER "v0.0.0"
#define TINYUSB_VER "v0.0.0"


// =========================================================================================
// Delays for LED blinking
// =========================================================================================
#define sDELAY 200
#define DELAY  400

#include "abstraction_arduino.h"

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
  digitalWrite(LED, LOW);

// =========================================================================================
// Serial Port Definition
// =========================================================================================
//  Serial =USB / Serial1 =UART0/COM1 / Serial2 =UART1/COM2

    Serial1.setRX(1); // Pin 2
    Serial1.setTX(0); // Pin 1

//  Serial2.setRX(21); // Pin 27
//  Serial2.setTX(20); // Pin 26
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
  while (!Serial1) {    // Wait until serial1 is connected
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
  _puts("           Revision         [" TEXT_BOLD "");
  _puts(GL_REV);
  _puts("" TEXT_NORMAL "]\r\n");

  _puts("----------------------------------------------\r\n");

  _puts("BIOS              at [" TEXT_BOLD "0x");
  _puthex16(BIOSjmppage);
  _puts("" TEXT_NORMAL "]\r\n");

  #ifdef ABDOS
  _puts("ABDOS.SYS " TEXT_BOLD "enabled" TEXT_NORMAL " at [" TEXT_BOLD "0x");
  _puthex16(BDOSjmppage);
  _puts("" TEXT_NORMAL "]\r\n");
  #else
  _puts("BDOS              at [" TEXT_BOLD "0x");
  _puthex16(BDOSjmppage);
  _puts("" TEXT_NORMAL "]\r\n");
  #endif

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
  SPI.setSCK(2);  // Clock
  SPI.setTX(3);   // MOSI
  SPI.setRX(4);   // MISO
  SPI.setCS(5);   // Card Select

// =========================================================================================
// Setup SD card writing settings
// Info at: https://github.com/greiman/SdFat/issues/285#issuecomment-823562829
// =========================================================================================

#undef SDFAT_FILE_TYPE
#define SDFAT_FILE_TYPE 1           // Uncomment for Due, Teensy or RPi Pico
#define ENABLE_DEDICATED_SPI 1      // Dedicated SPI 1=ON 0=OFF
#define SDMHZ_TXT "19"              // for outputing SDMHZ-Text
// normal is 12Mhz because of https://www.pschatzmann.ch/home/2021/03/14/rasperry-pico-with-the-sdfat-library/
#define SDMHZ 19                    // setting 19 Mhz for SPI-Bus
#define SS 5
// select required SPI-Bus : (&SPI) = SPI0 / (&SPI1) = SPI1
#define SD_CONFIG SdSpiConfig(SS, DEDICATED_SPI, SD_SCK_MHZ(SDMHZ), &SPI)
// #define SD_CONFIG SdSpiConfig(SS, DEDICATED_SPI, SPI_FULL_SPEED, &SPI)
// =========================================================================================


  _puts("Init MicroSD-Card    [   " TEXT_BOLD "");
//  old SDINIT
//  if (SD.begin(SDINIT)) {

// NEW SDCONFIG = formerly SDINIT
if (SD.begin(SD_CONFIG)) {
      _puts(SDMHZ_TXT);
      _puts("Mhz" TEXT_NORMAL "]\r\n");
      _puts("----------------------------------------------");


    if (VersionCCP >= 0x10 || SD.exists(CCPname)) {
#ifdef ABDOS
      _PatchBIOS();
#endif
      while (true) {
        _puts(CCPHEAD);
        _PatchCPM();
  Status = 0;

#ifdef CCP_INTERNAL
        _ccp();
#else
        if (!_RamLoad((uint8 *)CCPname, CCPaddr, 0)) {
          _puts("Unable to load the CCP.\r\nCPU halted.\r\n");
          break;
        }
        // Loads an autoexec file if it exists and this is the first boot
        // The file contents are loaded at ccpAddr+8 up to 126 bytes then the size loaded is stored at ccpAddr+7
        if (firstBoot) {
          if (_sys_exists((uint8*)AUTOEXEC)) {
            uint16 cmd = CCPaddr + 8;
            uint8 bytesread = (uint8)_RamLoad((uint8*)AUTOEXEC, cmd, 125);
            uint8 blen = 0;
            while (blen < bytesread && _RamRead(cmd + blen) > 31)
              blen++;
            _RamWrite(cmd + blen, 0x00);
            _RamWrite(--cmd, blen);
          }
          if (BOOTONLY)
            firstBoot = FALSE;
        }
        Z80reset();
        SET_LOW_REGISTER(BC, _RamRead(DSKByte));
        PC = CCPaddr;
        Z80run();
#endif
        if (Status == 1)
#ifdef DEBUG
    #ifdef DEBUGONHALT
                Debug = 1;
                Z80debug();
    #endif
#endif
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
