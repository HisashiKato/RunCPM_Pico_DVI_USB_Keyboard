// SPDX-FileCopyrightText: 2023 Mockba the Borg
// SPDX-FileCopyrightText: 2023 Jeff Epler for Adafruit Industries
// SPDX-FileCopyrightText: 2024 Hisashi Kato
//
// SPDX-License-Identifier: MIT

#include <SdFat.h> // SDFat - Adafruit Fork
#include <PicoDVI.h>
#include <Adafruit_TinyUSB.h>
#include "../../console.h"
#include "../../arduino_hooks.h"

#include "keymapperUS.h"

#define USE_DISPLAY (1)
#define USE_KEYBOARD (1)


#ifndef USE_DISPLAY
#define USE_DISPLAY (0)
#endif

#ifndef USE_KEYBOARD
#define USE_KEYBOARD (0)
#endif

#ifndef USE_MSC
#define USE_MSC (0)
#endif


uint8_t getch_serial1(void) {
    while(true) {
        int r = Serial1.read();
        if(r != -1) {
            return r;
        }
    }
}

bool kbhit_serial1(void) {
    return Serial1.available();
}


#define USBH_KEY_BUFFER_SIZE 64

uint8_t usbhkbuf[USBH_KEY_BUFFER_SIZE];
uint8_t usbhkbufnum = 0;

bool usbhkbd_write(uint8_t code) {
    if (usbhkbufnum > USBH_KEY_BUFFER_SIZE) {
        return false;
    } else {
        usbhkbuf[usbhkbufnum] = code;
        usbhkbufnum++;
        return true;
    }
}

uint8_t usbhkbd_available(void) {
    if (usbhkbufnum == 0) return 0;
    else return usbhkbufnum;
}

int usbhkbd_read(void) {
    if (usbhkbufnum == 0) {
        return (-1);
    } else {
        usbhkbufnum-- ;
        uint8_t code = usbhkbuf[usbhkbufnum];
        return code;
    }
}

uint8_t getch_usbh(void) {
    while(true) {
        int r = usbhkbd_read();
        if(r != -1) {
            return r;
        }
    }
}

bool kbhit_usbh(void) {
    return usbhkbd_available();
}



// USB Keyboard
#if USE_KEYBOARD

#ifndef USE_TINYUSB_HOST
#error This sketch requires usb stack configured as host in "Tools -> USB Stack -> Adafruit TinyUSB Host"
#endif

#define KBD_INT_TIME 100 // USB HOST processing interval us

static repeating_timer_t rtimer;

#define LANGUAGE_ID 0x0409 // Language ID: English
Adafruit_USBH_Host USBHost; // USB Host object

#define MAX_REPORT  4

static struct {  // Each HID instance can has multiple reports
  uint8_t report_count;
  tuh_hid_report_info_t report_info[MAX_REPORT];
}hid_info[CFG_TUH_HID];

static bool keyboard_mounted = false;
static uint8_t keyboard_dev_addr = 0;
static uint8_t keyboard_idx = 0;
static uint8_t keyboard_leds = 0;
static bool keyboard_leds_changed = false;

int old_ascii = -1;
uint32_t repeat_timeout;
// this matches Linux default of 500ms to first repeat, 1/20s thereafter
const uint32_t default_repeat_time = 50;
const uint32_t initial_repeat_time = 500;

void send_ascii(uint8_t code, uint32_t repeat_time=default_repeat_time) {
  old_ascii = code;
  repeat_timeout = millis() + repeat_time;
  usbhkbd_write(code);
}

void usb_host_task(void) {
  USBHost.task();
  uint32_t now = millis();
  uint32_t deadline = repeat_timeout - now;
  if (old_ascii >= 0 && deadline > INT32_MAX) {
    send_ascii(old_ascii);
    deadline = repeat_timeout - now;
  } else if (old_ascii < 0) {
    deadline = UINT32_MAX;
  }
  if (keyboard_leds_changed) {
    tuh_hid_set_report(keyboard_dev_addr, keyboard_idx, 0/*report_id*/, HID_REPORT_TYPE_OUTPUT, &keyboard_leds, sizeof(keyboard_leds));
  }
}

bool timer_callback(repeating_timer_t *rtimer) { // USB Host is executed by timer interrupt.
  usb_host_task();
  return true;
}

#endif


