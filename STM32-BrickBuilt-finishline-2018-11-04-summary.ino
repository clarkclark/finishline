/* STM32-BrickBuilt-finishline.ino	https://github.com/clarkclark/finishline
 Clark Wood clark@cementhorizon.com
 Simple Finish Line monitor for Brickbuilt Derby, BrickCon 2011, Seattle
 thanks, Pinewood Derby Timer by Jason Scholten
 created 2011-09-02

 2018-10-21 Major revision to a new STM32 'Blue Pill' 72MHz 32-bit microcontroller for speed
		but... PB4 is pulled-up with no connection, but never GOES high.
		Something makes this port different. Moved WINNER outputs from 4-7 to PB12 - PB15

 2018-10-27 Completed new hardware board and it works!
		Corrected program logic to stop checking for winners when we have a winner (goto is deprecated!)
		Timed test (run a minute, enter blinkenlites mode to show cyclecount)
		**** ABOUT 2 MILLION CYCLES PER MINUTE OR 30,000 CYCLES PER SECOND! **** 
                (32-bit cycle counter should roll over every 32 hours)

 2018-10-30 Blinkenlites jumper open makes the pin float (even after adding a 10K pull-up).
		INVERTING the logic: now REMOVING jumper ENTERS Blinkenlites
                (tried converting blinkenlites variable to bool; became very slow. Fail.)

 GOAL -         A structure merging Arduino & Lego to adjudicate the winner of a downhill race, the BrickBuilt Derby.
                The four-lane track is approx 19 feet long, constructed from a Pinewood Derby practice track.
                First used at BrickCon 2011, Seattle.  The track and this device are set up each year.
                First version was built from a raw Atmel Atmega328P chip (thanks, Dale Wheat!)
		In recent races, Occasional ties required speeding up the lane-scanning process.
                I learned from a recent Make Magazine about the much faster STM32 'Blue Pill' processor.

 PROGRAM LOGIC - Each lane has an IR LED that reflects off the track to an adjacent IR photodiode.
		During setup, we read the analog level of each lane's reflected light and add a buffer amount.
		This value becomes the threshold for this lane.
		The lanes are scanned in sequence (as fast as we can go, since it's one at a time)
		When the first car passes under the finishline device, the detected IR light level falls 
		(and the value seen at an analog pin INCREASES), so we declare a winner.
		Keep the WINNER LED illuminated for 4 seconds, then reset and start scanning again.

 PROGRAMMING -	Since I can't locate drivers for the ST-Link device, I'm using an FTDI USB-to-serial convertor.
                Uploading steps: setup Arduino IDE (Tools) for Generic STM32F103C series, 20K RAM, 128K Flash;
                Upload Method: Serial;  Port: (use Device Manager to find this);
                Connect Blue Pill board to USB.  Move jumper 0 to position 1.  Press reset button.
                NOW you may compile and upload. Watch serial LEDs on USB-to-serial convertor.
                When "Upload Complete", restore STM32 jumper 0 to position 0.  Press reset. Let's race!

 CONSTRUCTION - photodiode has LONG leg to Arduino analog port, and through 470 ohm resistor to Vcc
		Short leg to ground.  Creates voltage divider. IR bright 0.1V to analog pin; IR dark sends 3.9V
		10-bit analog (at least 12-bit on the Blue Pill) sense value is low for IR bright, high for IR dark.

		IR LEDs powered by 5V, through (shared) 100 ohm resistor.

		Track Ready, and four WINNER LEDs powered by DS75492 hex driver in this new config for extra brightness.
		The chip connects LED cathode THROUGH EXTERNAL 100 ohm RESISTORS to ground. (long-leg Anodes to 5VDC)

 BLINKENLIGHTS MODE - (PB9 to ground through a handbag jumper, with a 10K pull-up resistor) active when jumper REMOVED
		changed to: with jumper installed, low voltage is read as a logical zero, do NOT go into BL mode.
		BL marquees the WIN lights and sends output to serial monitor of the thresholds and detected IR light levels.
		BL also displays the cycle count, to prove that we're normally scanning every 30 microseconds.
		NOTE: We delay each light and the serial monitor output. This mode won't detect the winner of a race.
               (BL scan rate is about 2 cycles per second; about 150,000 times slower than normal)
 
 RUNNING MODE - replace the blinkenlights jumper; only illuminates WINNNER and doesn't send stats to serial monitor.
*/


  // declaring global variables & initial values
  // physical (pin) inputs & outputs
  int ln1sense = PA1;	  // lane 1 ANALOG photodiode
  int ln2sense = PA2;	  // lane 2 ANALOG photodiode
  int ln3sense = PA3;	  // lane 3 ANALOG photodiode
  int ln4sense = PA4;	  // lane 4 ANALOG photodiode

  int ln1win = PB12;	  // lane 1 WIN output LED (through hex driver chip)
  int ln2win = PB13;	  // lane 2 WIN output LED (through hex driver chip)
  int ln3win = PB14;	  // lane 3 WIN output LED (through hex driver chip)
  int ln4win = PB15;	  // lane 4 WIN output LED (through hex driver chip)
  int trkready = PB8;	  // Big Green (or blue) track ready LED (through hex driver chip)

  int BLjump = PB9;	  // BLjump is BLINKENLIGHTS jumper (this is an input)

    // now declare variables that AREN'T physical inputs, assign initial values
  int state1 = 0;         // a place to put the analog read result of lane 1
  int state2 = 0;         // a place to put the analog read result of lane 2
  int state3 = 0;         // a place to put the analog read result of lane 3
  int state4 = 0;         // a place to put the analog read result of lane 4
  int ambient1 = 0;       // the photodiode reading FOR THIS LANE with the IR LED energized
  int ambient2 = 0;       // the photodiode reading FOR THIS LANE with the IR LED energized
  int ambient3 = 0;       // the photodiode reading FOR THIS LANE with the IR LED energized
  int ambient4 = 0;       // the photodiode reading FOR THIS LANE with the IR LED energized
  int blinkenlites = 0;   // state of jumper, enables light show & data spew
  bool winner = false;    // used in nested 'if' statements to stop checking when we have a winner

  long cyclecount = 0;	  // 32-bit count of cycles since reset or start


  // setup code, to run once
