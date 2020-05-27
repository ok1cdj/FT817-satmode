/*  SAT mode emulation for 2x FT817 and  Arduino NANO
 *   
 *              by OK1CDJ v 0.1 2020
 *  
 *  Uses FT817 CAT library
 *  2 SW serials are set to 9600, HW serial is free for easy debug 
 *  Short press off button enable / disable tracking, 
 *  Long press invert mode
 *  
 *  Master radio - 2 RX, 3 TX
 *  Slave  radio - 4 RX, 5 TX
 *  Button is connected to PIN  6
 *  Piezo speaker is connected to PIN 9

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define SER // SW serial

/*************************************************************************************************/
/* Configure the FT 817 stuff */
#include <FT817.h>
#define INIT_WAIT_TIME 1000
long freq;

SoftwareSerial mySerial(2, 3); // RX, TX master radio connected here
SoftwareSerial mySerial2(4, 5); // RX, TX slave radio connected here

#define FT817_SPEED 9600

#define SMETER_LEN 4
#define MODE_LEN 5

#define LED 13
#define BUTTON 6
#define BUZZER 9

#define PRESSED LOW
#define NOT_PRESSED HIGH

const unsigned long shortPress = 100;
const unsigned long  longPress = 300;

long freq_master_old = 0;
long freq_slave_diff = 0;
long freq_slave = 0;
int active = 0;
int inverted = 1;

typedef struct
{
  // current status
  long freq;
  char mode[MODE_LEN];
  char smeter[SMETER_LEN];
  byte smeterbyte;
}
t_status;
t_status rig_master;
t_status rig_slave;

typedef struct Buttons {
  const byte pin = BUTTON;
  const int debounce = 10;

  unsigned long counter = 0;
  bool prevState = NOT_PRESSED;
  bool currentState;
} Button;

// create a Button variable
Button button;

FT817 ft817_master(&mySerial);
FT817 ft817_slave(&mySerial2);

void initialize_ft817 ()
{
  Serial.println("Init FT817 Master");
  ft817_master.begin(FT817_SPEED);
  delay(INIT_WAIT_TIME);
  Serial.println("Init FT817 Slave");
  ft817_slave.begin(FT817_SPEED);
  delay(INIT_WAIT_TIME);

}



/*************************************************************************************************/
void read_rig_master ()
{
  rig_master.freq = ft817_master.getFreqMode(rig_master.mode);
  Serial.print("FRQ master:");
  Serial.print(rig_master.freq);
  Serial.print(" ");
  Serial.println(rig_master.mode);
}

void read_rig_slave ()
{
  rig_slave.freq = ft817_slave.getFreqMode(rig_slave.mode);
  Serial.print("FRQ slave:");
  Serial.print(rig_slave.freq);
  Serial.print(" ");
  Serial.println(rig_slave.mode);
  freq_slave = rig_slave.freq;
}


/*************************************************************************************************/
// Global Setup Routing
void setup ()
{
  pinMode(button.pin, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  tone(BUZZER, 650, 100);
  Serial.begin(9600);
  pinMode(LED, OUTPUT);
  initialize_ft817();
  read_rig_slave ();
  delay(100);
  read_rig_master();

}

/*************************************************************************************************/
// Main loop
void loop ()
{
  button.currentState = digitalRead(button.pin);

  // has it changed?
  if (button.currentState != button.prevState) {
    delay(button.debounce);
    // update status in case of bounce
    button.currentState = digitalRead(button.pin);
    if (button.currentState == PRESSED) {
      // a new press event occured
      // record when button went down
      button.counter = millis();
    }

    if (button.currentState == NOT_PRESSED) {
      // but no longer pressed, how long was it down?
      unsigned long currentMillis = millis();
      if ((currentMillis - button.counter >= shortPress) && !(currentMillis - button.counter >= longPress)) {
        // short press detected - activate tracking
        tone(BUZZER, 650, 50);
        delay(50);
        if (active)
        {
          active = 0;
          digitalWrite(LED,LOW);
        } else {
          active = 1;
          digitalWrite(LED,HIGH);
        }
        read_rig_slave ();
        read_rig_master();

      }
      if ((currentMillis - button.counter >= longPress)) {
        // the long press was detected
        tone(BUZZER, 650, 50);
        delay(100);
        tone(BUZZER, 650, 50);
        delay(50);
        if (inverted==1) inverted = -1; else inverted = 1;
        read_rig_slave ();
        read_rig_master();
      }
    }
    // used to detect when state changes
    button.prevState = button.currentState;
  }
  if (active) {
    freq_master_old = rig_master.freq;
    read_rig_master();
    freq_slave_diff = rig_master.freq - freq_master_old;
    Serial.print("Diff: ");
    Serial.println(freq_slave_diff);
    if (freq_slave_diff != 0) {
      Serial.print("FRQ slave:");
      Serial.println(freq_slave);
      freq_slave = freq_slave + freq_slave_diff * inverted;
      ft817_slave.setFreq(freq_slave);
    }
    delay(100);
  }
}

