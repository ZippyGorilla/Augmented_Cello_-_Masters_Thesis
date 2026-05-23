#define NumberOf(arg) ((unsigned int) (sizeof (arg) / sizeof (arg [0]))) // calculates the number of layers (in this case 3)
#define _2_OPTIMIZE 0B00000000 // Enable 0B01.. for NO_BIAS or 0B001.. for MULTIPLE_BIASES_PER_LAYER
#define _1_OPTIMIZE 0B00010000 // https://github.com/GiorgosXou/NeuralNetworks#define-macro-properties
#define Tanh                   // Comment this line to use Sigmoid (the default) activation function

#include <NeuralNetwork.h>
#include <vector>
#include <CircularBuffer.hpp>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>
#include "I2C_MPU6886.h"

// **TODO?**For saving trianing data to internal EEPROM
//#define IN_EEPROM_ADDRESS 0   // The position at which the NN will be saved at the internal EEPROM
//#include <EEPROM.h>

// **TODO?** For reading saved training data from internal EEPROM. 
// https://github.com/MusicallyEmbodiedML/NeuralNetworks/blob/master/examples/Media/FeedForward_from_internal_EEPROM/FeedForward_from_internal_EEPROM.ino
//#define _1_OPTIMIZE 0B01000110 // https://github.com/GiorgosXou/NeuralNetworks#define-macro-properties
//#define _2_OPTIMIZE 0B10000000 // Enable the use of Internal EEPROM
//#define FROM_EEPROM_ADDRESS 0  // The position at which the NN will be saved in the internal EEPROM

float *output;

// For M5Stack Atom, connect to its esp32 board (instead of "M5Stack" option, etc).

I2C_MPU6886 imu(I2C_MPU6886_DEFAULT_ADDRESS, Wire1);

  float ax;
  float ay;
  float az;
  float x;
  float y;
  float z;

  float td; // Train NN to detect a down bow-change.
  float tu; // Train NN to detect an up bow-change.

WiFiUDP Udp;
const IPAddress outIp(192,168,4,2);         //default IP, will change with received udp
const unsigned int outPort = 9999;          // remote port to receive OSC
const unsigned int localPort = 8888;        // local port to listen for OSC packets (actually not used for sending)
const char *ssid = "HorizontalBow-OSC";

// For Neural network
const int buttonPin= 14;

const size_t nInputs=6;
const size_t nOutputs=1;
const size_t patternElements=20;
const size_t patternSize = patternElements * nInputs;

const unsigned int layers[] = {patternSize, 10, 10, nOutputs}; // 3 layers (1st)layer with 3 input neurons (2nd)layer 5 hidden neurons each and (3rd)layer with 1 output neuron

// **TODO? EEPROM** float *output;


std::vector<std::vector<float> > trainingInputs;
std::vector<std::vector<float> > trainingOutputs;

CircularBuffer<float, patternSize> patternBuffer;

enum NNMODES {TRAINING, INFERENCE};

NNMODES nnMode = NNMODES::TRAINING;


std::vector<std::vector<float>> expectedOutput {{0}, {0.5}, {1}};


NeuralNetwork NN(layers, NumberOf(layers)); // Creating a Neural-Network with default learning-rates

void addTrainingPoint(std::vector<float> x, size_t y) { //training inputs, and index to a set of outputs defined in expectedOutputs
  if (x.size() == patternSize) {
    trainingInputs.push_back(x);
    trainingOutputs.push_back(expectedOutput[y]);
    Serial.print("Training point added: ");
    for(auto &v: x) {
      Serial.print(v);
      Serial.print("\t");
    }
    Serial.print(" Output: ");
    Serial.println(y);
  }else{
    Serial.print("The number of inputs should be: ");
    Serial.println(patternSize);
  }
}

void undoLastTraining() {
  if (trainingInputs.size() > 0) {
    trainingInputs.pop_back();
    trainingOutputs.pop_back();
    Serial.println("Removed most recent training point");
  }else{
    Serial.println("There are no training points to remove");
  }
}

void train() {
  size_t maxEpochs = 500;
  do{
    for (unsigned int j = 0; j < trainingInputs.size(); j++) // Epoch
    {
      NN.FeedForward(trainingInputs[j].data());      // FeedForwards the input arrays through the NN | stores the output array internally
      NN.BackProp(trainingOutputs[j].data()); // "Tells" to the NN if the output was the-expected-correct one | then, "teaches" it
    }

    // Prints the Error.
    Serial.print("MSE: ");
    Serial.println(NN.MeanSqrdError,6);

  }while(NN.getMeanSqrdError(trainingInputs.size()) > 0.01 && maxEpochs-- > 0);
}

void resetTraining() {
  trainingInputs.clear();
  trainingOutputs.clear();
  Serial.println("Reset training data");
}

void resetModel() {
  NN = NeuralNetwork(layers, NumberOf(layers)); // Creating a Neural-Network with default learning-rates
}

void printTrainingData() {
  Serial.println("Training data:");
  for(size_t i=0; i < trainingInputs.size(); i++) {
    Serial.print(i);
    Serial.print(": ");
    for(size_t j=0; j < patternSize; j++) {
      Serial.print(trainingInputs[i][j]);
      Serial.print("\t");
    }
    Serial.print(" :: ");
    for(size_t j=0; j < nOutputs; j++) {
      Serial.print(trainingOutputs[i][j]);
      Serial.print("\t");
    }
    Serial.println("");
  }
}

