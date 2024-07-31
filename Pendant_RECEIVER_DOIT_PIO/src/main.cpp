
// Steve M. Potter steve@stevempotter.tech
// Last update: See setup.
// This is uses ESP-NOW for receiving data (Manual Pulse Generator, joystick, etc) from a CNC pendant.
// It relays commands to the Teensy 4.1 controller which is on Phil Barrett's breakout board, running grblHAL.
// ESP-NOW: I found with this and the Sender sketch that there is a 1.1 to 1.3ms latency of each transmission.
// Reboots happen in a fraction of a second, but once I got the code working, they have not happened since.
// MAX LENGTH OF ESP-NOW PACKET: 250 BYTES+NULL TERMINATOR.

/*  Many thanks to 
  Rui Santos
  Complete ESP-NOW example details at https://RandomNerdTutorials.com/esp-now-esp32-arduino-ide/
*/

#include <Arduino.h>
#include <esp_now.h> // C:\Users\USER\Dropbox Sync Folder\Dropbox\Arduino\esp-idf-v4.4\components\esp_wifi\include
#include <WiFi.h>


////////////////  CONSTANTS  ///////////////////////////

#define TeensyBaud 2000000
#define CLICK_PIN 21 // to make a click noise.
#define LED_PIN 2 // built in blue LED on RECEIVER ESP32.
#define MPG_NOT_ACTIVE_PIN  4 // Also built-in blue LED. Connects to pin 41 of Teensy.
#define SERIAL_SIZE_RX  1024    // For receiving long messages from Teensy.
#define MSG_SIZE 195  // Max bytes for the message string variable in ESPNOW packet struct.
#define  STATUS_POLL_INTERVAL 2000  // interval to ask Teensy for status report.
#define RX_WAIT_MS 0 // time in ms to wait for a reply from Teensy.
#define STATUS_REPORT_BITFIELD 3170 // 14 usually Used for the $10 Status Report Options grbl command.
long LastTime_us = micros();  // Used to measure delays in the code. See TimeElapsed() funtion.
// below is the MAC address of the other ESP32 board in the pendant, called SENDER 
uint8_t PeerMACaddress[] = {0xAC, 0x67, 0xB2, 0x2A, 0xED, 0x78}; // ESP32 LilyGO v1.1 MAC address Pendant "Sender"
esp_now_peer_info_t peerInfo; // create peerInfo as an instance of class esp_now_peer_info_t. Must be global.
const bool debugging = false; // turns on some println() statements.
const int ShowPacketInterval_ms = 200;
const int LongPress_ms = 1000; // ms to hold down a button for long presses.
const int MenuLongPress_ms = 1000; // for the mechanical encoder button
const int MPGmodeChangeDelay_ms = 2; // Used in ToggleMPGmodeTo()
const int JogletInterval_ms = 15; // Planner buffer holds 35 commands, or a bit more than 1 sec worth if 50 ms.
const long ClickDelay_us = 20; // to make a clicking noise on a small speaker, in lieu of a detent.
const float ProbePlateThickness_mm = 3.06; // Height above work surface that probe will stop when it hits the plate during Z probing.
                          // Change this and value of series cap (470uF) to adjust volume of clicks.
bool StatusReports = true; // toggle the periodic requesting of status reports from Teensy.
bool ShowPacketsFlag = false; // Change this to true if you want to see all the packet data in Serial Monitor as they arrive.


//PPPPPPPPPPPPPPPPPPPPPP    ESPNOW PACKET stuff    PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

typedef struct packet_struct {  // Struct of data sent between Pendant and Receiver. 
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
  bool MPGmode; // indicates the !state of the pin 4 on Receiver, Pin 41 on Teensy. 
 
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
  bool Park = 0; // to park the spindle out of the way
  char MessageText[MSG_SIZE]; // Brings us very close to the 250 byte limit.
                              // Regardless of how many chars are in the message, it sends the full size of the array.
} packet_struct;

packet_struct ESPNOWpacket;  // Create a structure variable of type packet_struct called ESPNOWpacket
packet_struct OLDpacket = ESPNOWpacket; // To detect changes.

int PacketLength = 0; // global for Length of incoming packet.

