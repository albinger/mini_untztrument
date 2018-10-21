// Based on example code from Adafruit.
// Altered by Andrew Albinger
// Albinger Machine Company
//
// Changes: simplified to 1 trellis for mini-untz
//          added some light sequences beyond note select
//          added outgoing CC changes on 4 analog potentiometers
//          incoming CC16 will change lowest note
//          incoming CC17 value 1 will set to chromatic (boot up behavior)
//          incoming CC17 any other value will set to minor pentatonic
//
// Original comments:
// Super-basic UNTZtrument MIDI example.  Maps buttons to MIDI note
// on/off events; this is NOT a sequencer or anything fancy.
// Requires an Arduino Leonardo w/TeeOnArdu config (or a PJRC Teensy),
// software on host computer for synth or to route to other devices.

#include <Wire.h>
#include <Adafruit_Trellis.h>
#include <Adafruit_UNTZtrument.h>
#include "MIDIUSB.h"

#define LED     13 // Pin for heartbeat LED (shows code is working)
#define CHANNEL 1  // MIDI channel number
#define N_POTS 4
uint8_t firstnote = 36;

byte notes[] = {

  12, 13, 14, 15,
  8, 9, 10, 11,
  4, 5, 6, 7,
  0, 1, 2, 3
};


Adafruit_Trellis     T[1];
Adafruit_UNTZtrument untztrument(&T[0]);

uint8_t       heart        = 0;  // Heartbeat LED counter
unsigned long prevReadTime = 0L; // Keypad polling timer

// Pots
const int potPin[] = {3, 2, 1, 0 };  // Pot pins
const uint8_t potCN[] = {0x0A, 0x0B, 0x0C, 0x0D };  // MIDI control values
uint8_t potValues[N_POTS];  // Initial values
uint8_t potValuePrev[] = {0, 0, 0, 0, 0}; // previous values for comparison



void setup() {
  pinMode(LED, OUTPUT);

  untztrument.begin(0x70);

  // Default Arduino I2C speed is 100 KHz, but the HT16K33 supports
  // 400 KHz.  We can force this for faster read & refresh, but may
  // break compatibility with other I2C devices...so be prepared to
  // comment this out, or save & restore value as needed.
#ifdef ARDUINO_ARCH_SAMD
  Wire.setClock(400000L);
#endif
#ifdef __AVR__
  TWBR = 12; // 400 KHz I2C on 16 MHz AVR
#endif

  testDisplay(70);

  untztrument.clear();
  untztrument.writeDisplay();
}

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
  MidiUSB.flush();
}


void readPots() {
  for (int i = 0; i < N_POTS; i++) {
    int val = analogRead(potPin[i]);
    potValues[i] = (uint8_t) (map(val, 0, 1023, 0, 127));
  }
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}


void testDisplay(int mils) {
  // Startup light animation //

  // light up all the LEDs in order
  for (byte y = 0; y < 16; y++) {

    untztrument.setLED(y);
    untztrument.writeDisplay();
    delay(mils);

  }

  // then turn them off
  for (byte y = 0; y < 16; y++) {

    untztrument.clrLED(y);
    untztrument.writeDisplay();
    delay(mils);
    //MidiUSB.setHandleControlChange(myControlChange);
  }

  // End of animation //
}

void displayBars(int mil) {
  for (byte y = 0; y < 4; y++) {

    untztrument.setLED(y);
    untztrument.setLED(y + 4);
    untztrument.setLED(y + 8);
    untztrument.setLED(y + 12);
    untztrument.writeDisplay();
    delay(mil);

  }


  for (byte y = 0; y < 4; y++) {

    untztrument.clrLED(y);
    untztrument.clrLED(y + 4);
    untztrument.clrLED(y + 8);
    untztrument.clrLED(y + 12);
    untztrument.writeDisplay();
    delay(mil);
  }
}

void sendMIDI()
{
  for (int i = 0; i < N_POTS; i++) {
    if (abs(potValuePrev[i] - potValues[i]) > 1)
    {
      potValuePrev[i] = potValues[i];
      controlChange(0, potCN[i], potValues[i]);
    }
  }
}





void loop() {

  readPots();
  sendMIDI();
  MidiUSB.flush();
  midiEventPacket_t CCmessage =  MidiUSB.read();
  if (CCmessage.byte2 == 16) {
    firstnote = CCmessage.byte3;
    testDisplay(10);
  } else if (CCmessage.byte2 == 17) {
    if (CCmessage.byte3 == 1) {
      notes[0] = 12;
      notes[1] = 13;
      notes[2] = 14;
      notes[3] = 15;
      notes[4] = 8;
      notes[5] = 9;
      notes[6] = 10;
      notes[7] = 11;
      notes[8] = 4;
      notes[9] = 5;
      notes[10] = 6;
      notes[11] = 7;
      notes[12] = 0;
      notes[13] = 1;
      notes[14] = 2;
      notes[15] = 3;

    } else {

      notes[0] = 29;
      notes[1] = 31;
      notes[2] = 34;
      notes[3] = 36;
      notes[4] = 19;
      notes[5] = 22;
      notes[6] = 24;
      notes[7] = 27;
      notes[8] = 10;
      notes[9] = 12;
      notes[10] = 15;
      notes[11] = 17;
      notes[12] = 0;
      notes[13] = 3;
      notes[14] = 5;
      notes[15] = 7;
    }
    displayBars(30);
  }

  MidiUSB.flush();
  unsigned long t = millis();
  if ((t - prevReadTime) >= 20L) { // 20ms = min Trellis poll time
    if (untztrument.readSwitches()) { // Button state change?
      for (uint8_t i = 0; i < 16; i++) { // For each button...
        if (untztrument.justPressed(i)) {
          noteOn(CHANNEL, notes[i] + firstnote, 127);
          untztrument.setLED(i);
        } else if (untztrument.justReleased(i)) {
          noteOff(CHANNEL, notes[i] + firstnote, 0);
          untztrument.clrLED(i);
        }

      }
      untztrument.writeDisplay();
    }
    prevReadTime = t;
    digitalWrite(LED, ++heart & 32); // Blink = alive
  }
}
