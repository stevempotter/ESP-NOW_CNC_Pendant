

void SendMsgToPendant(String tempstr) // Send <message> from Teensy to the Pendant in a packet.
{                                 // These could be X, Y, Z coords.
   
   //Serial.println(" In SendMsgToPendant...");
   

   int StatusLength = (tempstr.length()); 
   int ParseStatusDelay_ms = 2; // Give pendant some time to parse this message before sending any more.
   //Serial.print("Bytes in msg to send: ");
   //Serial.print(StatusLength);
   //Serial.print("  ");
   //Serial.print(sizeof(ESPNOWpacket));
   //Serial.println(" bytes in whole packet.");
   if (StatusLength < MSG_SIZE) 
   {    
    tempstr.toCharArray(ESPNOWpacket.MessageText, StatusLength);
    //Serial.print(micros());
    //Serial.print(ESPNOWpacket.MessageText);
    esp_err_t result = esp_now_send(PeerMACaddress, (uint8_t *) &ESPNOWpacket, sizeof(ESPNOWpacket)); // send ESP-NOW packet to Pendant.
    delay(ParseStatusDelay_ms);
    if (result == ESP_OK) 
    {
     //Serial.println(" <- packet to pendant with this message field. ");     
     PacketSent = true; // Don't send another packet until this one is successfully received.
     
     //LastTeensyMsg = tempstr;  // I used this to see if the message has changed. With the timestamp appended, they always change.
    }
    else // message not sent to pendant
    {
     Serial.println(" Packet send error ---- Packet send error ---- Packet send error ---- Packet send error ---- Packet send error ---- Packet send error ----");
     Serial.print("Error sending a packet to the pendant. Error:");
     Serial.print(result);
     Serial.print(" Should be:");
     Serial.println(ESP_OK);    
     Beep();
    } // end else msg not sent
   }
   else // string is too long to sent to pendant
   {
    Serial.println(" MESSAGE from Teensy was too long to forward to Pendant. (>= MSG_SIZE bytes)");
    // *** TO DO: Break up long message into several packets, if Pendant needs to get long messages.  
    /*
    Beep();
    delay(100);
    Beep();
    delay(200);
    Beep();
    */   
   }
   if (debugging) TimeElapsed("SendMsgToPendant");
} // end SendMsgToPendant


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void PacketChanges()  // mostly for debugging the packet comm.
{
  
  if (PacketReceived == true)  
  {
    // 23 vars in Packet struct
    Click();
    if (ESPNOWpacket.X_ROEenabled != OLDpacket.X_ROEenabled)   PacketChangeCount++; //  // *** MAKE an array of flags?
    if (ESPNOWpacket.Y_ROEenabled != OLDpacket.Y_ROEenabled)   PacketChangeCount++; // 
    if (ESPNOWpacket.Z_ROEenabled != OLDpacket.Z_ROEenabled)   PacketChangeCount++; // 
    if (ESPNOWpacket.ROE_RotDir != OLDpacket.ROE_RotDir)       PacketChangeCount++; // 
    if (ESPNOWpacket.ROE_DeltaCounts != OLDpacket.ROE_DeltaCounts)     PacketChangeCount++; // 
    if (ESPNOWpacket.ROEmultiplier != OLDpacket.ROEmultiplier) PacketChangeCount++; //     
    if (ESPNOWpacket.JOYenabled != OLDpacket.JOYenabled)       PacketChangeCount++; // 
    if (ESPNOWpacket.JoyX != OLDpacket.JoyX)                   PacketChangeCount++; // 
    if (ESPNOWpacket.JoyY != OLDpacket.JoyY)                   PacketChangeCount++; // 
    if (ESPNOWpacket.MPGmode != OLDpacket.MPGmode)             PacketChangeCount++; //  NOTE: msg from Receiver to Pendant
    if (ESPNOWpacket.MenuButton != OLDpacket.MenuButton)       PacketChangeCount++; // 
    if (ESPNOWpacket.MenuClicksCount != OLDpacket.MenuClicksCount)       PacketChangeCount++; //
    if (ESPNOWpacket.Vacuum != OLDpacket.Vacuum)               PacketChangeCount++; // 
    if (ESPNOWpacket.SetXYto0 != OLDpacket.SetXYto0)           PacketChangeCount++; // 
    if (ESPNOWpacket.Light != OLDpacket.Light)                 PacketChangeCount++; // 
    if (ESPNOWpacket.ProbeZ != OLDpacket.ProbeZ)               PacketChangeCount++; // 
    if (ESPNOWpacket.GoTo00 != OLDpacket.GoTo00)               PacketChangeCount++; // 
    if (ESPNOWpacket.HomeXYZ != OLDpacket.HomeXYZ)             PacketChangeCount++; // 
    if (ESPNOWpacket.SetZto0 != OLDpacket.SetZto0)             PacketChangeCount++; // 
    if (ESPNOWpacket.Start != OLDpacket.Start)                 PacketChangeCount++; // 
    if (ESPNOWpacket.Pause_Halt != OLDpacket.Pause_Halt)       PacketChangeCount++; // 
    if (ESPNOWpacket.Spindle_ON != OLDpacket.Spindle_ON)       PacketChangeCount++; // 
    if (ESPNOWpacket.IncrementButton != OLDpacket.IncrementButton)       PacketChangeCount++; // 
    if (ESPNOWpacket.DecrementButton != OLDpacket.DecrementButton)       PacketChangeCount++; // 
    if (ESPNOWpacket.Park != OLDpacket.Park)                   PacketChangeCount++; // 
    if (ESPNOWpacket.MessageText != OLDpacket.MessageText)     PacketChangeCount++; //   NOTE: messages sent both ways. Mostly Teensy status reports to Pendant.
    Serial.print(PacketReceivedTimeStamp_ms);
    Serial.print(" New Packet from Pendant with ");
    Serial.print(PacketChangeCount);
    Serial.print(" changes since last packet. Interval_ms:");
    Serial.println(PacketReceivedTimeStamp_ms - PrevPacketReceivedTimeStamp_ms);
  } // end New packet received.

} // end PacketChanges


