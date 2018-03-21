#include <TimerOne.h>
#include <QTRSensors.h>
//------Pins------
//0-7 DAC
//17-20 IR Sensors
//21 Grayscale
//22-23 Motor Controller PWM
//8-11 Motor Controller H-Bridge
//14-16 Buttons
//13 LED
//12 Free
//----------------

//buttons
int inPinFast = A2, inPinMed = 13, inPinSlow = 10, buttonReading = 0 ;
bool buttonFast = false,  buttonSlow = false, buttonMed = false;

//const int noteC = 30;//262; 
//const int noteD = 27;//294.8;
//const int noteE = 24;//327.5;
//const int noteF = 22;//349.3;
//const int noteG = 20;//393;
//const int noteA = 18;//436.7;
//const int noteB = 16;//491.2;
//const int noteC2 = 15;//524;

const int noteG = 20;//196;
const int noteA = 18;//220;
const int noteB = 16;//246.94;
const int noteC = 15;//261.63; 
const int noteD = 13;//293.66;
const int noteE = 12;//329.63;
const int noteF = 11;//349.23;
const int noteG2 = 10;//392;

int freq = 0; //To keep track of current frequency
int lastFreq = 0;

//sinwave:
int sinWave [] = {0,
                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                  1, 1, 1, 1, 2, 2, 2, 3, 3, 3,
                  4, 4, 5, 5, 5, 6, 6, 7, 7, 8,
                  8, 9, 10, 10, 11, 11, 12, 13, 13, 14,
                  15, 15, 16, 17, 17, 18, 19, 19, 20, 21,
                  22, 22, 23, 24, 25, 25, 26, 27, 28, 29,
                  29, 30, 31, 32, 32, 33, 34, 35, 36, 36,
                  37, 38, 39, 39, 40, 41, 42, 42, 43, 44,
                  44, 45, 46, 46, 47, 48, 48, 49, 50, 50,
                  51, 51, 52, 53, 53, 54, 54, 55, 55, 56,
                  56, 56, 57, 57, 58, 58, 58, 59, 59, 59,
                  60, 60, 60, 60, 61, 61, 61, 61, 61, 61,
                  61, 61, 61, 61, 62, 61, 61, 61, 61, 61,
                  61, 61, 61, 61, 61, 60, 60, 60, 60, 59,
                  59, 59, 58, 58, 58, 57, 57, 56, 56, 56,
                  55, 55, 54, 54, 53, 53, 52, 51, 51, 50,
                  50, 49, 48, 48, 47, 46, 46, 45, 44, 44,
                  43, 42, 42, 41, 40, 39, 39, 38, 37, 36,
                  36, 35, 34, 33, 32, 32, 31, 30, 29, 29,
                  28, 27, 26, 25, 25, 24, 23, 22, 22, 21,
                  20, 19, 19, 18, 17, 17, 16, 15, 15, 14,
                  13, 13, 12, 11, 11, 10, 10, 9, 8, 8,
                  7, 7, 6, 6, 5, 5, 5, 4, 4, 3,
                  3, 3, 2, 2, 2, 1, 1, 1, 1, 0,
                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                  0, 0, 0, 0, 0
                 };
int sinIndex = 0;

//IR
int sensorIRLeft = A0, sensorIRRight = A1;

//Grayscale
int grayscale = A5;

//QTRSensorsAnalog qtra((unsigned char[]) {17,18,19,20},4);
unsigned int sensorIR[4] = {0}; //0,1 left, 2,3 right
bool readingsIR [4] = {0};

int motorRight[2] = {8, 9};
int motorLeft[2] = {12, 11};
const int MAXPWM = 150, MIDPWM = 100, SLOWPWM = 56;
int usedPWM = 48;

void getButtonPress(int buttonReading, bool & but)
{
  if (buttonReading == LOW)
  {
    but = !but;
    delay(400);
  }
}

