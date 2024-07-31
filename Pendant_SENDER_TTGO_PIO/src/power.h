// For managing the battery. There is a LiPo 1S charger PCB (with 4056H IC) in the pendant, or I can just swap out the cell with a fresh one.
// The charger is powered by a microUSB input. It cuts off output when the battery voltage is below 2.6V and cuts in when it is 
// above 3V. The output is always the same voltage as the LiIon 18650 cell, which is charged up to 4.15V.
// The charger's output is boosted to 5V with the Adafruit VERTER PCB. That works down to 2V but the cell will never get that low.
// The voltage of the 18650 LiIon cell is measured at center of a voltage divider with two 10k resistors.
// I am using the ADC of the SEESAW device S4A because it is better than the ESP32 ADC.

// *** TO DO: Add some sleep modes to save battery if not being actively used. My circuit consumes about 0.3A so with a 
// 2000mAh LiIon cell, that should last about 6 hours of full on-time.

// *** TO DO: Implement light sleep code that would allow a rechargeable 3V coin cell to keep the variables in the ESP32's memory and
// keep the RTC running. This might allow faster startup.

void Checkbattery()
{
    if ((millis() - LastBattCheck_ms) < 5000) return;
    LastBattCheck_ms = millis();
    int Batt_ADC;
    float Vcal = 151.0; // conversion from 10-bit ADC to volts. It is not quite linear but this works for 2.7V.
    float BattV = 0.0;
    Batt_ADC = S4A.analogRead(S4A_BATT_V_ADC_PIN);
    Serial.print(" Battery ADC value=");
    Serial.print(Batt_ADC);
    BattV = float(Batt_ADC) / Vcal;
    Serial.print(" Voltage=");
    Serial.println(BattV);
    if (BattV < 1) return; // when Pendant is powered by USB plugged into ESP32.
    lcd.drawString("LiPo ", 155, 65 );
    lcd.drawFloat(BattV, 2, 155, 95 );
    if (BattV < 3.5) // LiPo is getting low and in danger of being damaged by overdischarge
    {        
        S4A.digitalWrite(S4A_BATT_LOW_LED, HIGH);
        Beep(); 
        Beep();
        Beep();
        Beep();
        Beep();
        Beep();
        Beep(); // Long beep
        S4A.digitalWrite(S4A_BATT_LOW_LED, LOW);
        lcd.fillScreen(0xFFE0); // Yellow
        lcd.setTextColor(0x780F, 0xFFE0); // Purple on Yellow
        String tmpstring = String(BattV);
        tmpstring.concat("V");
        lcd.drawString(tmpstring, 5, 20);
        if (BattV < 3)
        {
         lcd.drawString("CHANGE BATT", 5, 100);
        }      
        if ((BattV < 3.5) && (BattV >=3))
        {
         lcd.drawString("BATT LOW!", 5, 100);
        }
        delay(1000);
        lcd.setTextColor(TFT_YELLOW, 0x915C); // Yellow on violet
        lcd.fillScreen(0x915C);
    }

}  // end Checkbattery()