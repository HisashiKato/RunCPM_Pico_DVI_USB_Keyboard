// SPDX-FileCopyrightText: 2023 Mockba the Borg
//
// SPDX-License-Identifier: MIT

#ifndef CONSOLE_H
#define CONSOLE_H

extern int _kbhit(void);
uint8_t _getch(void);
void _putch(uint8 ch);

/* see main.c for definition */

uint8 mask8bit = 0x7f;		// TO be used for masking 8 bit characters (XMODEM related)
							// If set to 0x7f, RunCPM masks the 8th bit of characters sent
							// to the console. This is the standard CP/M behavior.
							// If set to 0xff, RunCPM passes 8 bit characters. This is
							// required for XMODEM to work.
							// Use the CONSOLE7 and CONSOLE8 programs to change this on the fly.

/*
uint8 _chready(void)		// Checks if there's a character ready for input
{
	return(_kbhit() ? 0xff : 0x00);
}
*/

uint8 _getchNB(void)		// Gets a character, non-blocking, no echo
{
	return(_kbhit() ? _getch() : 0x00);
}

void _putcon(uint8 ch)		// Puts a character
{
	_putch(ch & mask8bit);
}

void _puts(const char* str)	// Puts a \0 terminated string
{
	while (*str)
		_putcon(*(str++));
}

void _puthex8(uint8 c)		// Puts a HH hex string
{
	_putcon(tohex(c >> 4));
	_putcon(tohex(c & 0x0f));
}

void _puthex16(uint16 w)	// puts a HHHH hex string
{
	_puthex8(w >> 8);
	_puthex8(w & 0x00ff);
}

void _putdec(uint16_t w) {
    char buf[] = "    0";
    size_t i=sizeof(buf)-1;
    while(w) {
        assert(i > 0);
        buf[--i] = '0' + (w % 10);
        w /= 10;
    }
    _puts(buf);
}



#ifdef STREAMIO
int _nextStreamInChar;

void _getNextStreamInChar(void)
{
	_nextStreamInChar = streamInputFile ? fgetc(streamInputFile) : EOF;
	if (EOF == _nextStreamInChar) {
		streamInputActive = FALSE;
	}
}

uint8 _getStreamInChar(void)
{
	uint8 result = _nextStreamInChar;
	_getNextStreamInChar();
	// TODO: delegate to abstrction_posix.h
	if (0x0a == result) result = 0x0d;
	return result;
}

uint8 _getStreamInCharEcho()
{
	uint8 result = _getStreamInChar();
	_putcon(result);
	return result;
}

void _streamioInit(void)
{
	_getNextStreamInChar();
}

void _streamioReset(void)
{
	if (streamOutputFile) fclose(streamOutputFile);
}
#endif



uint8 _chready(void)		// Checks if there's a character ready for input
{
#ifdef STREAMIO
	if (streamInputActive) return 0xff;
	// TODO: Consider adding/keeping _abort_if_kbd_eof() here.
	_abort_if_kbd_eof();
#endif
	return(_kbhit() ? 0xff : 0x00);
}

uint8 _getcon(void)	   // Gets a character, blocking, no echo
{
#ifdef STREAMIO
	if (streamInputActive) return _getStreamInChar();
	// TODO: Consider adding/keeping _abort_if_kbd_eof() here.
	_abort_if_kbd_eof();
#endif
	return _getch();
}

/*
  When compiling with "Arduino IDE", an error "Compilation error: '_getche' was not declared in this scope;" occurred,
  so I moved the "_getche()" function from the "abstraction_arduino.h" file to the "console.h" file.
*/
uint8 _getche(void) {
	uint8 ch = _getch();
	_putch(ch);
	return(ch);
}


uint8 _getconE(void)   // Gets a character, blocking, with echo
{
#ifdef STREAMIO
	if (streamInputActive) return _getStreamInCharEcho();
	// TODO: Consider adding/keeping _abort_if_kbd_eof() here.
	_abort_if_kbd_eof();
#endif
	return _getche();
}

uint8 _getconNB(void)	  // Gets a character, non-blocking, no echo
{
#ifdef STREAMIO
	if (streamInputActive) return _getStreamInChar();
	// TODO: Consider adding/keeping _abort_if_kbd_eof() here.
	_abort_if_kbd_eof();
#endif
	return(_kbhit() ? _getch() : 0x00);
}

#endif
