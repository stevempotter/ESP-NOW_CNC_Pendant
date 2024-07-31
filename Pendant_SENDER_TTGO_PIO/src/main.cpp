// ESP-NOW pendant for manually controlling a CNC machine
// by Steve M. Potter, PhD   steve@steveMpotter.tech  If you use this code, let me know!
// Wireless remote controller for my MPCNC Portable Primo CNC machine.
// Uses ESP-NOW for sending pendant data to CNC, including ROE, joystick, and switch states. 
// Displays current coordinates and other messages on a small color LCD on an ESP32 board.
// This talks wirelessly to a "receiver" ESP32 board in the CNC controller that talks to the Teensy via a UART line.
// ESP-NOW has very low latency and virtually no overhead compared to Wifi of Bluetooth.
// My MPCNC is running grblHAL on a Teensy 4.1 mpu, with Phil Bennett's breakout board.
// I use ioSender as the main interface for running jobs. This pendant uses the MPG mode flag to take over control of the 
// CNC machine from ioSender.

// Last update: See Setup.

// I am using ESP32 TTGO LilyGO V1.1 board for the SENDER. 9 euros each from tinytronics.nl
// LilyGO advises to select board "ESP32 Dev Module" in the Arduino IDE Tools menu.
//// MAX LENGTH OF ESP-NOW PACKET: 250 BYTES+NULL TERMINATOR.
// I am using two Adafruit SEESAW boards to give me a bunch of GPIOs and ADCs.
// https://www.adafruit.com/product/5233
// The pendant also has a LiPo charging PCB, designed for a single 18650 LiIon cell.
// The power from the LiIon cell is boosted to 5V with an Adafruit Verter buck/boost board.
// https://www.adafruit.com/product/2190
// There is a small speaker for some beeps and clicks. 

// TO DOs:
// Add auto-pairing: https://randomnerdtutorials.com/esp-now-auto-pairing-esp32-esp8266/
// see also: https://youtu.be/Ydi0M3Xd_vs?t=1342
// Add Move-by-Increments buttons, perhaps using the rotary encoder menu.
// Add Move-to-Specified-Coordinates to MenuEncoder system

///////////////////// INCLUDES /////////////////////////////////////////
#include <Arduino.h> // needed when compiling with Platform IO in VS Code.
#include <ESP32Encoder.h>  //Rotary encoder library for ESP32.  https://github.com/madhephaestus/ESP32Encoder
//#include <esp_now.h>
#include <ESPNowW.h> // Wrapper for ESPNOW
#include <WiFi.h>
#include "Adafruit_seesaw.h" // Breakout board used for port expander and ADC:
                             // https://learn.adafruit.com/adafruit-attiny817-seesaw/analog-in
#include <SPI.h>            // for comm with the TFT LCD
#include <TFT_eSPI.h>       // See FONTS.h tab for important details about the TFT screen settings.
TFT_eSPI lcd = TFT_eSPI(); // create an instance of class TFT_eSPI called lcd for screen writing.
                          // TTGO LilyGO V1.1 screen is 240w X 135h
TFT_eSprite BannerSprite = TFT_eSprite(&lcd);  // Create a Sprite object called BannerSprite

// Alternative faster screen drawing library. Loads in under half a second.
// Ran into some bugs so I went back to the good old TFT_eSPI library.
// lovyanGFX library faster than TFT_eSPI https://github.com/lovyan03/LovyanGFX
//#define LGFX_TTGO_TDISPLAY         // TTGO T-Display
//#define LGFX_USE_V1
//#include <LovyanGFX.hpp>
//#include <LGFX_AUTODETECT.hpp>
//static LGFX lcd;                 // create an instance of LGFX called lcd
//static LGFX_Sprite LongMsgSprite; // In case I want to use sprites (moving graphics and scrolling text)
//static LGFX_Sprite sprite(&lcd); 

/////////////////////////  CONSTANTS: pins & settings  /////////////////////////////////

/////////////  PINS  /////////////////////////////////////// 

