
// This is for debugging and testing UART communication between the CNC Pendant Receiver ESP32 and the Teensy 4.1
// After everything is working, I will seldom if ever use it again.
// Teensy 4.1 RX1 is Pin 0, TX1 is Pin 1. 
// ThingPlus ESP32-S2 RX1 is Pin 33, TX1 is Pin 34
// DOIT ESP32 Devkit v.1 RX is pin 16, TX is pin 17
// Connect RX to TX and TX to RX. 
// I use yellow wire for Teensy -> ESP32, Blue for ESP32 -> Teensy.
// (Same colors I use for those on Ch1 and Ch2 of scope.)
// On the Teensy, I am running grblHAL. It uses 115200-8-N-1 by default.
// I am using this to test other baud rates. 2,000,000 works. Edit driver.c of the grblHAL firmware around line 2410.

// The single-character commands used in the Keypad plugin only work if MPG mode is off (MPG:0) as set by the wired MPG toggle.
// I am using Teensy pin 41 for MPG toggle, which is the Barrett BoB I2C Strobe line, repurposed.
// It is controlled by MPG_NOT_ACTIVE_PIN of the RECEIVER ESP32, once the Teensy has finished starting up.
// The Status command ? and the $ commands and the Gcode commands only work in MPG:1 mode.
// See all the commands at: https://github.com/gnea/grbl/blob/master/doc/markdown/commands.md

void ToggleMPGmodeTo(String TargetMPGmode); // forward declarations
void SendCommand(bool WaitForReply, String Command);
void SendCommand(bool WaitForReply, byte Command);
String ReadUserInput();


void ProcessManualCommand()  // This should be polled in the main loop a few times per second.
{
 InputString = "";
 uint16_t StringLength = 0; // Used for serial commands entered manually in the Serial Monitor.
 char OneChar;
 if (Serial.available()) { InputString = ReadUserInput();}
 
  // Possible outcomes:
  // 0. typed a command to change this sketch's settings.
  // 1. Skip over because nothing is waiting in the Serial input buffer.
  // 2. User just hit return or Send. Skip over.
  // 3. User typed a 0xXX Realtime Command (including an MPG mode change, 0x8B)
  // 4. User typed a $ grbl command, which only work in MPG:1 mode.
  // 5. User typed a single-char Keypad command, which only work in MPG:0 mode.
  // 6. User typed a legacy one-char command, ?(brief status) ~(cycle start) !(feed hold) ctrl-x(reset grbl)
  //    I think these only work in MPG:1 mode.

  if (InputString != "") // There is something to send...
  {
   OneChar = InputString.charAt(0);
   StringLength = InputString.length();
   switch (StringLength)
   {
    case 0:  
     break; // Empty string. do nothing.
    
    case 1: // A single-char command (Probably in Keyboard mode, MPG:0)
     // lots of other single-char keyboard commands I could add here.
     // See the Commands_Reference tab.
     
     if (OneChar == '\r') break; // Just a CR 
     if (OneChar == '\n') break; // juse a NL
     if (OneChar == '!') Serial2.write(0x21); // Pause
     if (OneChar == '?') Serial2.write(0x3F);  // Brief status report. Can also use 0x80
     if (OneChar == '~') Serial2.write(0x7E); // Start/Resume
     break;
    
    default: // longer command than one char. Or one-char not listed above.     
     if (InputString.startsWith("toggle show packets")) // startsWith(): to be immune to which endline it may have.
     {
      ShowPacketsFlag = !ShowPacketsFlag;
      Serial.println(" Toggled Show Packets. ");
      Beep();
      return;
     }
     if (InputString.startsWith("toggle status"))
     { // toggle polling of periodic status reports.
      StatusReports = !StatusReports;
      Beep();
      Serial.print("Periodic status reports:");
      Serial.print(StatusReports);
      return;
     }
     
     if (InputString.startsWith("0x"))
     {        
      InputString = InputString.substring(0,4); // trim off CR or NL chars. Strings' index starts at 0, and substring EXCLUDES the 2nd indexed char and beyond.
      Serial.print(" Trimmed command string: [");
      Serial.print(InputString); // Echo string to the Serial Monitor 
      Serial.println("]"); 
      RTcommand = 0; // initialize as the null char. // There are more RT commands I could add. See Reference tab.
      if (InputString == "0x18") RTcommand = 0x18; // Soft reset
      if (InputString == "0x83") RTcommand = 0x83; // Gcode_report
      if (InputString == "0x87") RTcommand = 0x87; // Status_report_all. Only works in MPG:1 mode. <Idle|WPos:0.000,0.000,0.000|WCS:G54|A:|Sc:|MPG:1|H:0|T:0|TLR:0|FW:grblHAL>
      if (InputString == "0x80") RTcommand = 0x80; // Status_brief  <Idle|WPos:0.000,0.000,0.000>      
      if (InputString == "0x81") RTcommand = 0x81; // Cycle start or resume after hold      
      if (InputString == "0x82") RTcommand = 0x82; // Feed hold (pause execution of gcode gracefully)
      if (InputString == "0x84") RTcommand = 0x84; // Door open (pause and lift spindle)
      if (InputString == "0x85") RTcommand = 0x85; // Jog Cancel immediately
      if (InputString == "0x91") RTcommand = 0x91; // Increase FEED rate by 10%.
      if (InputString == "0x90") RTcommand = 0x90; // Set back to 100% of programmed FEED rate.
      if (InputString == "0x95") RTcommand = 0x95; // Set back to 100% of programmed RAPID rate.            
      if (InputString.startsWith("0x8B") )  // This command is "sent" via the MPG mode wire, not UART RX and TX.
      {
        InputString = ""; 
        if (Not_MPGmode == 1) 
         {ToggleMPGmodeTo("MPG:1");}// MPG:0 to MPG:1 
        else
         { ToggleMPGmodeTo("MPG:0");}// MPG:1 to MPG:0                 
      }
      
      if (RTcommand != 0) SendCommand(true, RTcommand); // Some elicit a reply and some don't.
      else //no RTcommand. starts with 0x but not on the above list.
      {
        Serial.println("0xXX Command not recognized. Not sent.");
      }
      
     } // end Starts with 0x
     else // InputString does not begin with "0x"
     {      
      SendCommand(true, InputString); // for other commands.   
     }
   } // end Switch case
  } // end Input != ""
  if (debugging) TimeElapsed("ProcessManualCommand");
}  // end  ProcessManualCommand()

