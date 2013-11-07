## Smart car interior light delay
=====================

> *Firstly designed for my Lifan Smily (Lifan 320) car*

This is a device to make car interior light more comfortable for a driver.
Device is designed for cars which interior light only turns on when a door is open and momentally turns off when it's closed.


Also, there should be 3 wires connected to a lamp:

* permanent +12V;
* permanent GND;
* wire connected to GND when a door is open.


The device provides next functions:

* Turn light on when a door is opened;
* Turn light off in 10 seconds after all doors are closed;
* Turn light off when engine is started with closed doors;
* Turn light on when engine is stopped with closed doors;
* Turn light off when door is closed with engine on without delay;
* Automatically turn light off after two minutes it's on.

Turning light on is always accompanied by "fade in" effect in ~1 second.
Turning light off is always accompanied by "fade out" effect in ~4 seconds;

##Component list
* [pic12F683 MCU](http://ww1.microchip.com/downloads/en/devicedoc/41211d_.pdf);
* [IRLR024NPBF MOSFET](http://www.irf.com/product-info/datasheets/data/irlr024npbf.pdf);
* [NCP5501DT50RKG Linear voltage regulator](http://www.onsemi.ru.com/pub_link/Collateral/NCP5500-D.PDF);
* 0805 diode;
* 2 x 0805 10kOhms resistors;
* 0805 20kOhms resistor;
* Can supplied with connectors 3x3.96 и 2x3.96.

##Directory tree
* circuit — device schematic diagram designed in Splan 7;
* layout — PCB layout designed in Sprint Layout 6;
* firmware — source and hex file compiled with Hi-Tech PicC STD 9.60 PL3;

##Time constants
You can choose your own delays time changing next constants in source code:


`FADEOUT_TIMER_MAX 4`

Default 4 seconds for "fade out" effect *(max 255)*;


`FADEOUT_WAIT_TIME 1220`

Default 1220 ticks or 10 seconds *(122 ticks = 1 second)* for delay before light fades off *(max 65535 ticks)*;


`AUTO_OFF_TIME 14640`

Default 14640 ticks or 120 seconds *(122 ticks = 1 second)* for delay before light automatically fades off *(max 65535 ticks)*;

##Photos, etc
You can read about this device in my car blog here: http://www.drive2.ru/cars/lifan/smily/smily/urvin/journal/2275141/ (in Russian)