#define TX_LED                  2  // Yellow TX packet LED
#define TFT_BACKLIGHT           4 // LCD backlight control pin
#define RX_LED                  12 // Blue RX packet LED
#define JOYSTICK_NOT_ENABLE_PIN 13 // Enabled when low.
#define ROE_A                   15 // Black wire
#define ClickPin                17 // To hear or see ROE pulses.
#define Z_ROE_NOT_EN            25 // Enabled when low.
#define SEESAW_INTRUPT_IN       26 // Goes low for 10ms on any transition of watched pins on the 0x49 SEESAW board.
#define ROE_B                   27 // White wire
#define X_ROE_NOT_EN            32 // Enabled when low.
#define Y_ROE_NOT_EN            33 // Enabled when low.
#define MENU_CLK_PIN            36 // menu encoder CLK Bn
#define MENU_DT_PIN             37 // menu encoder DT (orange) 

// NOTE: On LilyGO ESP32, pins 34-39 do not have software pullup or pulldown. 
// I removed C49 and C32 (270pF)on the ESP32 board so that there is no connection between 36 and 37, 38 and 39.

// The following pins use the Adafruit SEESAW I2C board with their ATtiny817 firmware.
// ADC capable pins on the SEESAW are: 0, 1, 2, 3, 6, 7, 18, 19, 20 - There are nine 10-bit ADC pins.
// GPIO pins: 0, 1, 2, 3, 5, 6, 7, 8, 9, 12, 13, 14, 18, 19, 20 - These are the 15 GPIO pins available. 
// Any one of these can control a 60-LED Neopixel array.

// 1st SEESAW I2C address 0x49 -- All inputs.  Address pins 16, 17 left unconnected for default addr. 0x49

#define S49_ADC_PIN_SPEED   1
#define S49_ADC_PIN_JOY_X   2
#define S49_ADC_PIN_JOY_Y   3
// Set these all to INPUT_PULLUP. Inverse logic: button pressed causes high to low:
// If any of these are changed, be sure to alter the pin mask (near line 220.)
#define S49_PIN_MENU_BUTTON 5  // yellow. Goes to GND when pressed (N. O.)
#define S49_PIN_VAC         6  
#define S49_PIN_SETXYTO_00  7
#define S49_PIN_CAL_JOYSTICK       8 // Do the calibration routine for the joystick.
#define S49_PIN_PROBE_Z     9
#define S49_PIN_GOTO_00     12  
#define S49_PIN_HOME_XYZ    13
#define S49_PIN_SETZTO_0    14
#define S49_PIN_START       18
#define S49_PIN_PAUSE       19
#define S49_PIN_NOT_RESET   20 // INPUT  yellow. Active LOW (Inverse logic)       

// Reserved with current SEESAW firmware -- DO NOT assign to anything else:
#define S49_PIN_INTRUPT_OUT 15  // Used to indicate a GPIO event has occured to the ESP32. 
#define S49_PIN_I2CADDR_0   16  // The default address is 0x49. Ground this to increment by 1
#define S49_PIN_I2CADDR_1   17  // Ground this to increment the address by 2. Ground both to increment by 3.

// 2nd SEESAW 0x4A -- Outputs & More Inputs
// Grounded pin 16 to increment address by 1 to 0x4A. 17 is floating.
#define S4A_MPGMODE_LED        0  // orange: MPG:1 if on.
#define S4A_SPINDLE_START_PIN  3  // Goes HIGH when button is latched/depressed.
#define S4A_LIGHT_PIN          5  // This pin is not being used yet
#define S4A_BATT_V_ADC_PIN     7  // Analog input. grey. LiPo voltage is at middle of a 10k + 10k voltage divider
#define S4A_BATT_LOW_LED       8  // purple
#define S4A_INCR_PIN           9  // To increment something up by one unit when button is pressed. Used for jogging by a set distance.
#define S4A_DECR_PIN           12 // To decrement something by one unit when the button is pressed 
#define S4A_PARK_PIN           13 // To park the spindle out of the way.
/*
To use more than one SEESAW board, use a different I2C address for each and
#define SEESAW_BASE_ADDR   0x49  // I2C address, starts with 0x49
  void setI2CAddr(uint8_t addr);
  uint8_t getI2CAddr();

One Adafruit forum admin said, "Just create one Seesaw control object for each device you want to use, 
and give each object the I2C address of the board it should talk to."
*/

