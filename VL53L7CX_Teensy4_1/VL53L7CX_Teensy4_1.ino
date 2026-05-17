/*
  Read an 8x8 array of distances from the VL53L5CX
  By: Nathan Seidle
  SparkFun Electronics
  Date: October 26, 2021
  License: MIT. See license file for more information but you can
  basically do whatever you want with this code.

  This example shows how to increase output frequency.

  Default is 1Hz.
  Using 4x4, min frequency is 1Hz and max is 60Hz
  Using 8x8, min frequency is 1Hz and max is 15Hz

  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/18642

*/
// Modified by Alexander Levinson <alex.levinson555@gmail.com>.

#include <Wire.h>

#include <SparkFun_VL53L5CX_Library.h> //http://librarymanager/All#SparkFun_VL53L5CX

SparkFun_VL53L5CX myImager;
VL53L5CX_ResultsData measurementData; // Result data class structure, 1356 byes of RAM

int imageResolution = 0; //Used to pretty print output
int imageWidth = 0; //Used to pretty print output

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("SparkFun VL53L5CX Imager Example");

  Wire.begin(); //This resets I2C bus to 100kHz
  Wire.setClock(1000000); //Sensor has max I2C freq of 1MHz

  Serial.println("Initializing sensor board. This can take up to 10s. Please wait.");
  if (myImager.begin() == false)
  {
    Serial.println(F("Sensor not found - check your wiring. Freezing"));
    while (1) ;
  }

  myImager.setResolution(8 * 8); //Enable all 64 pads

  imageResolution = myImager.getResolution(); //Query sensor for current resolution - either 4x4 or 8x8
  imageWidth = sqrt(imageResolution); //Calculate printing width

  //Using 4x4, min frequency is 1Hz and max is 60Hz
  //Using 8x8, min frequency is 1Hz and max is 15Hz
  bool response = myImager.setRangingFrequency(15);
  if (response == true)
  {
    int frequency = myImager.getRangingFrequency();
    if (frequency > 0)
    {
      Serial.print("Ranging frequency set to ");
      Serial.print(frequency);
      Serial.println(" Hz.");
    }
    else
      Serial.println(F("Error recovering ranging frequency."));
  }
  else
  {
    Serial.println(F("Cannot set ranging frequency requested. Freezing..."));
    while (1) ;
  }

  myImager.startRanging();
}

void loop()
{
  //Poll sensor for new data
  if (myImager.isDataReady() == true)
  {
    if (myImager.getRangingData(&measurementData)) //Read distance data into array
    {
      Serial.write(0xFF); //Header

      //Average of mid quadrant of sensor
      int Data = (measurementData.distance_mm[27] + measurementData.distance_mm[28] + 
        measurementData.distance_mm[35] + measurementData.distance_mm[36])/4; 
    
      byte Lo = Data & 0x00FF;
      if (Lo == 0xFF) {
        Lo = Lo - 1; // Crteate a small gap
      }

      Data = Data >> 8;
      byte Hi = Data & 0x00FF;
      if (Hi == 0xFF) {
        Hi = Hi - 1; // Crteate a small gap. If we send 255, our header will lose its original value.
      }
      
      Serial.write(Lo);
      Serial.write(Hi);

      //Average of mid quadrant of sensor.
      //Serial.println(measurementData.distance_mm[27] + measurementData.distance_mm[28] + 
        //measurementData.distance_mm[35] + measurementData.distance_mm[36]); 
    }
  }

  delay(5); //Small delay between polling
}
