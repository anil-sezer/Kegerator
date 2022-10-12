#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TM1637.h>

// todo: use multiple files

// Enable either the kegerator or the fermenter temperatures to adapt the fridge to that setting.

// KEGERATOR
float idealMinimum = 4;
float idealMaximum = 8;
// FERMENTER
// float idealMinimum = 20;
// float idealMaximum = 23;

// ******************************

// TEMPERATURE
// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// ******************************

// RELAY
int relay = 8;
volatile byte relayState = LOW;

// ******************************

// DISPLAY
int CLK = 3;
int DIO = 4;
TM1637 tm(CLK,DIO);
// 1 2 3 4 5 6 7 8 9 10(a) 11(b) 12(c) 13(d) 14(e) 15(f)

// ******************************

// GENERIC
int oneSecond = 1000;
long oneMinute = 60000;
long workingTimeBeforeCooldown = oneMinute * 10;
long cooldownPeriod = oneMinute * 2;

float prevtemp = 0.0;
unsigned long totalRuntime = 0;
long runTimeBeforeCooldown = 0;

void triggerTheFridge(float temp);
float getTemp(void);
float errorResistance(float temp);
void setDisplay(int currentTemp, int prevTemp);
bool isTempIsNotBugged(float temp);
void coolDownTheCompressor();
void blinkWhileDelaying(long delayAmount, int blinkFrequency);

void setup(void)
{
  // start serial port
  Serial.begin(9600);
  Serial.println("Ready to roll!\n****************************************");

  // Initialize Display
  int aa = 10;
  tm.set(5);
  tm.point(1);
  tm.display(0, aa);
  tm.display(1, aa);
  tm.display(2, aa);
  tm.display(3, aa);

  // Start up the library
  sensors.begin();

  // Begin relay
  // Pin for relay module set as output
  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH);

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

  if (runTimeBeforeCooldown != 0)
  {
    Serial.println("Runtime before cooldown is: " + String(runTimeBeforeCooldown / oneMinute) + " minutes. Will enter cooldown at minute " + String(workingTimeBeforeCooldown / oneMinute)); // This runtime feature needs improvement
  }
  
  coolDownTheCompressor();
  blinkWhileDelaying(oneMinute / 2, oneSecond * 2);
  // Serial.println("Loop complete. Total runtime is: " + String(totalRuntime / oneMinute) + " minutes."); // todo: needs a solution. Maybe only add when setting it to zero?
  Serial.println("\n\n");
}

// The temp sensor is probably having some mechanical problems. This band aids it.
float errorResistance(float temp)
{
  if (isTempIsNotBugged(temp))
  {
    Serial.println("Temp is ok, not bugged.");
    return temp;
  }

  Serial.println("Temp is bugged!");
  long tenSecs = oneSecond*10;
  long fiveMins = oneMinute*5;
  for (long i = tenSecs; i <= fiveMins; i += tenSecs)
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

bool isTempIsNotBugged(float temp)
{
  return temp > -4 && temp < 50;
}

float getTemp(void)
{
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  sensors.requestTemperatures(); // Send the command to get temperatures
  float temp = sensors.getTempCByIndex(0);
  Serial.println("Temp: " + String(temp, 3));

  return temp;
}

void setDisplay(int currentTemp, int prevTemp)
{
  int ff = 15;  

  if (isTempIsNotBugged(prevTemp))
  {
    tm.display(0, prevTemp / 10 % 10);
    tm.display(1, prevTemp % 10);
  } else {
    tm.display(0, ff);
    tm.display(1, ff);
  }

  if (isTempIsNotBugged(currentTemp))
  {
    tm.display(2, currentTemp / 10 % 10);
    tm.display(3, currentTemp % 10);
  } else {
    tm.display(2, ff);
    tm.display(3, ff);
  }
}

void triggerTheFridge(float temp)
{
  if (temp > idealMinimum && temp < idealMaximum)
  {
    digitalWrite(relay, LOW);
    Serial.println("Temperature is in the ideal range, SUSPEND. Temp: " + String(temp, 3));
  }
  else if(!isTempIsNotBugged(temp)){
    digitalWrite(relay, LOW);
    Serial.println("Looks like a buggy temp, SUSPEND. Temp: " + String(temp, 3));
  }
  else if (temp < idealMinimum)
  {
    digitalWrite(relay, LOW);
    Serial.println("Temperature is low, SUSPEND. Temp: " + String(temp, 3));
  }
  else if (temp > idealMaximum)
  {
    Serial.println("Temperature is too high, START. Temp: " + String(temp, 3));
    digitalWrite(relay, HIGH);
  }
}

void coolDownTheCompressor()
{
  if (runTimeBeforeCooldown < workingTimeBeforeCooldown)
  {
    return;
  }
  if (digitalRead(relay) == LOW)
  {
    runTimeBeforeCooldown = 0;
    return;
  }
  
  Serial.print("Compressor needs to cool down for " + String(cooldownPeriod/oneMinute) + " minutes... ");

  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(relay, LOW);
  tm.display(2, 12); // c
  tm.display(3, 13); // d

  delay(cooldownPeriod);
  runTimeBeforeCooldown = 0;
  Serial.print("Cooldown complete.\n");
}

void blinkWhileDelaying(long delayAmount, int blinkFrequency)
{
  runTimeBeforeCooldown += delayAmount; // This will be used to cool down the fridge compressor if needed. Compressor is struggling.

  for (;delayAmount >= 0;)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(blinkFrequency/2);
    digitalWrite(LED_BUILTIN, LOW);
    delay(blinkFrequency/2);
    delayAmount = delayAmount - blinkFrequency;
  }
}
