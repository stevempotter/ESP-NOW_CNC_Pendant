
void ProcessError(String ErrorMsg); // forward declaration (see below)

void ProcessTeensyStatusMsg()  // parse out the numbers in a status message from grbl Teensy.
                               // Display the latest DRO coordinates on the LCD
{
  if (NewPacketReceived == true) // Process the message even if it is identical to the last one.
  {    
    S4A.digitalWrite(S4A_MPGMODE_LED, ReceivedESPNOWpacket.MPGmode); // Big Green LED indicates that the Pendant has control when lit.
    //Serial.println();
    //Serial.print(" MPG:");
    // Serial.println(ReceivedESPNOWpacket.MPGmode);    
    String msg = ReceivedESPNOWpacket.MessageText;
    Serial.print("NewPacketReceived: ");
    Serial.println(msg);  
    NewPacketReceived = false; // This 'uses up' the new packet from the Receiver.
    if (msg.length() == 0) return; 
    if ((msg.indexOf("error") != -1 ) || (msg.indexOf("Alarm") != -1))
    {
     ProcessError(msg); 
    }
    else
    {
      Error = false;
    }
    memcpy(ReceivedESPNOWpacket.MessageText, &EmptyCharArray, MSG_SIZE)  ;  // erase the received packet message.
    //long nowMillis = millis();
    //long TimeSinceLastPacketRead = nowMillis - LastPacketRead_ms;
    LastPacketRead_ms = millis();
    //Serial.print(" Time since last packet read (ms): " + String(TimeSinceLastPacketRead) + ". Receiver says:\n ");
    Serial.println(msg);
    Click();
    digitalWrite(RX_LED, HIGH); // blue LED
    delayMicroseconds(BlinkFast_us);
    digitalWrite(RX_LED, LOW);

    if ( msg.indexOf('<') == -1) 
    {
     Serial.println(" < MISSING FROM TEENSY'S STATUS REPORT. MSG:["); // ESP32-S2 was cutting off the first few chars. LilyGO ESP32 doesn't.
     Serial.print(msg);
     Serial.println(']');
 /*   debugging stuff
     int MsgLength = msg.length();
     Serial.print(MsgLength);
     for (int i = 0; i < MsgLength; i++)
     {
      Serial.print(msg[i]);
      Serial.print('[');
      Serial.print(int(msg[i]));
      Serial.print(']');
     }
     Serial.println();
 */
     // *** To Do: PROCESS other messages, like ALARM and MSG: messages.
        
    } // end if no < in message

    if (msg.indexOf('<') != -1) msg.remove(0, (msg.indexOf('<') + 1)); // exclude the < char at the beginning of msg.
    int BarCharPos = msg.indexOf('|');
    MoveMode = msg.substring(0, BarCharPos); 
    msg.remove(0, (BarCharPos));   // remove the move mode from beginning of message.  
    if (msg.indexOf("|WPos:") == -1) 
    {
      Serial.print("No |Wpos: found in Teensy message:[");
      Serial.print(msg);
      Serial.println(']');
    }
    else // There are coordinates in message
    { 
      int Colon = msg.indexOf(':');    
      int Comma = msg.indexOf(','); // index of comma between X and Y coords
      DRO_X = msg.substring((Colon + 1), Comma-1); // substring(from(inclusive), to(exclusive)) 
      // I am truncating the last digit (microns) because my machine is not that precise.
      msg.remove(0, (Comma + 1));// remove(index,count) // +1 because index starts at 0
                                  // Starting at index 0, remove up to and including the first comma.
      Comma = msg.indexOf(','); // find the comma between Y and Z.
      DRO_Y = msg.substring(0, Comma-1);
      msg.remove(0, (Comma + 1));// remove up to and including 2nd comma 
      int Vbar = msg.indexOf('|');  // end of coords
      DRO_Z = msg.substring(0 , Vbar-1);

      
    /* For debugging the status report parsing:

      Serial.print("Mode: ");
      Serial.print(MoveMode);
      Serial.print(" (");
      Serial.print(DRO_X);
      Serial.print(", ");
      Serial.print(DRO_Y);
      Serial.print(", ");
      Serial.print(DRO_Z);
      Serial.println(") ");
    */
      //printf("Mode: %s (%f, %f, %f)\n", MoveMode, DRO_X, DRO_Y, DRO_Z); // printf was having a hard time showing Mode reliably.
      RefreshDRO();
    } // end Parse coordinates.
      int MPGstatusPos = msg.indexOf("|MPG:"); // This may not be present in brief reports 
      if (MPGstatusPos != -1) 
      {      
        char MPGchar = msg[MPGstatusPos + 5];
        MPGstatus = MPGchar - '0'; // ASCII value of '0' is 48. So '0'-48=0, '1'-'0' = 1
        if (MPGstatus != PrevMPGstatus)
        {        
          if (MPGstatus != ReceivedESPNOWpacket.MPGmode)
          {
            Serial.println(" WARNING: Status report MPG: mode is out of sync with MPGmode pin. ");
          }
          Serial.print(" MPGstatus:");
          Serial.print(MPGstatus);
          //Beep();
          PrevMPGstatus = MPGstatus;
        } // end if MPG mode changed.
      } // end msg has an MPG: status in it.

  } // end NewPacketReceived 
}  // end ProcessTeensyStatusMsg



// EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE  Errors  EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
//*** TO DO: Add unlock command somewhere to clear an Alarm lock.
// //SendCommand(true, "$X\n"); // Unlock command. 
void ProcessError(String ErrorMsg)
{
  // scrolling banner Sprites vars:
  int VerticalPosition = 50; // Upper edge of scrolling banner.
  int ScrollStepCounter = 0;
  int ScrollStep = -3; // Must be negative for scrolling from right to left. Use a more neg. number for faster scrolling, but jumpier.
  int16_t MsgPixWidth; // The width of the entire message to be scrolled, in pixels.
  int SpaceBetweenRepeats = 10; // in pixels.
  int TopPadding = 10; // to keep the letters from touching the top edge of banner.
  String TemporaryString; // used to convert char[] to String.

  Serial.println(ErrorMsg);
  Error = true;
  ErrorMsg = ErrorMsg.substring(0, 30); // Just show the first part of error or alarm message.
  MsgPixWidth = lcd.textWidth(ErrorMsg);
  lcd.fillScreen(TFT_YELLOW);
  BannerSprite.createSprite(MsgPixWidth + lcd.width(), (lcd.fontHeight() + TopPadding)); 
  BannerSprite.setTextColor(TFT_YELLOW, TFT_BLACK); // Yellow text, black background
    // Sprite width is display plus the space to allow text to scroll from the right.

  Beep();
  delay(100);
  Beep();
  delay(100);
  Beep();
  
  BannerSprite.drawString(ErrorMsg, lcd.width(),TopPadding, GFXFF); 
  ScrollStepCounter = (MsgPixWidth / abs(ScrollStep)) + SpaceBetweenRepeats;

  ESPNOWpacket2send.MenuButton = !S49.digitalRead(S49_PIN_MENU_BUTTON);
  int ScrollCounter = 1;
  while (ESPNOWpacket2send.MenuButton == 0) // get stuck here until menu button is pressed for one scroll loop.
  {
    ScrollStepCounter--; 
      if (ScrollStepCounter <= 0)
      { // Redraw the text to the right, just off the screen.      
        Beep();
        ScrollStepCounter = (MsgPixWidth / abs(ScrollStep))+ SpaceBetweenRepeats;
        BannerSprite.drawString(ErrorMsg, lcd.width(),TopPadding, GFXFF);
        ScrollCounter++;
      }
    BannerSprite.scroll(ScrollStep);    // Shift the Sprite over by ScrollStep pixels.
    BannerSprite.pushSprite(0, VerticalPosition); // Upper left corner of sprite 
    ESPNOWpacket2send.MenuButton = !S49.digitalRead(S49_PIN_MENU_BUTTON);
    TemporaryString = ReceivedESPNOWpacket.MessageText;
    //if (TemporaryString.indexOf("Idle") != -1 ) break; // escape if error is fixed.
    if (ScrollCounter > 3) break; //Probably need to switch off pendant and go fix problem with ioSender.
  }

    if (TemporaryString.indexOf("Idle") != -1 ) 
    {
      Error = false;
      return; // escape if error is fixed.
    }
  
  
  
} // end ProcessError