//////////////  SETTINGS and CONSTANTS /////////////
#define Number_of_bits 9 // 9 to 12 bits resolution on the ESP32 ADC. I don't use these any more. I use the 10-bit ADC on the SEESAW instead.
#define MSG_SIZE 195 // Max bytes for the message string variable in ESPNOW packet struct. All the numbers are about 50 bytes.
const int JoystickSampleInterval_ms = 15; //Determines the max frequency that the joystick is sampled when enabled.
const int ROEintegTime_us = 5000;  // microsec to integrate ROE pulse counts
const int BlinkFast_us = 500; // used for TX-RX LED
const int ColorFlash_ms = 500;

long ClickDelay_us = 50; // 50us Change this and value of series cap to adjust volume of clicks.

///////////////////  GLOBAL VARIABLE INITIALIZATIONS  ///////////////////////////////////////
// I did not get the memo that one should not be using globals....

Adafruit_seesaw S49; // Creates an instance of the SEESAW I2C breakout board called S49 for inputs
Adafruit_seesaw S4A;  // And one for outputs.
uint32_t SeesawDigInPinMask;
uint32_t PrevButtons; 

ESP32Encoder ROE; // Setup a RotaryEncoder instance called ROE, using the ESP32's hardware counters, not interrupts.
ESP32Encoder MenuEncoder; // for the mechanical encoder. I added 0.47uF caps to debounce CLK and DT lines.
long StartupTime_ms = millis(); // to keep track of how long the pendant has been on.
long LastTime_us = micros();  // Used to measure delays in the code. See TimeElapsed(whereAt) function below.
long LastRefresh_ms = 0; // makes first DRO update happen right away.
long LastSpeedknobRead_ms = 0;
long LastCalJoyRead_ms = 0;
long LastBattCheck_ms = millis();
long LastPacketRead_ms = millis(); // Keep track of time between reading packets.
long LastPacketSent_ms = -1;
long LastPacketReceived_ms = millis(); //Keep track of when packets are received.
int HelloPacketInterval_ms = 500; // Pendant should be sending packets at least this often to keep control of CNC machine.
long NewPacketReceived_ms = millis();
bool NewPacketReceived = false; // To decide whether to process message from Teensy.
bool PacketDelivered = true;  //  allows sending of the first packet.
long LastROEtime_us = micros(); // last time the ROE was read.
bool ROEenabled = false;
long ThisTime_us = micros(); // Used by TimeElapsed() to clock duration of things.
long CurrentCount; // ROE pulse count.
long PrevCount = 0; // Used for ROE pulse counting.
uint16_t PrevROEmultiplier = 1; // for previous speed setting.
int DeltaT_us; // used in TimeElapsed function.
String DRO_X = "..."; // coordinates to display on the LCD
String oldDRO_X = "."; // for detecting changes.
String DRO_Y = "...";
String oldDRO_Y = ".";
String DRO_Z = "...";
String oldDRO_Z = ".";
long LastDROupdate_ms = millis();
String MoveMode = "NoCon"; // Idle, Jog, Hold, Door, Home
String oldMoveMode = "";
bool MPGstatus; // 1 is MPG:1 Pendant is in control, 0 is MPG:0 ioSender is in control 
bool PrevMPGstatus;
bool PrevReset = HIGH;
bool PrevROEenabled = false;
bool PrevJOYenabled = false;
bool PrevSpindle_ONstate = 0;
bool Error = false; // display error sent by Teensy.

// Set up ESP-NOW:
//uint8_t BroadcastMACaddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // This is for broadcasting to any listeners.
//uint8_t PeerMACaddress[] = {0xAC, 0x67, 0xB2, 0x2A, 0xED, 0x78}; // LilyGO Board 2 MAC address
//uint8_t PeerMACaddress[] = {0x7C, 0xDF, 0xA1, 0x4E, 0xB6, 0x96};// ESP32-S2 ThingPlus MAC address (Receiver peer)
//uint8_t PeerMACaddress[] = {0xAC, 0x67, 0xB2, 0x2A, 0xDA, 0xB8}; // LilyGO Board MAC address
//uint8_t PeerMACaddress[] = {0x4C, 0x11, 0xAE, 0xA5, 0xF2, 0x98}; // LilyGO Board 1 MAC address
uint8_t PeerMACaddress[] = {0x3C, 0x71, 0xBF, 0x48, 0x16, 0x88}; // DOIT Devkit v.1 ESP32 is my latest "receiver"
// TO DO *** :Scan for all the available ESP-NOW MAC addresses: https://youtu.be/Ydi0M3Xd_vs?t=1342
char MACstr[18];
esp_now_peer_info_t peerInfo; // This has to be up here to make it global.

