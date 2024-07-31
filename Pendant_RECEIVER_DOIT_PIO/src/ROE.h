

// *** TO DO: add feedrate and spindle adj indicator to DRO.

void ProcessROE()     // ONLY ONE MOVE PER PACKET RECEIVED.
{
 if (PacketReceived) // Skip over this function if no new packet is here.
 { 
   if ( (PrevROEenabled == true) && (ROEenabled == false) && (MoveMode.startsWith("Jog") ) )
    {  // Halt any jog in progress.
     PrevROEenabled = false;
     Serial.println("Halting because ROE is no longer enabled.");
     HaltJogging();
     PacketReceived = false;
     return;
    } 
  if (ROEenabled == false) return; // don't do any joglets if no enable button is pressed.
  if (MoveMode.startsWith("Jog") )  // halt jogging if appropriate.
   {
     if (ESPNOWpacket.ROE_RotDir != OLDpacket.ROE_RotDir) 
     {
      Serial.println("Halting because ROE changed direction or stopped rotating.");
      HaltJogging();
     }
     
   } // end if it was already ROE-jogging.
   
   if ( (ESPNOWpacket.ROE_DeltaCounts == 0) && (ROEenabled == true) )  
    // ROE is enabled. Check if the pendant sent ROE rotation info
   {
    PacketReceived = false;
    return; // Only process if there is new ROE counts from pendant.
   }
    
   if ((ROEenabled == true) && PacketReceived)  
    // ROEenabled == true and new data. Send some movement commands to Teensy and update DRO.
   { 
    if (PrevROEenabled == false) Serial.println("                                    ROE is enabled! ");
    PrevROEenabled = true;
    const int XYminSpeed = 20; // mm/min // These will need a lot of tweaking.
    const int ZminSpeed = 2;  // need Z to go slower or less per tick when setting Z-zero manually.
    const int MaxJogletInterval_ms = 50;
    int ExtraDt_ms = 10; // extra ms to add to the joglet time to force the planner buffer to fill up a bit. 
    long dt_ms = millis() - LastJog_ms; // time since last jog command was sent.
    if (dt_ms > MaxJogletInterval_ms) dt_ms = MaxJogletInterval_ms; // Used for the first joglet of a series of joglets.
    dt_ms += ExtraDt_ms; 
    Serial.print("                                                                               dt_ms + ExtraDt_ms: ");
    Serial.println(dt_ms);
    double dt_minutes = ((dt_ms / 1000.0) / 60.0); // If I used (dt_ms/1000)/60 it truncates the result to zero! Must include .0!
    double dX = 0.0; // mm distance moved for one joglet
    double dY = 0.0;
    double dZ = 0.0;
    int XYfeedrate = 0;
    int Zfeedrate = 0;
    const double CountsWeight = 0.025; // This is to allow faster twiddling to speed up the jog speed.
                                  // This gives a factor of 5X for the fastest twiddling of ROE knob.

    
    String CommandStr = "$J=G21G91 "; // All the stuff that doesn't change. Use mm and incremental movement.
    if (ESPNOWpacket.ROE_DeltaCounts != 0)  // Knob is turning: do some moving
    {   
     PacketReceived = false; // 'use up' this packet.
     Serial.print(" ROE_RotDir, ROE_DeltaCounts: ");
     Serial.print(ESPNOWpacket.ROE_RotDir); // is -1 for CCW, 1 for CW rotation.
     Serial.print(", ");
     Serial.println(ESPNOWpacket.ROE_DeltaCounts); // is neg for CCW rotation.
    
     ROE_Counts[0] = ROE_Counts[1]; // These get reset to zero for each halting in Jogs.
     ROE_Counts[1] = ROE_Counts[2];
     ROE_Counts[2] = ESPNOWpacket.ROE_DeltaCounts;  // This is negative for CCW rotation.
     CountsRunningAvg = (ROE_Counts[0] + ROE_Counts[1] + ROE_Counts[2]) / 3;
     if (CountsRunningAvg < 6) CountsRunningAvg = ESPNOWpacket.ROE_DeltaCounts; // Don't use avg if turning very slowly.
     Serial.print("                                             CountsRunningAvg: ");
     Serial.println(CountsRunningAvg);

     if ( (ESPNOWpacket.X_ROEenabled == true) || (ESPNOWpacket.Y_ROEenabled == true) ) // move X and/or Y
     {
      XYfeedrate = (XYminSpeed * ESPNOWpacket.ROEmultiplier) * ((CountsRunningAvg) * CountsWeight) ;  // signed.
      if (XYfeedrate == 0) return; // Can't send a zero feedrate. Error 22.
      if (ESPNOWpacket.X_ROEenabled == true)  // This allows moving diagonally (LL to UR) if X and Y buttons are held down together.
      {
       dX = XYfeedrate * dt_minutes; // floats get truncated to nearest 10 microns.
       if ( (abs(dX) < 0.01) && (dX > 0) ) dX = 0.01;
       if ( (abs(dX) < 0.01) && (dX < 0) ) dX = -0.01;
       CommandStr += "X";
       CommandStr += String(dX,3);  
      }   
     
      if (ESPNOWpacket.Y_ROEenabled == true)
      {
       dY = XYfeedrate * dt_minutes;
       if ( (abs(dY) < 0.01) && (dY > 0) ) dY = 0.01;
       if ( (abs(dY) < 0.01) && (dY < 0) ) dY = -0.01;
       CommandStr += " Y";
       CommandStr += String(dY,3);   
      }
      if (abs(XYfeedrate) < XYminSpeed) 
      {
       if (XYfeedrate >= 0) XYfeedrate = XYminSpeed;
       if (XYfeedrate < 0) XYfeedrate = -XYminSpeed;
      }
      Serial.print("XYfeedrate: ");
      Serial.println(XYfeedrate);
      CommandStr += " F";
      CommandStr += abs(XYfeedrate); 
     } // end jogging with ROE in X and/or Y 
         
             
     if ( (ESPNOWpacket.Z_ROEenabled == true) && (ESPNOWpacket.X_ROEenabled == false) && (ESPNOWpacket.Y_ROEenabled == false) )  
     // only allow Z move by itself, not combined with X or Y moving.
     {
      Zfeedrate = (ZminSpeed * ESPNOWpacket.ROEmultiplier) * ((CountsRunningAvg) * CountsWeight) ;
      if (Zfeedrate == 0) return; // Can't send a zero feedrate. Error 22.
      if (abs(Zfeedrate) < ZminSpeed) 
      {
       if (Zfeedrate > 0) Zfeedrate = ZminSpeed;
       if (Zfeedrate < 0) Zfeedrate = -ZminSpeed;
      }
      dZ = Zfeedrate * dt_minutes;
      if ( (abs(dZ) < 0.001) && (dZ > 0) ) dZ = 0.001;
      if ( (abs(dZ) < 0.001) && (dZ < 0) ) dZ = -0.001; // 10 microns. To avoid a very small jog distance rounding to zero.
      CommandStr += " Z";
      CommandStr += String(dZ,3);    
      CommandStr += " F";
      CommandStr += abs(Zfeedrate);      
     }     
     CommandStr += '\n';  // all commands must have a newline symbol at the end.
     SendJog(" ROE", CommandStr);// send movemement command to the Teensy.       
 
   }  // end if ROE knob turning
   else // Knob stopped turning. 
   { 
    Serial.println(" Halting because ROE stopped rotating. ");
    HaltJogging();
    PacketReceived = false;
   }  
 
  } // end ROE enabled and new data to process.
  
 } // end if PacketReceived  
 if (debugging) TimeElapsed("ROE");
}  // end ProcessROE
