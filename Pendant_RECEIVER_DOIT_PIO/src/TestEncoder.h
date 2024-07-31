/*******Interrupt-based Rotary Encoder Menu Sketch*******
 * by Simon Merrett, based on insight from Oleg Mazurov, Nick Gammon, rt and Steve Spence, and code from Nick Gammon
 * 3,638 bytes with debugging on UNO, 1,604 bytes without debugging
 */
// Rotary encoder declarations
static int pinA = 2; // Our first hardware interrupt pin is digital pin 2
static int pinB = 3; // Our second hardware interrupt pin is digital pin 3
volatile byte aFlag = 0; // let's us know when we're expecting a rising edge on pinA to signal that the encoder has arrived at a detent
volatile byte bFlag = 0; // let's us know when we're expecting a rising edge on pinB to signal that the encoder has arrived at a detent (opposite direction to when aFlag is set)
volatile byte encoderPos = 0; //this variable stores our current value of encoder position. Change to int or uin16_t instead of byte if you want to record a larger range than 0-255
volatile byte oldEncPos = 0; //stores the last encoder position value so we can compare to the current reading and see if it has changed (so we know when to print to the serial monitor)
volatile byte reading = 0; //somewhere to store the direct values we read from our interrupt pins before checking to see if we have moved a whole detent
// Button reading, including debounce without delay function declarations
const byte buttonPin = 4; // this is the Arduino pin we are connecting the push button to
byte oldButtonState = HIGH;  // assume switch open because of pull-up resistor
const unsigned long debounceTime = 10;  // milliseconds
unsigned long buttonPressTime;  // when the switch last changed state
boolean buttonPressed = 0; // a flag variable
// Menu and submenu/setting declarations
byte Mode = 0;   // This is which menu mode we are in at any given time (top level or one of the submenus)
const byte modeMax = 3; // This is the number of submenus/settings you want
byte setting1 = 0;  // a variable which holds the value we set 
byte setting2 = 0;  // a variable which holds the value we set 
byte setting3 = 0;  // a variable which holds the value we set 
/* Note: you may wish to change settingN etc to int, float or boolean to suit your application. 
 Remember to change "void setAdmin(byte name,*BYTE* setting)" to match and probably add some 
 "modeMax"-type overflow code in the "if(Mode == N && buttonPressed)" section*/

void setup() {
  //Rotary encoder section of setup
  pinMode(pinA, INPUT_PULLUP); // set pinA as an input, pulled HIGH to the logic voltage (5V or 3.3V for most cases)
  pinMode(pinB, INPUT_PULLUP); // set pinB as an input, pulled HIGH to the logic voltage (5V or 3.3V for most cases)
  attachInterrupt(0,PinA,RISING); // set an interrupt on PinA, looking for a rising edge signal and executing the "PinA" Interrupt Service Routine (below)
  attachInterrupt(1,PinB,RISING); // set an interrupt on PinB, looking for a rising edge signal and executing the "PinB" Interrupt Service Routine (below)
  // button section of setup
  pinMode (buttonPin, INPUT_PULLUP); // setup the button pin
  // DEBUGGING section of setup
  Serial.begin(9600);     // DEBUGGING: opens serial port, sets data rate to 9600 bps
}

void loop() {
  rotaryMenu();
  // carry out other loop code here 
}