/*
#define SPI_CLOCK (20'000'000)
#define SD_CS_PIN (17)
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SPI_CLOCK)
DedicatedSpiCard blockdevice;
*/
//FatFileSystem SD;
// =========================================================================================
// Define SdFat as alias for SD
// =========================================================================================
SdFat SD;

// =========================================================================================
// Define Board-Data
// GP25 green onboard LED
// =========================================================================================
#define LED 25  // GPIO25
#define LEDinv 0
#define board_pico
#define board_analog_io
#define board_digital_io
#define BOARD "Raspberry Pi Pico"

// =========================================================================================
// SPIINIT !!ONLY!! for ESP32-boards
// #define SPIINIT Clock, MISO, MOSI, Card-Select
// =========================================================================================
#define SPIINIT 18,16,19,SS 
#define SPIINIT_TXT "18,16,19,17"

// =========================================================================================
// Pin Documentation
// =========================================================================================
// Normal RPi Pico 
// MISO - Pin 21 - GPIO 16
// MOSI - Pin 25 - GPIO 19
// CS   - Pin 22 - GPIO 17
// SCK  - Pin 24 - GPIO 18
 
// MicroSD Pin Definition for RC2040 board
// Pin 6 - GPIO 4 MISO
// Pin 7 - GPIO 5 Chip/Card-Select (CS / SS)
// Pin 4 - GPIO 2 Clock (SCK)
// Pin 5 - GPIO 3 MOSI

// FUNCTIONS REQUIRED FOR USB MASS STORAGE ---------------------------------

#if USE_MSC
Adafruit_USBD_MSC usb_msc; // USB mass storage object
static bool msc_changed = true; // Is set true on filesystem changes

// Callback on READ10 command.
int32_t msc_read_cb(uint32_t lba, void *buffer, uint32_t bufsize) {
  return blockdevice.readBlocks(lba, (uint8_t *)buffer, bufsize / 512) ? bufsize : -1;
}

// Callback on WRITE10 command.
int32_t msc_write_cb(uint32_t lba, uint8_t *buffer, uint32_t bufsize) {
  digitalWrite(LED_BUILTIN, HIGH);
  return blockdevice.writeBlocks(lba, buffer, bufsize / 512) ? bufsize : -1;
}

// Callback on WRITE10 completion.
void msc_flush_cb(void) {
  blockdevice.syncBlocks();   // Sync with blockdevice
  SD.cacheClear(); // Clear filesystem cache to force refresh
  digitalWrite(LED_BUILTIN, LOW);
  msc_changed = true;
}
#endif



// DVI Display
#if USE_DISPLAY
DVItext1 display(DVI_RES_640x240p60, pimoroni_demo_hdmi_cfg);
//DVItext1 display(DVI_RES_800x240p30, pimoroni_demo_hdmi_cfg);

#define H_TAB 8
#define V_TAB 1

uint16_t underCursor = ' ';

void putch_display(uint8_t ch) {
    auto x = display.getCursorX();
    auto y = display.getCursorY();
    display.drawPixel(x, y, underCursor);
    if(((ch >= 0x20) && (ch <= 0x7E)) || ((ch >= 0x80) && (ch <= 0xFF))) { //ASCII Character
        display.write(ch);
    } else {
        switch(ch) {
            case 0x08: //Backspace
                if(x > 0) {
                    display.setCursor(--x, y);
                    display.drawPixel(x, y, ' ');
                }
                break;

            case 0x09: //HTab
                if(x >= 0) {
                    int n = x % H_TAB;
                    for (int i = 0; i < (H_TAB - n); ++i) {
                        display.setCursor(++x, y);
                        display.drawPixel(x, y, ' ');
                    }
                }
                break;

            case 0x0A: //LF
                display.write('\n');
                break;

            case 0x0B: //VTab
                for (int i = 0; i < V_TAB; ++i) {
                    display.write('\r');
                    display.write('\n');
                }
                y = display.getCursorY();
                for (int i = 0; i <= x; ++i) {
                    display.setCursor(i, y);
                    display.drawPixel(i, y, ' ');
                }
                break;

            case 0x0D: //CR
                display.write('\r');
                break;

        }
    }
    x = display.getCursorX();
    y = display.getCursorY();
    underCursor = display.getPixel(x, y);
    display.drawPixel(x, y, 0xDB);
}
#endif