//////////////////////////////////////////////////////////////////////////////
void ShowPacket() // Show what was in the most recent data packet. 
{  
 if ( PacketReceived && (ShowPacketsFlag == true) )
 { 
 Serial.println("------------------------Begin Packet----------------------------------------------------------------");
 // ESP32 includes printf() by default. 
  Serial.println();
  Serial.print(ms_sinceLastPacket);
  Serial.println("ms since last packet received. ");
  printf(" Bytes Rc'd: %d \n", PacketLength);

  printf(" X_ROE enabled? %d \n", ESPNOWpacket.X_ROEenabled);
  printf(" Y_ROE enabled? %d \n", ESPNOWpacket.Y_ROEenabled);
  printf(" Z_ROE enabled? %d \n", ESPNOWpacket.Z_ROEenabled);
  printf(" ROE_RotDir: %i \n", ESPNOWpacket.ROE_RotDir);
  printf(" ROE_DeltaCounts: %d \n", ESPNOWpacket.ROE_DeltaCounts );
  printf(" ROEmultiplier: %d \n",ESPNOWpacket.ROEmultiplier); 
  printf(" Joystick enabled == %d \n", ESPNOWpacket.JOYenabled);
  printf(" JoyX: %d \n", ESPNOWpacket.JoyX);
  printf(" JoyY: %d \n", ESPNOWpacket.JoyY);
  printf(" Menu Button: %d \n", ESPNOWpacket.MenuButton);
  printf("Menu encoder: %d \n", ESPNOWpacket.MenuClicksCount);
  printf(" Vac Button: %d \n", ESPNOWpacket.Vacuum);
  printf(" SetXYto0 Button: %d \n", ESPNOWpacket.SetXYto0);
  printf(" Light Button: %d \n", ESPNOWpacket.Light); 
  printf(" Probe Z Button: %d \n", ESPNOWpacket.ProbeZ);
  printf(" Go to (0,0)Button: %d \n", ESPNOWpacket.GoTo00);
  printf(" Home XYZ Button: %d \n", ESPNOWpacket.HomeXYZ);
  printf(" Set Z to 0 Button: %d \n", ESPNOWpacket.SetZto0); 
  printf(" Cycle Start/Resume Button: %d \n", ESPNOWpacket.Start);
  printf(" Pause/Halt Button: %d \n", ESPNOWpacket.Pause_Halt);
  printf(" Spindle Start/Stop: %d \n", ESPNOWpacket.Spindle_ON);
  printf(" Increment button: %d \n", ESPNOWpacket.IncrementButton);
  printf(" Decrement button: %d \n", ESPNOWpacket.DecrementButton);
  printf(" Park %d \n", ESPNOWpacket.Park);
  printf(" RESET button: %d \n", ESPNOWpacket.Reset);
  int MsgLength = sizeof(ESPNOWpacket.MessageText); // 
  Serial.print(MsgLength);
  Serial.print(" bytes-long message: ["); 
  Serial.print(ESPNOWpacket.MessageText);
  Serial.println(']');
  //PacketChanges();
  //PacketReceived = false; // Use up this packet. // This is now done locally by each function that uses a variable.
  Serial.println("_______________________________________________________________End Packet____________________________");
 }  
}
