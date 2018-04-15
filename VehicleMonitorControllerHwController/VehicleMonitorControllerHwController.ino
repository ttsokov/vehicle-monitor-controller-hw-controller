#include <LiquidCrystal.h>

const boolean IS_SIMULATION = false;
const float MAX_MAP_VALUE = 2.0;//bar

//LCD 
const int RS = 8;
const int E = 7;
const int DB4 = 6;
const int DB5 = 5;
const int DB6 = 4;
const int DB7 = 3;
LiquidCrystal lcd(RS, E, DB4, DB5, DB6, DB7); // pins for RS, E, DB4, DB5, DB6, DB7 previous:4, 5, 6, 7, 8, 9

//Lambda
const int voltageDividerAnalogPin = A0;
const int lambdaAnalogPin = A2;
int lambdaAnalogValue;
float lambdaVoltageValue;
float lambdaValue;
float afrValue;
float maxAFRValue = 0.0;
float minAFRValue = 0.0;

//MAP
const int mapAnalogPin = A1;
int mapAnalogValue;
float mapVoltageValue;
float mapValue;
float maxMAPValue = 0.0;

//TEMP
const int tempAnalogPin = 3; //means A3
int tempValue;
int maxTEMPValue = 0;

//Button
const int maxValuesResetButtonPin = 9;
int maxValuesResetButtonState = 0;

//GPIO Output LED
const int ledOutput = 10;

byte serialNumber = 0;

String stringToTransmit = "";
long lastSerialTransmition = 0;
const long serialTransmitionDelay = 4000;
boolean shouldTransmitAFR = true;
boolean shouldTransmitMAP = false;
boolean shouldTransmitTEMP = false;

void setup(){
  Serial.begin(9600); 
  setupLCD();
  pinMode(maxValuesResetButtonPin,INPUT);
}

void loop(){
  getAFR(IS_SIMULATION);
  getMAP(IS_SIMULATION);
  getTEMP(IS_SIMULATION);

  populateLCD();
  handleButton();

  transmitDetectedAFR();
  transmitDetectedMAP();
  transmitDetectedTEMP();

/*  
  if(Serial.available()){
    serialNumber = Serial.read();
    Serial.print("Character received: ");
    Serial.println(serialNumber);
  }
*/
}

void getAFR(boolean isSimulation) {
  if (isSimulation) {
    getLambdaSimulation();
  } else {
    getLambdaReal();
  }

  if (afrValue > maxAFRValue) {
    maxAFRValue = afrValue;
  }
  if (afrValue < minAFRValue) {
    minAFRValue = afrValue;
  }
}

void getLambdaSimulation(){
  lambdaAnalogValue = analogRead(voltageDividerAnalogPin);// 0>1024
  lambdaVoltageValue = lambdaAnalogValue*(5.0/1024.0);
  lambdaValue = constrain(lambdaVoltageValue, 0.5 , 1.5);
  afrValue = lambdaValue*14.7;
}

void getLambdaReal(){
  lambdaAnalogValue = analogRead(lambdaAnalogPin);
  lambdaVoltageValue = lambdaAnalogValue*(5.0/1024.0);
  afrValue = (-5)*lambdaVoltageValue + 17;
}

void getMAP(boolean isSimulation) {
  if (isSimulation) {
    getMAPSimulation();
  } else {
    getMAPReal();
  }

  if (mapValue > maxMAPValue) {
    maxMAPValue = mapValue;
  }
}

void getMAPSimulation(){
  mapAnalogValue = analogRead(voltageDividerAnalogPin);
  mapVoltageValue = mapAnalogValue*(25.0/1024.0);
  mapValue = mapVoltageValue*(MAX_MAP_VALUE/5.0);
}

void getMAPReal(){
  mapAnalogValue = analogRead(mapAnalogPin);
  mapVoltageValue = mapAnalogValue*(5.0/1024.0);
  float mapValueKPa = (mapVoltageValue / 0.022 + 20);
  mapValue = (mapValueKPa * 0.01) - 1.0172; //multiply (1 kPa x 0.01 bar) and deduct atmospheric pressure
}

