/********************************************
 *
 * Sends battery information (battery percentage)
 *
 * Parameters
 * - force : Forces transmission of a value
 *
 *******************************************/
void sendBattLevel(bool force)
{
  if (force) lastBattery = -1;
  long vcc = readVcc();
  if (vcc != lastBattery) {
    lastBattery = vcc;
    float send_voltage = float(vcc)/1000.0f;
    send(msgBatt.set(send_voltage,3));         // Sends VOLTAGE as a sensor
    Serial.print("Battery Voltage = ");
    Serial.println(send_voltage);

    // Calculate percentage
    vcc = vcc - 1900; // subtract 1.9V from vcc, as this is the lowest voltage we will operate at

    long percent = vcc / 14.0;
    if (percent>100) percent = 100;
    sendBatteryLevel(percent);                // sends PERCENTAGE, this function is part of MYSENSORS library
    Serial.print("Battery Percent sent: ");
    Serial.print(percent);
    Serial.println("%");
    transmission_occured = true;
  }
}


