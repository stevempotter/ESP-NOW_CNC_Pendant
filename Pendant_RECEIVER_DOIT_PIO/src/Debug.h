

// Debugging stuff:

void wait(String prompt)  // 
{
   Beep();   
   Serial.print(prompt);
   Serial.println(" Hit return key to continue...");
   while (Serial.available() == 0) 
   { 
    Serial.print('.'); //wait
    delay(333);
   }
   String junk = Serial.readStringUntil('\n'); // to clear the Serial input buffer.
}




long TimeElapsed(String WhereAt)
 {  
  ThisTime_us = micros(); 
  DeltaT_us = ThisTime_us - LastTime_us; // Time since last call.
  if (debugging) 
  {
   Serial.print(" Time elapsed in ");
   Serial.print(WhereAt);
   Serial.print(":");
   Serial.print(DeltaT_us);
   Serial.println(" microSec");
  }  
  LastTime_us = micros();
  return DeltaT_us;
 }  // end TimeElapsed


// Use $$=n to find out the options for grbl setting n.

 /*
  * Grbl error codes: https://github.com/gnea/grbl/wiki/Grbl-v1.1-Interface#grbl-response-messages
  * 
  * Machine states
  * Valid states types: Idle, Run, Hold, Jog, Alarm, Door, Check, Home, Sleep
  * 
  * Current sub-states are:

- `Hold:0` Hold complete. Ready to resume.
- `Hold:1` Hold in-progress. Reset will throw an alarm.
- `Door:0` Door closed. Ready to resume.
- `Door:1` Machine stopped. Door still ajar. Can't resume until closed.
- `Door:2` Door opened. Hold (or parking retract) in-progress. Reset will throw an alarm.
- `Door:3` Door closed and resuming. Restoring from park, if applicable.
  */
