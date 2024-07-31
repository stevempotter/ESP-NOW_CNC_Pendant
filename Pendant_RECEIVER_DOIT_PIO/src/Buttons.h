/*
// Set Current as XY0: G10 L20 P0 X0 Y0
// P is the chosen coordinate system.
// See G10 L2 in gcode commands ref. 0 is active coord system. 
Machine coord is G53
These are "Coordinate System Offsets":
 P 1 G54
 P 2 G55
 P 3 G56
 P 4 G57
 P 5 G58
 P 6 G59
 P 7 G59.1
 P 8 G59.2
 P 9 G59.3

 Global Offsets: G92
 Local Offsets: G52


*/

void ParkSpindle(int parkX, int parkY); // forward declaration

void ProcessButtons()

// *** To do: Use key-up signal to evoke actions?
// Only one pass per packet recieved, so usually only one action is taken.

{
  if (PacketReceived == true)
  {
    if ((ESPNOWpacket.SetXYto0 == 1) && (OLDpacket.SetXYto0 == 0))
    {
      SendCommand(true, "G90G10L20P0X0.000Y0.000\n"); // set work coord X and Y to zero
      PacketReceived = false; // use up that packet.
      Beep();
      delay(10);
      Beep();
      SendCommand(true, 0x87);
      Serial.println(" Set current XY to 0. ");
    }
  
    if ((ESPNOWpacket.SetZto0 == 1) && (OLDpacket.SetZto0 == 0))
    {
      SendCommand(true, "G90G10L20P0Z0.000\n"); // set work coord Z to zero
      PacketReceived = false; // use up that packet.
      Beep();
      delay(10);
      Beep();
      SendCommand(true, 0x87);
      Serial.println(" Set current Z to 0. ");
    }

    if ((ESPNOWpacket.GoTo00 == 1) && (OLDpacket.GoTo00 == 0))
    {
      if (!Homed)
        {
          Beep();
          delay(50);
          Beep();
          delay(50);
          Beep();
          String tempstr = "Alarm:6 Home machine!";
          Serial.print(" Need to home the machine. ");
          SendMsgToPendant(tempstr);
          return; // if (0,0) is outside the machine's movement area, it may damage things.
        } // end if not homed.
      
      SendCommand(true, "G90G0X0Y0\n"); // RAPID jog to work coords home
      PacketReceived = false; // use up that packet.
      Beep();
      delay(10);
      Beep();
      SendCommand(true, 0x87);
      Serial.println(" Go to (0,0). ");
    }

//SSSSSSSSSSSSSSSSSSSSSss   Step Moves   SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS

    if (ROEenabled) // do a step move if Incr./Decr. button is also pressed.
    {
      String CommandStr = "$J=G21G91 "; // First part of step command. Use mm and incremental movement.
      if ((ESPNOWpacket.IncrementButton == 1) && (OLDpacket.IncrementButton == 0))
      { // Only step if prev. step signal ended.
        
        if (ESPNOWpacket.X_ROEenabled) 
        { 
          CommandStr += "X";
          CommandStr += StepMove_mm;
          CommandStr += "F";
          CommandStr += StepSpeed;
          CommandStr += '\n';  // all commands must have a newline symbol at the end.
          SendJog(" Step: +X", CommandStr); 
          PacketReceived = false; // use up that packet.
          Beep();
          delay(10);
          Beep();
          Serial.print(CommandStr);
          Serial.println(" Moved right one step. ");
        }
      
        if (ESPNOWpacket.Y_ROEenabled) 
        { 
          CommandStr += "Y";
          CommandStr += StepMove_mm;
          CommandStr += "F";
          CommandStr += StepSpeed;
          CommandStr += '\n';  // all commands must have a newline symbol at the end.
          SendJog(" Step: +Y", CommandStr); 
          PacketReceived = false; // use up that packet.
          Beep();
          delay(10);
          Beep();
          Serial.print(CommandStr);
          Serial.println(" Moved away one step. ");
        }     

        if (ESPNOWpacket.Z_ROEenabled) 
        { 
          CommandStr += "Z";
          CommandStr += StepMove_mm;
          CommandStr += "F";
          CommandStr += StepSpeed;
          CommandStr += '\n';  // all commands must have a newline symbol at the end.
          SendJog(" Step: +Z", CommandStr); 
          PacketReceived = false; // use up that packet.
          Beep();
          delay(10);
          Beep();
          Serial.print(CommandStr);
          Serial.println(" Moved spindle up one step. ");
        }
      } // end any positive step moves (up, right, away)

      if ((ESPNOWpacket.DecrementButton == 1) && (OLDpacket.DecrementButton == 0))
      { // Only step if prev. step signal ended.
        
        if (ESPNOWpacket.X_ROEenabled) 
        { 
          CommandStr += "X";
          CommandStr += -StepMove_mm;
          CommandStr += "F";
          CommandStr += StepSpeed;
          CommandStr += '\n';  // all commands must have a newline symbol at the end.
          SendJog(" Step: -X", CommandStr); 
          PacketReceived = false; // use up that packet.
          Beep();
          delay(10);
          Beep();
          Serial.print(CommandStr);
          Serial.println(" Moved left one step. ");
        }
      
        if (ESPNOWpacket.Y_ROEenabled) 
        { 
          CommandStr += "Y";
          CommandStr += -StepMove_mm;
          CommandStr += "F";
          CommandStr += StepSpeed;
          CommandStr += '\n';  // all commands must have a newline symbol at the end.
          SendJog(" Step: -Y", CommandStr); 
          PacketReceived = false; // use up that packet.
          Beep();
          delay(10);
          Beep();
          Serial.print(CommandStr);
          Serial.println(" Moved closer one step. ");
        }     

        if (ESPNOWpacket.Z_ROEenabled) 
        { 
          CommandStr += "Z";
          CommandStr += -StepMove_mm;
          CommandStr += "F";
          CommandStr += StepSpeed;
          CommandStr += '\n';  // all commands must have a newline symbol at the end.
          SendJog(" Step: -Z", CommandStr); 
          PacketReceived = false; // use up that packet.
          Beep();
          delay(10);
          Beep();
          Serial.print(CommandStr);
          Serial.println(" Moved spindle down one step. ");
        }
      } // end any negative step moves (down, left, closer)
    }  // end if ROE enabled (for step moves)
   
   // HHHHHHHHHHHHHHHHHH    HALT   HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Uses the DOOR function of grbl.
// *** TO DO: Add more sophisticated DOOR alarm handling:
/*
- `Door:0` Door closed. Ready to resume.
- `Door:1` Machine stopped. Door still ajar. Can't resume until closed.
- `Door:2` Door opened. Hold (or parking retract) in-progress. Reset will throw an alarm.
- `Door:3` Door closed and resuming. Restoring from park, if applicable. Reset will throw an alarm.
*/


    if ((ESPNOWpacket.Pause_Halt == 1) && (OLDpacket.Pause_Halt == 0))
    {
      SendCommand(true, 0x84); // The Open Door command
                               // This pulls up and stops the spindle; 0x82 and 0x88 don't so I don't implement those.
      PacketReceived = false; // use up that packet.
      Beep();
      delay(10);
      Beep();
      SendCommand(true, 0x87);
      Serial.println(" Pause/Halt (Door) ");
    }

    if ((ESPNOWpacket.Start == 1) && (OLDpacket.Start == 0))
    { 
      //SendCommand(false, 0x81); // The Cycle Start/Resume command // 0x81 doesn't seem to work.
      SendCommand(false, "~\n");  // ~ is the legacy Cycle Start grbl command.
      PacketReceived = false; // use up that packet.
      Beep();
      delay(10);
      Beep();
      SendCommand(true, 0x87);
      Serial.println("************************************************* Start/Resume *************************************************************************");
    }
    
// PPPPPPPPPPPPPPPPP    Park   PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

    if ((ESPNOWpacket.Park == 1) && (OLDpacket.Park == 0))
    { //  Park the spindle out of the way for bit changing, etc.
      
      ParkSpindle(290, 200); // (X, Y) in mm.   
      Beep();
      delay(100); 
      Beep(); // Sound to tell you parking has been invoked.
      delay(100); // get fingers out of the way.
      Beep();
      delay(50);
      Beep();
      PacketReceived = false; // use up that packet.      
      SendCommand(true, 0x87);
      MenuLongPressPending = false;
    }

  // RRRRRRRRRRRRRRRRRRR    Reset   RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR
    
    if ( (ESPNOWpacket.Reset == 1) && (OLDpacket.Reset == 0) )
    {
      PacketReceived = false; // use up that packet.
      SendCommand(true, 0x18); // Soft reset command (AKA Crtl-X)
      SendCommand(true, "$X\n"); // Unlock command
      Beep();
      delay(50); 
      Beep(); // Sound to tell you reset has been invoked.
      SendCommand(true, 0x87);
      Serial.println(" Soft reset and unlock! ");

    }

    // Long-press detection needs three parts if button is pressed:
    // 1. Start the button's down-timer if this one is not already started.
    // 1.5 If button is being held down, do nothing until later.
    // 2. Check if timer has elapsed. If so, trigger the longpress action.
    // 3. Reset things when button is released.

    // HHHHHHH        Homing   (long press) HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
    if ( (ESPNOWpacket.HomeXYZ == 1) && (OLDpacket.HomeXYZ == 0) && !HomeXYZpending )
    {
      HomeXYZbuttonDown_millis = millis(); // begin button longpress timer.
      HomeXYZpending = true;
      PacketReceived = false; // use up that packet.

    }
    if ( (ESPNOWpacket.HomeXYZ == 1) && (OLDpacket.HomeXYZ == 1))
    {
      Serial.print(" Keep pressing Home XYZ for 3 seconds. ms so far: ");
      int HomeXYZbuttonTimer = millis() - HomeXYZbuttonDown_millis;
      Serial.println(HomeXYZbuttonTimer);
      PacketReceived = false; // use up that packet.
    }

    // Check if timer has elapsed
    if ( (ESPNOWpacket.HomeXYZ == 1) && (HomeXYZpending) && ( (millis() - HomeXYZbuttonDown_millis) > LongPress_ms ) )
    {
      SendCommand(true, "$H\n"); // Homing command
      // It will only home the axes set in the Settings:Grbl window.
      PacketReceived = false; // use up that packet.
      Beep();
      delay(10); 
      Beep(); // Sound to tell you Homing has been invoked.
      SendCommand(true, 0x87);
      ResetPending = false;
      do
      {
        delay(500);
        Serial.println(" Busy homing...");
        SendCommand(true, 0x87);
      } while (MoveMode.startsWith("Home"));
       // In case homing does not zero out the coordinates, set them all to zero:
      SendCommand(true, "G90G10L20P0X0.000Y0.000Z0.000\n"); // set work coords to zero
      Serial.println(" Setting current XY and Z to 0. ");
      Homed = true; // assumes there was no homing error.
      // *** TO DO: add some error checking.
    }

    // Reset things on button up.
    if  ((ESPNOWpacket.HomeXYZ == 0) && (HomeXYZpending)) // Button released.
    {
      HomeXYZpending = false; // Used in case the button is released too soon.
      Serial.println(" Home XYZ button up. ");
      PacketReceived = false; // use up that packet.
    }

  // PPPPPPPPPPP     Probing  (long press)  PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
    if ( (ESPNOWpacket.ProbeZ == 1) && (OLDpacket.ProbeZ == 0) && !ProbeZpending )
    {
      ProbeZbuttonDown_millis = millis(); // begin button longpress timer.
      ProbeZpending = true;
      PacketReceived = false; // use up that packet.

    }
    if ( (ESPNOWpacket.ProbeZ == 1) && (OLDpacket.ProbeZ == 1))
    {
      Serial.print(" Keep pressing Probe Z for 3 seconds. ms so far: ");
      int ProbeZbuttonTimer = millis() - ProbeZbuttonDown_millis;
      Serial.println(ProbeZbuttonTimer);
      PacketReceived = false; // use up that packet.
    }

    // Check if timer has elapsed
    if ( (ESPNOWpacket.ProbeZ == 1) && (ProbeZpending) && ( (millis() - ProbeZbuttonDown_millis) > LongPress_ms ) )
    {
      Beep();
      delay(10); 
      Beep(); // Sound to tell you Homing has been invoked.
      Serial.println(" Probing! Check DRO to see if it worked. ");
      SendCommand(true, "G21 G91 G38.2 F60 Z-10\n"); // ProbeZ command.
                                          // Probe must be wired up correctly and bit must be within 10 mm of plate surface.
                                          // Stops when it hits the plate top and sets that to Z zero.
      int x = 0;
      do
      {
        delay(1000);
        SendCommand(true, 0x87); // keep asking for updates on what machine is doing until it is finished.
        x++;        
        if (x > 12)
        {
          Serial.println(" Probing aborted. ");
          return; // Something is wrong. Probing 10mm at 1mm/sec should take 10 sec.
        }
      } while (!MoveMode.startsWith("Idle"));                            
      String ZoffsetCmd = "G92 Z";
      ZoffsetCmd += String(ProbePlateThickness_mm);  // offset Z coords so that zero is at plate base/workpiece top.
      ZoffsetCmd += "\n";
      SendCommand(true, ZoffsetCmd);
      SendCommand(true, "G91 G21 G0 Z9.8\n"); // Move the bit back up 9.8mm.
      PacketReceived = false; // use up that packet.      
      SendCommand(true, 0x87);
      ProbeZpending = false;

    }

    // Reset things on button up.
    if  ((ESPNOWpacket.ProbeZ == 0) && (ProbeZpending)) // Button released.
    {
      ProbeZpending = false; // Used in case the button is released too soon.
      Serial.println(" Probe Z button up. ");
      PacketReceived = false; // use up that packet.
    }
 
    // mmmmmMMMMMmmmmMMMM     Menu Button    mmmmMMMMMmmmmmMMM
    // *** To Do: Other menufunctions to be added...
      if ( (ESPNOWpacket.MenuButton == 1) && (OLDpacket.MenuButton == 0) && !MenuLongPressPending )
    {
      MenuButtonDown_millis = millis(); // begin button longpress timer.
      MenuLongPressPending = true;
      PacketReceived = false; // use up that packet.

    }
    if ( (ESPNOWpacket.MenuButton == 1) && (OLDpacket.MenuButton == 1))
    {
      Serial.print(" Keep pressing menu button for 1 sec. ms so far: ");
      int MenuButtonTimer = millis() - MenuButtonDown_millis;
      Serial.println(MenuButtonTimer);
      PacketReceived = false; // use up that packet.
    }

    // Check if timer has elapsed
    if ( (ESPNOWpacket.MenuButton == 1) && (MenuLongPressPending) && ( (millis() - MenuButtonDown_millis) > MenuLongPress_ms ) )
    { 
      // add menu stuff
    }

    // Reset things on button up.
    if  ((ESPNOWpacket.MenuButton == 0) && (MenuLongPressPending)) // Button released.
    {
      MenuLongPressPending = false; // Used in case the button is released too soon.
      Serial.println(" Menu button up. ");
      PacketReceived = false; // use up that packet.
    }



  // *** TO BE ADDED: Vacuum switch, light switch

/* *** TO DO : Add the ability to alter spindle speed and feedrate during gcode program.
      // *** NEED TO CHANGE THIS TO USE INCR and DECR BUTTONS, NOT THE ROE.]
       case 'S': // Spindle speed adj. 
       {
        if (ESPNOWpacket.ROE_RotDir == -1) // CCW rotation: Slow spindle down.
        {
         if (abs(ESPNOWpacket.ROE_DeltaCounts) < 100) SendCommand(true, 0x9D); // - by 1%
         if (abs(ESPNOWpacket.ROE_DeltaCounts) >= 100) SendCommand(true, 0x9B); // - by 10%
        } // end CCW
        else // CW rotation: Speed spindle up
        {
         if (abs(ESPNOWpacket.ROE_DeltaCounts) < 100) SendCommand(true, 0x9C); // + by 1%
         if (abs(ESPNOWpacket.ROE_DeltaCounts) >= 100) SendCommand(true, 0x9A); // + by 10%
        } // end CW
        PrevROE_DeltaCounts = ESPNOWpacket.ROE_DeltaCounts;
        break;
       } // end case S, Spindle speed adj.
  
       case 'F': // Adjust feedrate during ongoing job.
       {
        if (ESPNOWpacket.ROE_RotDir == -1) // CCW rotation: Slow feedrate down.
        {
         if (abs(ESPNOWpacket.ROE_DeltaCounts) < 100) SendCommand(true, 0x94); // - by 1%
         if (abs(ESPNOWpacket.ROE_DeltaCounts) >= 100) SendCommand(true, 0x92); // - by 10%
        }
        else // CW rotation: Speed feedrate up
        {
         if (abs(ESPNOWpacket.ROE_DeltaCounts) < 100) SendCommand(true, 0x93); // + by 1%
         if (abs(ESPNOWpacket.ROE_DeltaCounts) >= 100) SendCommand(true, 0x91); // + by 10%
        }
        PrevROE_DeltaCounts = ESPNOWpacket.ROE_DeltaCounts;
        break;
       } // end case F, Adj. feedrate.
     */

  } // end if packet received


  
  if (debugging) TimeElapsed("ProcessButtons");
} // end ProcessButtons


