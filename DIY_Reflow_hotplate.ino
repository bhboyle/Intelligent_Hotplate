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
double Setpoint, Input, Output;
unsigned int millis_before, millis_before_2;
unsigned int millis_now = 0;
int refresh_rate = 1000;
int temp_refresh_rate = 300;
unsigned int seconds = 0;
bool but_1_state = false;
unsigned long but_1_timer = 0;
int max_temp = 260;
float temp_setpoint = 0;
float temperature = 0;
int heatStage = 0;
int stage_2_set_point = 150;
unsigned long stage2Timer = 0;
unsigned long stage4Timer = 0;
unsigned long soakTime = 100;
int reflowTime = 60;
String Names[] = {
    "Off",
    "Heat",
    "Soak",
    "Blast",
    "Reflow", // I am sure there is a real name for this stage but I dont know it and this seemed cool... thats funny cuz the next stage is cool..
    "Cool",
};

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO); // Start MAX6675 SPI communication
LiquidCrystal_I2C lcd(0x27, 20, 4);                  // Address could be 0x3f or 0x27
PID myPID(&Input, &Output, &Setpoint, 7, .01, 0, DIRECT);

void setup()
{

  // turn the PID on
  myPID.SetMode(AUTOMATIC);

  // Define the OUTPUTs
  pinMode(SSR, OUTPUT); // Start with the SSR off
  digitalWrite(SSR, LOW);

  // Define the INPUTS
  pinMode(but_1, INPUT_PULLUP);
  // pinMode(but_2, INPUT_PULLUP);

  Serial.begin(9600);
  // Serial.
  ("Startup");
  lcd.init();      // Init the LCD
  lcd.backlight(); // Activate backlight

  millis_before = millis();
  millis_now = millis();
}

void loop()
{

  millis_now = millis();
  if (millis_now - millis_before_2 > temp_refresh_rate)
  {
    millis_before_2 = millis();
    temperature = thermocouple.readCelsius();
    Input = temperature;
  }

  myPID.Compute();

  // This is the first heating stage if handler
  if (heatStage == 1)
  {
    Setpoint = stage_2_set_point;
    myPID.Compute();

    analogWrite(SSR, Output); // We change the Duty Cycle

    if (temperature >= stage_2_set_point)
    {
      heatStage++;
      stage2Timer = seconds;
    }
  }

  // This is the second heating stage if handler
  if (heatStage == 2)
  {
    analogWrite(SSR, Output); // We change the Duty Cycle

    int stage2Temp = seconds - stage2Timer;
    if (stage2Temp > soakTime)
    {
      heatStage++;
    }
  }

  // This is the third heating stage if handler
  if (heatStage == 3)
  {
    Setpoint = max_temp;
    myPID.Compute();

    analogWrite(SSR, Output); // We change the Duty Cycle

    if (temperature >= max_temp)
    {
      heatStage++;
      stage4Timer = seconds;
    }
  }

  // This is the forth heating stage if handler
  if (heatStage == 4)
  {
    analogWrite(SSR, Output); // We change the Duty Cycle

    int temp = seconds - stage4Timer;
    if (temp > reflowTime)
    {
      heatStage++;
      analogWrite(SSR, LOW);
    }
  }

  // This is the fifth and last heating stage if handler
  if (heatStage == 5)
  {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("      COMPLETE      ");
    seconds = 0; // Reset timer
    heatStage = 0;
    delay(5000);
  }

  // millis_now = millis();
  if (millis_now - millis_before > refresh_rate)
  {
    millis_before = millis();
    seconds++; // We count time

    Serial.println(temperature);

    if (heatStage == 0)
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
      if (temperature > 50)
      {
        lcd.print("       HOT!!!       ");
      }
      else
      {
        lcd.print("NOT RUNNING");
      }
    } // end of heatStage == 0

    else if (heatStage > 0 && heatStage < 7)
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
    but_1_state = true;
    but_1_timer = millis();
    // if the sequence is not running and the button is pressed, start it
    if (heatStage < 1)
    {
      heatStage = 1;
      seconds = 0;
    }
    else if (heatStage > 0)
    { // if the sequence is running and the button is pressed, stop it
      heatStage = 0;
      digitalWrite(SSR, LOW);
    }
  } // end of button press check

  // reset the state flag for button press if the be-dounce timer has elapsed
  if (but_1_state)
  {
    if (millis() - but_1_timer > 250)
    {
      but_1_state = false;
    }
  } // end of button press timer reset

} // end of void loop
