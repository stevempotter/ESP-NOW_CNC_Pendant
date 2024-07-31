// Read the mechanical encoder knob, which can be used to go through and select menu items.
// Don't forget to debounce! At least 3ms to settle.
// ***Look for handy libraries for encoder  menus 

// 5-cond ribbon
// Gn GND
// Y push switch signal (NO, grounded when pressed)
// o DT 
// R +3.3V
// Bn CLK

void MenuActions()  // The menu knob pushbutton is read in the Buttons.h function.
{
    int MenuClicks = MenuEncoder.getCount(); // Read the MenuEncoder pulse counter.    
    MenuEncoder.clearCount();
    ESPNOWpacket2send.MenuClicksCount = MenuClicks;
    if (ESPNOWpacket2send.MenuClicksCount != 0)
    {
     Serial.print(" Menu encoder value: ");
     Serial.println(MenuClicks);
     SendESPNOWpacket("Menu");
    }
}