// Digital Readout screen on pendant
// DRO values are those read from the Teensy status reports.


void RefreshDRO()
{  
  String tmpstring = "";
  if ( ( (millis() - LastRefresh_ms) > 2000) && (!Error) ) // refresh screen every 2 seconds, in case there is lingering cruft.
  {
   Click();
   lcd.fillScreen(0x915C); // violet
   //tft.fillScreen(TFT_VIOLET);
   LastRefresh_ms = millis();
   Checkbattery();
   tmpstring.concat("X: " + DRO_X + "  " + MoveMode);
 //inline size_t drawString(const char *string, int32_t x, int32_t y, uint8_t      font)

   lcd.drawString(tmpstring, 5, 5); 
   lcd.drawString("Y: " + DRO_Y, 5, 55);
   lcd.drawString("Z: " + DRO_Z, 5, 105);
  // tft.drawString(tmpstring, 5, 5, GFXFF); // sans bold 18 font
  // tft.drawString("Y: " + DRO_Y, 5, 55, GFXFF);
  // tft.drawString("Z: " + DRO_Z, 5, 105, GFXFF);
  } // end background refresh
  
  if ((millis() - LastDROupdate_ms) > 100) // Don't bother updating DRO faster than we can see.
  {
   LastDROupdate_ms = millis();
   if ( (oldDRO_X != DRO_X) || (MoveMode != oldMoveMode) )
   {
    tmpstring = "X: ";
    tmpstring.concat( DRO_X + "  " + MoveMode);
    lcd.drawString(tmpstring, 5, 5);
    oldDRO_X = DRO_X;
   }

   if (oldDRO_Y != DRO_Y)
   {
    lcd.drawString("Y: " + DRO_Y, 5, 55);
    oldDRO_Y = DRO_Y;
   }

   if (oldDRO_Z != DRO_Z)
   {
    lcd.drawString("Z: " + DRO_Z, 5, 105);
    oldDRO_Z = DRO_Z;
   }
   oldMoveMode = MoveMode;
  } // end update some coords.
} // end Refresh whole DRO


/*
void FlashColors()  // to see what all the colors look like.
{
    tft.fillScreen(TFT_NAVY);    
    delay(ColorFlash_ms);  
    tft.fillScreen(TFT_DARKGREEN);
    delay(ColorFlash_ms); 
    tft.fillScreen(TFT_DARKCYAN);  
    tft.fillScreen(TFT_MAROON    );
    delay(ColorFlash_ms);
    tft.fillScreen(TFT_PURPLE );
    delay(ColorFlash_ms);    
    tft.fillScreen(TFT_OLIVE     );
    delay(ColorFlash_ms); 
    tft.fillScreen(TFT_LIGHTGREY  );
    delay(ColorFlash_ms);
    tft.fillScreen(TFT_DARKGREY );
    delay(ColorFlash_ms);  
    tft.fillScreen(TFT_BLUE    );
    delay(ColorFlash_ms);   
    tft.fillScreen(TFT_GREEN );
    delay(ColorFlash_ms);     
    tft.fillScreen(TFT_CYAN  );
    delay(ColorFlash_ms);     
    tft.fillScreen(TFT_RED   );
    delay(ColorFlash_ms);     
    tft.fillScreen(TFT_MAGENTA  );
    delay(ColorFlash_ms);  
    tft.fillScreen(TFT_YELLOW );
    delay(ColorFlash_ms);    
    tft.fillScreen(TFT_WHITE   );
    delay(ColorFlash_ms);   
    tft.fillScreen(TFT_ORANGE     );
    delay(ColorFlash_ms);
    tft.fillScreen(TFT_GREENYELLOW );
    delay(ColorFlash_ms);
    tft.fillScreen(TFT_PINK   );
    delay(ColorFlash_ms);         
    tft.fillScreen(TFT_BROWN  );
    delay(ColorFlash_ms);    
    tft.fillScreen(TFT_GOLD    );
    delay(ColorFlash_ms);   
    tft.fillScreen(TFT_SILVER      );
    delay(ColorFlash_ms);
    tft.fillScreen(TFT_SKYBLUE    );
    delay(ColorFlash_ms);
    tft.fillScreen(TFT_BLACK);  
    delay(ColorFlash_ms);
    tft.fillScreen(TFT_VIOLET );
 
}*/
