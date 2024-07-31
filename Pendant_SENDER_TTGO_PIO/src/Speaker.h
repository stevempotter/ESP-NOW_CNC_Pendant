// Speaker on the pendant to hear the ROE clicking and alarm beeps. 8ohm, 2W
// May want to use this for other alarms, beeps and announcements in the future.
// May want to add an audio amplifier, like I have in the main controller box. I have a spare.

// Bk to GND
// Gn ClickPin digital line. Put 47uF electrolytic in series. (or other value to get the right sound)


void Click()
{
  digitalWrite(ClickPin, HIGH); // to make a click sound use 470uF cap in series with buzzer or speaker.
  delayMicroseconds(ClickDelay_us);
  digitalWrite(ClickPin, LOW); // turn the pulse off.
}

void Beep() // probably there is already a built in function for this.
{
  int beeplength = 20;
  for (int i = 1; i <= beeplength; i++) 
  {
    Click();
    delayMicroseconds(1200); // to set the frequency of the beep, change this.
  }
}