void getTEMP(boolean isSimulation) {
  if (isSimulation) {
    getTEMPSimulation();
  } else {
    getTEMPReal();
  }

  if (tempValue > maxTEMPValue) {
    maxTEMPValue = tempValue;
  }
}

void getTEMPSimulation(){
  int tempAnalogValue = analogRead(voltageDividerAnalogPin);
  tempValue = tempAnalogValue*(200/1024.0);
}

void getTEMPReal(){
  tempValue = (int) kty(tempAnalogPin);
}

float kty(unsigned int port) {
  float temp = 82;
  ADCSRA = 0x00;
  ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
  ADMUX = 0x00;
  ADMUX = (1<<REFS0);
  ADMUX |= port;   

  for (int i=0;i<=64;i++) {
    ADCSRA|=(1<<ADSC);
    while (ADCSRA & (1<<ADSC));
      temp += (ADCL + ADCH*256);
    }

    temp /= 101;
    temp -= 156;
    return (temp);
}

void setupLCD(){
  lcd.begin(16, 4);
  lcd.clear();
}

void populateLCD(){
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("AFR = ");
  lcd.print(lambdaVoltageValue);
  lcd.print(" ");
  lcd.print(afrValue);
  //lcd.print("%");

  lcd.setCursor(0, 1);
  lcd.print("MAP = ");
  lcd.print(mapValue);
  lcd.print(" ");
  lcd.print(mapVoltageValue);
  //lcd.print(" bar");

  lcd.setCursor(0, 2);
  lcd.print("TEMP = ");
  lcd.print(tempValue);
  lcd.print(" C");

  lcd.setCursor(0, 3);
  lcd.print(maxAFRValue);
  lcd.print(" ");
  lcd.print(minAFRValue);
  lcd.print(" ");
  lcd.print(maxMAPValue);

  delay(500);
}

void handleButton(){
  maxValuesResetButtonState = digitalRead(maxValuesResetButtonPin);
  if (maxValuesResetButtonState == HIGH) {
    maxAFRValue = 0.0;
    minAFRValue = 17.0;
    maxMAPValue = 0.0;
    maxTEMPValue = 0;
  }
}

void transmitDetectedAFR () {
  if (!shouldTransmitAFR || ((millis() - lastSerialTransmition) <= serialTransmitionDelay)) {
    return;
  }
  
  //AFR,22.05,
  stringToTransmit += "AFR,";
  stringToTransmit += afrValue;
  stringToTransmit += ",";
  Serial.print(stringToTransmit);
  lastSerialTransmition = millis();
  stringToTransmit = "";
  shouldTransmitAFR = false;
  shouldTransmitMAP = true;
  shouldTransmitTEMP = false;
}

void transmitDetectedMAP () {
  if (!shouldTransmitMAP || ((millis() - lastSerialTransmition) <= serialTransmitionDelay)) {
    return;
  }
  
  //MAP,1.5,
  stringToTransmit += "MAP,";
  stringToTransmit += mapValue;
  stringToTransmit += ",";
  //mapValue++;
  Serial.print(stringToTransmit);
  lastSerialTransmition = millis();
  stringToTransmit = "";
  shouldTransmitAFR = false;
  shouldTransmitMAP = false;
  shouldTransmitTEMP = true;
}

void transmitDetectedTEMP () {
  if (!shouldTransmitTEMP || ((millis() - lastSerialTransmition) <= serialTransmitionDelay)) {
    return;
  }
  
  //TEMP,32,
  stringToTransmit += "TEMP,";
  stringToTransmit += tempValue;
  stringToTransmit += ",";

  Serial.print(stringToTransmit);
  lastSerialTransmition = millis();
  stringToTransmit = "";
  shouldTransmitAFR = true;
  shouldTransmitMAP = false;
  shouldTransmitTEMP = false;
}

/*
// Receive Serial Event Listener
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
  }
}
*/
