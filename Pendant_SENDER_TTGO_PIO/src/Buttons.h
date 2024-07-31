// Read the state of the pendant's buttons.
// Some debouncing needed.
// Use the Seesaw board to get enough ports to read all of these.
// Adafruit ATtiny SEESAW Stemma QT (Qwiic daisy chainable) breakout, ***see p. 143 of Maker 9. 
// Connects to I2C. Each board has 14 GPIOs with selectable pulls (maybe 15?)

// 10-cond Ribbon cable color and function (p. 47 of Maker 9)
// W Joystick Calibrate routine
// Gy Light on/off
// P set current position as x,y (0,0)
// Blu Go to (0,0)
// Gn Home X,Y
// Y Home Z
// o Vac on/off
// R Set current Z to 0
// Bn Job Start/Resume (AKA Cycle Start)

// Pendant ON/OFF 5-cond ribbon
// Bk 2 LED
// W 1 LED
// Gy Common
// P Normally Open
// Blu Normally Closed

// Spindle ON/OFF 5-cond ribbon
// R 2 LED
// Bn 1 LED
// Gn Common
// Y Normally Open
// o Normally Closed

// Pause/Halt
// Bk with white stripe/red heatshrink: signal
// Bk Gnd

// Park button: Rd signal, Bk is GND
// Increment buttons:
// Y Incr.
// O Decr.
// Gn GND

void ReadButtons()  // Only does one scan of all the pendant buttons.
{
  bool SomethingChanged = false;
  

  ESPNOWpacket2send.JOYenabled = !digitalRead(JOYSTICK_NOT_ENABLE_PIN); // 1 when enabled.
  //Serial.print(ESPNOWpacket2send.JOYenabled);
  if (ESPNOWpacket2send.JOYenabled != SENTpacket.JOYenabled) SomethingChanged = true; 
  
  ESPNOWpacket2send.X_ROEenabled = !digitalRead(X_ROE_NOT_EN); // X_ROEenabled == 1 when enabled.
  if (ESPNOWpacket2send.X_ROEenabled != SENTpacket.X_ROEenabled) SomethingChanged = true;

  ESPNOWpacket2send.Y_ROEenabled = !digitalRead(Y_ROE_NOT_EN); //  Y_ROEenabled == 1 when enabled.
  if (ESPNOWpacket2send.Y_ROEenabled != SENTpacket.Y_ROEenabled) SomethingChanged = true;

  ESPNOWpacket2send.Z_ROEenabled = !digitalRead(Z_ROE_NOT_EN); //  Z_ROEenabled == 1 when enabled.
  if (ESPNOWpacket2send.Z_ROEenabled != SENTpacket.Z_ROEenabled) SomethingChanged = true;

  ROEenabled = (ESPNOWpacket2send.X_ROEenabled || ESPNOWpacket2send.Y_ROEenabled || ESPNOWpacket2send.Z_ROEenabled); 
                 // TRUE if any of the ROE_NOT_EN buttons is pushed.
// ***To Do: Add bulk read mask for S4A SEESAW chip if I end up using lots of S4A pins. Each read uses 3ms.
  ESPNOWpacket2send.IncrementButton = !S4A.digitalRead(S4A_INCR_PIN);
  ESPNOWpacket2send.DecrementButton = !S4A.digitalRead(S4A_DECR_PIN);
  if ((SENTpacket.IncrementButton == 0) && (SENTpacket.DecrementButton == 0)) // Only signal an increment button press if it has finished signalling the last one.
  {    
    if (ROEenabled) 
      // Ignore Incr/Decr buttons presses unless axis is enabled with the ROE enable buttons.
    {
      if (ESPNOWpacket2send.IncrementButton != SENTpacket.IncrementButton) SomethingChanged = true;
      if (ESPNOWpacket2send.DecrementButton != SENTpacket.DecrementButton) SomethingChanged = true;
    }
    else  // No axis is enabled. Turn off incr/decr button signals.
    {
      ESPNOWpacket2send.IncrementButton = 0;
      ESPNOWpacket2send.DecrementButton = 0;
    }
  }
  ESPNOWpacket2send.Light = !S4A.digitalRead(S4A_LIGHT_PIN);
  if (ESPNOWpacket2send.Light != SENTpacket.Light) SomethingChanged = true;

  ESPNOWpacket2send.Park = !S4A.digitalRead(S4A_PARK_PIN);
  if (ESPNOWpacket2send.Park != SENTpacket.Park) SomethingChanged = true;
 
  ESPNOWpacket2send.Spindle_ON = S4A.digitalRead(S4A_SPINDLE_START_PIN);
  if (ESPNOWpacket2send.Spindle_ON != SENTpacket.Spindle_ON) SomethingChanged = true;
 
 // LastTime_us = micros();
  uint32_t ButtonStates = S49.digitalReadBulk(SeesawDigInPinMask);
                       // These buttons are pulled to ground when pressed. 
  //TimeElapsed("ReadingButtonMask"); // Takes 3 ms
  
  
// This button mask approach takes 3ms. Reading only one button takes 3ms.
  if (ButtonStates != PrevButtons) // a button changed state. Flip inverse logic to 1 == ON (pressed)
  {
    PrevButtons = ButtonStates;
    ESPNOWpacket2send.MenuButton = !bitRead(ButtonStates, S49_PIN_MENU_BUTTON); // Here, the pin numbers are used as bit indexes.
    ESPNOWpacket2send.Vacuum = !bitRead(ButtonStates, S49_PIN_VAC);
    ESPNOWpacket2send.SetXYto0 = !bitRead(ButtonStates, S49_PIN_SETXYTO_00);  
    ESPNOWpacket2send.ProbeZ = !bitRead(ButtonStates, S49_PIN_PROBE_Z);
    ESPNOWpacket2send.GoTo00 = !bitRead(ButtonStates, S49_PIN_GOTO_00);
    ESPNOWpacket2send.HomeXYZ = !bitRead(ButtonStates, S49_PIN_HOME_XYZ);  
    ESPNOWpacket2send.SetZto0 = !bitRead(ButtonStates, S49_PIN_SETZTO_0);
    ESPNOWpacket2send.Start = !bitRead(ButtonStates, S49_PIN_START);
    ESPNOWpacket2send.Pause_Halt = !bitRead(ButtonStates, S49_PIN_PAUSE);
    ESPNOWpacket2send.Reset = !bitRead(ButtonStates, S49_PIN_NOT_RESET);

    SomethingChanged = true;
   //TimeElapsed("ReadingButtonBits");
  } 

  if (SomethingChanged) 
  {
   Beep(); // all buttons beep once when pushed to indicate they work.
   SendESPNOWpacket("Button scanner");
  }
  
} // end ReadButtons