//ppppppppppppppppppp        Packet Structure to send via ESP-NOW     pppppppppppppppppppppppppp
// Must match the receiver structure exactly.

typedef struct packet_struct 
{  // Struct of data sent between Pendant and Receiver. 
   // Max 250 bytes/transmission. Max rate is about 5ms per packet, round trip. 
   // That's about 200 Hz * 250bytes = 50kB/s bandwidth for peer-peer ESP-NOW, which has confirmations. (Broadcast ESPNOW does not)
 
  // ROE
  bool X_ROEenabled = false; //Momentary button held down during jogging with ROE.
  bool Y_ROEenabled = false;
  bool Z_ROEenabled = false;
  int ROE_RotDir = 0;  // 1 = CW = positive change, 0= no movement; -1 = CCW = neg.
  int ROE_DeltaCounts = 0;
  uint16_t ROEmultiplier = 1; 
  
  // Joystick stuff
  bool JOYenabled = false; // True means the joystick enable button is being held down.
  int JoyX = 0; //Values range from -1000 to 1000
  int JoyY = 0; //Values range from -1000 to 1000
  bool MPGmode; // indicates the !state of the pin 12 on Receiver, Pin 41 on Teensy.
 
  // Buttons pulled strongly to ground when pressed; Signals flipped upon reading in Pendant so 1 == ON
  bool IncrementButton = 0;
  bool DecrementButton = 0;
  bool MenuButton = 0; // If I get tight for bytes in the ESP-NOW packet, I could send these as two or four bytes.
  int  MenuClicksCount = 0; // mechanical encoder with detents for navigating menus.
  bool Vacuum = 0;     // bool uses one byte each.
  bool SetXYto0 = 0;  
  bool Light = 0;
  bool ProbeZ = 0;
  bool GoTo00 = 0;
  bool HomeXYZ = 0;  
  bool SetZto0 = 0;
  bool Start = 0;
  bool Pause_Halt = 0;
  bool Reset = 0; // Does not use inverse logic.
  bool Spindle_ON = 0; // Not inverse logic. High when latched/pressed,
                       // because that made the switch's LED easy to light when pressed.
                       // Signal is on the N.C. line to ground, so pulled up weakly when pressed.
  bool Park = 0; //to park the spindle out of the way
  char MessageText[MSG_SIZE]; // Brings us very close to the 250 byte limit.
                              // Regardless of how many chars are in the message, it sends the full size of the array.
} packet_struct;


packet_struct ESPNOWpacket2send; // Create a structure variable of type packet_struct called ESPNOWpacket
packet_struct ReceivedESPNOWpacket; // Info sent to Pendant by Receiver, such as .MPGmode and .MessageText
packet_struct SENTpacket = ESPNOWpacket2send; // To detect changes.

char EmptyCharArray[MSG_SIZE];

////////////////////////  end PACKET STRUCT  //////////////////////////////////////


// More joystick stuff

long StartRead;
int ConversionTime;

    // Joystick variables:
int JoyX_ADC; 
int JoyY_ADC;
int PrevJoyXspeed = 0;
int PrevJoyYspeed = 0;
long PrevJOYsample_ms = millis();
bool CalibrateJOY = false;
int Xc; // values when stick relaxed at center      
int Yc;
int Xmax; // Pushed to left extreme
int Xmin;  // pushed to right extreme
int Ymax; // Pushed to top
int Ymin;  // pushed to bottom
float Xspan; // Total span of X range.
float Yspan; // Has to be a float to avoid integer truncation.
float oldXspan;
float oldYspan;
bool JOYcalibrated = false; // Best to calibrate before using joystick. ESP32 ADC has bad noise and drift. Use the SEESAW ADC.
bool FirstDROloop = true; // To move init of TFT display into DRO refresh. It takes a while.

//ffffffffffffffffffffff    FUNCTIONS    ffffffffffffffffffffffffffffffffffffffffffffffffffffffff
// These were tabs in the Arduino IDE, which this project outgrew.
// The ordering of this list matters.

long TimeElapsed(String WhereAt); // forward declaration
#include "Speaker.h"
#include "power.h"
#include "DRO.h"
#include "FONTS.h"
#include "SendPacket.h"
#include "JoyStick.h"
#include "MenuEncoder.h"
#include "ProcessTeensyMsg.h"
#include "SelectorSwitches.h"
#include "ROE.h"
#include "Buttons.h"