void buildAndSendOSCMsg() {


   // compile OSC message 
   OSCMessage msg1("/ax"); // formatting name  
   msg1.add(ax); // x acc
   OSCMessage msg2("/ay"); // formatting name  
   msg2.add(ay); // 
   OSCMessage msg3("/az"); // formatting name  
   msg3.add(az); // 

   OSCMessage msg4("/x"); // formatting name  
   msg4.add(x); // 
   OSCMessage msg5("/y"); // formatting name  
   msg5.add(y); 
   OSCMessage msg6("/z"); // formatting name  
   msg6.add(z); 

   OSCMessage msg7("/train-down"); // formatting name  
   msg7.add(td); 
   OSCMessage msg8("/train-up"); // formatting name  
   msg8.add(tu); 

   // send message
   Udp.beginPacket(outIp, outPort); // ... transfer data over Network IP 1
   msg1.send(Udp);
   Udp.endPacket();
   msg1.empty();


   Udp.beginPacket(outIp, outPort); // ... transfer data over Network IP
   msg2.send(Udp);
   Udp.endPacket();
   msg2.empty();

   Udp.beginPacket(outIp, outPort); // ... transfer data over Network IP 1
   msg3.send(Udp);
   Udp.endPacket();
   msg3.empty();
  

  // send message
   Udp.beginPacket(outIp, outPort); // ... transfer data over Network IP 1
   msg4.send(Udp);
   Udp.endPacket();
   msg4.empty();


   Udp.beginPacket(outIp, outPort); // ... transfer data over Network IP
   msg5.send(Udp);
   Udp.endPacket();
   msg5.empty();

   Udp.beginPacket(outIp, outPort); // ... transfer data over Network IP 1
   msg6.send(Udp);
   Udp.endPacket();
   msg6.empty();

   Udp.beginPacket(outIp, outPort); 
   msg7.send(Udp);
   Udp.endPacket();
   msg7.empty();

   Udp.beginPacket(outIp, outPort); 
   msg8.send(Udp);
   Udp.endPacket();
   msg8.empty();


}

// For Wifi: baud 115250 (refresh rate - bits p/s), I2C, LED
void setup()
{
  pinMode(buttonPin, INPUT);
  Serial.begin(115200);
  Serial.setTimeout(1);
  Wire1.begin(25, 21);
  imu.begin();

  // Wifi below.
  Wire1.begin(25, 21);

  imu.begin();
  WiFi.softAP(ssid);
  IPAddress myIP = WiFi.softAPIP();
  //Serial.begin(115200);
  Serial.println("IP address: ");
  Serial.println(myIP);

  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Sending to port: ");
  Serial.println(outPort);
  // End Wifi.
}
int count=0;

std::vector<float> p(patternSize);

void loop() {

  // OSC data:
  imu.getAccel(&ax, &ay, &az);
  imu.getyro(&x, &y, &z);
  //Serial.println(x);

   buildAndSendOSCMsg();
  delay(20);
  // End OSC data

  int button = !digitalRead(buttonPin);
  float ax;
  float ay;
  float az;
  float x;
  float y;
  float z;

  imu.getAccel(&ax, &ay, &az);
  imu.getyro(&x, &y, &z);
  //push all inputs into the pattern buffer
  patternBuffer.push(ax/10.0);
  patternBuffer.push(ay/10.0);
  patternBuffer.push(az/10.0);
  patternBuffer.push(x/1000.0);
  patternBuffer.push(y/1000.0);
  patternBuffer.push(z/1000.0);
  patternBuffer.copyToArray(p.data());

  if (Serial.available()) {
    String command = Serial.readString();
    if (command != "") {
      command = command[0]; //strip out \n
      Serial.println(command);
      if (command == "t") { //train
        train();
      }
      else if (command == "i") { //toggle inference
        if (nnMode == NNMODES::TRAINING) {
          nnMode = NNMODES::INFERENCE;
          Serial.println("Mode: Inference");
        }else {
          nnMode = NNMODES::TRAINING;
          Serial.println("Mode: Training");
        }
      }   
      else if (command == "a") { //status
        printTrainingData();
      }   
      else if (command == "r") { //reset training
        resetTraining();
      }   
      else if (command == "m") { //reset model
        resetModel();
      }   
      else if (command == "u") { //undo last data point
        undoLastTraining();       
      }   
      else if (command == "s") {
        //**TODO?**
        // From https://github.com/MusicallyEmbodiedML/NeuralNetworks/blob/master/examples/Media/Save_NN_to_internal_EEPROM/Save_NN_to_internal_EEPROM.ino
        //unsigned int endAddress = NN.save(IN_EEPROM_ADDRESS); // Saves the NN IN_EEPROM_ADDRESS and (optionally)returns where it ended
        //Serial.println("Saved neural-network of " + String(endAddress - IN_EEPROM_ADDRESS) + "-Bytes into the internal EEPROM of the MCU");
      }
      else if (isDigit(command[0])) {
        addTrainingPoint(p, command.toInt());
      }   
    }
  }
  if (nnMode == NNMODES::INFERENCE) {
    output = NN.FeedForward(p.data());
    for(size_t j=0; j < nOutputs; j++) {
      Serial.print(output[j], 7);       // Prints the first 7 digits after the comma.
      Serial.print("\t");
    }
    Serial.println("");
  }
  delay(20);
  Serial.printf("%f,%f,%f,%f,%f,%f\n", (ax/10.0), (ay/10.0), (az/10.0), (x/1000.0), (y/1000.0), (z/1000.0));
  //Serial.printf("%f,%f,%f\n", (ax/10.0), (az/10.0), x/10.0);
}
