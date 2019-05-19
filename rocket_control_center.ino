/*
 * 
 * 
 * Barometer:
 * VCC -------- 3.3V
 * GND -------- GND
 * SCL -------- 13
 * SDA -------- 11
 * CSB -------- 10
 * SDO -------- 12
 * Source https://arduino.ua/prod1758-barometr-datchik-atmosfernogo-davleniya-na-bmp280
 * 
 */

/*
 * EEPROM
 * VCC -------- 5V
 * GND -------- GND
 * SCL -------- A5
 * SDA -------- A4
 *
 * https://github.com/adamjezek98/Eeprom_at24c256
 */

/*
 * ADXL345
 * GND -------> GND
 * VIN -------> +5v
 * SDA -------> SDA (Analog 4 on "Classic Arduinos") -> 16
 * SCL -------> SCL (Analog 5 on "Classic Arduinos") -> 17
 * 
 * https://learn.adafruit.com/adxl345-digital-accelerometer/assembly-and-wiring
 */

/*
 * Relay
 * S ----------> 7
 * 
 * LEDs
 * Writing started -> 2
 * Sensors OK      -> 3
 * Pressure Sensor Error -> 4
 * Accell  Sensor Error  -> 5
 */
 
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_ADXL345_U.h>
#include <MyEeprom_at24c256.h>

#define BMP_SCK 13
#define BMP_MISO 12
#define BMP_MOSI 11 
#define BMP_CS 10

/* Assign a unique ID to this sensor at the same time */
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

//Adafruit_BMP280 bme; // I2C
Adafruit_BMP280 bme(BMP_CS); // hardware SPI

//create eeprom at address 0x50
MyEeprom_at24c256 eeprom(0x50);

//pressure sensor status indicators
const int ledWriting     = 2;
const int ledSensorOK    = 3;
const int ledPressureSensorError = 4;
const int ledAccelSensorError = 5;

//Rele
const int pinRelay       = 7;

// main loop delay
const int MainLoopDelay    = 10;  // delay in milliseconds in main loop, delay between actual sensor readings
const int EEPROMWriteDelay = 10;  // delay in milliseconds after eeprom write call
const int warmLoops        = 1000;// number of loops the readings of altitude will be ignored (warmLoops*MainLoopDelay/1000 seconds)

//Rocket
//MainLoopDelay = 10;
//warmLoops     = 1000;

//Lift testing
//MainLoopDelay = 50;
//warmLoops     = 200;

//data structures for storing readings

//Raw sensors data
const int avgNum = 5;             // number of values to calc average, avgNum*MainLoopDelay - interval of avg measurement in milliseconds
int  Index_real  = 0;             // index for real readings

long altitude[avgNum];            // the readings from altitude sensor
long accel_x[avgNum];             // the readings from accelerpmeter X sensor
long accel_y[avgNum];             // the readings from accelerpmeter Y sensor
long accel_z[avgNum];             // the readings from accelerpmeter Z sensor

//Averaged sensors data
const int numReadings = 16;       // number of readings in a cyclic buffer
int   Index_avg = 0;              // the index of the averaged reading
long  avg_altitude[numReadings];  // average altitude readings
long  avg_accel_x[numReadings];        // average readings from accelerpmeter X sensor
long  avg_accel_y[numReadings];        // average readings from accelerpmeter Y sensor
long  avg_accel_z[numReadings];        // average readings from accelerpmeter Z sensor

const int coeff = 1000;           //reading stored precision

//global loop conter
long g_cnt = 0;
long addr  = 0;

//number of sequential readings to analyze for making decision, sequentialReadings*avgNum*MainLoopDelay - minimal time duration after the apogee after which the decision can be made 
const int sequentialReadings = numReadings; 

int flag_relay_on = 0;

void setup() {
  
  Serial.begin(9600);
  
  /****************************************************/
  pinMode(ledSensorOK,    OUTPUT);
  pinMode(ledPressureSensorError, OUTPUT);
  pinMode(ledAccelSensorError, OUTPUT);
  
  pinMode(ledWriting,     OUTPUT);
  
  digitalWrite(ledSensorOK, HIGH);
  digitalWrite(ledPressureSensorError, HIGH);
  digitalWrite(ledAccelSensorError, HIGH);
  digitalWrite(ledWriting, HIGH);
  
  /****************************************************/
  pinMode(pinRelay, OUTPUT);
  digitalWrite(pinRelay, LOW);
  
  /****************************************************/
  /* Initialise the sensor */
  Serial.println(F("Accelerometer test..."));
  if(!accel.begin())
  {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
    while (1) {
      digitalWrite(ledAccelSensorError, HIGH);
      delay(200);                       
      digitalWrite(ledAccelSensorError, LOW);
      delay(200);     
    }
  }

  /* Set the range to whatever is appropriate for your project */
  accel.setRange(ADXL345_RANGE_16_G);
  Serial.println(F("Accelerometer test OK"));
  
  /****************************************************/
  Serial.println(F("Pressure BMP280 test..."));
  
  if (!bme.begin()) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1) {
      digitalWrite(ledPressureSensorError, HIGH);
      delay(200);                       
      digitalWrite(ledPressureSensorError, LOW);
      delay(200);     
    }
  }
  Serial.println(F("Pressure BMP280 test OK"));
  
  /****************************************************/
  digitalWrite(ledSensorOK, LOW);
}

