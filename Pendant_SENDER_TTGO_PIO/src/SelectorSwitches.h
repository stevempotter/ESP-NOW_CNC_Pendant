// Read the state of the pendant's ROE speed selector switch.

// (I replaced the axis selector switch with 3 enable buttons.)

////////////////////// SPEED SELECTOR  ///////////////////////////////////

// 6 position multiplier switch (ROE jogwheel sensitivity)
// (p. 42 of Maker 9) 2k2 resitor ladder with 4k7 at the top and 820ohm at the bottom; 13k total. Six positions.
// Reading clockwise: 1x, 3x, 10x, 30x 100x, 500x.
// SEESAW ADC values: 737, 600, 462, 324, 187, 49
// 3-cond ribbon
// Gy Signal
// Bk GND
// W +3.3V

void SpeedSelector()
{
 int SpeedSW_DU = S49.analogRead(S49_ADC_PIN_SPEED);
 //Serial.print("Speed selector DU: ");
 //Serial.println(SpeedSW_DU); // Digital Units output by selector switch.
 // Put a lookup table here to turn the DU into multipliers, as with the joystick.
 // Position     Selection    ADC_DU  Halfway 
 // 1            1x           737     betweens
 //                                      669
 // 2            3x           600 
 //                                      531
 // 3            10x          462
 //                                      393
 // 4            30x          324
 //                                      256
 // 5            100x         187
 //                                      118
 // 6            500x         49

 if (SpeedSW_DU > 669)                          ESPNOWpacket2send.ROEmultiplier = 1;
 if ((SpeedSW_DU <= 669) && (SpeedSW_DU > 531)) ESPNOWpacket2send.ROEmultiplier = 3;
 if ((SpeedSW_DU <= 531) && (SpeedSW_DU > 393)) ESPNOWpacket2send.ROEmultiplier = 10;
 if ((SpeedSW_DU <= 393) && (SpeedSW_DU > 256)) ESPNOWpacket2send.ROEmultiplier = 30;
 if ((SpeedSW_DU <= 256) && (SpeedSW_DU > 118)) ESPNOWpacket2send.ROEmultiplier = 100;
 if ((SpeedSW_DU <= 118) && (SpeedSW_DU > 0))   ESPNOWpacket2send.ROEmultiplier = 500; 
 //Serial.print(" ADC value: ");
 //Serial.print(SpeedSW_DU);
 //Serial.print(" ROE Multiplier: ");
 //Serial.print(ESPNOWpacket.ROEmultiplier);
 //Serial.print(" Prev: ");
 //Serial.println(PrevROEmultiplier);
 if (ESPNOWpacket2send.ROEmultiplier != PrevROEmultiplier)
 {
  PrevROEmultiplier = ESPNOWpacket2send.ROEmultiplier;
  SendESPNOWpacket("Speed selector");
 }
} // end SpeedSelector()
