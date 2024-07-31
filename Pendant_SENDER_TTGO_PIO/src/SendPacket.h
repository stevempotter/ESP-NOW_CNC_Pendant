

void SendESPNOWpacket(String WhereFrom)
{
 // If ESP-NOW fails to get status==0 from sending, it retries 2 more times at slower bps. That takes about 20ms.
   { 
    // put a message into the MessageText char array.      
    //String tempstr = ("Hello from Pendant! Millis since last packet received: " +  String( millis() - LastPacketRead_ms ) );
    //int str_len = tempstr.length() +1; // added 1 for the \0 char.
   // tempstr.toCharArray(ESPNOWpacket2send.MessageText, str_len);
    //Serial.println(tempstr);
    //Serial.print(tempstr);
    //Serial.print(" Packet sent to Receiver from ");
    //Serial.print(WhereFrom);
    //Serial.print(". Message to Receiver:\n[");
    //Serial.print(ESPNOWpacket2send.MessageText);
    //Serial.println(']');

    // Send a packet via ESP-NOW:
   // LastTime_us = micros();
    esp_err_t result = esp_now_send(PeerMACaddress, (uint8_t *) &ESPNOWpacket2send, sizeof(ESPNOWpacket2send)); 
    LastPacketSent_ms = millis(); // may or may not be delivered successfully.
    PacketDelivered = false; // gets set to true once receipt is confirmed.
    //memcpy(ESPNOWpacket2send.MessageText, &EmptyCharArray, MSG_SIZE)  ;  // erase the packet message.
   // TimeElapsed("PacketSending"); // takes only 163 us
    if (result == ESP_OK) // == 0
    {
     //Serial.println("Sent with success"); // Doesn't necessarily mean it was received.
     //Click();
    // LastTime_us = micros();
     digitalWrite(TX_LED, HIGH); // Yellow LED
     delayMicroseconds(BlinkFast_us);
     digitalWrite(TX_LED, LOW);
     //TimeElapsed("blinking TX LED");


    }
    else // failed send
    {
     Serial.print(result);
     Serial.println(" error sending the packet."); 
     // result == 12391 is no wifi (ESP-NOW) connection error.
     //Beep();
    } // end else
   }
}
