/*
MyEeprom_at24c256.cpp - Library for AT24C256 IIC EEPROM.
*/

#include "Arduino.h"
#include "Wire.h"
#include "MyEeprom_at24c256.h"

MyEeprom_at24c256::MyEeprom_at24c256(int address)
{	
	Wire.begin();
	_address = address;
}

void MyEeprom_at24c256::write(unsigned  int writeAddress, byte* data, byte len)
{
	
	Wire.beginTransmission(_address);
	Wire.write((int)(writeAddress >> 8));
	Wire.write((int)(writeAddress & 0xFF));
	byte c;
	for (c = 0; c < len; c++)
		Wire.write(data[c]);
	Wire.endTransmission();
}

void MyEeprom_at24c256::read(unsigned  int readAdress, byte *buffer, byte len)
{
	Wire.beginTransmission(_address);
	Wire.write((int)(readAdress >> 8));
	Wire.write((int)(readAdress & 0xFF)); 
	Wire.endTransmission();
	Wire.requestFrom(_address, len);
	int c = 0;
	for (c = 0; c < len; c++)
		if (Wire.available()) buffer[c] = Wire.read();
}

long MyEeprom_at24c256::getnum(char ch0, char ch1, char ch2, char ch3)
{
  long num0;    
  long num1;
  long num2;
  long num3;
  
  num0 = ch0 & 0x000000ff;
  num1 = ch1 & 0x000000ff;
  num2 = ch2 & 0x000000ff;
  num3 = ch3 & 0x000000ff;
  
  num0 = num0 << 24;
  num1 = num1 << 16;
  num2 = num2 << 8;
  return num0 | num1 | num2 | num3;
}

void MyEeprom_at24c256::write4longs(int addr, long n0, long n1, long n2, long n3)
{
  char message[16]; //buffer
  int i,j;
  for(j=0,i=3; i>=0; i--,j++){
    message[j]= (n0>>(i*8))&0xff;
  }
  
  for(j=4,i=3; i>=0; i--,j++){
    message[j]= (n1>>(i*8))&0xff;
  }

  for(j=8,i=3; i>=0; i--,j++){
    message[j]= (n2>>(i*8))&0xff;
  }
  
  for(j=12,i=3; i>=0; i--,j++){
    message[j]= (n3>>(i*8))&0xff;
  }  
  write(addr, (byte*) message, sizeof(message));
/*
  Serial.print("Written: <|");
  for(int z=0; z < 16; z++){
    Serial.print(int(message[z]) + " ");
  }
  Serial.println("|>");
*/	
  delay(10); 
}

void MyEeprom_at24c256::read4longs(int addr, long* n0, long* n1, long* n2, long* n3)
{
  char message[16]; //buffer
  read(addr, (byte *) message, sizeof(message));
/*	
  Serial.print("Read: <|");
  for(int z=0; z < 16; z++){
    Serial.print(int(message[z]) + " "); 
  }
  Serial.println("|>");
*/	
  *n0 = getnum(message[0], message[1], message[2], message[3]);
  *n1 = getnum(message[4], message[5], message[6], message[7]);
  *n2 = getnum(message[8], message[9], message[10], message[11]);
  *n3 = getnum(message[12], message[13], message[14], message[15]);
}