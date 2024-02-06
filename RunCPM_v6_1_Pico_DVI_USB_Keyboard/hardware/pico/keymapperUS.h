#pragma once

#define FLAG_ALPHABETIC (1)
#define FLAG_SHIFT (2)
#define FLAG_NUMLOCK (4)
#define FLAG_CTRL (8)
#define FLAG_LUT (16)

const char * const lut[] = {
  "!@#$%^&*()",                                        /* 0 - shifted numeric keys */
  "\r\x1b\b\t -=[]\\#;'`,./",                          /* 1 - symbol keys */
  "\n\x1b\x7f\t _+{}|~:\"~<>?",                        /* 2 - shifted */
/*"\x1c\x1d\x1f\x1e",                               */ /* 3 - arrow keys RLDU */
/*"/*-+\n1234567890.",                              */ /* 4 - keypad w/numlock */
/*"/*-+\n\xff\x1f\xff\x1d\xff\x1b\xff\x1e\xff\xff.",*/ /* 5 - keypad w/o numlock */
};

struct keycode_mapper {
  uint8_t first, last, code, flags;
} keycode_to_ascii[] = {
  { HID_KEY_A, HID_KEY_Z, 'a', FLAG_ALPHABETIC, },

  { HID_KEY_1, HID_KEY_9, 0, FLAG_SHIFT | FLAG_LUT, },
  { HID_KEY_1, HID_KEY_9, '1', 0, },
  { HID_KEY_0, HID_KEY_0, ')', FLAG_SHIFT, },
  { HID_KEY_0, HID_KEY_0, '0', 0, },

  { HID_KEY_ENTER, HID_KEY_ENTER, '\n', FLAG_CTRL },
  { HID_KEY_ENTER, HID_KEY_SLASH, 2, FLAG_SHIFT | FLAG_LUT, },
  { HID_KEY_ENTER, HID_KEY_SLASH, 1, FLAG_LUT, },

//{ HID_KEY_F1, HID_KEY_F1, 0x1e, 0, }, // help key on xerox 820 kbd

//{ HID_KEY_ARROW_RIGHT, HID_KEY_ARROW_UP, 3, FLAG_LUT },

//{ HID_KEY_KEYPAD_DIVIDE, HID_KEY_KEYPAD_DECIMAL, 4, FLAG_NUMLOCK | FLAG_LUT },
//{ HID_KEY_KEYPAD_DIVIDE, HID_KEY_KEYPAD_DECIMAL, 5, FLAG_LUT },
};
