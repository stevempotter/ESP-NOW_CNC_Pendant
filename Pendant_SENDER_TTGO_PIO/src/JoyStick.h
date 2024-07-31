
// To read a Hall Effect RC-type analog Joystick. (Made by Jumper)

// This joystick consumes about 4.3 mA.
// DU = Digital (ADC) Units
// 4-cond ribbon cable
// R +3.3V
// Gn GND

// S49_ADC_PIN_JOY_X     2
// orange wire   X Horizontal signal - 1.24V span  0.043-1.28V
// S49_ADC_PIN_JOY_Y     3
// Yellow wire   Y Vertical signal - 1.44V span at 3.3Vcc   0.32-1.76V See Maker 9, p. 25, Maker 10 2022-02-22

// JOYSTICK_NOT_ENABLE_PIN 33  Purplish wire. Enabled when low.
// Using the Adafruit SEESAW 10-bit ADC:
// Xspan = 382  (left is 397, right is 12) Xcenter = 200
// Yspan = 448  (bottom is 98, top is 550) Ycenter = 332
// Center = (200, 332). The origin is towards the lower-right, but never gets to (0,0).

// Details were empirically determined. Joystick values may drift. 
// ESP32 ADC (not using any more): See Maker 9, p.44 for 6db, p.6 for 11db attenuation. ESP32 ADC Linear range (for 11db atten.) of 150-2450 mV. 
// SEESAW ADC: See Maker 10, 26/02/2022

/* 
 *** May wish to use EEPROM on the seesaw breakout board to save calibration data:
 *   void EEPROMWrite8(uint8_t addr, uint8_t val);
  void EEPROMWrite(uint8_t addr, uint8_t *buf, uint8_t size);
  uint8_t EEPROMRead8(uint8_t addr);
 */

void CalibrateJoystick();  // Forward declaration (see below)

void ReadJoystick()  //Only one read, except when calibrating the joystick. 1 new packet per loop.
{
  // Joystick global variables: (copied here for reference, may be out of date)

/*
    const int JoystickSampleInterval_ms = 25; // Determines the frequency that the joystick is sampled when enabled.
    int JoyX_ADC; 
    int JoyY_ADC;
    int PrevJoyXspeed = 0;
    int PrevJoyYspeed = 0;
    long PrevJOYsample_ms = millis();
    bool CalibrateJOY = false;
    
    int Xc; // values when stick relaxed at center      
    int Yc;
    int Xmax; // Pushed to left extreme
    int Xmin;  // pushed to right extreme
    int Ymax; // Pushed to top
    int Ymin;  // pushed to bottom
    int Xsp; // Total span of X range.
    int Ysp;
    int oldXsp;
    int oldYsp;
    float Bands = 12.0; // The number of speed categories across the span - 1 (The 0x center zone is 2x2 bands wide) 
    float Xdelta; // How close the different speed ranges are.
    float Ydelta;
    bool FirstDROloop = true; // To move init of TFT display into DRO refresh. It takes a while.
    bool JOYcalibrated = false; // Best to calibrate before using joystick. Joystick readings drift due to ???
*/
  int Xnorm = 0; // Normalized joystick values.
  int Ynorm = 0;
  const float normMax = 2000.0; // Values will range from -1000 to +1000
  
  ESPNOWpacket2send.JoyX = 0;
  ESPNOWpacket2send.JoyY = 0;
  if (!JOYcalibrated)   // approximately OK values.
  {
    Xc = 199; // ADC values when stick relaxed at center      
    Yc = 333;
    Xmax = 397; // Pushed to left extreme
    Xmin = 12;  // pushed to right extreme
    Ymax = 550; // Pushed to top
    Ymin = 98;  // pushed to bottom
    Xspan = Xmax - Xmin; // Total span of X range.
    oldXspan = Xspan;
    Yspan = Ymax - Ymin;
    oldYspan = Yspan;      
  }
  
  if (ESPNOWpacket2send.JOYenabled) 
  {
    if ((millis() - PrevJOYsample_ms) > JoystickSampleInterval_ms) // Don't bother sampling the joystick too often.
    {   
      StartRead = micros();
      JoyX_ADC = S49.analogRead(S49_ADC_PIN_JOY_X);   // *** TO DO: put into a separate Task loop. This takes 10ms to sample.
      JoyY_ADC = S49.analogRead(S49_ADC_PIN_JOY_Y); 
      ConversionTime = micros() - StartRead;
      Serial.print("us to digitize joystick: ");
      Serial.print(ConversionTime);
      Serial.print(" JoyX_ADC= "); // For debugging or showing in Serial Plotter window.
      Serial.print(JoyX_ADC);
      Serial.print("   JoyY_ADC= ");
      Serial.println(JoyY_ADC);
      // Normalize the ADC values so that the joystick ranges from -1000 to 1000.
      Xnorm = ( -(JoyX_ADC - Xmin) + (Xspan / 2) ) * (normMax / Xspan); // ADC axis needed to be reversed so #s increase to the right.
      Ynorm = ( (JoyY_ADC - Ymin) - (Yspan / 2) ) * (normMax / Yspan); // scaling of ADC values.
      ESPNOWpacket2send.JoyX = Xnorm;
      ESPNOWpacket2send.JoyY = Ynorm;
      Serial.print("                                          Joystick X: ");
      Serial.print(ESPNOWpacket2send.JoyX);
      Serial.print("  Joystick Y: ");
      Serial.println(ESPNOWpacket2send.JoyY);
      SendESPNOWpacket("Joystick");
      Click();
      RefreshDRO(); 

    } // end if it is time to sample the joystick again
     TimeElapsed("ReadJoystick"); 
  } // end if joystick enabled.
   
} // end ReadJoystick


