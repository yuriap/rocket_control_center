/*
 * 
 * 
 * 
 * VCC --------3.3V
 * GND ------- GND
 * SCL -------- 13
 * SDA -------- 11
 * CSB -------- 10
 * SDO -------- 12
 * Source https://arduino.ua/prod1758-barometr-datchik-atmosfernogo-davleniya-na-bmp280
 * 
 */
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

#define BMP_SCK 13
#define BMP_MISO 12
#define BMP_MOSI 11 
#define BMP_CS 10

//Adafruit_BMP280 bme; // I2C
Adafruit_BMP280 bme(BMP_CS); // hardware SPI
//Adafruit_BMP280 bme(BMP_CS, BMP_MOSI, BMP_MISO,  BMP_SCK);


//pressure sensor status indicators
const int ledSensorOK    = 3;
const int ledSensorError = 2;

// main loop delay
const int MainLoopDelay=10; // delay in milliseconds in main loop, delay between actual sensor readings
const int warmLoops=500;      // number of loops the readings of altitude will be ignored (warmLoops*MainLoopDelay/1000 seconds)

//data structures for storing readings
const int numReadings = 15;     // number of readings in a cyclic buffer
int Index_calc = 0;              // the index of the current reading

const int avgNum = 5;           // number of values to calc average, avgNum*MainLoopDelay - interval of avg measurement in milliseconds
//long pressure[avgNum];          // the readings from pressure sensor
long altitude[avgNum];          // the readings from pressure sensor
int  Index_real = 0;

//long avg_pressure[numReadings];    // average pressure readings (cyclic buffer)
long avg_altitude[numReadings];   // average altitude readings (cyclic buffer)

//global loop conter
long g_cnt = 0;

const int sequentialReadings = 15; //number of sequential reading to analyze for making decision, sequentialReadings*avgNum*MainLoopDelay - minimal time duration after the apogee after which the decision can be made 


void setup() {
  pinMode(ledSensorOK, OUTPUT);
  pinMode(ledSensorError, OUTPUT);
  
  Serial.begin(9600);
  Serial.println(F("BMP280 test"));
  
  if (!bme.begin()) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1) {
      digitalWrite(ledSensorError, HIGH);
      delay(200);                       
      digitalWrite(ledSensorError, LOW);
      delay(200);     
    }
  }
  digitalWrite(ledSensorOK, HIGH);
}

void loop() {
  g_cnt++;

  savereadings();
  calc_avg_altitude();

  delay(MainLoopDelay);
}

void savereadings() {

  altitude[Index_real] = bme.readAltitude(1013.25)*10; //bme.readPressure();

  //Serial.println(g_cnt);
  //Serial.print(" Pressure = ");
  //Serial.print(pressure[Index_real]);
  //Serial.println(" Pa");

  Index_real = Index_real + 1;
  if (Index_real >= avgNum) {
    Index_real = 0;
  }   
}

void calc_avg_altitude() {
  if (g_cnt % avgNum == 0) {
    avg_altitude[Index_calc] = get_avg();

    Serial.print(" AVG Altitude = ");
    Serial.print(avg_altitude[Index_calc]);
    Serial.println(" m");

    if ((checkdecreasing(sequentialReadings) == 1) && (g_cnt > warmLoops )){
      Serial.println("PARASHUT!!!");
      while (1) {
        digitalWrite(ledSensorOK, HIGH);
        delay(200);                       
        digitalWrite(ledSensorOK, LOW);
        delay(200);     
      }      
    }
    
    Index_calc = Index_calc + 1;
    if (Index_calc >= numReadings) {
      Index_calc = 0;
    }    
  }
}

long get_avg() {
long avg;
  avg = 0;
  for (int i = 0; i < avgNum; i++) {
    avg = avg + altitude[i];
  }
  avg = avg / avgNum;
  return avg;
}

long checkdecreasing(int delay_num) {
  int delta[delay_num];
  int Index = Index_calc;
  int cnt_i;
  int cnt_delta;

  //fill an array with delta of reading values
  cnt_i = 0;
  while (cnt_i < delay_num) {  
    if (Index > 0) {
      delta[cnt_i] = avg_altitude[Index] - avg_altitude[Index-1];
      Index = Index - 1;
    }
    else {
      delta[cnt_i] = avg_altitude[Index] - avg_altitude[numReadings-1];
      Index = numReadings-1;
    }
    cnt_i = cnt_i + 1;    
  }
 
  //check how many deltas lesser then zero (means readings are decreasing)
  cnt_i = 0;
  cnt_delta = 0;
  while (cnt_i < delay_num) {
    if (delta[cnt_i] < 0) {
      cnt_delta = cnt_delta + 1;
    }
    cnt_i = cnt_i + 1;    
  } 

  //if number of decreased reading equal to delay_num it means readings are steadily decreasing
  if (cnt_delta == delay_num) {
    return 1;
  }
  else {
    return 0;
  }
}
