
void Click()
{
  digitalWrite(CLICK_PIN, HIGH); // to make a click sound use 470uF cap in series with buzzer or speaker.
  delayMicroseconds(ClickDelay_us);
  digitalWrite(CLICK_PIN, LOW); // turn the pulse off.
}

void Beep() // probably there is already a built in function for this.
{
  int beeplength = 100;
  for (int i = 1; i <= beeplength; i++) 
  {
    Click();
    delayMicroseconds(1500); // to set the frequency of the beep, change this.
  }
}