////////////////////////////////////////////////////////////
void SendCommand(bool WaitForReply, String Command) // some commands want CR or LF, some don't. Set as needed in Serial Monitor.
{ // Send a String command. Must already end with a CR (0x0D) or LF (0x0A) but not both, or Teensy gets confused.
  //if (Not_MPGmode == 1)  ToggleMPGmodeTo("MPG:1"); // to MPG:1// we are in MPG:0 keypad mode
  StartMicros = micros();
  Serial2.print(Command);  // send string to the Teensy. If it is not recognized, Teensy just ignores it.
  Serial.print("String sent to Teensy: [");
  Serial.print(Command);
  Serial.println(']'); // so I can see if it is sending blanks
  //delay(RX_WAIT_MS);
  ReadTeensyMsg(WaitForReply, " SendCommand:Str "); // read reply from Teensy, send to Serial Monitor
  //if ( (ESPNOWpacket.JOYenabled == false) && (ROEenabled == false) ) ToggleMPGmodeTo("MPG:0"); // put it back to keypad mode MPG:0 
}

// Overloading the function to allow different types of input.
void SendCommand(bool WaitForReply, byte Command)
{
   //if (Not_MPGmode == 1)  ToggleMPGmodeTo("MPG:1"); // to MPG:1// we are in MPG:0 keypad mode
   StartMicros = micros();
   Serial2.write(Command);  // send one byte to the Teensy.  // Sends bytes out TX1 of ESP32-S2 pin 34. If it is not recognized, Teensy just ignores it.
   Serial.print("Byte sent to Teensy: [0x");
   Serial.print(Command, HEX);
   Serial.println(']');
   delay(RX_WAIT_MS);
   ReadTeensyMsg(WaitForReply, " SendCommand:Byte "); // read reply from Teensy, send to Serial Monitor
   //if ( (ESPNOWpacket.JOYenabled == false) && (ROEenabled == false) ) ToggleMPGmodeTo("MPG:0"); // put it back to keypad mode MPG:0 
}

