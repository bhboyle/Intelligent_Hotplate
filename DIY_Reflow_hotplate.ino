/*
This is a rework of the Electronoobs DIY reflow hotplate project found here. https://electronoobs.com/eng_arduino_tut155.php

I did a major rewrite of the control code and added PID control of the heater. This made a big difference to the heating and
in my opinion helps a lot.

I also used a small electric hot plate rather than a cloths iron because it was cheap and this way I can make changes to that
portion of the system if I like. https://www.amazon.ca/gp/product/B08R6F5JH8/ref=ppx_yo_dt_b_search_asin_image?ie=UTF8&psc=1

Started April 20th 2022

December 2022
Major changes..

Added additional cooking mode. This means that this is no longer just for SMD soldering.

There is a "Normal mode" that is just like a regular hot plate. You set a setting number that is anywhere between 0 - 100%
that will PWM modulate the SSR on and off at that percentage. Just like a standard hotplate. This mode does not look at
the thermocouple for feedback at all.

There is "intelligent mode" that uses the thermocouple to set the hotplate at a specific temperature and keep it there until
the hotplate is turned off.

Turning off the hotplate is done by pressing button 2 at any point.

*/

// Libraries
#include "max6675.h" //Download it here: http://electronoobs.com/eng_arduino_max6675.php
#include <Wire.h>
#include "LiquidCrystal_I2C.h" //Download it here: http://electronoobs.com/eng_arduino_liq_crystal.php
#include <PID_v1.h>
#include <Encoder.h>
#include <Adafruit_NeoPixel.h>

// definitions - pin definitions and constants
#define neopixelPIN 9        // The output pin for the Neopixels
#define NUMPIXELS 8          // This how many Neopixels are connected to PIN
#define lowTempThreshold 50  // temperature threshold to start turn the LEDs from red to green
#define highTempThreshold 80 // The temperature that the LEDs are fully red
#define SSR 7                // the output pin that the SSR is connected to
#define thermoDO 4           // Data pin for MAX6675
#define thermoCS 5           // CS pin for MAX6675
#define thermoCLK 6          // Clock pin for MAX6675
#define but_1 11             // Button 1 input
#define but_2 12             // This pin will be used to end cooking
#define DT 2                 // Data pin for encoder
#define CLK 3                // Clock pin for encoder

// Variables
double Setpoint, Input, Output;              // variable needed for the PID loop
unsigned long LEDtimer;                      // used to determine when to update the LEDs
unsigned long LEDinterval = 500;             // how often in milliseconds to update the LEDs
unsigned int millis_before, millis_before_2; // used for time tracking in the loop
unsigned int millis_now = 0;                 // used to keep track of the current time of the loop
int refresh_rate = 1000;                     // how often to update the display in milliseconds
int temp_refresh_rate = 300;                 // how often to check the temperature in milliseconds
unsigned int seconds = 0;                    // used in the display to show how long the sequence has been running
bool but_1_state = false;                    // used to track of the button has been pushed. used for debouncing the button
unsigned long but_1_timer = 0;               // used for checking the time for debouncing the button push
int max_temp = 260;                          // ****** this is the high temperature set point. *******
float temperature = 0;                       // this is the variable that holds the current temperature reading
int heatStage = 0;                           // used for keeping track of where we are in the heating sequence
int stage_2_set_point = 150;                 // this is the "soak" temperature set point
unsigned long stage2Timer = 0;               // used to keeping track of when we went into stage two
unsigned long stage4Timer = 0;               // used for keeping track of when we went into stage 4
unsigned long soakTime = 100;                // how long to soak the board for in seconds
int reflowTime = 60;                         // how long to reflow the board for in seconds
int cookMode = 1;                            // This is used to know what mode is selected
boolean cookStatus = 0;                      // this is used to know if we are actively cooking
String Names[] = {
    // this is the text displayed in the display in the various stages
    "Off",
    "Heat",
    "Soak",
    "Blast", // I am sure there is a real name for this stage but I dont know it and this seemed cool...
    "Reflow",
};

// instantiate the objects
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO); // Start MAX6675 SPI communication
LiquidCrystal_I2C lcd(0x27, 20, 4);                  // Address could be 0x3f or 0x27
PID myPID(&Input, &Output, &Setpoint, 7, .01, 0, DIRECT); // create the PID object
Encoder myEnc(DT, CLK);                                   // setup the encoder
Adafruit_NeoPixel pixels(NUMPIXELS, neopixelPIN, NEO_GRB + NEO_KHZ800);

void setup()
{

  myPID.SetMode(AUTOMATIC); // turn the PID controller on

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear(); // Set all pixel colors to 'off'

  pinMode(SSR, OUTPUT);   // Define the OUTPUT for the Solid State Relay
  digitalWrite(SSR, LOW); // Start with the SSR off

  pinMode(but_1, INPUT_PULLUP); // Setup the button input
  pinMode(but_2, INPUT_PULLUP); // Setup the end button input

  Serial.begin(115200);

  lcd.init();      // Init the LCD
  lcd.backlight(); // Activate backlight

  millis_before = millis();
  millis_now = millis();
  displayTemperature();
}