//PPPPPPPPPPPPPPPPPPPPPPPP      end ESPNOW PACKET STUFF     PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP


/////////////////////// more global VARIABLES  /////////////////////
bool Not_MPGmode; // This is for MPG_NOT_ACTIVE_PIN -> Teensy pin 41
                  // Confusingly, the mode switch line goes LOW to activate the pendant. MPG:1

String TeensyMsg = ""; // Info coming back from the Teensy.
String LastTeensyMsg = "";
String OldMsg = "";
long StartMicros = micros(); // to time things.
bool ROEchanged = false; // Rotary Optical Encoder variable(s) changed
bool MsgChanged = false;
bool PacketSent = false;
bool JogOKreply = true; // Used to know if Teensy has replied to a jog command with 'ok'
long LastJog_ms = millis();
bool StatusReply; // used to know if Teensy has just sent a status report.
long TeensyInterval_us = 0;
long ThisTime_us; // Used to mark NOW in TimeElapsed function.
long Now_us; // Used to mark a NOW elsewhere.
bool MPGstatus; // Indicates whether pendant or ioSender is in control. 1 is Pendant controlling.
int MaxPlannerBlocks = 12; // It is 35 by default in grblHAL on Teensy. That is too unresponsive.
                          // I want this to be about 200ms worth of joglets.
long DeltaT_us = 0;
byte RTcommand = 0; // Real-time command for Teensy. initialize as the null char.
String InputString = "";
long MainLoopTimer_ms;
String MoveMode = ""; // AKA Machine State: Idle, Run, Hold, Jog, Alarm, Door, Check, Home, Sleep
                      // Beware of testing for equality with constants like "Jog" which lack the ending chars a string has. Use str.startsWith() instead.
bool ROEenabled = false;
bool PrevROEenabled = false;
int PrevROE_RotDir = 0;
int ROE_Counts[3]; // Keep a list of the last three ROE counts to average.
int CountsRunningAvg = 0;
uint8_t PrevROEmultiplier = 0;
int StepMove_mm = 10; // Default step move increment in mm. *** To Do: allow changing in menu.
int StepSpeed = 3000; // feed rate of incremental stepping, in mm/min. Default: 50mm/sec
bool PrevJOYenabled = false;
bool PrevSpindle_ONstate = 0;
bool TeensyReady = true; // Ready to accept jog command.
bool PacketReceived = false; // True when new packet arrives; false when packet info gets used in ROE and Joystick.
int PacketChangeCount = 0; // Tally of items that were updated in the most recent packet.
long PacketReceivedTimeStamp_ms;
int HelloPacketInterval_ms = 600; // Pendant should be sending packets at least this often to keep control of CNC machine.
long LastPacketShown;
long PrevPacketReceivedTimeStamp_ms;
long ms_sinceLastPacket;
bool PendantOffline = false; // To take actions when pendant is switched off, goes to sleep, battery runs down, or goes out of range.
bool ResetPending = false;
bool HomeXYZpending = false;
bool ProbeZpending = false;
bool MenuLongPressPending = false;
long ResetButtonDown_millis; // Start timing a long press of dangerous buttons.
long HomeXYZbuttonDown_millis;
long ProbeZbuttonDown_millis;
long MenuButtonDown_millis;
bool Homed = false; // To keep track of whether the machine has been homed since power-on.

/////////////////////////  FUNCTIONS  ///////////////////////////////////////////////////////////
// Needed forward declarations
long TimeElapsed(String WhereAt);
void ReadTeensyMsg(bool WaitForReply, String CalledBy);
// Ordering matters here. Each one below must only call later ones.
#include "Speaker.h"
#include "UART.h"
#include "Packet.h"
#include "Debug.h"
#include "ReadTeensyMsg.h"
#include "Jogs.h"
#include "Buttons.h"
#include "Joystick.h"
#include "ROE.h"




////////////////////// callback when PACKET SENT to Pendant via ESP-NOW  ////////////////////////////

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) // to the PENDANT (not the Teensy.)
{

  if (status == !ESP_NOW_SEND_SUCCESS) // Not received by Pendant. Keep waiting for confirmation.
  {    
   Serial.println(" Packet sent to pendant, but delivery failed. ");      
  } // end unsuccessful packet sending.
  else   // Packet sent successfully.
  {
   PacketSent = false; // This allows the next packet to be sent.
   PendantOffline = false;
   //Serial.println(" SUCCESS! Packet sent to pendant ");
  }
  
}// end Packet Sent callback


