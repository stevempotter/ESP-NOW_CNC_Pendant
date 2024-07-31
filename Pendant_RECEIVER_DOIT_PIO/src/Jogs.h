// To process handshaking with the Teensy while jogging, send out jog commands, and deal with the responses.
// After a jog command issued: ok from Teensy. Elapsed time: 217us
// after a 0x80 command: Brief Status from Teensy, change MoveMode. Elapsed time: 946us
// after a 0x87 command: long Status from Teensy. Elapsed time: 2559us
// Of course, that is with the Teensy not busy doing anything else. Watch out for when it is controlling jogging.

void CheckIfTeensyReady(int CommandLength); // forward declaration (see below
  

void SendJog(String WhereFrom, String JogCommand) 
  // Called by Joystick and ROE to send just one joglet command to Teensy.
  // a joglet is one small jog = one jog command sent.
{
 if  ( ESPNOWpacket.JOYenabled || ROEenabled ) 
 {
   Now_us = micros();
   int i = 0;
   int CommandLength = JogCommand.length();
   //Serial.print(JogCommand);
   //Serial.print(" Jog Command length (chars): ");
   //Serial.println(CommandLength); 
   i = 0;
   CheckIfTeensyReady(CommandLength); 
   while (!TeensyReady)  // See if Teensy planner and serial RX buffer have space.
   {
    CheckIfTeensyReady(CommandLength); // keep checking.
    //Serial.print(" Teensy not ready to enqueue new joglet. ");
    i++;
    Serial.print("                                Wasted packet count (planner full): ");
    Serial.print(i);  
    delay(JogletInterval_ms);
    if (!ESPNOWpacket.JOYenabled &&  !ROEenabled) break; // stop waiting to send the jog.
   }
  
   if (TeensyReady)
    {
     if ( (MoveMode.startsWith("Idle")) || (MoveMode.startsWith("Jog")) ) // You can send more jog commands to enqueue if a jog is in progress.
     {
      if  (MoveMode.startsWith("Idle") )  {JogOKreply = true;} // If it is not already jogging, then we should not be waiting for an OK.
      int i = 0;      
      while ( JogOKreply == false )  //wait for ok after previous command is queued.
      {    
        i++;
        delay(3);
        Serial.print("noOK.");
        ReadTeensyMsg(false, " Waiting for OK in sendjog ");    
        if (i > 20) break;
      }
      
      if ( JogOKreply == true ) // Not waiting for an ok reply from last jog.
      {
        JogOKreply = false; // gets set to true in ReadTeensyMsg when it sees an ok reply after a jog command.
        ClearRXbuffer(); // remove any remaining junk in the RX buffer.
        //if (Not_MPGmode == 1) ToggleMPGmodeTo("MPG:1"); // Give the pendant command of CNC machine.
        StartMicros = micros();
        Serial2.print(JogCommand); // Send jog command to Teensy.  
        Serial.print(millis() - LastJog_ms);
        Serial.print(" ms since prev jog. Sent jog command: ");
        Serial.print(JogCommand); // echo in Serial monitor.
        //Serial.print(" The above jog command was sent to Teensy from ");
        //Serial.println(WhereFrom);
        LastJog_ms = millis();  // to time the inter-joglet-interval
        //delay(RX_WAIT_MS);
        ReadTeensyMsg(true, " SendJog:JogCmd ");
        if (i > 0)
        {
         Serial.print(i);
         Serial.println(" loops waiting for ok after prev. jog");       
        }
        SendCommand(true, 0x87); // To keep the DRO updated.
      }
      else // No ok from Teensy.
      {
        Serial.println(" No ok.  No ok.  No ok.  No ok.  No ok.  No ok.  No ok.  No ok.  No ok.  No ok.  No ok.  No ok.  No ok.  No ok.  No ok.  No ok.  No ok.  No ok.  No ok. ");
        Serial.println(" Error waiting for prev. jog to be queued. Resetting JogOKreply to true. ");
        Beep();
        JogOKreply = true;
      }
     } // end OK to send jog
    } // end Teensy can receive command
   } // end Either Joystick or ROE is enabled, and the joglet interval has elapsed.
   //if (Not_MPGmode == 0) ToggleMPGmodeTo("MPG:0");  // Give command back to ioSender.
   
  } // end SendJog
  

  void CheckIfTeensyReady(int CommandLength)
  {
    Serial.println(" Checking if TeensyReady. ");
    SendCommand(true, 0x87);
    TeensyReady = true; // it is ready unless proven otherwise.
    int BytesAvailable; // bytes left in Serial2 RX buffer of Teensy. (Max of 1023)
    int BlocksAvailable; // Commands left in planner. (Max of 35)
    int BfPos = TeensyMsg.indexOf("|Bf:");
    if (BfPos != -1) // There is buffer info in TeensyMsg
    {
      TeensyMsg.remove(0, BfPos + 4);
      BlocksAvailable = TeensyMsg.toInt();    
      Serial.println(); 
      Serial.print("Planner buffer: ");
      int BlocksInQueue = 35 - BlocksAvailable;
      for (int blocks = 1; blocks < (BlocksInQueue + 1); blocks++)
      {
        // to make a bar chart showing the Planner buffer status.   
        Serial.print("|--");      
      }
      Serial.println(BlocksInQueue);    
      
      if (BlocksInQueue >= MaxPlannerBlocks) 
      {
        TeensyReady = false;
      }
      int Comma = TeensyMsg.indexOf(',');
      TeensyMsg.remove(0, Comma + 1);
      //Serial.println(TeensyMsg);
      BytesAvailable = TeensyMsg.toInt();
      if (BytesAvailable < CommandLength) // This was always 1023 due to the fast baud rate and Teensy clock speed.
      {
        TeensyReady = false;
        Serial.print(BytesAvailable);
        Serial.print(" Waiting for Teensy RX buffer space...");
        Serial.print(CommandLength);
      }
    
    }  // end if there is a buffers field in the status message. 
    else // no "|Bf:"
    {
       //  Query $10 to know current bitfield.
       while (!MoveMode.startsWith("Idle")) // Can only change $10 in Idle mode.
       {
        SendCommand(true, 0x87);
        delay(20);
       }
      String StatusBitfield = "3170"; // set flag to include buffers' states.
      SendCommand(true, ("$10=" + StatusBitfield + '\n')); // To make sure the Bf: fields are in status reports. (Planner and RX buffer)
    }
  if (debugging) TimeElapsed("CheckIfTeensyReady");
}  // end CheckIfTeensyReady


void HaltJogging() // Halt any jog in progress.
{ 
      if (MoveMode.startsWith("Jog")) // Skip over if it is already stopped.
      {
        LastTime_us = micros();
        int HaltTime_ms = millis();
        SendCommand(false, 0x85); // Cancel jog and flush the commands buffer in Teensy. No reply expected.
        SendCommand(true, "G4P0\n"); // A zero-delay dwell command. Will cause an ok response to be sent when 0x85 completes.
        Serial.print("***** Halting time_ms: ");
        Serial.println(millis() - HaltTime_ms);
        //Beep();
        //Serial.println(" Halting jog in progress. ");
        SendCommand(true, 0x87);
        ROE_Counts[0] = 0;
        ROE_Counts[1] = 0;
        ROE_Counts[2] = 0;
      } // end if Jog mode
     if (debugging) TimeElapsed("HaltJogging"); 
}  // end HaltJogging