// The main loop
void loop()
{

  // Read encoder and change the cooking mode variable based on the direction
  // the encoder is turned
  int temp = myEnc.readAndReset();
  if (temp > 0)
  {

    cookMode++;
    if (cookMode > 3)
    {
      cookMode = 1;
    }
  }
  else if (temp < 0)
  {

    cookMode--;
    if (cookMode < 1)
    {
      cookMode = 3;
    }
  }

  if (!digitalRead(but_1))
  {
    delay(150);
    switch (cookMode)
    {
    case 1:
      regularCook();
      break;

    case 2:
      intelligentCook();
      break;

    case 3:
      smdCook();
      break;

    default:
      break;
    }
  }

  displayMode();
  displayTemperature();

} // end of void loop

// **************** Functions begin here   **************************

// Check the temperature of the cooktop surface via the thermocouple
// and change the LEDs color based on the temperature reading
void displayTemperature()
{

  if (millis() - LEDtimer > LEDinterval) // Check to see if it is time to update the LEDs with the most recent temperature
  {
    int tempReading = thermocouple.readCelsius(); // read the temperature sensor

    if (tempReading > highTempThreshold)
    { // if temperature is HIGHER than tempThreshold turn the LEDs all RED
      for (int i = 0; i < NUMPIXELS; i++)
      {
        pixels.setPixelColor(i, pixels.Color(150, 0, 0));
        pixels.show(); // Send the updated pixel colors to the hardware.
      }
    }
    else if (tempReading < lowTempThreshold)
    { // if temperature is LOWER than tempThreshold turn the LEDs all GREEN
      for (int i = 0; i < NUMPIXELS; i++)
      {
        pixels.setPixelColor(i, pixels.Color(0, 150, 0));
        pixels.show(); // Send the updated pixel colors to the hardware.
      }
    }
    else
    { // if the temperature is in between the low and high temperature thresholds then
      // transition from green to red

      int temperatureColor = map(temperature - lowTempThreshold, lowTempThreshold, highTempThreshold, 1, 150);
      for (int i = 0; i < NUMPIXELS; i++)
      {
        pixels.setPixelColor(i, pixels.Color(temperatureColor, 150 - temperatureColor, 0));
        pixels.show(); // Send the updated pixel colors to the hardware.
      }
    }
    LEDtimer = millis(); // take note of the current time for the next pass of this function
  }
} // end of displayTemperature function

// This function displays the currently selected cooking mode on the LCD
void displayMode()
{

  lcd.setCursor(0, 0);
  switch (cookMode)
  {
  case 1:
    lcd.print("Standard Mode   ");
    break;
  case 2:
    lcd.print("Intelligent Mode");
    break;
  case 3:
    lcd.print("SMD Mode        ");
  default:
    lcd.print("     FAULT!     ");
    break;
  }
  lcd.setCursor(1, 0);
  lcd.print("                ");
}

// This function will cook using the thermocouple
// and keep the burner at a specific temperature
void intelligentCook()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Intelli-Cook");
  lcd.setCursor(1, 0);
  lcd.print("Set Temp:");
  int temperatureSetPoint = 0;
  int temp = 0;

  while (1) // start of the loop for the intelligent cooking loop
  {
    temp = myEnc.readAndReset();

    if (temp > 0) // keep the temperature within bounds
    {
      temperatureSetPoint++;
      if (temperatureSetPoint > 600)
      {
        temperatureSetPoint = 600;
      }
    }
    else if (temp < 0)
    {
      temperatureSetPoint--;
      if (temperatureSetPoint < 0)
      {
        temperatureSetPoint = 0;
      }
    }
    // this is the code that manages the cooking

    millis_now = millis(); // track the current time

    if (millis_now - millis_before_2 > temp_refresh_rate) // if it has been more than RefreshRate then get the current temperature
    {
      millis_before_2 = millis_now;             // track the current time for the next loop
      temperature = thermocouple.readCelsius(); // read the temperature sensor
      Input = temperature;                      // set the input field for the PID loop
                                                //
      Setpoint = temperatureSetPoint;           // set the set point for the PID
      myPID.Compute();                          // run the compute for the pid using the current variables

      analogWrite(SSR, Output); // We change the Duty Cycle of the relay

      lcd.setCursor(1, 11);
      lcd.print(String(temperature) + "   ");
    }

    // If button 2 is pressed, exit cooking.
    if (!digitalRead(but_2))
    {
      analogWrite(SSR, 0);
      delay(500);
      break;
    }
    displayTemperature();
  } // end of While loop for the intelligent cook mode

} // ends of intelligentCook function