//ccccccccccccccccccccccccccc     ESP-NOW CALLBACKS  cccccccccccccccccccccccccccccccccccccccccccccccccccc

//////// callback when ESP-NOW PACKET SENT ////////////////////////////////////
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) 
{
  if (status == !ESP_NOW_SEND_SUCCESS) // == 0
  {
   PacketDelivered = false; // not received.
   Serial.print(" Packet sending failed. Status = ");
   Serial.print(status); // status of 1 is a fail
   snprintf(MACstr, sizeof(MACstr), "%02x:%02x:%02x:%02x:%02x:%02x",
          mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
   Serial.print(" Attempted Packet send to: "); 
   Serial.println(MACstr);
   MoveMode = "NoCon";
   ProcessError(" No connection.");
  }
  else // success sending a packet, status == 0
  {
   PacketDelivered = true;   // Received by Receiver.
   SENTpacket = ESPNOWpacket2send;
   //Serial.print(" ms since last packet SENT (includes confirmation time of 4ms): ");
   //Serial.println(millis() - LastPacketSent_ms);
  }
  
}
////////////////////////////// RECEIVE PACKET ////////////////////////////
// callback function that will be executed when ESP-NOW PACKET IS RECEIVED
// This can be used to show CNC current coordinates on pendant display.
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int PacketLength) 
{  
  LastPacketReceived_ms = NewPacketReceived_ms;
  NewPacketReceived_ms = millis();
  memcpy(&ReceivedESPNOWpacket, incomingData, PacketLength);
  ESPNOWpacket2send.MPGmode = ReceivedESPNOWpacket.MPGmode; // reflect MPGmode back to the Receiver.
  NewPacketReceived = true;
  //Serial.print(PacketLength);
  //Serial.println(" bytes in new packet.");
  //Serial.print(" Packet received at ");
  //Serial.print(NewPacketReceived_ms);
 // Serial.print(" ms since last packet RECEIVED: ");
 // Serial.println(NewPacketReceived_ms - LastPacketReceived_ms);
}

 //ssssssssssssssssssssss    SETUP    sssssssssssssssssssssssssssssssssssssssssssssssssss
