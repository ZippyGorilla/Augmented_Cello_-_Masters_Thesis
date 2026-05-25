/**
This file is based off of the below. Edited by Alex.levinson555@gmail.com. 
Help from Claude.ai was used on lines 107-110 of this code (removing ‘data-ready’ flags from the sensor) 
to help with integration between the VL53L3CX and the Teensy 4.1 .
 ******************************************************************************
 * @file    VL53L3CX_Sat_HelloWorld.ino
 * @author  SRA
 * @version V1.0.0
 * @date    30 July 2020
 * @brief   Arduino test application for the STMicrolectronics VL53L3CX
 *          proximity sensor satellite based on FlightSense.
 *          This application makes use of C++ classes obtained from the C
 *          components' drivers.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2020 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */
/*
 * To use this sketch you need to connect the VL53L3CX satellite sensor directly to the Nucleo board with wires in this way:
 * pin 1 (Interrupt) of the VL53L3CX satellite connected to pin A2 of the Nucleo board 
 * pin 2 (SCL_I) of the VL53L3CX satellite connected to pin D15 (SCL) of the Nucleo board with a Pull-Up resistor of 4.7 KOhm
 * pin 3 (XSDN_I) of the VL53L3CX satellite connected to pin A1 of the Nucleo board
 * pin 4 (SDA_I) of the VL53L3CX satellite connected to pin D14 (SDA) of the Nucleo board with a Pull-Up resistor of 4.7 KOhm
 * pin 5 (VDD) of the VL53L3CX satellite connected to 3V3 pin of the Nucleo board
 * pin 6 (GND) of the VL53L3CX satellite connected to GND of the Nucleo board
 * pins 7, 8, 9 and 10 are not connected.
 */
/* Includes ------------------------------------------------------------------*/
#include <Arduino.h>
#include <Wire.h>
#include <vl53lx_class.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>

#define DEV_I2C Wire
#define SerialPort Serial

#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif
#define LedPin LED_BUILTIN

// Components.
VL53LX sensor_vl53lx_sat(&DEV_I2C, 14);


/* Setup ---------------------------------------------------------------------*/

void setup()
{

   // Led.
   pinMode(LedPin, OUTPUT);

   // Initialize serial for output.
   SerialPort.begin(115200);
   SerialPort.println("Starting...");

   DEV_I2C.begin();
   SerialPort.println("I2C started");

   sensor_vl53lx_sat.begin();
   SerialPort.println("Sensor begin done");

   sensor_vl53lx_sat.VL53LX_Off();
   SerialPort.println("Sensor off done");

   sensor_vl53lx_sat.InitSensor(0x52);
   SerialPort.println("Sensor init done");

   // 9ms --> most accuracy for the response time needed.
   //sensor_vl53lx_sat.VL53LX_SetMeasurementTimingBudgetMicroSeconds(15000); 

   sensor_vl53lx_sat.VL53LX_StartMeasurement();
   SerialPort.println("Measurement started");
}

void loop()
{
   VL53LX_MultiRangingData_t MultiRangingData;
   VL53LX_MultiRangingData_t *pMultiRangingData = &MultiRangingData;
   int no_of_object_found = 0, j;
   int status;

   delay(5); //50 is stable. 5 or 9 are possible, but get really jittery, even breaking through max msp limiters?

   status = sensor_vl53lx_sat.VL53LX_GetMultiRangingData(pMultiRangingData);

   if (status == 0)
   {
      no_of_object_found = pMultiRangingData->NumberOfObjectsFound;
      //SerialPort.print("Total objects found: ");
      //SerialPort.println(no_of_object_found);
      for (j = 0; j < no_of_object_found; j++)
      {
         //700 is the max length of a cello string between bridge and nut.
         if (pMultiRangingData->RangeData[j].RangeMilliMeter < 700) { 

            Serial.write(0xFF); //Header

            int Data = pMultiRangingData->RangeData[j].RangeMilliMeter;
    
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
         } // Trying to set else 'open string' statement creates some jumpyness...

      }
      sensor_vl53lx_sat.VL53LX_ClearInterruptAndStartMeasurement();
   }
   else
   {
      SerialPort.print("GetMultiRangingData failed, status: ");
      SerialPort.println(status);
   }
}