bool port_init_early() {
#if USE_DISPLAY
//vreg_set_voltage(VREG_VOLTAGE_1_20);
//delay(10);
  if (!display.begin()) {
    return false;
  }
  _putch_hook = putch_display;
#endif

#if USE_KEYBOARD
  USBHost.begin(0);
  // USB Host is executed by timer interrupt.
  add_repeating_timer_us( KBD_INT_TIME/*us*/, timer_callback, NULL, &rtimer );

  _getch_hook = getch_usbh;
  _kbhit_hook = kbhit_usbh;
#endif


  // USB mass storage / filesystem setup (do BEFORE Serial init)
/*
  if (!blockdevice.begin(SD_CONFIG)) { _puts("Failed to initialize SD card"); return false; }
#if USE_MSC
  // Set disk vendor id, product id and revision
  usb_msc.setID("Adafruit", "Internal Flash", "1.0");
  // Set disk size, block size is 512 regardless of blockdevice page size
  usb_msc.setCapacity(blockdevice.sectorCount(), 512);
  usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);
  usb_msc.setUnitReady(true); // MSC is ready for read/write
  if (!usb_msc.begin()) {
      _puts("Failed to initialize USB MSC"); return false;
  }
#endif
*/
  return true;
}

/*
bool port_flash_begin() {
  if (!SD.begin(&blockdevice, true, 1)) { // Start filesystem on the blockdevice
      _puts("!SD.begin()"); return false;
  }
  return true;
}
*/