void setup() 
{
  Serial.begin(2000000); // Set the Arduino Serial Monitor to the fastest ESP32 will go.
  while (!(Serial))
  {
    // Wait for serial port to be ready.
    if ((micros() - LastTime_us) > 1000000) break;
  }

  Serial.print("us to begin Serial():");
  Serial.println(micros() - LastTime_us);
  LastTime_us = micros();
  Serial.println(" Welcome to the ESPNOW CNC Pendant Sender by Steve M. Potter. Last update: 4 Jan 2023 ");

////  TFT LCD stuff  /////////

 lcd.init();
 BannerSprite.setColorDepth(8);
 lcd.setRotation(1); // 1 landscape, 3 for USB at left. 1 is USB at right
 lcd.setFreeFont(FSSB18);  // FreeSansBold18
 BannerSprite.setFreeFont(FSSB18);
 lcd.fillScreen(0x915C); // violet
 lcd.setTextColor(TFT_YELLOW, 0x915C); // Yellow on violet
 Serial.print("us to init lcd:");
 Serial.println(micros() - LastTime_us);
 LastTime_us = micros();
  for (int i = 0; i < MSG_SIZE; i++)  
  {
    EmptyCharArray[i] = 0x00;
  } 
  
  /* Set up Adafruit SEESAW ATtiny817 port expander boards.
   * bool Adafruit_seesaw::begin ( uint8_t   addr = SEESAW_ADDRESS, int8_t  flow = -1, bool  reset = true ) 
   * #define SEESAW_ADDRESS (0x49) ///< Default Seesaw I2C address
   * I don't know what flow is.
   */
  if(!S4A.begin(0x4A)) // initialize the SEESAW boards comm on I2C port
  {                     // base address plus one (ground its pin 16)
    Serial.println(F("SEESAW 0x4A not found!"));
    while(1) delay(10);
  } 
  if(!S49.begin(0x49)) // default base address
  {  
    Serial.println(F("SEESAW 0x49 not found!"));
    while(1) delay(10);
  }
  Wire.setClock(100000); // 400000 worked but may have caused some problems.
  Serial.print("I2C (SEESAW) speed:");
  Serial.println(Wire.getClock());
  S4A.pinMode(S49_PIN_NOT_RESET, INPUT_PULLUP); // Reset button.
  S4A.pinMode(S4A_MPGMODE_LED, OUTPUT);
  S4A.digitalWrite(S4A_MPGMODE_LED, HIGH);
  delay(5);
  S4A.digitalWrite(S4A_MPGMODE_LED, LOW);
  S4A.pinMode(S4A_BATT_LOW_LED, OUTPUT);
  S4A.digitalWrite(S4A_BATT_LOW_LED, HIGH);
  delay(5);
  S4A.digitalWrite(S4A_BATT_LOW_LED, LOW);
  S4A.pinMode(S4A_LIGHT_PIN, INPUT_PULLUP); // hold down to calibrate joystick. See Serial Monitor for instructions.
  S4A.pinMode(S4A_SPINDLE_START_PIN, INPUT_PULLUP); // Goes HIGH when latched/pressed! (and green LED in switch comes on)
  S4A.pinMode(S4A_INCR_PIN, INPUT_PULLUP);
  S4A.pinMode(S4A_DECR_PIN, INPUT_PULLUP);
  S4A.pinMode(S4A_PARK_PIN, INPUT_PULLUP);

  pinMode(SEESAW_INTRUPT_IN, INPUT_PULLUP); // The pin on the ESP32 that is connected to pin 15 on S49 SEESAW. Goes low 10ms when a SEESAW digital pin changes state.
                                            // May wish to set up an interrupt on the ESP32 for this.
  // *** TO DO: convert this to bit shifting code to create the mask automatically, in case pin assignments change.
  SeesawDigInPinMask =     ((const uint32_t)0b00000000000111000111001111100000) ; // mask to select which SEESAW pins to monitor. All set to INPUT_PULLUP
           //                                210987654321098765432109876543210  // pin number
           //                                |           |         |         |
           //                                32          20        10        0
  S49.pinModeBulk(SeesawDigInPinMask, INPUT_PULLUP);  // The switches get sent to ground when pressed.
  PrevButtons = S49.digitalReadBulk(SeesawDigInPinMask); // 
  Serial.print(" uint32_t of all the button states: ");
  Serial.println(PrevButtons, BIN); 
  S49.setGPIOInterrupts(SeesawDigInPinMask, 1); // void setGPIOInterrupts(uint32_t pins, bool enabled);
  // Another useful bulk function: uint32_t digitalReadBulk(uint32_t pins); e.g. Serial.println(S49.digitalReadBulk(mask), BIN);
  Serial.println(F("SEESAW boards started OK. "));
 
  pinMode(JOYSTICK_NOT_ENABLE_PIN, INPUT_PULLUP); // When grounded, enables the reading of the joystick. 
  pinMode(X_ROE_NOT_EN, INPUT_PULLUP); // When grounded, enables ROE to jog in X
  pinMode(Y_ROE_NOT_EN, INPUT_PULLUP); // When grounded, enables ROE to jog in Y
  pinMode(Z_ROE_NOT_EN, INPUT_PULLUP); // When grounded, enables ROE to jog in Z
  pinMode(RX_LED, OUTPUT);// Packet Received. These blink often so having them on SEESAW slowed things down too much.
  pinMode(TX_LED, OUTPUT); // Packet Transmitted.
  pinMode(ClickPin, OUTPUT); // this is used to make a click sound on a speaker in the pendant.
  digitalWrite(ClickPin, LOW);
  Serial.print("us to set up pins: ");
  Serial.println(micros() - LastTime_us);
  LastTime_us = micros();
// Blink the LEDs used to indicate TX & RX with teensy.
  digitalWrite(RX_LED, HIGH);
  delay(5);
  digitalWrite(RX_LED, LOW); 
  digitalWrite(TX_LED, HIGH);
  delay(5);
  digitalWrite(TX_LED, LOW);
  Serial.print(" Blinking RX and TX LEDs took (us):");
  Serial.println(micros()- LastTime_us);
  LastTime_us = micros();
  // set up encoders:
  ESP32Encoder::useInternalWeakPullResistors = UP; // No need to use external pullups.
  ROE.attachFullQuad(ROE_B, ROE_A); // (B=White, A=Black)   // Putting B pin then A pin (for rising signals) gave me CW = Positive counts, CCW = neg.
  //ROE inputs pulled up to 3.3V
  ROE.setCount(0); // I beleive the counters use 64 bits, so they will not roll over in my lifetime.
  MenuEncoder.attachSingleEdge(MENU_DT_PIN, MENU_CLK_PIN); // SingleEdge increments/decrements once per click reliably.
  MenuEncoder.setCount(0);
  Serial.print(" Setting up the two encoders took (us):");
  Serial.println(micros()- LastTime_us);
  LastTime_us = micros();
  // Init ESP-NOW
  WiFi.mode(WIFI_STA);  // Set device as a Wi-Fi Station, not Access Point. RNT shows how to do Wifi and ESP-NOW concurrently.
  // https://randomnerdtutorials.com/esp-now-auto-pairing-esp32-esp8266/
  Serial.print(" This board's STA MAC address: ");
  Serial.println(WiFi.macAddress());
  if (esp_now_init() != ESP_OK) 
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  WiFi.disconnect(); // to keep it focused on the ESP-NOW tasks at hand and save battery power.

  esp_now_register_send_cb(OnDataSent); // See these callbacks above.
  esp_now_register_recv_cb(OnDataRecv);
  
  // Register peer
  memcpy(peerInfo.peer_addr, PeerMACaddress, 6); // memcpy(dest, source, size in bytes)
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer - this is the Pairing process.
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;   
  }
  Serial.print(" ESP-NOW Wifi peer setup took (us):");
  Serial.println(micros()- LastTime_us);
  LastTime_us = micros();
  Serial.print(" This Pendant code runs on core "); // ESP32 has two equally fast processors: 0, 1
  Serial.println(xPortGetCoreID());
  String tempstr = ("Pendant switched on. " +  String(millis()) ); // Let the Receiver know Pendant is now active.
  int str_len = tempstr.length() +1; // added 1 for the \0 char.
  tempstr.toCharArray(ESPNOWpacket2send.MessageText, str_len);
  SendESPNOWpacket("Setup");
  Beep(); 
  Serial.println(" Finished setup! ");
  Serial.print(" Total Setup time took (ms):"); // This is taking about 0.9 sec. 560ms just for the TFT init. 
                                                // *** TO DO: speed things up by having a sleep mode and a small backup battery to keep all the vars in memory.
                                                
  Serial.println(millis()- StartupTime_ms);
  LastTime_us = micros(); // Reset the TimeElapsed clock.
 
} // end setup