/*
ID	Alarm Code Description
1	Hard limit triggered. Machine position is likely lost due to sudden and immediate halt. Re-homing is highly recommended.
2	G-code motion target exceeds machine travel. Machine position safely retained. Alarm may be unlocked.
3	Reset while in motion. Grbl cannot guarantee position. Lost steps are likely. Re-homing is highly recommended.
4	Probe fail. The probe is not in the expected initial state before starting probe cycle, where G38.2 and G38.3 is not triggered and G38.4 and G38.5 is triggered.
5	Probe fail. Probe did not contact the workpiece within the programmed travel for G38.2 and G38.4.
6	Homing fail. Reset during active homing cycle.
7	Homing fail. Safety door was opened during active homing cycle.
8	Homing fail. Cycle failed to clear limit switch when pulling off. Try increasing pull-off setting or check wiring.
9	Homing fail. Could not find limit switch within search distance. Defined as 1.5 * max_travel on search and 5 * pulloff on locate phases.
10	Homing fail. On dual axis machines, could not find the second limit switch for self-squaring.


ID	Error Code Description
1	G-code words consist of a letter and a value. Letter was not found.
2	Numeric value format is not valid or missing an expected value.
3	Grbl '$' system command was not recognized or supported.
4	Negative value received for an expected positive value.
5	Homing cycle is not enabled via settings.
6	Minimum step pulse time must be greater than 3usec
7	EEPROM read failed. Reset and restored to default values.
8	Grbl '$' command cannot be used unless Grbl is IDLE. Ensures smooth operation during a job.
9	G-code locked out during alarm or jog state
10	Soft limits cannot be enabled without homing also enabled.
11	Max characters per line exceeded. Line was not processed and executed.
12	(Compile Option) Grbl '$' setting value exceeds the maximum step rate supported.
13	Safety door detected as opened and door state initiated.
14	(Grbl-Mega Only) Build info or startup line exceeded EEPROM line length limit.
15	Jog target exceeds machine travel. Command ignored.
16	Jog command with no '=' or contains prohibited g-code.
17	Laser mode requires PWM output.
20	Unsupported or invalid g-code command found in block.
21	More than one g-code command from same modal group found in block.
22	Feed rate has not yet been set or is undefined.
23	G-code command in block requires an integer value.
24	Two G-code commands that both require the use of the XYZ axis words were detected in the block.
25	A G-code word was repeated in the block.
26	A G-code command implicitly or explicitly requires XYZ axis words in the block, but none were detected.
27	N line number value is not within the valid range of 1 - 9,999,999.
28	A G-code command was sent, but is missing some required P or L value words in the line.
29	Grbl supports six work coordinate systems G54-G59. G59.1, G59.2, and G59.3 are not supported.
30	The G53 G-code command requires either a G0 seek or G1 feed motion mode to be active. A different motion was active.
31	There are unused axis words in the block and G80 motion mode cancel is active.
32	A G2 or G3 arc was commanded but there are no XYZ axis words in the selected plane to trace the arc.
33	The motion command has an invalid target. G2, G3, and G38.2 generates this error, if the arc is impossible to generate or if the probe target is the current position.
34	A G2 or G3 arc, traced with the radius definition, had a mathematical error when computing the arc geometry. Try either breaking up the arc into semi-circles or quadrants, or redefine them with the arc offset definition.
35	A G2 or G3 arc, traced with the offset definition, is missing the IJK offset word in the selected plane to trace the arc.
36	There are unused, leftover G-code words that aren't used by any command in the block.
37	The G43.1 dynamic tool length offset command cannot apply an offset to an axis other than its configured axis. The Grbl default axis is the Z-axis.
38	Tool number greater than max supported value.


*/