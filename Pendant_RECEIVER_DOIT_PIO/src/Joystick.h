
void  ProcessJoystick()  // Create one incremental jog command for X and Y (no Z) if joystick is being deflected past the dead zone.
{      
  if  ( (ESPNOWpacket.JOYenabled == false) && (MoveMode.startsWith("Jog")) && (PrevJOYenabled == true) ) // Enable button released, perhaps while joystick still deflected.
                                                                           // Using PrevJOYenabled to ensure that ongoing movement is not due to ROE or incremental buttons.
  {
    Serial.println("Halting because Joystick no longer enabled.");
    HaltJogging();  // Halt immediately. In Jogs.h
    PrevJOYenabled = false;
    return;
  }

  if (ESPNOWpacket.JOYenabled == false) return; // don't do any joglets if the enable button is not pressed.

  if  ((ESPNOWpacket.JOYenabled == true) && (PacketReceived == true)) 
  {
    PrevJOYenabled = true; // To be used next time through this function.
    PacketReceived = false; // use up the current packet.
    const int Xzero = 15; // Coordinates when joystick is relaxed in center
    const int Yzero = 41;        
     // The Pendant Sender code normalized the joystick ADC values so that .JoyX and .JoyY range from approx. -1000 to 1000.
    int X_deflection = (ESPNOWpacket.JoyX - Xzero);  // Horizontal deflection, SIGNED.
                                          // Problems trying to read JoyX while it's being changed by new packets arriving?
    int Y_deflection = (ESPNOWpacket.JoyY - Yzero);  // Vertical deflection, SIGNED.

    Serial.print("X_deflection: ");
    Serial.print(X_deflection);
    Serial.print(" Y_defl: ");
    Serial.println(Y_deflection);

    double XY_deflection; // NOT SIGNED - always positive. abs(The diagonal distance from center that joystick is deflected. Ranges approx -1400 to 1400)
    // The Pendant Sender program normalizes the ADC output so that the total deflection ranges are +/- 1000 for both X and Y.
    const int MaxJogletInterval_ms = 50;
    double dX; // mm to move in this joglet
    double dY;
    int ExtraDt_ms = 10; // extra ms to add to the joglet time to force the planner buffer to fill up a bit. 
    int XYfeedrate; // in mm/min NOT mm/sec!!
    int Xfr = 0; // X component of feedrate, or feedrate if only X movement. NOTE: this is always positive.
    int Yfr = 0; // Y component of feedrate, or feedrate if only Y movement. NOTE: this is always positive.
    int MinFeedrate = 600; // mm/min or 10 mm/sec. If you want to go slower than this, use the ROE instead!
    int MaxFeedrate = 40000; // mm/min = 666 mm/sec This can cross my 600mm work area in about a second (thanks to MPCNC belt drive.)
    float FeedFactor = 0.0197 ;  // 0.01577 was a bit too slow.
    int DeadZone = 100; // if joystick is within this circle, it should not be commanding any movement.
                        // This is the inner 10% bull's eye. (+/- 1000 range)
   
    String CommandStr = "$J=G21G91 "; // All the stuff that doesn't change. Every command will have an X and Y value and all have F.
                                      // I left out the spaces to save room in the Teensy Serial Buffer.
                                      // May not need to send G21G91 every time. G21 is mm. G91 is incremental movement, which is usually not default.
                                      // Use $G to see what current settings are.
    XY_deflection = sqrt( pow(X_deflection, 2) + pow(Y_deflection, 2) ); // Pythagoras distance. Always positive.
    Serial.print("XY_defl: ");
    Serial.println(XY_deflection);
    bool JoystickCentered = (XY_deflection <= DeadZone);
    if ( JoystickCentered && (MoveMode.startsWith("Jog") ) ) // Joystick is in center.
      // Halt jogging to clear the buffer if joystick is near (0,0)
    {
      Serial.println("Halting because Joystick is centered.");
      HaltJogging();  // in Jogs.h
      PrevJOYenabled = false;
      return;
    }   

    if ( !JoystickCentered ) // At least one direction has deflection. Calculate feed, joglet distance, and make a command string.
    {  // The feed rate is based on joystick deflection. The incremental distance is based on time since last joglet.
      XYfeedrate = round( (FeedFactor * (pow (XY_deflection, 2) ) ) + (MinFeedrate - (FeedFactor * DeadZone * DeadZone)));  // in mm/min. Feedrate increases parabolically with deflection. //  pow(base, exponent)
       // This will be a very slow rate (MinFeedrate) when just outside of DeadZone. And MaxFeedrate at full diagonal deflection.
       // This parabola was fit to the desired range of values.
      Serial.print(" XYfeedrate: ");
      Serial.println(XYfeedrate);
      if (abs(X_deflection) > DeadZone) Xfr = round( (FeedFactor * X_deflection * X_deflection)  + (MinFeedrate - (FeedFactor * DeadZone * DeadZone)) ); 
      if (abs(Y_deflection) > DeadZone) Yfr = round( (FeedFactor * Y_deflection * Y_deflection)  + (MinFeedrate - (FeedFactor * DeadZone * DeadZone)) ); 
      long dt_ms = millis() - LastJog_ms; // time since last jog command was sent.
      if (dt_ms > MaxJogletInterval_ms) dt_ms = MaxJogletInterval_ms; // Used for the first joglet of a series of joglets.
      dt_ms += ExtraDt_ms; 
      Serial.print("                                                                               dt_ms + ExtraDt_ms: ");
      Serial.println(dt_ms);
      double dt_minutes = ((dt_ms / 1000.0) / 60.0); // If I used (dt_ms/1000)/60 it truncates the result to zero! Must include .0!
      dX = dt_minutes * Xfr; 
      if (signbit(X_deflection) != 0) dX = -dX;
      dY = dt_minutes * Yfr;  // distance to move in this joglet
      if (signbit(Y_deflection) != 0) dY = -dY;  // if deflection is to the left, move to more negative X value. ( signbit(x) returns 0 if variable value is positive.)

      // Three types of movement, parallel to an axis or diagonal:
      // 1. Just X movement
      if ( (abs(X_deflection) > DeadZone) && (abs(Y_deflection) <= DeadZone) ) 
      {
       if ( (MoveMode.startsWith("Jog")) &&  (abs(OLDpacket.JoyY - Yzero) > DeadZone) ) // It was moving in Y and is no longer. Clear the buffer.
        {
          Serial.println("Halting because JoyY in dead zone.");
          HaltJogging();
          PrevJOYenabled = false;
          return;
        }
        CommandStr += "X";
        CommandStr += String(dX,3);    
        CommandStr += " F";
        CommandStr += String(Xfr); // converts the int to a string.
        CommandStr += '\n'; // terminate the command.
        if (Xfr >= MinFeedrate)  SendJog("Joystick", CommandStr);// send it to the Teensy.  
        return;
      }  // end Only-X joglet

      // 2. Just Y movement
      if ( (abs(Y_deflection) > DeadZone) && (abs(X_deflection) <= DeadZone) )
      {
       if ( (MoveMode.startsWith("Jog")) && (abs(OLDpacket.JoyX - Xzero) > DeadZone) ) // It was moving in X and is no longer. Clear the buffer.
        {
          Serial.println("Halting because JoyX in dead zone.");
          HaltJogging();
          PrevJOYenabled = false;
          return;
        }   
        CommandStr += " Y";
        CommandStr += String(dY,3);    
        CommandStr += " F";
        CommandStr += String(Yfr); // converts the int to a string.
        CommandStr += '\n'; // terminate the command.
        if (Yfr >= MinFeedrate)  SendJog("Joystick", CommandStr);// send it to the Teensy.  
        return;
      }  // end Y-only joglet

      // 3. X and Y diagonal movement
      if ((abs(Y_deflection) > DeadZone) && (abs(X_deflection) > DeadZone))
      {
        CommandStr += "X";
        CommandStr += String(dX,3);
        CommandStr += " Y";
        CommandStr += String(dY,3);   
        CommandStr += " F";
        CommandStr += String(XYfeedrate); // converts the int to a string.
        CommandStr += '\n'; // terminate the command.
        if (Yfr >= MinFeedrate)  SendJog("Joystick", CommandStr);// send it to the Teensy.  
        return; 
      }  // end diagonal movement
      
     } // end if joystick is deflected from zero center.
    
  } // end if JOYenabled
  
  
  if (debugging) TimeElapsed("ProcessJoystick");
} // end ProcessJoystick

