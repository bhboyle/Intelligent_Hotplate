/*
This is a rework of the Electronoobs DIY reflow hotplate project found here. https://electronoobs.com/eng_arduino_tut155.php

I did a major rewrite of the control code and added PID control of the heater. This made a big difference to the heating and
in my opinion helps alot.

I also used a small electric hot plate rather than a cloths iron because it was cheap and this way I can make changes to that
portion of the system if I like. https://www.amazon.ca/gp/product/B08R6F5JH8/ref=ppx_yo_dt_b_search_asin_image?ie=UTF8&psc=1

Started April 20th 2022

I would very much like to add some features like a encoder dial to adjust the max tempurture on the fly without having to
adjust the code and re-upload.

*/

// Libraries
#include "max6675.h" //Download it here: http://electronoobs.com/eng_arduino_max6675.php
#include <Wire.h>
#include "LiquidCrystal_I2C.h" //Download it here: http://electronoobs.com/eng_arduino_liq_crystal.php
#include <PID_v1.h>

// Inputs Outputs
int SSR = 3;
int thermoDO = 4;  // Data pin for MAX6675
int thermoCS = 5;  // CS pin for MAX6675
int thermoCLK = 6; // Clock pin for MAX6675
int but_1 = 11;    // Button 1 input

// Variables
double Setpoint, Input, Output;              // variable needed for the PID loop
unsigned int millis_before, millis_before_2; // used for time tracking in the loop
unsigned int millis_now = 0;                 // used to keep track of the current time of the loop
int refresh_rate = 1000;                     // how often to update the display in milliseconds
int temp_refresh_rate = 300;                 // how often to check the temperature in milliseconds
unsigned int seconds = 0;                    // used in the display to show how long the sequence has been running
bool but_1_state = false;                    // used to track of the button has been pushed. used for debouncing the button
unsigned long but_1_timer = 0;               // used for checking the time for debouncing the button push
int max_temp = 260;                          // ****** this is the high temperature set point. *******
float temperature = 0;                       // this is the varilable that holds the current temperature reading
int heatStage = 0;                           // used for keeping track of where we are in the heating sequence
int stage_2_set_point = 150;                 // this is the "soak" temperature set point
unsigned long stage2Timer = 0;               // used to keeping track of when we went into stage two
unsigned long stage4Timer = 0;               // used for keeping track of when we went into stage 4
unsigned long soakTime = 100;                // how long to soak the board for in seconds
int reflowTime = 60;                         // how long to reflow the board for in seconds
String Names[] = {
    // this is the text displayed in the display in the various stages
    "Off",
    "Heat",
    "Soak",
    "Blast", // I am sure there is a real name for this stage but I dont know it and this seemed cool...
    "Reflow",
};

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO); // Start MAX6675 SPI communication
LiquidCrystal_I2C lcd(0x27, 20, 4);                  // Address could be 0x3f or 0x27
PID myPID(&Input, &Output, &Setpoint, 7, .01, 0, DIRECT); // create the PID object

void setup()
{

  // turn the PID on
  myPID.SetMode(AUTOMATIC);

  // Define the OUTPUTs
  pinMode(SSR, OUTPUT); // Start with the SSR off
  digitalWrite(SSR, LOW);

  // Define the button input
  pinMode(but_1, INPUT_PULLUP);

  Serial.begin(9600);
  lcd.init();      // Init the LCD
  lcd.backlight(); // Activate backlight

  millis_before = millis();
  millis_now = millis();
}

void loop()
{

  millis_now = millis();                                // track the current time
  if (millis_now - millis_before_2 > temp_refresh_rate) // if it has been more than RefreshRate then get the current temperature
  {
    millis_before_2 = millis();
    temperature = thermocouple.readCelsius(); // read the temperature sensor
    Input = temperature;                      // set the input field for the PID loop
  }

  myPID.Compute(); // run the PID compute cycle

  // This is the first heating stage if handler
  if (heatStage == 1)
  {
    Setpoint = stage_2_set_point; // set the setpoint for the PID
    myPID.Compute();              // run the compute again becuase we just made a change to the setpoint

    analogWrite(SSR, Output); // We change the Duty Cycle of the relay

    if (temperature >= stage_2_set_point) // check to see if se need to move onto the next stage
    {
      heatStage++;           // incrment the stage counter and
      stage2Timer = seconds; // track the current time
    }
  }

  // This is the second heating stage if handler
  if (heatStage == 2)
  {
    analogWrite(SSR, Output); // We change the Duty Cycle of the relay

    int stage2Temp = seconds - stage2Timer; // see how long we have been in stage two
    if (stage2Temp > soakTime)              // if longer than the soakTime variable
    {
      heatStage++; // move to the next stage
    }
  }

  // This is the third heating stage if handler
  if (heatStage == 3)
  {
    Setpoint = max_temp; // now set the Setpont to the max_temp setting
    myPID.Compute();     // recalculate the PID value becuase we just changed the Setpoint

    analogWrite(SSR, Output); // We change the Duty Cycle of the relay

    if (temperature >= max_temp) // check to see if we reached the max_temp set point
    {
      heatStage++;           // if we did move on to the next stage
      stage4Timer = seconds; // take note of the time we moved into the next stage
    }
  }

  // This is the forth heating stage if handler
  if (heatStage == 4)
  {
    analogWrite(SSR, Output); // We change the Duty Cycle of the relay

    int temp = seconds - stage4Timer; // see how long we have been in this stage
    if (temp > reflowTime)            // if we have been here for the full time
    {
      heatStage++;           // move on to the next stage
      analogWrite(SSR, LOW); // turn of the relay
    }
  }

  // This is the fifth and last heating stage if handler
  if (heatStage == 5)
  {
    lcd.clear(); // clear the display and write complete to let the user know we are done
    lcd.setCursor(0, 1);
    lcd.print("      COMPLETE      ");
    seconds = 0; // Reset timer
    heatStage = 0;
    delay(5000);
  }

  if (millis_now - millis_before > refresh_rate) // ever second update the display
  {
    millis_before = millis(); // track the current time
    seconds++;                // increment the time counter

    Serial.println(temperature); // output the temerature to the serial monitor for ploting

    if (heatStage == 0) // if we are in stage zero the do the display for that stage
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
      if (temperature > 50) // check the temperatue and display Hot if above 50 degrees
      {
        lcd.print("       HOT!!!       ");
      }
      else // if not just say Not Running
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

  // Button press handling
  if (!digitalRead(but_1) && !but_1_state)
  {
    but_1_state = true;     // change the button press flag
    but_1_timer = millis(); // track the current time of the button press
    // if the sequence is not running and the button is pressed, start it
    if (heatStage < 1)
    {
      heatStage = 1; // put us in the first stage of the sequence
      seconds = 0;   // start the clock for the display
    }
    else if (heatStage > 0)
    { // if the sequence is running and the button is pressed, stop it
      heatStage = 0; // turn everything off
      digitalWrite(SSR, LOW);
    }
  } // end of button press check

  // reset the state flag for button press if the be-dounce timer has elapsed
  if (but_1_state) // if the flag is true then
  {
    if (millis() - but_1_timer > 250) // check the timer and see of 250 milliseconds have elapsed
    {
      but_1_state = false; // if they have reset the button press flag
    }
  } // end of button press timer reset

} // end of void loop
