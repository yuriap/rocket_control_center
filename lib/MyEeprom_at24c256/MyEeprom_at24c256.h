/*
MyEeprom_at24c256.h - Library for AT24C256 IIC EEPROM.
*/
#ifndef MyEeprom_at24c256_h
#define MyEeprom_at24c256_h

#include "Arduino.h"
#include "Wire.h"

class MyEeprom_at24c256
{
public:
	MyEeprom_at24c256(int address);
	void write(unsigned  int writeAddress, byte* data, byte len);
	void read(unsigned  int readAdress, byte *buffer, byte len);
	void write4longs(int addr, long n0, long n1, long n2, long n3);
	void read4longs(int addr, long* n0, long* n1, long* n2, long* n3);
private:
	int _address;
	long getnum(char ch0, char ch1, char ch2, char ch3);
};

#endif
