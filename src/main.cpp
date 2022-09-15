#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TM1637.h>

// TEMP
// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// RELAY
int relay = 8;
volatile byte relayState = LOW;

// DISPLAY
int CLK = 3;
int DIO = 4;
TM1637 tm(CLK,DIO);

// GENERIC
int oneSecond = 1000;
long oneMinute = 60000;
float prevtemp = 0.0;

void triggerTheFridge(float temp);
float getTemp(void);
float errorResistance(float temp);
void setDisplay(int currentTemp, int prevTemp);
bool isTempIsNotBugged(float temp);
void blinkWhileDelaying(long delayAmount, int blinkFrequency);

void setup(void)
{
  // start serial port
  Serial.begin(9600);

  // Start up the library
  sensors.begin();
  tm.set(5);

  // Begin relay
  // Pin for relay module set as output
  pinMode(relay, OUTPUT);
  digitalWrite(relay, LOW); // Begin as low

  // Init built in led for signalling error handling
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop(void)
{
  float temp = getTemp();
  temp = errorResistance(temp);

  setDisplay((int)temp, (int)prevtemp);

  triggerTheFridge(temp);

  prevtemp = temp;

  blinkWhileDelaying(oneMinute / 2, oneSecond * 2);
}

// The temp sensor is probably having some mechanical problems. This bandaids it.
float errorResistance(float temp){
  if (isTempIsNotBugged(temp))
  {
    Serial.println("Temp is ok");
    return temp;
  }

  Serial.println("Temp is bugged!");
  for (long i = oneSecond*10; i <= oneMinute*5; i += oneSecond*10)
  {
    Serial.println("Error resistance has been triggered. Delay for " + String(i / oneSecond) + " seconds");

    blinkWhileDelaying(i, 300);

    temp = getTemp();

    if (isTempIsNotBugged(temp))
    {
      Serial.println("Error resolved, correct temp is: " + String(temp, 3));
      return temp;
    }
  }

  Serial.println("Error still continues: " + String(temp, 3));
  return temp;
}

bool isTempIsNotBugged(float temp){
  return temp > -4 && temp < 40;
}

float getTemp(void){
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  sensors.requestTemperatures(); // Send the command to get temperatures
  float temp = sensors.getTempCByIndex(0);
  Serial.println("Temp: " + String(temp, 3));

  return temp;
}

void setDisplay(int currentTemp, int prevTemp){
    // put your main code here, to run repeatedly:
  
  // example: "12:ab"
  // tm.display(position, character);

  if (isTempIsNotBugged(prevTemp))
  {
    tm.display(0, prevTemp / 10 % 10);
    tm.display(1, prevTemp % 10);
  } else {
    tm.display(0,15);
    tm.display(1,15);
  }

  tm.point(1);

  if (isTempIsNotBugged(currentTemp))
  {
    tm.display(2,currentTemp / 10 % 10);
    tm.display(3, currentTemp % 10);
  } else {
    tm.display(2,15);
    tm.display(3,15);
  }
}

void triggerTheFridge(float temp){

  float idealMinimum = 20;
  float idealMaximum = 23;

  if (temp > idealMinimum && temp < idealMaximum)
  {
    digitalWrite(relay, LOW);
    Serial.println("Temperature is in the ideal range. Temp: " + String(temp, 3));
  }
  else if(temp < 10 || temp > 40){
    digitalWrite(relay, LOW);
    Serial.println("Looks like a buggy temp, suspend. Temp: " + String(temp, 3));
  }
  else if (temp < idealMinimum)
  {
    digitalWrite(relay, LOW);
    Serial.println("Temperature is low, suspend. Temp: " + String(temp, 3));
  }
  else if (temp > idealMaximum)
  {
    Serial.println("Temperature is too high, start. Temp: " + String(temp, 3));
    digitalWrite(relay, HIGH);
  }
}

void blinkWhileDelaying(long delayAmount, int blinkFrequency)
{
  for (;delayAmount >= 0;)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(blinkFrequency/2);
    digitalWrite(LED_BUILTIN, LOW);
    delay(blinkFrequency/2);
    delayAmount = delayAmount - blinkFrequency;
  }
}