void rotaryMenu() { //This handles the bulk of the menu functions without needing to install/include/compile a menu library
  //DEBUGGING: Rotary encoder update display if turned
  if(oldEncPos != encoderPos) { // DEBUGGING
    Serial.println(encoderPos);// DEBUGGING. Sometimes the serial monitor may show a value just outside modeMax due to this function. The menu shouldn't be affected.
    oldEncPos = encoderPos;// DEBUGGING
  }// DEBUGGING
  // Button reading with non-delay() debounce - thank you Nick Gammon!
  byte buttonState = digitalRead (buttonPin); 
  if (buttonState != oldButtonState){
    if (millis () - buttonPressTime >= debounceTime){ // debounce
      buttonPressTime = millis ();  // when we closed the switch 
      oldButtonState =  buttonState;  // remember for next time 
      if (buttonState == LOW){
        Serial.println ("Button closed"); // DEBUGGING: print that button has been closed
        buttonPressed = 1;
      }
      else {
        Serial.println ("Button opened"); // DEBUGGING: print that button has been opened
        buttonPressed = 0;  
      }  
    }  // end if debounce time up
  } // end of state change

  //Main menu section
  if (Mode == 0) {
    if (encoderPos > (modeMax+10)) encoderPos = modeMax; // check we haven't gone out of bounds below 0 and correct if we have
    else if (encoderPos > modeMax) encoderPos = 0; // check we haven't gone out of bounds above modeMax and correct if we have
    if (buttonPressed){ 
      Mode = encoderPos; // set the Mode to the current value of input if button has been pressed
      Serial.print("Mode selected: "); //DEBUGGING: print which mode has been selected
      Serial.println(Mode); //DEBUGGING: print which mode has been selected
      buttonPressed = 0; // reset the button status so one press results in one action
      if (Mode == 1) {
        Serial.println("Mode 1"); //DEBUGGING: print which mode has been selected
        encoderPos = setting1; // start adjusting Vout from last set point
      }
      if (Mode == 2) {
        Serial.println("Mode 2"); //DEBUGGING: print which mode has been selected
        encoderPos = setting2; // start adjusting Imax from last set point
      }
      if (Mode == 3) {
        Serial.println("Mode 3"); //DEBUGGING: print which mode has been selected
        encoderPos = setting3; // start adjusting Vmin from last set point
      }
    }
  }
  if (Mode == 1 && buttonPressed) {
    setting1 = encoderPos; // record whatever value your encoder has been turned to, to setting 3
    setAdmin(1,setting1);
    //code to do other things with setting1 here, perhaps update display  
  }
  if (Mode == 2 && buttonPressed) {
    setting2 = encoderPos; // record whatever value your encoder has been turned to, to setting 2
    setAdmin(2,setting2);
    //code to do other things with setting2 here, perhaps update display   
  }
  if (Mode == 3 && buttonPressed){
    setting3 = encoderPos; // record whatever value your encoder has been turned to, to setting 3
    setAdmin(3,setting3);
    //code to do other things with setting3 here, perhaps update display 
  }
} 

// Carry out common activities each time a setting is changed
void setAdmin(byte name, byte setting){
  Serial.print("Setting "); //DEBUGGING
  Serial.print(name); //DEBUGGING
  Serial.print(" = "); //DEBUGGING
  Serial.println(setting);//DEBUGGING
  encoderPos = 0; // reorientate the menu index - optional as we have overflow check code elsewhere
  buttonPressed = 0; // reset the button status so one press results in one action
  Mode = 0; // go back to top level of menu, now that we've set values
  Serial.println("Main Menu"); //DEBUGGING
}

//Rotary encoder interrupt service routine for one encoder pin
void PinA(){
  cli(); //stop interrupts happening before we read pin values
  reading = PIND & 0xC; // read all eight pin values then strip away all but pinA and pinB's values
  if(reading == B00001100 && aFlag) { //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
    encoderPos --; //decrement the encoder's position count
    bFlag = 0; //reset flags for the next turn
    aFlag = 0; //reset flags for the next turn
  }
  else if (reading == B00000100) bFlag = 1; //signal that we're expecting pinB to signal the transition to detent from free rotation
  sei(); //restart interrupts
}

//Rotary encoder interrupt service routine for the other encoder pin
void PinB(){
  cli(); //stop interrupts happening before we read pin values
  reading = PIND & 0xC; //read all eight pin values then strip away all but pinA and pinB's values
  if (reading == B00001100 && bFlag) { //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
    encoderPos ++; //increment the encoder's position count
    bFlag = 0; //reset flags for the next turn
    aFlag = 0; //reset flags for the next turn
  }
  else if (reading == B00001000) aFlag = 1; //signal that we're expecting pinA to signal the transition to detent from free rotation
  sei(); //restart interrupts
}
// end of sketch