void setup() {

  pinMode(ln1sense, INPUT);      //we read these pins
  pinMode(ln2sense, INPUT);
  pinMode(ln3sense, INPUT);
  pinMode(ln4sense, INPUT);
  pinMode(BLjump, INPUT);

  pinMode(trkready, OUTPUT);     // we write to these pins
  pinMode(ln1win, OUTPUT);
  pinMode(ln2win, OUTPUT);
  pinMode(ln3win, OUTPUT);
  pinMode(ln4win, OUTPUT);

  digitalWrite(ln1win, LOW);     // we want this LED to be off (unless it's on)
  digitalWrite(ln2win, LOW);     // we want this LED to be off (unless it's on)
  digitalWrite(ln3win, LOW);     // we want this LED to be off (unless it's on)
  digitalWrite(ln4win, LOW);     // we want this LED to be off (unless it's on)
  digitalWrite(trkready, HIGH);  // we want this LED to illuminate right away
  digitalWrite(BLjump, HIGH);    // required WRITE to INPUT sets pullup on digital pin (not well enough, added external)

  Serial.begin(9600);            // setup serial monitor

// read the photodiodes to establish ambient level, in today's location and setup
  state1 = analogRead(ln1sense);
  state2 = analogRead(ln2sense);
  state3 = analogRead(ln3sense);
  state4 = analogRead(ln4sense);

  ambient1 = (state1 + 25);      // this margin should eliminate false alarms
  ambient2 = (state2 + 25);
  ambient3 = (state3 + 25);
  ambient4 = (state4 + 25);
}

  // main code, to loop:
