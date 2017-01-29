/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *e
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Joao E Borges
 * 
 * DESCRIPTION
 * Water leak sensor
 * Act as a temperature / humidity sensor by default.
 *  
 * Battery voltage is as battery percentage (Internal message), and optionally as a sensor value (See defines below)
 *
 */


#define MY_DEBUG
#define MY_RADIO_NRF24
//#define BATT_SENSOR

#include <MySensors.h>
#include <SPI.h>
#include <Wire.h>
#include <SI7021.h>
#ifndef MY_OTA_FIRMWARE_FEATURE
#include "drivers/SPIFlash/SPIFlash.cpp"
#endif
//#include <EEPROM.h>  
#include <sha204_lib_return_codes.h>
#include <sha204_library.h>
#include <RunningAverage.h>

#define RELEASE "1.4"
#define AVERAGES 2

// Child sensor ID's
#define CHILD_ID_TEMP  1
#define CHILD_ID_HUM   2
#define CHILD_ID_WaterStatus 3
#define CHILD_ID_WaterSensor 4
#define CHILD_ID_BATT_SENSOR 5

// How many milli seconds between each measurement
#define MEASURE_INTERVAL 20000        //60 segundos (tempo entre medidasa, que o micro fica dormindo)

// How many milli seconds should we wait for OTA?
#define OTA_WAIT_PERIOD 300

// FORCE_TRANSMIT_INTERVAL, this number of times of wakeup, the sensor is forced to report all values to the controller
#define FORCE_TRANSMIT_INTERVAL 5 

// When MEASURE_INTERVAL is 60000 and FORCE_TRANSMIT_INTERVAL is 30, we force a transmission every 30 minutes.
// Between the forced transmissions a tranmission will only occur if the measured value differs from the previous measurement

// HUMI_TRANSMIT_THRESHOLD tells how much the humidity should have changed since last time it was transmitted. Likewise with
// TEMP_TRANSMIT_THRESHOLD for temperature threshold.
#define HUMI_TRANSMIT_THRESHOLD 0.5
#define TEMP_TRANSMIT_THRESHOLD 0.5

// Pin definitions
#define TEST_PIN       A0
#define Water_AnalogPin A1
#define Water_DigitalPin D3
#define LED_PIN        A2
#define ATSHA204_PIN   17 // A3



const int sha204Pin = ATSHA204_PIN;
atsha204Class sha204(sha204Pin);

SI7021 humiditySensor;
SPIFlash flash(8, 0x1F65);

// Sensor messages
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgWaterStatus(CHILD_ID_WaterStatus, V_TRIPPED);
MyMessage msgWaterLevel(CHILD_ID_WaterSensor, V_FLOW);
MyMessage msgBatt(CHILD_ID_BATT_SENSOR, V_VOLTAGE);


// Global settings
int measureCount = 0;
int sendBattery = 0;
boolean isMetric = true;
boolean highfreq = true;
boolean transmission_occured = false;

// Storage of old measurements
float lastTemperature = -100;
int lastHumidity = -100;
long lastBattery = -100;
int lastWater = -100;
int oldWaterValue=1023;

RunningAverage raHum(AVERAGES);

/****************************************************
 *
 * Setup code 
 *
 ****************************************************/
void setup() {

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN, LOW);
  delay(1000);

  Serial.begin(115200);
  Serial.print(F("Sensebender Micro FW "));
  Serial.print(RELEASE);
  Serial.flush();

  // First check if we should boot into test mode

   pinMode(TEST_PIN,INPUT);
//  digitalWrite(TEST_PIN, HIGH); // Enable pullup
 // if (!digitalRead(TEST_PIN)) testMode();

  // Make sure that ATSHA204 is not floating
  pinMode(ATSHA204_PIN, INPUT);
  digitalWrite(ATSHA204_PIN, HIGH);
//  digitalWrite(TEST_PIN,LOW);

  humiditySensor.begin();

  Serial.flush();
  Serial.println(F(" - Online!"));

 // isMetric = getConfig().isMetric;
//  Serial.print(F("isMetric: ")); Serial.println(isMetric);
  raHum.clear();
  sendTempHumidityMeasurements(false);
  sendBattLevel(false);

#ifdef MY_OTA_FIRMWARE_FEATURE  
  Serial.println("OTA FW update enabled");
#endif

}

void presentation()  {
  sendSketchInfo("Sensebender Micro", RELEASE);

  present(CHILD_ID_TEMP,S_TEMP);
  present(CHILD_ID_HUM,S_HUM);
  present(CHILD_ID_WaterStatus,S_WATER_LEAK);
  present(CHILD_ID_WaterSensor,S_WATER);
  present(CHILD_ID_BATT_SENSOR, S_MULTIMETER);

}


/***********************************************
 *
 *  Main loop function
 *
 ***********************************************/
void loop() {

  measureCount ++;
  sendBattery ++;
  bool forceTransmit = false;
  transmission_occured = false;
//#ifndef MY_OTA_FIRMWARE_FEATURE
//  if ((measureCount == 5) && highfreq) 
//  {
//    clock_prescale_set(clock_div_8); // Switch to 1Mhz for the reminder of the sketch, save power.
//    highfreq = false;
//  } 
//#endif

// CHECK IF ITS TIME TO FORCE SEND THE TEMP, HUMIDITY AND WATER SENSOR <<<<

  if (measureCount > FORCE_TRANSMIT_INTERVAL) 
    { 
      forceTransmit = true; 
      measureCount = 0;
    }
  sendTempHumidityMeasurements(forceTransmit);
  SendWaterSensor(forceTransmit);
  
  
  if (sendBattery > 30) 
  {
     sendBattLevel(forceTransmit); // Not needed to send battery info that often
     sendBattery = 0;
  }
  sendBattLevel(forceTransmit);
//#ifdef MY_OTA_FIRMWARE_FEATURE
//  if (transmission_occured) {
//      wait(OTA_WAIT_PERIOD);
//  }
//#endif

  sleep(MEASURE_INTERVAL);  
}