void loop() {
   
  int write_delay = 0;
  
  readsensors();
 //Serial.println("g_cnt: " + String(g_cnt));  
  if (g_cnt % avgNum == 0) {
 
    int curr_idx = calc_averages();
    
    if (g_cnt > warmLoops) {
      if (addr < 2047) {
        digitalWrite(ledWriting, LOW);
        if (flag_relay_on == 1) {
          eeprom.write4longs(addr * 16, 0, 0, 0, 0);
          addr = addr + 1;
          flag_relay_on = 2;
        }
        eeprom.write4longs(addr * 16, avg_altitude[curr_idx], avg_accel_x[curr_idx], avg_accel_y[curr_idx], avg_accel_z[curr_idx]);
        //Serial.println("written["+String((addr * 16))+"]: " + String(avg_altitude[curr_idx])+" "+String(avg_accel_x[curr_idx])+" "+String(avg_accel_y[curr_idx])+" "+String(avg_accel_z[curr_idx]));
        write_delay = EEPROMWriteDelay;
        addr = addr + 1;
      }
      else
      {
        while (1) {
          digitalWrite(ledWriting, HIGH);
          delay(200);                       
          digitalWrite(ledWriting, LOW);
          delay(200);    
        }
      }
      
      if (checkdecreasing(sequentialReadings) == 1){
        if (flag_relay_on == 0) {
          flag_relay_on = 1;
        }
        digitalWrite(pinRelay, HIGH);    
        digitalWrite(ledSensorOK, LOW);                  
        digitalWrite(ledPressureSensorError, LOW);
        digitalWrite(ledAccelSensorError, LOW);        
      }      
    }  

  }
 
  delay(MainLoopDelay-write_delay);
  g_cnt = g_cnt + 1;   
}

void readsensors() {

  altitude[Index_real] = bme.readAltitude(1013.25) * 10 * coeff; 
  
  sensors_event_t event; 
  accel.getEvent(&event);
  accel_x[Index_real] = event.acceleration.x * coeff;
  accel_y[Index_real] = event.acceleration.y * coeff;
  accel_z[Index_real] = event.acceleration.z * coeff;

  Index_real = Index_real + 1;
  if (Index_real >= avgNum) {
    Index_real = 0;
  }   
}

int calc_averages() {
  int curr_idx = Index_avg;
  avg_altitude[Index_avg] = getavgreadings(altitude);//get_avg();
  avg_accel_x[Index_avg]  = getavgreadings(accel_x); 
  avg_accel_y[Index_avg]  = getavgreadings(accel_y);
  avg_accel_z[Index_avg]  = getavgreadings(accel_z);  
  Index_avg = Index_avg + 1;
  if (Index_avg >= numReadings) {
    Index_avg = 0;
  }
  return curr_idx;
}

long getavgreadings(long data[avgNum]) {
  long avg;
  avg = 0;
  for (int i = 0; i < avgNum; i++) {
    avg = avg + data[i];
  }
  avg = avg / avgNum;
  return avg;
}

long checkdecreasing(int delay_num) {
  long delta[delay_num];
  int Index = Index_avg;
  int cnt_i;
  long cnt_delta;

  //fill an array with delta of reading values
  cnt_i = 0;
  while (cnt_i < delay_num) {  
    if (Index > 0) {
      delta[cnt_i] = (avg_altitude[Index] - avg_altitude[Index-1])/coeff;
      Index = Index - 1;
    }
    else {
      delta[cnt_i] = (avg_altitude[Index] - avg_altitude[numReadings-1])/coeff;
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

  //if 2/3 of readings of a sample (delay_num) are decreasing it means readings are steadily decreasing
  if (cnt_delta*3 > delay_num*2) {
    //Serial.println("checkdecreasing: " + String(cnt_delta)+" "+String(delay_num/2));
    return 1;
  }
  else {
    return 0;
  }
}