void setup() 
{
  //MOTORS
  for (int i = 0; i < 2; i++)
  {
    pinMode(motorLeft[i], OUTPUT);
    pinMode(motorRight[i], OUTPUT);
  }

  //IR
  pinMode(sensorIRLeft, INPUT);
  pinMode(sensorIRRight, INPUT);

  //DAC
  for (int i = 0; i < 8; i++)
  {
    pinMode(i, OUTPUT);
  }
  pinMode(A4, OUTPUT);
  pinMode(A3, OUTPUT);

  //buttons
  pinMode(inPinFast, INPUT);
  pinMode(inPinMed, INPUT);
  pinMode(inPinSlow, INPUT);

    while(!(buttonFast||buttonMed||buttonSlow))
    {
      getButtonPress(digitalRead(inPinFast), buttonFast);
      getButtonPress(digitalRead(inPinMed), buttonMed);
      getButtonPress(digitalRead(inPinSlow), buttonSlow);
    }
  
    if(buttonFast)
      usedPWM = MAXPWM;
    if(buttonMed)
      usedPWM = MIDPWM;
    if(buttonSlow)
      usedPWM = SLOWPWM;

  Timer1.initialize(20); //Setting initial frequency to 0
  Timer1.attachInterrupt(timerIsr);

  Serial.begin(9600);
} 

void loop() 
{
    runMotors2(digitalRead(sensorIRLeft),digitalRead(sensorIRRight));

    cli();
    if(getFrequency(analogRead(grayscale)) != freq)
      freq = getFrequency(analogRead(grayscale));
    sei();

    Serial.print(analogRead(grayscale), DEC);
    
    Serial.print("   ");
    Serial.print(lastFreq);
    Serial.print("   ");
    Serial.println(freq);
}

void runMotors2(bool left, bool right)
{
  float motorLeftSpeed = 1.9 * usedPWM;
  float motorRightSpeed = 1.6*usedPWM;
  float lowerSpeedRight = 0;//0.5;
  float lowerSpeedLeft = 0;// 0.65;
  float higherSpeedRight = 2.6;//2.4;
  float higherSpeedLeft = 1;//2.4;
  if (left && right)
  {
    analogWrite(motorLeft[1], motorLeftSpeed);
    analogWrite(motorRight[1], motorRightSpeed);
    //Serial.println("full");
  } else if (left)
  {
    analogWrite(motorLeft[1], (int)(motorLeftSpeed));
    analogWrite(motorRight[1], (int)(motorRightSpeed)*lowerSpeedRight);
    //Serial.println("right");
  } else if (right)
  {
    analogWrite(motorLeft[1], (int)(motorLeftSpeed)*lowerSpeedLeft);
    analogWrite(motorRight[1], (int)(motorRightSpeed)*higherSpeedRight);
    delay(400);
    //Serial.println("left");
  } else
  {
    analogWrite(motorLeft[1], 0);
    analogWrite(motorRight[1], 0);
    //Serial.println("none");
  }
  digitalWrite(motorLeft[0], HIGH);
  digitalWrite(motorRight[0], HIGH);
}

void notBlack(unsigned int* rawReadings, bool* readings )
{

  for (int i = 0; i < 4; i++)
  {
    Serial.print(rawReadings[i]);
    Serial.print(' ');
    if (rawReadings[i] < 750)
      readings[i] = 1;
    else
      readings[i] = 0;
  }
  Serial.println();

}

int getFrequency(int grayScaleReading)
{
 if (grayScaleReading < 108 )
    return noteG;
  else if (grayScaleReading < 120)
    return noteA;
  else if (grayScaleReading < 126)
    return noteB;
  else if (grayScaleReading < 145)
    return noteC;
  else if (grayScaleReading < 168)
    return noteD;
  else if (grayScaleReading < 185)
    return noteE;
  else if (grayScaleReading < 200)
    return noteF;
  else if (grayScaleReading < 450)
    return noteG2;

  return 0;
}

void DAC()
{
  for (int i = 0; i < 256; i+=127)
  {
    PORTD = (byte(sinWave[i]) & 63) << 2;
    Serial.println(byte(sinWave[i]) / 64);
  }
  Serial.print(" ");
  delay(100);
}

void timerIsr()
{
    if(freq != lastFreq)
    {
      delay(400);
      Timer1.initialize(freq);
      lastFreq = freq;
      sinIndex = 0;
    }    
  
    PORTD = (byte(sinWave[sinIndex]) & 63) << 2;
    if(sinIndex == 255)
      sinIndex = 0;
    else
      sinIndex++;
}
