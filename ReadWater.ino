void SendWaterSensor(bool forcing)
{
   int WaterLevelReading = ReadWater(Water_AnalogPin);
   if (forcing)
      {
        send(msgWaterLevel.set((int)(WaterLevelReading)));
        Serial.print("Water Level = ");
        Serial.println(WaterLevelReading);
      }
    if (WaterLevelReading<615) 
            { 
               send(msgWaterStatus.set(true));
               Serial.print("WATER MESSAGE SENT  >>  ");
               Serial.println(WaterLevelReading);
            }
}



int ReadWater(int pin)
{
   
   int sensorValue= analogRead(pin);
    if (sensorValue >= 820 && oldWaterValue<820)
 {
 Serial.println("Reading very high - no water");
 send(msgWaterStatus.set(false));
}
else if (sensorValue >= 615  && sensorValue < 820 && oldWaterValue<615)
 {
 Serial.println(" high - some water detected");
 send(msgWaterStatus.set(false));
}  
else if (sensorValue >= 410 && sensorValue < 615 && oldWaterValue>615)
 {
 Serial.println(" medium - more water detected - TRIGGER THE ALARM");
 send(msgWaterStatus.set(true));
 
}    
else if (sensorValue >= 250 && sensorValue < 410 && oldWaterValue>410)
 {
 Serial.println(" low - lots of water detected!!");
 send(msgWaterStatus.set(true));
}
else if (sensorValue >= 0 && sensorValue < 250 && oldWaterValue>250)
 {
 Serial.println(" very low - FLOODING!");
 send(msgWaterStatus.set(true));
}
 oldWaterValue=sensorValue;
 return(sensorValue);
}