//////////////////////////  MAIN LOOP  ///////////////////////////////////////////////////////////
void loop() 
{  
 LastTime_us = micros();
//Click();   // Useful to get an idea of how fast the main loop is cycling.
 ProcessTeensyStatusMsg(); // spontaneous message from Receiver from Teensy
 // TimeElapsed("ProcessTeensyStatusMsg");
 MenuActions();
 //TimeElapsed("MenuActions");
 if (ESPNOWpacket2send.JOYenabled) ReadJoystick();
 ReadButtons();
 //TimeElapsed("ReadButtons");
 if ((ROEenabled) || (PrevROEenabled)) ReadROE();
 //TimeElapsed("ReadROE");
 RefreshDRO(); // refresh screen as needed.
 //TimeElapsed("RefreshDRO");
 if ((millis() - LastPacketSent_ms) > HelloPacketInterval_ms)
 {
  SendESPNOWpacket("main loop Hello packet."); 
 }
 if (millis() - LastCalJoyRead_ms > 200) 
 {
  CalibrateJOY = !S49.digitalRead(S49_PIN_CAL_JOYSTICK); // Inverse logic on pin.
  if (CalibrateJOY) CalibrateJoystick(); 
  LastCalJoyRead_ms = millis();
 }
 //TimeElapsed("main loop");
} // end main loop
 
long TimeElapsed(String WhereAt)  // mostly for debugging purposes.
 {
  ThisTime_us = micros(); 
  DeltaT_us = ThisTime_us - LastTime_us; // Time since last call.
  Serial.print(" Time elapsed in ");
  Serial.print(WhereAt);
  Serial.print(":");
  Serial.print(DeltaT_us);
  Serial.println(" microSec");
  //printf(" Time elapsed in %s: %d microSec \n", WhereAt, DeltaT_us); // This can't display the string for some reason.
  LastTime_us = micros();
  return DeltaT_us;
 }  // end TimeElapsed