////////////// callback function when PACKET RECEIVED from Pendant via ESP-NOW  /////////////

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int Length) 
{
  PrevPacketReceivedTimeStamp_ms = PacketReceivedTimeStamp_ms;
  OLDpacket = ESPNOWpacket; // Put the previous packet in OLDpacket just before adding new stuff.
  memcpy(&ESPNOWpacket, incomingData, Length); // Load up the Struct ESPNOWpacket with new pendant info.  
  PacketLength = Length;
  PacketReceived = true;
  PacketChangeCount = 0; 
  PacketReceivedTimeStamp_ms = millis();
  ms_sinceLastPacket = PacketReceivedTimeStamp_ms - PrevPacketReceivedTimeStamp_ms;
 // Serial.print( "               Packet received from pendant. Packet interval (ms): ");
 // Serial.println(PacketReceivedTimeStamp_ms - PrevPacketReceivedTimeStamp_ms);

  ROEenabled = (ESPNOWpacket.X_ROEenabled || ESPNOWpacket.Y_ROEenabled || ESPNOWpacket.Z_ROEenabled); // TRUE if any combination of the ROE_NOT_EN buttons is pushed.
}

/////////////////////////////   SETUP  ////////////////////////////////////////
 
void setup() {

  // Initialize two UARTs
  Serial.begin(500000);  // Port 0. Serial Monitor: as fast as ESP32 can go. For user manual input and message display.
  while (!(Serial))
  {
    // Wait for serial port to be ready.
    if ((micros() - LastTime_us) > 1000000) break;
  }
  Serial.print(micros() - LastTime_us);
  Serial.println(" Welcome to the ESPNOW CNC Pendant Receiver by Steve M. Potter. Last update: 19 Jan 2024 ");
  
  // Serial1.begin(TeensyBaud, SERIAL_8N1, 33, 32); // LilyGO Port 1. Used to communicate with the Teensy. Teensy grblHAL is set to 115200 baud by default. 2,000,000 works.
  //Serial2.begin(baud-rate, protocol, RX pin, TX pin);
  Serial2.begin(TeensyBaud, SERIAL_8N1, 16, 17); // GPIO pins 16, 17
 
  while (!(Serial2))
  {
    // Wait for serial port to be ready.
    if ((micros() - LastTime_us) > 1000000) break;
  }

  Serial.print(micros() - LastTime_us);
  Serial2.setRxBufferSize(SERIAL_SIZE_RX);
  Serial.print(" Serial2 TeensyRX buffer size:");
  Serial.println(SERIAL_SIZE_RX);
  
  Serial.println(" Enter a command to be sent to the Teensy...");
  Serial.println(" Enter 0x8B to manually toggle between MPG mode (MPG:1)and ioSender mode (MPG:0). ");
  pinMode(CLICK_PIN, OUTPUT);  // To make clicks and beeps.
  digitalWrite(CLICK_PIN, LOW);

  pinMode(MPG_NOT_ACTIVE_PIN, INPUT); // wait for this pin to go high, signalling that the Teensy is ready to receive MPG input.

  while (digitalRead(MPG_NOT_ACTIVE_PIN) == LOW) 
  {
    Serial.println("Waiting for Teensy MPG_NOT_ACTIVE_PIN to go HIGH...Is the Teensy POWERED UP and MPG_NOT_ACTIVE_PIN of ESP32 connected to pin 41 of Teensy?");
    Serial.println(" Also, the receiver ESP32 and Teensy must have their grounds connected. ");
    Beep();
    delay(300);
    Beep();
    delay(2000);
  }
  pinMode(LED_PIN, OUTPUT);
  pinMode(MPG_NOT_ACTIVE_PIN, OUTPUT); // The Teensy is ready now for us to switch modes.
  Not_MPGmode = HIGH;  // LOW = MPG:1
  digitalWrite(MPG_NOT_ACTIVE_PIN, HIGH); //Leave this HIGH by default and just switch modes when pendant is on.
  digitalWrite(LED_PIN, LOW); // Blue built-in LED will indicate when Pendant is in control and Pin 4 is LOW
  ESPNOWpacket.MPGmode = !Not_MPGmode; // double-negative
  // Init ESP-NOW
  WiFi.mode(WIFI_STA);  // Set device as a Wi-Fi Station
  Serial.print(" This board's STA MAC address: ");
  Serial.println(WiFi.macAddress());
  if (esp_now_init() != ESP_OK) {  // initialize ESP-NOW
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  WiFi.disconnect(); // to keep it focused on the ESP-NOW tasks at hand.
  // Once ESPNow is successfully Initialized, we will register a Send callback to
  // get the status of Transmitted packet
  esp_now_register_send_cb(OnDataSent);
  // And a Receive Callback to get received packet info
  esp_now_register_recv_cb(OnDataRecv);
  
  // Register peer
  memcpy(peerInfo.peer_addr, PeerMACaddress, 6);  // // memcpy(dest buf, source buf, size in bytes). Put the peer's address into the peer_addr variable.
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  ROE_Counts[0] = 0;
  ROE_Counts[1] = 0;
  ROE_Counts[2] = 0;
  String Greeting = " Hello from the Pendant Receiver! ";   
  Greeting.toCharArray(ESPNOWpacket.MessageText, Greeting.length());
  esp_err_t result = esp_now_send(PeerMACaddress, (uint8_t *) &ESPNOWpacket, sizeof(ESPNOWpacket)); // send ESP-NOW packet to Pendant.
  if (result != ESP_OK) Serial.println(" Error sending greeting packet to pendant. ");
  MainLoopTimer_ms = millis();
  LastPacketShown = millis();
  Serial.print(" This code runs on core "); // ESP32 has two equally fast processors: 0, 1
  Serial.print(xPortGetCoreID());
  Serial.print(" Finished setup at: ");
  Serial.println(MainLoopTimer_ms);
  Beep();

}  // end setup
 
void loop() 
{
  ShowPacket(); // in Packet.h. Turn on flag in main.cpp line 45 to show packet data for debugging. Hampers performance, so don't forget to switch it back off.
  Now_us = TimeElapsed("main loop"); 
    //PacketChanges(); 
  StartMicros = micros();
  ReadTeensyMsg(false, " main loop "); // read spontaneous message from Teensy, forward to Pendant
  ProcessManualCommand();
  if (!PendantOffline)
  {
   ProcessJoystick();
   ProcessButtons();
   ProcessROE();
  }
  
  if ( (ESPNOWpacket.JOYenabled == false) && (ROEenabled == false) ) // neither is enabled.
    {      
      if (MoveMode.startsWith("Jog")) HaltJogging();              
    }
  
  if (StatusReports)
  {
    if  ( (millis() - MainLoopTimer_ms ) > STATUS_POLL_INTERVAL)  
    {
     SendCommand(true, 0x87); // status report  
     //SendCommand(true, "$#\n"); // alternative way to learn whether the machine has been homed. (and possibly other things)
     MainLoopTimer_ms = millis();
     //Click();
    }
  }

  if  ( (millis() - PacketReceivedTimeStamp_ms) > HelloPacketInterval_ms) 
  {    
    if (Not_MPGmode == 0) ToggleMPGmodeTo("MPG:0");
    if (PendantOffline == false)
    {
     Serial.print(" Pendant offline. Last packet received from pendant (ms ago): ");
     Serial.println((millis() - PacketReceivedTimeStamp_ms));
     PendantOffline = true;
    }
  }
  else // Pendant is (back) online
  {
     if (Not_MPGmode == 1) ToggleMPGmodeTo("MPG:1"); // Leave Pendant in control as long as it is on.
     if (PendantOffline == true)
      {
       Serial.print(" Pendant online! Last packet received from pendant (ms ago): ");
       Beep();
       Serial.println((millis() - PacketReceivedTimeStamp_ms));
       PendantOffline = false;
      }
  }

  if (PacketReceived && ( (millis() - PacketReceivedTimeStamp_ms) > 200) ) // packet has probably outlived its usefulness.
  {
   Serial.print(" Pendant packet not used in 200ms. Discarded.");
   PacketReceived = false;
  }

}  // end main loop