void CalibrateJoystick()  // ***To Do: put instructions and results onto pendant LCD
// Actually, the Hall effect gimbal and SEESAW ADC seem to stay calibrated very well. May never need this again.
{
    if (!ESPNOWpacket2send.JOYenabled)  // only calibrate while joystick is NOT enabled.
    {  // (Only calibrate if the joystick enable pin is not being pressed.)
      Beep();
      int oldXmax = -1; // some out-of-range values to start with when uncalibrated.
      int oldXmin = 1000;
      int oldYmax = -1;
      int oldYmin = 1000;
      Serial.println("Leave joystick centered for 3 sec...");
      delay(2000); // time to read the message.
      Xc= S49.analogRead(S49_ADC_PIN_JOY_X);
      Yc= S49.analogRead(S49_ADC_PIN_JOY_Y);
      Serial.print(" Joystick center is now at (");
      Serial.print(Xc);
      Serial.print(", ");
      Serial.print(Yc);
      Serial.println(")");
      Serial.println(" Holding down the Zero Joystick button (and NOT enable), slowly move joystick to its extremes.");
      delay(4000); // time to read the message and begin holding down the Zero Joystick button.
      
      while (CalibrateJOY)
      {
       JoyX_ADC = S49.analogRead(S49_ADC_PIN_JOY_X);        
       Xmax = max(oldXmax, JoyX_ADC);
       oldXmax = Xmax;
       Xmin = min(oldXmin, JoyX_ADC);
       oldXmin = Xmin;
       JoyY_ADC = S49.analogRead(S49_ADC_PIN_JOY_Y);
       Ymax = max(oldYmax, JoyY_ADC);
       oldYmax = Ymax;
       Ymin = min(oldYmin, JoyY_ADC);
       oldYmin = Ymin;
       Serial.print("Calibrate Joystick - move to extremes...JoyX_ADC: ");
       Serial.print(JoyX_ADC); // Just to show data changing.
       Serial.print(" JoyY_ADC: ");
       Serial.println(JoyY_ADC);
       Click(); // Just to indicate how fast it is looping
       CalibrateJOY = !S49.digitalRead(S49_PIN_CAL_JOYSTICK);  // Exit the calibration when Cal. Joystick button is released and pin goes high.
      } // end calibration of Joystick diagonal extremes.

      if ( ((Xmax - Xmin) > (oldXspan - 15)) && ((Ymax - Ymin) > (oldYspan -15)) ) // CHECK TO SEE IF THE NEW Xsp AND Ysp ARE REASONABLE. ) 
      {
       JOYcalibrated = true;
       Serial.println(" Joystick calibration finished. Have a look at the max values to see if they are reasonable.");
       Xspan = Xmax - Xmin; // Total span of X range.
       Yspan = Ymax - Ymin;
       Beep();
      }
      else 
      {
       JOYcalibrated = false;
       Beep();
       Beep();
       Beep();
       Beep(); // long beep.
       Serial.println( " Calibration failed. Try again!"); 
      }  
    
      Serial.print(" Xmax= ");
      Serial.print(Xmax);
      Serial.print(" Ymax= ");
      Serial.print(Ymax);
      Serial.print(" Xmin= "); 
      Serial.print(Xmin);
      Serial.print(" Ymin= ");
      Serial.println(Ymin);
     
  } // end calibration routine.
} // end CalibrateJoystick