///////////////////////////////////////////////////////////////
String ReadUserInput() // stay stuck here until user hits RETURN key.
{
  if (Serial.available()) // Arduino IDE: even if it is set to "No line ending", you have to hit SEND or the return key to enter any chars.  
  {
   char inChar;
   char CarriageReturn = 0x0D; // ASCII 13
   int x = 0;
   do
   {
    x++;
    if (x > 300 ) 
    {
      Serial.println(" User input timed out. Next time, hit return to enter a command.");
      break;
    }
    delay(50);
    inChar = Serial.read(); // Reads bytes from the text entry to Serial Monitor window.
    //  if the Serial input buffer is empty, it returns 255.
    if (inChar != 255)
    {
    // Serial.print("[");
    // Serial.print(int(inChar)); // handy for debugging what happens when return key is hit
    // Serial.print("] ");
     InputString += inChar; // appends to the growing String.
     Serial.print(inChar);
    }
   } while (inChar != CarriageReturn);
   
    
  } 
  Serial.print(" Read from Arduino Serial Monitor input: [");
  Serial.print(InputString); // Echo string to the Serial Monitor 
  Serial.println("]");  
  return InputString;
}


////////////////////////////////////////////////////////////////
void ToggleMPGmodeTo(String TargetMPGmode) // "MPG:1" or "MPG:0" . Switch modes using a digital line, not the 0x8B command. 
{                    // MPG_NOT_ACTIVE_PIN // Connects to pin 41 of Teensy.
  // Confusingly, the mode switch line goes LOW to activate the pendant. MPG:1
  if  ((TargetMPGmode == "MPG:1") &&  (Not_MPGmode == 1)) // Don't switch if it is already there.
  {
    Not_MPGmode = 0; // Switch to MPG:1
    digitalWrite(MPG_NOT_ACTIVE_PIN, Not_MPGmode); // toggle the mode using the MPG_NOT_ACTIVE_PIN
                                                 // Toggling this pin either way evokes an automated status report from Teensy.
                                                 // I think it is an 0x87 report if going to MPG:1 and a brief one for going to MPG:0.
    digitalWrite(LED_PIN, !Not_MPGmode); // Blue built-in LED is on when MPG is active.
    ESPNOWpacket.MPGmode = true; // Turns on MPG Active LED on pendant.
    Serial.print(" Not_MPGmode=");
    Serial.print(Not_MPGmode);
    Serial.println(": MPG mode changed. (0 is MPG:1,   1 is MPG:0) ");
    delay(MPGmodeChangeDelay_ms); // Wait a bit before doing anything else.
    //ReadTeensyMsg(false, " ToggleMPGmodeTo to MPG1 "); // Teensy replies with a 0x87 report in 3.7 us
                                                    // Or, sometimes it does not reply. 
  } // end if target is MPG:1 and it is not already there.
  
  if ((TargetMPGmode == "MPG:0") &&  (Not_MPGmode == 0)) // Switch back to MPG:0 if neither ROE nor joystick is enabled.
  {                              // So it should stay in MPG:1 the whole time an enable button is pressed.
    if ( (ESPNOWpacket.JOYenabled == false) && (ROEenabled == false) ) 
    {
      Not_MPGmode = 1; 
      digitalWrite(MPG_NOT_ACTIVE_PIN, Not_MPGmode); // toggle the mode using the MPG_NOT_ACTIVE_PIN
                                                 // Toggling this pin either way evokes an automated status report from Teensy.
                                                 // I think it is an 0x87 report if going to MPG:1 and a brief one for going to MPG:0.
      digitalWrite(LED_PIN, !Not_MPGmode); // Blue built-in LED is on when MPG is active.
      ESPNOWpacket.MPGmode = false; // Turns off big green MPG Active LED on pendant.
      Serial.print(" Not_MPGmode=");
      Serial.print(Not_MPGmode);
      Serial.println(": MPG mode changed. (0 is MPG:1,   1 is MPG:0) ");
      delay(MPGmodeChangeDelay_ms); // Wait a bit before doing anything else.
      //ReadTeensyMsg(false, " ToggleMPGmodeTo back to MPG0 "); // Teensy usually replies with a 0x80 report in 3.5 us
    } // end if neither joystick nor ROE is enabled.
  } // end if target is MPG:0 and it is not already there.

if (debugging) TimeElapsed("ToggleMPGmodeTo");

} // end ToggleMPGmodeTo