// This function will cook like a normal burner
// and set the burner at a percent of maximum
void regularCook()
{
  int percent = 0; // used for setting the PWM percent for cooking
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Regular Cook");
  lcd.setCursor(1, 0);
  lcd.print("Set point:");

  while (1) // start loop for the heating and wait until the exit button is pressed to end cooking
  {
    int temp = myEnc.readAndReset();
    if (temp > 0)
    {
      percent++;
      if (percent > 100)
      {
        percent = 100;
      }
    }
    else if (temp < 0)
    {
      percent--;
      if (percent < 0)
      {
        percent = 0;
      }
    }
    analogWrite(SSR, map(percent, 0, 100, 0, 255));
    lcd.setCursor(1, 11);
    lcd.print(String(percent) + "%  ");

    // if the end button is pressed then exit cooking
    if (!digitalRead(but_2))
    {
      analogWrite(SSR, 0);
      delay(100);
      break;
    }
    displayTemperature();
  } // end of While loop

} // end of regular cook function

// This function is used for cooking SMD PC boards
// using a SMD reflow profile
void smdCook()
{

  seconds = 0;
  heatStage = 1;

  // start of the loop for the SMD cooking
  while (1)
  {

    displayTemperature();
    millis_now = millis(); // track the current time

    if (millis_now - millis_before > temp_refresh_rate) // if it has been more than RefreshRate then get the current temperature
    {
      millis_before = millis();                 // track the current time for the next loop
      temperature = thermocouple.readCelsius(); // read the temperature sensor
      Input = temperature;                      // set the input field for the PID loop
    }

    // This is the first heating stage handler
    if (heatStage == 1)
    {
      Setpoint = stage_2_set_point; // set the set point for the PID
      myPID.Compute();              // run the compute again because we just made a change to the set point

      analogWrite(SSR, Output); // We change the Duty Cycle of the relay

      if (temperature >= stage_2_set_point) // check to see if se need to move onto the next stage
      {
        heatStage++;           // increment the stage counter and
        stage2Timer = seconds; // track the current time
      }
    }

    // This is the second heating stage handler
    if (heatStage == 2)
    {
      myPID.Compute();          // run the PID compute cycle
      analogWrite(SSR, Output); // We change the Duty Cycle of the relay

      int stage2Temp = seconds - stage2Timer; // see how long we have been in stage two
      if (stage2Temp > soakTime)              // if longer than the soakTime variable
      {
        heatStage++; // move to the next stage
      }
    }

    // This is the third heating stage handler
    if (heatStage == 3)
    {
      Setpoint = max_temp; // now set the set pont to the max_temp setting
      myPID.Compute();     // recalculate the PID value because we just changed the Setpoint

      analogWrite(SSR, Output); // We change the Duty Cycle of the relay

      if (temperature >= max_temp) // check to see if we reached the max_temp set point
      {
        heatStage++;           // if we did move on to the next stage
        stage4Timer = seconds; // take note of the time we moved into the next stage
      }
    }

    // This is the forth heating stage handler
    if (heatStage == 4)
    {
      myPID.Compute();          // run the PID compute cycle
      analogWrite(SSR, Output); // We change the Duty Cycle of the relay

      int temp = seconds - stage4Timer; // see how long we have been in this stage
      if (temp > reflowTime)            // if we have been here for the full time
      {
        heatStage++;           // move on to the next stage
        analogWrite(SSR, LOW); // turn of the relay
      }
    }

    // This is the fifth and last heating stage handler
    if (heatStage == 5)
    {
      lcd.clear(); // clear the display and write complete to let the user know we are done
      lcd.setCursor(0, 1);
      lcd.print("      COMPLETE      ");
      seconds = 0; // Reset timer
      heatStage = 0;
      delay(5000);
    }

    if (millis_now - millis_before > refresh_rate) // every second, update the display
    {
      millis_before = millis(); // track the current time
      seconds++;                // increment the time counter

      Serial.println(temperature); // output the temperature to the serial monitor for plotting

      if (heatStage == 0) // if we are in stage zero then update display for that stage
      {
        digitalWrite(SSR, LOW);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Temp: ");
        lcd.print(temperature, 1);
        lcd.setCursor(0, 1);
        lcd.print("SSR: OFF");
        lcd.setCursor(0, 2);
        lcd.print("Stage - ");
        lcd.print(Names[heatStage]);
        lcd.setCursor(0, 3);
        if (temperature > 50) // check the temperature and display Hot if above 50 degrees
        {
          lcd.print("       HOT!!!       ");
        }
        else // if not, just say Not Running
        {
          lcd.print("NOT RUNNING");
        }
      } // end of heatStage == 0

      else if (heatStage > 0 && heatStage < 7) // display the appropriate thing if a sequence is running
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Temp: ");
        lcd.print(temperature, 1);
        lcd.setCursor(0, 1);
        lcd.print("SSR: ON ");
        lcd.print("Time: ");
        lcd.print(seconds);
        lcd.setCursor(0, 2);
        lcd.print("Stage - ");
        lcd.print(Names[heatStage]);
        lcd.setCursor(0, 3);
        lcd.print("RUNNING - PWM: ");
        lcd.print(Output, 1);
      } // end of display update for all active stages
    }   // end of millis_now - millis_before > refresh_rate

    // If button 2 is pressed, exit cooking.
    if (!digitalRead(but_2))
    {
      analogWrite(SSR, 0);
      delay(500);
      break;
    }
  }

} // End of SMDcook
