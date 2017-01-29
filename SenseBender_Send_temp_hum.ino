/*********************************************
 *
 * Sends temperature and humidity from Si7021 sensor
 *
 * Parameters
 * - force : Forces transmission of a value (even if it's the same as previous measurement)
 *
 *********************************************/
void sendTempHumidityMeasurements(bool force)
{
  bool tx = force;

  si7021_env data = humiditySensor.getHumidityAndTemperature();

  raHum.addValue(data.humidityPercent);

  float diffTemp = abs(lastTemperature - (isMetric ? data.celsiusHundredths : data.fahrenheitHundredths)/100.0);
  float diffHum = abs(lastHumidity - raHum.getAverage());

  Serial.print(F("TempDiff :"));Serial.println(diffTemp);
  Serial.print(F("HumDiff  :"));Serial.println(diffHum); 

  if (isnan(diffHum)) tx = true; 
  if (diffTemp > TEMP_TRANSMIT_THRESHOLD) tx = true;
  if (diffHum > HUMI_TRANSMIT_THRESHOLD) tx = true;

  if (tx) {
    measureCount = 0;
    float temperature = (isMetric ? data.celsiusHundredths : data.fahrenheitHundredths) / 100.0;

    int humidity = data.humidityPercent;
    Serial.print("T: ");Serial.println(temperature);
    Serial.print("H: ");Serial.println(humidity);

    send(msgTemp.set(temperature,1));
    send(msgHum.set(humidity));
    lastTemperature = temperature;
    lastHumidity = humidity;
    transmission_occured = true;
    if (sendBattery > 60) {
     sendBattLevel(true); // Not needed to send battery info that often
     sendBattery = 0;
    }
  }
}


