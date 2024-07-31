/*  From the Readme of https://github.com/grblHAL/Plugin_keypad
 *   SEE ALSO: https://github.com/grblHAL/core/wiki/MPG-and-DRO-interfaces#keypad-plugin
 *   NOTE: These commands are CASE-SENSITIVE and are Terje Io's own creations, not a grbl standard.
Character Action
! Enter feed hold
~ Cycle start
0x8B  Enable MPG full control1
0x85  Cancel jog motion2
0 Enable step mode jogging
1 Enable slow jogging mode
2 Enable fast jogging mode
h Select next jog mode
H Home machine
R Continuous jog X+
L Continuous jog X-
F Continuous jog Y+
B Continuous jog Y-
U Continuous jog Z+
D Continuous jog Z-
r Continuous jog X+Y+
q Continuous jog X+Y-
s Continuous jog X-Y+
t Continuous jog X-Y-
w Continuous jog X+Z+
v Continuous jog X+Z-
u Continuous jog X-Z+
x Continuous jog X-Z-
0x84  Toggle safety door open status
0x88  Toggle optional stop mode
0x89  Toggle single block execution mode
0x8A  Toggle Fan 0 output3
I Restore feed override value to 100%
0x90  Restore feed override value to 100%
i Increase feed override value 10%
0x91  Increase feed override value 10%
j Decrease feed override value 10%
0x92  Decrease feed override value 10%
0x93  Increase feed override value 1%
0x94  Decrease feed override value 1%
0x95  Restore rapids override value to 100%
0x96  Set rapids override to 50%
0x97  Set rapids override to 25%
K Restore spindle RPM override value to 100%
0x99  Restore spindle RPM override value to 100%
k Increase spindle RPM override value 10%
0x9A  Increase spindle RPM override value 10%
z Decrease spindle RPM override value 10%
0x9B  Decrease spindle RPM override value 10%
0x9C  Increase spindle RPM override value 1%
0x9D  Decrease spindle RPM override value 1%
0x9E  Toggle spindle stop override4
C Toggle flood coolant output
0xA0  Toggle flood coolant output
M Toggle mist coolant output
0xA1  Toggle mist coolant output

////////////////////////////////////////////////
Here are all the standard grbl real-time commands:
      
      if (InputString == "0x18") RTcommand = 0x18; // Soft reset
      if (InputString == "0x83") RTcommand = 0x83; // Gcode_report
      if (InputString == "0x87") RTcommand = 0x87; // Status_report_all. Only works in MPG:1 mode. <Idle|WPos:0.000,0.000,0.000|WCS:G54|A:|Sc:|MPG:1|H:0|T:0|TLR:0|FW:grblHAL>
      if (InputString == "0x80") RTcommand = 0x80; // Status_brief  <Idle|WPos:0.000,0.000,0.000>      
      if (InputString == "0x81") RTcommand = 0x81; // Cycle start or resume after hold      
      if (InputString == "0x82") RTcommand = 0x82; // Feed hold (pause execution of gcode gracefully)
      if (InputString == "0x84") RTcommand = 0x84; // Door open (pause and lift spindle)
      if (InputString == "0x85") RTcommand = 0x85; // Jog Cancel immediately
      if (InputString == "0x90") RTcommand = 0x90; // Set back to 100% of programmed FEED rate.
      if (InputString == "0x95") RTcommand = 0x95; // Set back to 100% of programmed RAPID rate.            
      // There are more RT commands I could add. See Commands_Reference tab.


Ctrl-x : Soft-Reset      0x18
~ : Cycle Start / Resume 0x81
! : Feed Hold            0x82
? : Brief report         0x80

Feed overrides:
0x90 : Set 100% of programmed rate.
0x91 : Increase 10%
0x92 : Decrease 10%
0x93 : Increase 1%
0x94 : Decrease 1%

Rapid Overrides:
0x95 : Set to 100% full rapid rate.
0x96 : Set to 50% of rapid rate.
0x97 : Set to 25% of rapid rate.

Spindle Speed Overrides:
0x99 : Set 100% of programmed spindle speed
0x9A : Increase 10%
0x9B : Decrease 10%
0x9C : Increase 1%
0x9D : Decrease 1%

0x9E : Toggle Spindle Stop
0xA0 : Toggle Flood Coolant
0xA1 : Toggle Mist Coolant
            */

/////////////////////////////////////////////

/* 
 * Jogging settings using $ commands:
 * 
 * $50=<float> : default plugin dependent.
Jogging step speed in mm/min. Not used by core, indended use by driver and/or sender. Senders may query this for keyboard jogging modified by CTRL key.

$51=<float> : default plugin dependent.
Jogging slow speed in mm/min. Not used by core, indended use by driver and/or sender. Senders may query this for keyboard jogging.

$52=<float> : default plugin dependent.
Jogging fast speed in mm/min. Not used by core, indended use by driver and/or sender. Senders may query this for keyboard jogging modified by SHIFT key.

$53=<float> : default plugin dependent.
Jogging step distance in mm. Not used by core, indended use by driver and/or sender. Senders may query this for keyboard jogging modified by CTRL key.

$54=<float> : default plugin dependent.
Jogging slow distance in mm. Not used by core, indended use by driver and/or sender. Senders may query this for keyboard jogging.

$55=<float> : default plugin dependent.
Jogging fast distance in mm. Not used by core, indended use by driver and/or sender. Senders may query this for keyboard jogging modified by SHIFT key.
 */
