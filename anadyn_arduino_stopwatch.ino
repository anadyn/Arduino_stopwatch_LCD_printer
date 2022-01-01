/*

Anadyn Arduino Stopwatch
Arduino sketch for lap time measurements using external gates as triggers.

Features:
Measures total time and optional split time, maximum time measured is 60 minutes. 
LCD screen with 4x20 characters.
The measured times are shown as minutes:seconds.hundreds of second
Four inputs for timing gates, each can be configured for the split time measurement.
Manual start/stop button in addition to timing gates
Thermo printer that prints results, with switch to turn on/off
Switch+button to set split gate 1-4 or none
LED that blinks when measurement is active
Timestamps are sent on the serial port (UART), on the format mmssuu, where mm=minutes, ss=seconds and uu=hundreds of seconds
The previous two times are shown together with the latest time.
Cool animation on the LCD while measurement is running :-)

Pin layout:

Timing gates:  digital pin 8,9,10,11
Manual start/stop button:  digital pin 12 (cannot be used as split gate)
LED indicator:  digital pin 13 (add 330 ohm resistor in series)
Switch split time on/off: digital 14 (analog A0)
(Switch split programming on/off: digital 14 (analog A0)  ) <- not used in present version
(Button split programming:  digital 16 (analog A2)        ) <- not used in present version
Switch printer on/off:  digital 15 (analog A1)

Serial port for sending timestamps:
RX on digital pin 0
TX on digital pin 1

Printer:
RX on printer to digital pin 19 (analog A5)
TX on printer to digital pin 18 (analog A4)
(connect GND pin on printer to GND on Arduino)

LCD:
 * LCD RS pin to digital pin 6
 * LCD Enable pin to digital pin 7
 * LCD D4 pin to digital pin 2
 * LCD D5 pin to digital pin 3
 * LCD D6 pin to digital pin 4
 * LCD D7 pin to digital pin 5
 * LCD R/W pin to ground
 * 10K potentiometer:
 * ends to +5V and ground
 * potentiometer wiper to LCD VO pin (LCD pin 3)

Revision log:
2021-05-30: changed ports (changed from breadboard to soldered protoboard)
2021-07-04: no separate gate for split time, instead on/off split time
2021-12-29: output on serial port adapted for RGB matrix display


Copyright (c) 2021 Lars-Ove Jönsson
Available under the MIT license:

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

// include library codes
#include <LiquidCrystal.h>     // LCD
#include "Adafruit_Thermal.h"  // thermal printer

// add software serial port for thermal printer
#include "SoftwareSerial.h"
// pins for serial communication with printer
#define TX_PIN 19 // labeled RX on printer  (analog A5)
#define RX_PIN 18 // labeled TX on printer  (analog A4)
SoftwareSerial mySerial(RX_PIN, TX_PIN); // Declare SoftwareSerial obj first
Adafruit_Thermal printer(&mySerial);     // Pass addr to printer constructor

// define pins
// all gates and buttons are LOW when triggered, HIGH when untriggered
// add 10k pull-up resistor between Vcc and gate
const int pin_gate1=8;
const int pin_gate2=9;
const int pin_gate3=10;
const int pin_gate4=11;
const int pin_gate5=12;  // manual start/stop button
const int pin_LED=13;
const int pin_program_split=14;   // =pin A0  switch split time on/off
const int pin_enable_printer=15;  // =pin A1
const int pin_toggle_split=16;    // =pin A2  pushbutton to change split gate  <- not used

// variables
boolean measRunning=false;        // true if time measurement running
boolean splitRunning=false;       // true if split time measurement running
boolean splitEnabled;             // true if split time is enabled
int split_gate=4;           // gate number 1-4 gives split time, number=0 means split time disabled
int gate1;
int gate2;
int gate3;
int gate4;
int gate5;
int enable_printer;
int enable_split;
int program_split;
int toggle_split;
// boolean variables that are true if gate is activated
boolean gate1_Active=false;
boolean gate2_Active=false;
boolean gate3_Active=false;
boolean gate4_Active=false;
boolean gate5_Active=false;
boolean any_gate_Active;
boolean splitgate_Active;
boolean LED_on;
boolean printer_on;

// variables to control startup/measurement animation
boolean startup_screen;  // true when startup, 
boolean show_horse1;     // controls which custom character is shown
int horseposition;       // cursor position of horse animation
unsigned long startup_update;      // to check if startup animation shall be updated

unsigned long start_millis=0;   // start time in milliseconds, initial value=0
unsigned long stop_millis=0;    // stop time in milliseconds, initial value=0
unsigned long split_millis=0;   // split time in milliseconds, initial value=0
unsigned long lcd_update_time;      // to check if LCD shall be updated
float time_finish=0;            // time from start to finish in seconds
int finish_minutes;             // finish time minutes
int finish_seconds;             // finish time seconds 0-59
int finish_milliseconds;        // finish time milliseconds 0-999
float time_split=0;             // time from start to split gate in seconds
int split_minutes;              // split time minutes
int split_seconds;              // split time seconds 0-59
int split_milliseconds;         // split time milliseconds 0-999
int update_minutes;             // lcd update time minutes
int update_seconds;             // lcd update time seconds 0-59
int update_milliseconds;        // lcd update time milliseconds 0-999

// character arrays to store last 3 results
char str_row0[40];
char str_row1[40];
char str_newresult[40];
char str_oldresult1[40];
char str_oldresult2[40];

// character array for timestamp on serial
char timestamp[10];


// ------------------- LCD
// initiate LCD
//LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
LiquidCrystal lcd(6, 7, 2, 3, 4, 5);

// define LCD custom characters
byte horse_standing1[] = {
  B00000,
  B00000,
  B01000,
  B11000,
  B01111,
  B01110,
  B01010,
  B01010
};

byte horse_standing2[] = {
  B01000,
  B11000,
  B01111,
  B01110,
  B01010,
  B01010,
  B00000,
  B00000
};

byte horse_run1[] = {
  B00000,
  B00000,
  B01000,
  B11000,
  B01111,
  B01110,
  B01010,
  B00100
};

byte horse_run2[] = {
  B01000,
  B11001,
  B01110,
  B01110,
  B01010,
  B10001,
  B00000,
  B00000
};

byte flag[] = {
  B11010,
  B10101,
  B11010,
  B10101,
  B10000,
  B10000,
  B10000,
  B10000
};

byte startgate[] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00100,
  B00100,
  B00100
};

// numbering of custom characters
const byte horse_standing1_char=0;
const byte horse_standing2_char=1;
const byte horse_run1_char=2;
const byte horse_run2_char=3;
const byte flag_char=4;
const byte startgate_char=5;
const byte space_char=32;


// ------------------- setup starts here
void setup() {

  // init serial communication
  Serial.begin(9600);
  while (!Serial) ; // Needed for Leonardo only

  // init thermo printer
  mySerial.begin(19200);  // Initialize SoftwareSerial
  delay(500); // wait for serial connection
  printer.begin();        // Init printer (same regardless of serial type)

  // initialize the LED pin as an output:
  pinMode(pin_LED, OUTPUT);
  // initialize the gate pins and buttons as an input:
  pinMode(pin_gate1, INPUT);
  pinMode(pin_gate2, INPUT);
  pinMode(pin_gate3, INPUT);
  pinMode(pin_gate4, INPUT);
  pinMode(pin_gate5, INPUT);  // manual start/stop button
  pinMode(pin_program_split, INPUT);
  pinMode(pin_toggle_split, INPUT);
  pinMode(pin_enable_printer, INPUT);

  lcd.begin(20, 4);

  lcd.createChar(horse_standing1_char, horse_standing1);
  lcd.createChar(horse_standing2_char, horse_standing2);
  lcd.createChar(horse_run1_char, horse_run1);
  lcd.createChar(horse_run2_char, horse_run2);
  lcd.createChar(flag_char, flag);
  lcd.createChar(startgate_char, startgate);
  lcd.home();

  lcd.setCursor(0, 0);
  lcd.print(" Measurement ready  ");

  // send timestamp zeros to serial port
  Serial.println("000000");

  // check if printouts are wanted
  enable_printer = digitalRead(pin_enable_printer);

  
  // setup startup animation
  lcd.setCursor(2, 2);
  lcd.write(flag_char);
  lcd.setCursor(16, 2);
  lcd.write(horse_run1_char);
  lcd.write(startgate_char);
  startup_screen=true;
  show_horse1=false;  //show horse_run2_char next time
  horseposition=15;
  startup_update=millis();


}


// ------------------- loop starts here
void loop() {

  // read gates and buttons
  gate1 = digitalRead(pin_gate1);
  gate2 = digitalRead(pin_gate2);
  gate3 = digitalRead(pin_gate3);
  gate4 = digitalRead(pin_gate4);
  gate5 = digitalRead(pin_gate5);
  enable_split = digitalRead(pin_program_split);
  enable_printer = digitalRead(pin_enable_printer);

  // if measurement not running, check for split and printer on/off
  if ( !measRunning ) {
    if (enable_split == LOW) {
      // split time on
      splitEnabled=true;
    } else {
      // split time off
      splitEnabled=false;
    }
  }

  // check switch for enable_printer
  if (enable_printer == LOW) {
    // thermo printer on
    printer_on=true;
  } else {
    // thermo printer off
    printer_on=false;
  }

  // check if gates are activated = pulled to LOW
  gate1_Active=(gate1 == LOW);
  gate2_Active=(gate2 == LOW);
  gate3_Active=(gate3 == LOW);
  gate4_Active=(gate4 == LOW);
  gate5_Active=(gate5 == LOW);

  any_gate_Active=(gate1_Active||gate2_Active)||(gate3_Active||gate4_Active)||gate5_Active;

  if (any_gate_Active){
    if (!measRunning){
      // start measurement!
      start_millis=millis();
      measRunning=true;
      // send zeros to RGB matrix display
      Serial.println("000000");
      startup_screen=false;  // disable startup animation
      lcd_update_time=start_millis;
      lcd.setCursor(0,0);
      //lcd.print("Time  00:00.00      ");
      lcd.print("Measurement started!");
      if (splitEnabled){
        lcd.setCursor(0,1);
        lcd.print("Split --:--.--     ");
        splitRunning=true;
      }
      else {
        lcd.setCursor(0,1);
        lcd.print("                   ");
        splitRunning=false;
      }
      lcd.setCursor(0,2);
      lcd.print(str_oldresult1);
      lcd.print("   ");
      lcd.setCursor(0,3);
      lcd.print(str_oldresult2);
      lcd.print("   ");

      LED_on=true;
      digitalWrite(pin_LED, HIGH);
      // pause
      delay(2000);
      lcd.setCursor(0,0);
      lcd.print("Time  00:02.00      ");
    }
    else if (splitEnabled && splitRunning){
      // stop split time measurement!
      split_millis=millis();
      splitRunning=false;
      // calculate minutes/seconds/ms in split time, use integer division
      split_minutes=(split_millis-start_millis)/60000;
      split_seconds=(split_millis-start_millis)/1000-60*split_minutes;
      split_milliseconds=(split_millis-start_millis)%1000;
      sprintf(str_row1,"Split %02i:%02i.%02i      ",
              split_minutes,split_seconds,split_milliseconds/10);
      lcd.setCursor(0,1);
      lcd.print(str_row1);
      //lcd.print(" Gate");
      //lcd.print(split_gate);

      // send timestamp on serial
      sprintf(timestamp,"%02i%02i%02i",
            split_minutes,split_seconds,split_milliseconds/10);
      Serial.println(timestamp);
      // pause
      delay(4000);
    }
    else {
      // stop measurement!
      stop_millis=millis();
      measRunning=false;
      splitRunning=false;  // stop split measurement, if still active
      digitalWrite(pin_LED, LOW);
      // calculated total time in seconds
      time_finish=(stop_millis-start_millis)/float(1000);

      // calculate minutes/seconds/ms in finish time, use integer division
      finish_minutes=(stop_millis-start_millis)/60000;
      finish_seconds=(stop_millis-start_millis)/1000-60*finish_minutes;
      finish_milliseconds=(stop_millis-start_millis)%1000;
      sprintf(str_row0,"Time  %02i:%02i.%02i",
              finish_minutes,finish_seconds,finish_milliseconds/10);
      sprintf(str_newresult,"Previous %02i:%02i.%02i",
              finish_minutes,finish_seconds,finish_milliseconds/10);

      lcd.setCursor(0,0);
      lcd.print(str_row0);
      lcd.setCursor(16,0);
      lcd.print("stop");
      lcd.setCursor(0,2);
      lcd.print(str_oldresult1);
      lcd.setCursor(0,3);
      lcd.print(str_oldresult2);

      // send timestamp on serial
      sprintf(timestamp,"%02i%02i%02i",
            finish_minutes,finish_seconds,finish_milliseconds/10);
      Serial.println(timestamp);

      // overwrite old results
      for (int i=0;i<40;i++){
        str_oldresult2[i]=str_oldresult1[i];
      }
      for (int i=0;i<40;i++){
        str_oldresult1[i]=str_newresult[i];
      }

      // print on thermal printer
      if (printer_on){
        printer.setSize('L');
        printer.println("Name:");
        printer.println("Run:");
        printer.println(str_row0);
        if (splitEnabled) printer.println(str_row1);
        printer.println();
      }

      // pause
      delay(2000);
    }
  }

  if ( measRunning && ((millis()-lcd_update_time)>1000) ) {
    // when measuring, update LCD every second
    lcd_update_time = millis();
    // calculate minutes/seconds/ms in finish time, use integer division
    update_minutes=(lcd_update_time-start_millis)/60000;
    update_seconds=(lcd_update_time-start_millis)/1000-60*update_minutes;
    //update_milliseconds=(lcd_update_time-start_millis)%1000;
    sprintf(str_row0,"Time  %02i:%02i.00",
            update_minutes,update_seconds);
    lcd.setCursor(0,0);
    lcd.print(str_row0);

      // send timestamp on serial
      sprintf(timestamp,"%02i%02i00",
            update_minutes,update_seconds);
      Serial.println(timestamp);

    // blink LED while measurement is running
    if (LED_on) {
      digitalWrite(pin_LED, LOW);
      LED_on=false;
    }
    else {
      digitalWrite(pin_LED, HIGH);
      LED_on=true;
    }
  }

  if ( startup_screen && ((millis()-startup_update)>1000) ) {
    // make startup animation
    startup_update=millis();
    lcd.setCursor(4,2);
    lcd.write(" ");
    lcd.setCursor(horseposition--, 2);
    if (horseposition==3) horseposition=15;
    if (show_horse1) {
      lcd.write(horse_run1_char);
      lcd.write(" ");
      show_horse1=false;
    }
    else {
      lcd.write(horse_run2_char);
      lcd.write(" ");
      show_horse1=true;
    }
  }


}