void ParkSpindle(int parkX, int parkY) // to park the spindle out of the way for changing bits and vacuuming up.
{ // Could use G28 or G30 instead, if you trust that these were set properly.
 if (!Homed)
 {
   Beep();
   delay(50);
   Beep();
   delay(50);
   Beep();
   String tempstr = "Alarm:6 Home machine!";
   Serial.print(" Home the machine. ");
   SendMsgToPendant(tempstr);
 } // end if not homed.
 else
 { // machine has been homed by the pendant.
  Serial.println(" Parking! Raising spindle... ");  // 
  SendCommand(true, "G90 G21 G53 G0 Z0\n"); // Lift up the spindle to Z origin in machine coordinates.
  int x = 0;
    do
    {
      delay(1000);
      SendCommand(true, 0x87); // keep asking for updates on what machine is doing until it is finished.
      x++;        
      if (x > 5)
      {
        Serial.println(" Parking aborted. ");
        return; // Something is wrong. 
      }
    } while (!MoveMode.startsWith("Idle")); 
  String ParkCommand = "G90 G21 G53 G0 X" ; // move to park location in machine coords.
  ParkCommand += String(parkX);
  ParkCommand += " Y";
  ParkCommand += String(parkY);
  ParkCommand += "\n";
  SendCommand(true, ParkCommand);
  Serial.println(" Parking command queued. ");
 } // end if homed.
}  // end ParkSpindle(X,Y) function.
