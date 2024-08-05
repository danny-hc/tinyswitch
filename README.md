===============================================
Tinyswitch - v1.0 - (c)2024 - paalsma@gmail.com
===============================================

Tiny switch decoder based on the ATTINY85 in conjuction with a LD293D motor driver for 
usage on a single switch/turnout.

At boot decoder led will flash address info (if led setting is on), this is always 3 numbers with 
a small pause in between. A slow blink is a 0. Count fast blinks for other numbers. 
Send any command to switch address 255 to have all decoders blink their address (even if led setting is off).

CV's
====

CV1: Address from 1 - 200, default 9.
CV2: Mirror throw and close (1 = normal, 2 = mirror), default 1.
CV3: Led signal on boot (number), throw (double) & close (single), (1 = no, 2 = yes), default 1.
CV4: Pulse duration between 10 - 100ms in 10ms steps, value 1 - 10, default 5 (50ms).
CV8: Set to 8 or 255 for factory reset.

Functionality to read CV's hasn't been implemented.

On track programming / functions
================================
1) Close switch.
2) Throw switch X number of times, see table below (with interval < 4 seconds), led will turn on at X = 3.
3) Close switch to activate programming/function, led will blink fast to confirm and will stay on.
4) Throw Y number of times, Y being the new value or close for value = 0 (with interval < 4 seconds).
5) Close switch to store, led will blink fast twice.
6) Led is turned off. Programming stopped.

If interval is > 4 seconds programming mode is closed, old value is retained.

X  Function
===========
3  Change address (CV1).
4  Change mirror (CV2).
5  Change led (CV3).
6  Change pulse (CV4). 



Connections ATTINY85
====================
p1 (PB5) - Empty
p2 (PB3) - Status led
p3 (PB4) - LD293D p15 (IN4)
p4 (GND) - GND
p5 (PB0) - LD293D p9 (EN2)
p6 (PB1) - LD293D p10 (IN3)
p7 (PB2) - DCC signal  
p8 (VCC) - 5V