void loop() {

// POSSIBLE option to Serial.print the characters \ | / -  in a spinner as each lane is tested.
// Some indication of activity would be nice, but maybe slow things down?
// Nope. Can't get the serial monitor to backspace, AND the backslash is a special character.

  cyclecount = (cyclecount + 1); // cycle count for speed testing
                                 // only display this number during blinkenlites

// read each photo transistor and act on it in turn

  state1 = analogRead(ln1sense);
  if (state1 > ambient1)         // can't mix these two types in one if
    if (winner == false)         // can't mix these two types in one if
      {
      digitalWrite(ln1win, HIGH);  // winner LED lane 1
      Serial.println("LANE 1 WINS"); 
      winner = true;
      }
  
  state2 = analogRead(ln2sense);
  if (state2 > ambient2)
    if (winner == false)
      {
      digitalWrite(ln2win, HIGH);	 //winner LED lane 2
      Serial.println("LANE 2 WINS"); 
      winner = true;
      }
  
  state3 = analogRead(ln3sense);
  if (state3 > ambient3)
    if (winner == false)
      {
      digitalWrite(ln3win, HIGH);	 //winner LED lane 3
      Serial.println("LANE 3 WINS"); 
      winner = true;
      }
  
  state4 = analogRead(ln4sense);
  if (state4 > ambient4)
    if (winner == false)
      {
      digitalWrite(ln4win, HIGH);	 //winner LED lane 4
      Serial.println("LANE 4 WINS"); 
      winner = true;
      }

// these steps are what to do after a winner has been detected
  if (winner == true)
    {
    digitalWrite(trkready, LOW);    
    delay(4000);              	 // bask in glory for 4 seconds
    digitalWrite(ln1win, LOW);	 // reset all the LEDs
    digitalWrite(ln2win, LOW);
    digitalWrite(ln3win, LOW);
    digitalWrite(ln4win, LOW);
    digitalWrite(trkready, HIGH);
    winner = false;              // let's race
    }

// BLINKENLIGHTS to show that we're looping, for development and thrills
// all this delay could allow a car to slip through the gate without being detected.

  blinkenlites = digitalRead(BLjump);  // read the blinkenlites jumper (on EACH loop)

  if (blinkenlites == 1) 		// that is, handbag jumper to ground is REMOVED
    {
    Serial.print("cyclecount = "); 
    Serial.println(cyclecount); 

    Serial.print("lane 1 ambient1 "); 
    Serial.println(ambient1);		// the trigger point for detecting a car
    Serial.print("lane 1 state1 "); 
    Serial.println(state1);		// detected value, lane 1
    digitalWrite(ln1win, HIGH);		// set the LED on
    delay(50);				// wait for some milliseconds
    digitalWrite(ln1win, LOW);		// set the LED off

    Serial.print("lane 2 ambient2 "); 
    Serial.println(ambient2);		// the trigger point for detecting a car
    Serial.print("lane 2 state2 "); 
    Serial.println(state2);		// detected value, lane 2
    digitalWrite(ln2win, HIGH);		// set the LED on
    delay(50);				// wait for some milliseconds
    digitalWrite(ln2win, LOW);		// set the LED off

    Serial.print("lane 3 ambient3 "); 
    Serial.println(ambient3);		// the trigger point for detecting a car
    Serial.print("lane 3 state3 "); 
    Serial.println(state3);		// detected value, lane 3
    digitalWrite(ln3win, HIGH);		// set the LED on
    delay(50);				// wait for some milliseconds
    digitalWrite(ln3win, LOW);		// set the LED off

    Serial.print("lane 4 ambient4 "); 
    Serial.println(ambient4);		// the trigger point for detecting a car
    Serial.print("lane 4 state4 "); 
    Serial.println(state4);		// detected value, lane 4
    digitalWrite(ln4win, HIGH);		// set the LED on
    delay(50);				// wait for some milliseconds
    digitalWrite(ln4win, LOW);		// set the LED off

    delay(300);        // wait for some milliseconds so we can read the screen
    Serial.println();
    // end of optional (BL jumper OFF) BLINKENLIGHTS section
    }
}