#if defined(__cplusplus)
extern "C"
{
#endif


#if USE_KEYBOARD

hid_keyboard_report_t old_report;

bool report_contains(const hid_keyboard_report_t &report, uint8_t key) {
  for (int i = 0; i < 6; i++) {
    if (report.keycode[i] == key) return true;
  }
  return false;
}

void process_boot_kbd_report(uint8_t dev_addr, uint8_t idx, const hid_keyboard_report_t &report) {

  bool alt = report.modifier & 0x44;
  bool shift = report.modifier & 0x22;
  bool ctrl = report.modifier & 0x11;

  bool num = old_report.reserved & 1;
  bool caps = old_report.reserved & 2;

  uint8_t code = 0;

  if (report.keycode[0] == 1 && report.keycode[1] == 1) {
    // keyboard says it has exceeded max kro
    return;
  }

  // something was pressed or release, so cancel any key repeat
  old_ascii = -1;

  for (auto keycode : report.keycode) {
    if (keycode == 0) continue;
    if (report_contains(old_report, keycode)) continue;

    /* key is newly pressed */
    if (keycode == HID_KEY_NUM_LOCK) {
      num = !num;
#ifdef USE_JP_KEYBOARD
    } else if ((keycode == HID_KEY_CAPS_LOCK) && shift) {
#else
    } else if (keycode == HID_KEY_CAPS_LOCK) {
#endif
      caps = !caps;
    } else {
      for (const auto &mapper : keycode_to_ascii) {
        if (!(keycode >= mapper.first && keycode <= mapper.last))
          continue;
        if (mapper.flags & FLAG_SHIFT && !shift)
          continue;
        if (mapper.flags & FLAG_NUMLOCK && !num)
          continue;
        if (mapper.flags & FLAG_CTRL && !ctrl)
          continue;
        if (mapper.flags & FLAG_LUT) {
          code = lut[mapper.code][keycode - mapper.first];
        } else {
          code = keycode - mapper.first + mapper.code;
        }
        if (mapper.flags & FLAG_ALPHABETIC) {
          if (shift ^ caps) {
            code ^= ('a' ^ 'A');
          }
        }
        if (ctrl) code &= 0x1f;
        if (alt) code ^= 0x80;
        send_ascii(code, initial_repeat_time); // send code
        break;
      }
    }
  }

//uint8_t leds = (caps | (num << 1));
  keyboard_leds = (num | (caps << 1));
  if (keyboard_leds != old_report.reserved) {
    keyboard_leds_changed = true;
    // no worky
    //auto r = tuh_hid_set_report(dev_addr, idx/*idx*/, 0/*report_id*/, HID_REPORT_TYPE_OUTPUT/*report_type*/, &leds, sizeof(leds));
    //tuh_hid_set_report(dev_addr, idx/*idx*/, 0/*report_id*/, HID_REPORT_TYPE_OUTPUT/*report_type*/, &leds, sizeof(leds));
  } else {
    keyboard_leds_changed = false;
  }
  old_report = report;
  old_report.reserved = keyboard_leds;
}

/*
//--------------------------------------------------------------------+
// Generic Report
//--------------------------------------------------------------------+
void process_generic_report(uint8_t dev_addr, uint8_t idx, uint8_t const* report, uint16_t len)
{
  uint8_t const rpt_count = hid_info[idx].report_count;
  tuh_hid_report_info_t* rpt_info_arr = hid_info[idx].report_info;
  tuh_hid_report_info_t* rpt_info = NULL;

  if ( rpt_count == 1 && rpt_info_arr[0].report_id == 0)
  {
    // Simple report without report ID as 1st byte
    rpt_info = &rpt_info_arr[0];
  }else
  {
    // Composite report, 1st byte is report ID, data starts from 2nd byte
    uint8_t const rpt_id = report[0];

    // Find report id in the array
    for(uint8_t i=0; i<rpt_count; i++)
    {
      if (rpt_id == rpt_info_arr[i].report_id )
      {
        rpt_info = &rpt_info_arr[i];
        break;
      }
    }

    report++;
    len--;
  }

  // For complete list of Usage Page & Usage checkout src/class/hid/hid.h. For examples:
  // - Keyboard                     : Desktop, Keyboard
  // - Mouse                        : Desktop, Mouse
  // - Gamepad                      : Desktop, Gamepad
  // - Consumer Control (Media Key) : Consumer, Consumer Control
  // - System Control (Power key)   : Desktop, System Control
  // - Generic (vendor)             : 0xFFxx, xx

  if ( rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP )
  {
    switch (rpt_info->usage)
    {
      case HID_USAGE_DESKTOP_KEYBOARD:
        {
          // Assume keyboard follow boot report layout
          process_boot_kbd_report(dev_addr, idx, *(hid_keyboard_report_t const*) report );
        }
        break;

      case HID_USAGE_DESKTOP_MOUSE:
        {
          // Assume mouse follow boot report layout
        }
        break;

      default:
        break;
    }
  }
  else //Other than HID_USAGE_PAGE_DESKTOP
  {
  }
}
*/

//--------------------------------------------------------------------+
// TinyUSB Host callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted (configured)
void tuh_mount_cb (uint8_t dev_addr)
{
}

// Invoked when device is unmounted (bus reset/unplugged)
void tuh_umount_cb(uint8_t dev_addr)
{
}

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
// therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t idx, uint8_t const* desc_report, uint16_t desc_len)
{
  // By default host stack will use activate boot protocol on supported interface.
  // Therefore for this simple example, we only need to parse generic report descriptor (with built-in parser)
  hid_info[idx].report_count = tuh_hid_parse_report_descriptor(hid_info[idx].report_info, MAX_REPORT, desc_report, desc_len);
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, idx);

  switch(itf_protocol){
    case HID_ITF_PROTOCOL_NONE: //HID_PROTOCOL_BOOT:NONE
      break;

    case HID_ITF_PROTOCOL_KEYBOARD: //HID_PROTOCOL_BOOT:KEYBOARD
      if (keyboard_mounted != true) {
        keyboard_dev_addr = dev_addr;
        keyboard_idx = idx;
        keyboard_mounted = true;
      }
      break;

    case HID_ITF_PROTOCOL_MOUSE: //HID_PROTOCOL_BOOT:MOUSE
      break;
    }

  if ( !tuh_hid_receive_report(dev_addr, idx) )
  {
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t idx)
{
  if (dev_addr == keyboard_dev_addr && idx == keyboard_idx) {
    keyboard_mounted = false;
    keyboard_dev_addr = 0;
    keyboard_idx = 0;
    keyboard_leds = 0;
    old_report = {0};
  }
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t idx, uint8_t const* report, uint16_t len)
{
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, idx);

  switch (itf_protocol)
  {
    case HID_ITF_PROTOCOL_KEYBOARD:
      if (keyboard_mounted == true) {
        process_boot_kbd_report(dev_addr, idx, *(hid_keyboard_report_t const*) report );
      }
      break;

    case HID_ITF_PROTOCOL_MOUSE:
      break;

    default:
      //process_generic_report(dev_addr, idx, report, len);
      break;
  }
  // continue to request to receive report
  if ( !tuh_hid_receive_report(dev_addr, idx) )
  {
  }
}

#endif

#if defined(__cplusplus)
}
#endif


