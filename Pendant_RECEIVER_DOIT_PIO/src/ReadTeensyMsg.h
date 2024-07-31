
void ReadTeensyMsg(bool WaitForReply, String CalledBy) // This function goes once per line ('\n'-defined) in the Serial2 buffer. 
{  
  //Serial.print(" In ReadTeensyMsg...");

 TeensyMsg = "";
  


 if (WaitForReply)
 {
  int i = 0;
  int Timeout = 500; // ms Adjust according to how long it takes to halt a fast jog.
  while (!Serial2.available())
  {
   Serial.print('-');
   delay(1);
   i++;
   if (i > Timeout) break;
  }
  //Serial.print(" ms to get Teensy reply:");
  //Serial.println(i);
  if (i > Timeout) Serial.print(" Timed out - No reply. ");
 }
  
 if (Serial2.available())  // Allows skipping over this function if waitForReply was false and nothing arrived from Teensy.
 { 
     //Serial.print(Serial2.available());
    // Serial.println(" chars available. ");
    // Serial.print(" Reading bytes: ");   
     Serial2.setTimeout(20); //  (ms) Stop reading if message pauses for more than this.

     //TeensyMsg  += Serial2.readStringUntil('\n'); // Messages from Teensy end with CR LF (0x0D 0x0A) or dec 13 10.
                                                  // This function must be called multiple times for multi-line messages.
                                                  // Times out if it does not find '\n' in the ms of .setTimeout() (after what? Last byte? 1st byte?)
                                                  // 1024 bytes is about 10,000 bits; at 1e6 baud, that takes 10ms to transmit.
                                                  // Is Timeout based on receiving into RX buffer, or reading out of RX buffer?
     
     char OneChar;
     while (Serial2.available())
     {
      OneChar = Serial2.read(); // More versatile and reliable than Serial2.readStringUntil('\n').
      Serial.print(OneChar);
      if (int(OneChar) > 126) // TEST FOR NON-PRINTABLE CHARS INDICATING BAD SERIAL COMM. ASCII > 126; I think I got rid of these:
                              // I had too many CR/LF chars and something made bad chars if CR/LF arrive while grbl is trying to reply.
      {
        Serial.print("Junk char received, ASCII[");
        Serial.print(int(OneChar));
        Serial.print("] ???????????????????????????????????????????????????????????????????????????????????????????  ");
        OneChar = '?'; // don't add the bad char to the message; may avoid reset errors.
      }
      TeensyMsg.concat(OneChar);
     // Serial.print(OneChar);
      //Serial.print(OneChar);
      //Serial.print('[');
      //Serial.print( int(OneChar));
      //Serial.print(']');      
     } // end Read char from Serial2

     //Serial.print(millis());
     //Serial.println(" Serial2 read for " + CalledBy);
     // long ReadLatency_us = micros()- StartMicros;  // started in SendCommand.
     //Serial.print(ReadLatency_us);
     //Serial.print(" micros to read reply msg from Teensy, "); 
     Serial.print(TeensyMsg.length()); 
     Serial.println(" chars TeensyMsg.length.");
     //Serial.print(TeensyMsg); // Writes to the Arduino Serial Monitor, port 0 

     

     if ( (TeensyMsg.indexOf("k") != -1 ) && (JogOKreply == false) ) //  ok after a jog command. Sometimes the o is missing.
     {
      JogOKreply = true;  // Set to false in SendJog when a jog command is sent; 
                          // Used to block sending another command until this one is queued.
                          // Reset here to allow subsequent jog commands.
      //Serial.print(" Jog ok reply == true. Elapsed time_us: ");
      //Serial.println(millis() - LastJog_ms);
      SendCommand(true, 0x80); // Get the latest coords and verify jog state.
     }
     else
     {
      if (TeensyMsg.indexOf("rror:") != -1) // An error message in reply. Sometimes the e is missing.
      {
        JogOKreply = false; 
        Serial.println("!!!!!!!!!!!!!!!!!!!!!!  ERROR from Teensy !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        Serial.println(TeensyMsg);
        SendMsgToPendant(TeensyMsg); // Send the error message to the Pendant.
        Beep();
        delay(5);
        Beep();
        delay(5);
        Beep();
      }
     }
     // 0x87 causes a message like this: <Idle|WPos:0.000,0.000,50.000|Bf:35,1023|WCO:0.000,0.000,-50.000|WCS:G54|A:|Sc:|MPG:0|H:1,7|T:0|TLR:0|FW:grblHAL>
     if (TeensyMsg.indexOf("|WPos:") != -1) // there are coordinates in the message      
      {       
       SendMsgToPendant(TeensyMsg); // Send the message to the Pendant.
      }
           
      int HomedStatusIndex = TeensyMsg.indexOf("|H:");
      if (HomedStatusIndex == -1) // Homed status not found. Look for other useful info.
      {
        Serial.println(" No |H: (homed status) in 0x87 reply.");  
      }   // end no "|H:" in message.
      else // Homed status is present in message
      {
        char HOMEDstatus = TeensyMsg.charAt(HomedStatusIndex + 5);
        if (HOMEDstatus == '7') 
        {
          Homed = true;
          Serial.print("Homed is true. ");
        }
        else 
        {
          Homed = false;
          Serial.print("Homed is false. ");
        }
        Serial.print("Homed status bitmask: "); // X=1, Y=2, Z=4, so X&Y&Z=7 
        Serial.println(HOMEDstatus);

        /* An alternative way to get homed status, by sending the $# command. 
        int HOMEmsgIdx = TeensyMsg.indexOf("HOME:");   // Reply will be a lot of lines with coordinates in them, and one will be [HOME:0.000,0.000,0.000:7]
        if (HOMEmsgIdx != -1)     // Contains homed status info.
        {
          String HOMEDmsg = TeensyMsg.substring(HOMEmsgIdx, TeensyMsg.length()); // Chop off the beginning lines.
          int SecondColonIdx = HOMEDmsg.indexOf(':', 6); // look for ':' past first colon.
          char HOMEDstatus = HOMEDmsg.charAt(SecondColonIdx + 1);
        }
       */
      }
      int LessThanCharPos = TeensyMsg.indexOf('<');
      if (LessThanCharPos != -1) // it's a well-formed status report
      { // there is a < in message    
       int BarChar = TeensyMsg.indexOf('|'); // The first field separator.
       MoveMode = TeensyMsg.substring((LessThanCharPos +1), BarChar); 
       //Serial.print("Status from Teensy. Elapsed time: ");
       // Serial.print(micros() - Now_us);
       //Serial.print(" us elapsed. Current mode: ");
       //Serial.println();
       // Serial.print("                                                            [");
       //Serial.print(MoveMode);
       int MPGmodePos = TeensyMsg.indexOf("|MPG:");
       if (MPGmodePos != -1)
        {
          char MPGstatusChar = TeensyMsg.charAt(MPGmodePos + 5) ; // MPG:1 is Pendant controlling.
          MPGstatus = int(MPGstatusChar) - 48; // int('1') = 49
        }
       //Serial.print("  MPG:");
       //Serial.print(MPGstatus);
       //Serial.println(']');
      } // end if there's a < in message
     

   //Serial.print(" Leaving ReadTeensyMsg with Serial.available: ");
   //Serial.println(Serial2.available() );
 } // end if Serial.available() 
  if (debugging) TimeElapsed("ReadTeensyMsg");
  //Serial.println(" Leaving ReadTeensyMsg.");
} // end ReadTeensyMsg


void ClearRXbuffer() // flush out the RX buffer of chars from Teensy.
{
  char tempChar;
  //Serial.print("Clearing RX buffer: ");
  //Serial.println(Serial2.available());
  while (Serial2.available() )
  {
    tempChar = Serial2.read();
    Serial.print('[');
    Serial.print(tempChar);
    Serial.print(']');
    Click();
  }
} // end ClearRXbuffer
