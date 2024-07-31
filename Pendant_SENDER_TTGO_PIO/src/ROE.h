// Rotary Optical Encoder used to jog the machine manually in X, Y and Z. AKA MPG (manual pulse generator)
// No debouncing needed on this device, since the pulses are via optically-triggered switches 
// (LEDs shining on photodiodes through a thin glass mask ring with opaque ticks on it.)
// I am using the Omron ROE with 1024 pulses per rev. (4096 counts/rev.)
// Omron ROE model E6B2 version CWZ6C (1024ppr): Bn 5-24V (I am using 5V. LEDs inside do not work at 3.3V);
// Avoid harming the ESP32 by using INPUT_PULLUP on the A, B, and Z lines. (Don't pull up to 5V!)

 //Bn 5V, Blu GND; Bk OUT-A; W OUT-B; O OUT-Z (one ppr)
 /*
 ROE_A 15 // Black wire
 ROE_B 27 // White wire
 X_ROE_NOT_EN            32 // Enabled when low.
 Y_ROE_NOT_EN            33 // Enabled when low.
 Z_ROE_NOT_EN            25 // Enabled when low.
 ClickPin 17 // To hear or see that ROE rotation is being registered.
*/


 void ReadROE() // Only one datum is transmitted per pass through this function.
 {   
  
  if (ROEenabled == true)
  {      
    LastTime_us = micros();
    ROE.resumeCount();    //  I assume it is OK to resume even if it is not paused.
    delayMicroseconds(ROEintegTime_us); // ticks get counted in the background while wasting time.
           // I tried making this modal, but numbers varied too much due to other ongoing functions.
    ROE.pauseCount(); // stop adding/subtracting ticks to the counter.
    if (millis() - LastSpeedknobRead_ms > 200) // Only read the speed selector every 200ms
    {
      SpeedSelector(); // takes about 4ms to read.
      LastSpeedknobRead_ms = millis();
    }
    CurrentCount = ROE.getCount(); // Read the ROE pulse counter.
    ThisTime_us = micros();
    long ROEreadInterval_us = ThisTime_us - LastROEtime_us; // Time since last ROE reading.
    LastROEtime_us = ThisTime_us; // Marks each time ROE counts were last read.    
    if (CurrentCount != PrevCount) 
      {  // ROE was moved to a new angle.
        ESPNOWpacket2send.ROE_DeltaCounts = CurrentCount - PrevCount; // Calc total change in counts since last read.      
        PrevCount = CurrentCount;
        if (ESPNOWpacket2send.ROE_DeltaCounts > 1) { ESPNOWpacket2send.ROE_RotDir = 1;} // 1 = CW = positive change,
        else { ESPNOWpacket2send.ROE_RotDir = -1;}                           //-1 = CCW = neg change.
        //printf("Dir: %d dT: %d Count: %d  dCounts: %d \n", ESPNOWpacket.ROE_RotDir, ROEreadInterval_us, CurrentCount,  ESPNOWpacket.ROE_DeltaCounts);
        Serial.print("                                       ms between ROE readings: ");
        Serial.print(ROEreadInterval_us/1000);
        Serial.print(" Counts: ");
        Serial.println(ESPNOWpacket2send.ROE_DeltaCounts);
        LastTime_us = micros();
        SendESPNOWpacket("ROE");
        //TimeElapsed("SendingROEpacket");
        Click();  // one click per useful read, not per ROE pulse. (This ROE has so many PPR (4096) that it just squeaks if you do one per pulse)
      } // end if counts changed.    
    else  // ROE enabled but did not turn since last read. 
      {
       ESPNOWpacket2send.ROE_DeltaCounts = 0;
       ESPNOWpacket2send.ROE_RotDir = 0;
       SendESPNOWpacket("ROE (not turning)");
      } 
   PrevROEenabled = true;
   RefreshDRO();
  } // end if ROE enabled.
  
  if ((ROEenabled == false) && (PrevROEenabled == true)) // ROE enable button just released.
  {
    
    ESPNOWpacket2send.ROE_DeltaCounts = 0;
    ESPNOWpacket2send.ROE_RotDir = 0;
    SendESPNOWpacket(" ROE enable released "); // Let the receiver know the enable button was released.
    PrevROEenabled = false;
  } // end else ROE just released.

  // zoom past and do nothing if not enabled and not enabled last cycle either.
 } // end ReadROE
